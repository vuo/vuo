/**
 * @file
 * VuoApp implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"

#include "VuoApp.h"

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

#include <dlfcn.h>
#include <pthread.h>
#include <libproc.h>
#include <mach-o/dyld.h>
#import <libgen.h>
#import <dirent.h>

#include "VuoCompositionState.h"
#include "VuoEventLoop.h"
#import "VuoAppDelegate.h"
#import "VuoAppSplashWindow.h"

/**
 * The duration of the window fade in/out animation.
 * @version200New
 */
const double VuoApp_windowFadeSeconds = 0.45;

/**
 * Is the current thread the main thread?
 */
bool VuoApp_isMainThread(void)
{
	static void **VuoApp_mainThread;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoApp_mainThread = (void **)dlsym(RTLD_SELF, "VuoApp_mainThread");
		if (!VuoApp_mainThread)
			VuoApp_mainThread = (void **)dlsym(RTLD_DEFAULT, "VuoApp_mainThread");

		if (!VuoApp_mainThread)
		{
			VUserLog("Error: Couldn't find VuoApp_mainThread.");
			VuoLog_backtrace();
			exit(1);
		}

		if (!*VuoApp_mainThread)
		{
			VUserLog("Error: VuoApp_mainThread isn't set.");
			VuoLog_backtrace();
			exit(1);
		}
	});

	return *VuoApp_mainThread == (void *)pthread_self();
}

/**
 * Executes the specified block on the main thread, then returns.
 *
 * Can be called from any thread, including the main thread (avoids deadlock).
 *
 * Don't call this from `__attribute__((constructor))` functions,
 * since this function depends on initialization
 * which might not happen before your constructor is called.
 */
void VuoApp_executeOnMainThread(void (^block)(void))
{
	if (!block)
		return;

	if (VuoApp_isMainThread())
		block();
	else
	{
		VUOLOG_PROFILE_BEGIN(mainQueue);
		dispatch_sync(dispatch_get_main_queue(), ^{
			VUOLOG_PROFILE_END(mainQueue);
			block();
		});
	}
}

/**
 * Returns the composition's name.
 *
 * If the composition is running in Vuo Editor (via VuoCompositionLoader),
 * it extracts the name from the most-recently-loaded dylib's path.
 * If the composition has been saved, this matches the composition's source filename.
 * If not, it returns the text "Vuo Composition".
 *
 * If the composition is running via VuoRunner (but not via VuoCompositionLoader),
 * it returns the name of the VuoRunner process.
 *
 * If the composition is running standalone, it tries the following Info.plist keys:
 *
 *    - CFBundleDisplayName
 *    - CFBundleName
 *    - CFBundleExecutable
 *
 * If none of those keys exist, it uses the composition executable's filename.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoApp_getName(void)
{
	char **dylibPath = (char **)dlsym(RTLD_SELF, "VuoApp_dylibPath");
	if (!dylibPath)
		dylibPath = (char **)dlsym(RTLD_DEFAULT, "VuoApp_dylibPath");
	if (dylibPath)
	{
		char *filename = strrchr(*dylibPath, '/');
		if (filename)
		{
			char *name = strdup(filename + 1); // Trim leading slash.
			name[strlen(name) - strlen("-XXXXXX.dylib")] = 0;

			if (strcmp(name, "VuoComposition") == 0)
			{
				free(name);
				return strdup("Vuo Composition");
			}

			return name;
		}
	}

	pid_t runnerPid = VuoGetRunnerPid();
	if (runnerPid > 0)
	{
		char *runnerName = (char *)malloc(2*MAXCOMLEN);
		proc_name(runnerPid, runnerName, 2*MAXCOMLEN);
		return runnerName;
	}

	NSBundle *mainBundle = [NSBundle mainBundle];
	NSString *name = [mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
	if (!name)
		name = [mainBundle objectForInfoDictionaryKey:@"CFBundleName"];
	if (!name)
		name = [mainBundle objectForInfoDictionaryKey:@"CFBundleExecutable"];
	if (!name)
		name = [[[mainBundle executablePath] stringByDeletingPathExtension] lastPathComponent];

	if (name)
		return strdup([name UTF8String]);
	else
		return strdup("");
}

/**
 * Helper for @ref VuoApp_init.
 *
 * @threadMain
 */
static void VuoApp_initNSApplication(bool requiresDockIcon)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];

	// https://stackoverflow.com/a/11010614/238387
	NSApplication *app = [NSApplication sharedApplication];

	if (![app delegate])
		[app setDelegate:[VuoAppDelegate new]];

	// When there's no host app present,
	// create the default menu with the About and Quit menu items,
	// to be overridden if/when any windows get focus.
	VuoApp_setMenuItems(NULL);

	BOOL ret = [app setActivationPolicy:requiresDockIcon ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory];
	if (!ret)
		VUserLog("-[NSApplication setActivationPolicy:%d] failed", requiresDockIcon);

	// Stop bouncing in the dock.
	[app finishLaunching];

	VuoEventLoop_switchToAppMode();

	if (VuoShouldShowSplashWindow())
		VuoApp_showSplashWindow();

	[pool drain];
}

/**
 * True if `VuoApp_init()` has initialized an NSApplication.
 */
static bool VuoApp_initialized = false;

/**
 * Creates an NSApplication instance (if one doesn't already exist).
 *
 * VuoWindow methods call this automatically as needed,
 * so you only need to call this if you need an NSApplication instance without using VuoWindow
 * (such as when using certain Cocoa/AppKit services —
 * like, for example, @ref VuoAudioFile and @ref VuoScreen_getName do).
 *
 * If `requiresDockIcon` is true, the app will show up in the dock.
 * If `requiresDockIcon` is false, the app won't necessarily show up in the dock
 * (if `VuoApp_init` was previously called with `requiresDockIcon=true`, the icon will remain).
 *
 * @threadAny
 *
 * @version200Changed{Added `requiresDockIcon` parameter.}
 */
void VuoApp_init(bool requiresDockIcon)
{
	if (NSApp)
	{
		VuoApp_executeOnMainThread(^{
			if (requiresDockIcon
			 && NSApplication.sharedApplication.activationPolicy != NSApplicationActivationPolicyRegular)
				[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		});
		return;
	}

	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoApp_initialized = true;

		VuoApp_executeOnMainThread(^{
			VuoApp_initNSApplication(requiresDockIcon);
		});
	});
}

/**
 * Helper for @ref VuoApp_fini.
 *
 * @threadMain
 */
static void VuoApp_finiWindows(uint64_t compositionUid)
{
	SEL stopRecording = @selector(stopRecording);
	SEL compositionUidSel = @selector(compositionUid);
	for (NSWindow *window in [NSApp windows])
		// Stop any window recordings currently in progress.
		// This prompts the user for the save destination,
		// so make sure these complete before shutting the composition down.
		if ([window respondsToSelector:stopRecording]
		 && [window respondsToSelector:compositionUidSel]
		 && (uint64_t)[window performSelector:compositionUidSel] == compositionUid)
			[window performSelector:stopRecording];

	if ([NSApp windows].count)
	{
		// Animate removing the app from the dock while the window is fading out (instead of waiting until after).
		[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];

		double fudge = .1; // Wait a little longer, to ensure the animation's completionHandler gets called.
		[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:VuoApp_windowFadeSeconds + fudge]];
	}

	// Avoid leaving menubar remnants behind.
	// https://b33p.net/kosada/node/13384
	[NSApp hide:nil];
}

/**
 * Cleanly shuts the application down.
 *
 * Should only be called from within @ref vuoStopComposition().
 *
 * @threadAny
 */
void VuoApp_fini(void)
{
	if (! VuoApp_initialized)
		return;

	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		void *compositionState = vuoCopyCompositionStateFromThreadLocalStorage();
		uint64_t compositionUid = vuoGetCompositionUniqueIdentifier(compositionState);
		free(compositionState);

		if (VuoApp_isMainThread())
			VuoApp_finiWindows(compositionUid);
		else
		{
			VUOLOG_PROFILE_BEGIN(mainQueue);
			dispatch_sync(dispatch_get_main_queue(), ^{
				VUOLOG_PROFILE_END(mainQueue);
				VuoApp_finiWindows(compositionUid);
			});
		}
	});
}
