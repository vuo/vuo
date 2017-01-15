/**
 * @file
 * VuoWindow implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindow.h"
#import "VuoWindowTextInternal.h"
#import "VuoWindowOpenGLInternal.h"
#include "VuoEventLoop.h"

#include "module.h"

#include <pthread.h>
#include <mach-o/dyld.h>
#include <libgen.h>
#include <dirent.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindow",
					 "dependencies" : [
						 "AppKit.framework",
						 "VuoWindowTextInternal",
						 "VuoWindowOpenGLInternal"
					 ]
				 });
#endif

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
			exit(1);
		}

		if (!*VuoApp_mainThread)
		{
			VUserLog("Error: VuoApp_mainThread isn't set.");
			exit(1);
		}
	});

	return *VuoApp_mainThread == (void *)pthread_self();
}

/**
 * Executes the specified block on the main thread, then returns.
 *
 * Can be called from any thread, including the main thread (avoids deadlock).
 */
void VuoApp_executeOnMainThread(void (^block)(void))
{
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
 * NSApplication delegate, to shut down cleanly when the user requests to quit.
 */
@interface VuoApplicationDelegate : NSObject<NSApplicationDelegate>
@end

@implementation VuoApplicationDelegate

/**
 * When the user tries to quit the application, cleanly stops the composition.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	VuoStopComposition();
	return NSTerminateCancel;
}

@end

/**
 * The About dialog for a Vuo app.
 */
@interface VuoApplicationAboutDialog : NSObject
- (void)displayAboutPanel:(id)sender;
@end

@implementation VuoApplicationAboutDialog

/**
 * Loads license data and sends it to the standard "About" panel.
 */
- (void)displayAboutPanel:(id)sender
{
	[NSApp orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObject:[self licenseString] forKey:@"Credits"]];
}

/**
 * Builds an attributed string from the contents of each file in the app bundle's "Licenses" folder.
 */
- (NSAttributedString *)licenseString
{
	NSMutableAttributedString *mas = [NSMutableAttributedString new];

	[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[@"<p>This composition may include software licensed under the following terms:</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];


	// Find Vuo.framework.
	char frameworkPath[PATH_MAX+1] = "";
	for(unsigned int i=0; i<_dyld_image_count(); ++i)
	{
		const char *dylibPath = _dyld_get_image_name(i);
		char *pos;
		if ( (pos = strstr(dylibPath, "/Vuo.framework/")) )
		{
			strncpy(frameworkPath, dylibPath, pos-dylibPath);
			break;
		}
	}

	// Derive the path of "Licenses" directory.
	char licensesPath[PATH_MAX+1];
	strncpy(licensesPath, frameworkPath, PATH_MAX);
	strncat(licensesPath, "/Vuo.framework/Versions/" VUO_VERSION_STRING "/Documentation/Licenses", PATH_MAX);


	bool foundLicenses = false;
	if (access(licensesPath, 0) == 0)
	{
		DIR *dirp = opendir(licensesPath);
		struct dirent *dp;
		while ((dp = readdir(dirp)) != NULL)
		{
			if (dp->d_name[0] == '.')
				continue;

			char *nameWithoutExtension = strdup(dp->d_name);
			nameWithoutExtension[strlen(nameWithoutExtension) - 4] = 0;
			[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[[NSString stringWithFormat:@"<h2>%s</h2>", nameWithoutExtension] dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];
			free(nameWithoutExtension);

			char licensePath[strlen(licensesPath) + dp->d_namlen + 2];
			strcpy(licensePath, licensesPath);
			strcat(licensePath, "/");
			strcat(licensePath, dp->d_name);

			NSMutableData *mdata = [NSMutableData new];
			[mdata appendData:[[NSString stringWithFormat:@"<pre>"] dataUsingEncoding:NSUTF8StringEncoding]];
			[mdata appendData:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:licensePath]]];
			[mdata appendData:[[NSString stringWithFormat:@"</pre><br>"] dataUsingEncoding:NSUTF8StringEncoding]];
			[mas appendAttributedString:[[[NSAttributedString new]
										   initWithHTML:mdata
										   options:[NSDictionary dictionaryWithObject:[NSNumber numberWithInteger:NSUTF8StringEncoding] forKey:NSCharacterEncodingDocumentOption]
										   documentAttributes:nil] autorelease]];
			[mdata release];

			foundLicenses = true;
		}
		closedir(dirp);
	}

	if (!foundLicenses)
		[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[@"<p>(No license information found.)</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];

	return [mas autorelease];
}

@end

/**
 * Helper for @ref VuoApp_init.
 *
 * @threadMain
 */
static void VuoApp_initNSApplication(void)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];

	// http://stackoverflow.com/a/11010614/238387
	NSApplication *app = [NSApplication sharedApplication];

	if (![app delegate])
		[app setDelegate:[VuoApplicationDelegate new]];

	// When there's no host app present,
	// create the default menu with the About and Quit menu items,
	// to be overridden if/when any windows get focus.
	VuoApp_setMenuItems(NULL);

	// Show the app in the dock (since non-NIB apps are hidden by default).
	[app setActivationPolicy:NSApplicationActivationPolicyRegular];

	// Stop bouncing in the dock.
	[app finishLaunching];

	// Force the app to the front, so you can see the composition's window when running from Vuo Editor.
	/// @todo Have Vuo Editor activate the composition's process instead, using `-[NSRunningApplication activateWithOptions:NSApplicationActivateAllWindows]`, so Vuo apps behave more like typical Mac apps when launched from, say, Finder or Terminal.
	[app activateIgnoringOtherApps:YES];

	VuoEventLoop_switchToAppMode();

	[pool drain];
}

#ifndef DOXYGEN
void VuoApp_fini(void);
#endif

/**
 * Creates an NSApplication instance (if one doesn't already exist).
 *
 * This causes the process's icon to appear in the dock.
 *
 * VuoWindow methods call this automatically as needed,
 * so you only need to call this if you need an NSApplication without using VuoWindow
 * (like, for example, @ref VuoAudioFile does).
 *
 * @threadAny
 */
void VuoApp_init(void)
{
	if (NSApp)
		return;

	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		VuoApp_executeOnMainThread(^{
			VuoApp_initNSApplication();
		});
		VuoAddCompositionFiniCallback(VuoApp_fini);
	});
}

/**
 * Helper for @ref VuoApp_fini.
 *
 * @threadMain
 */
static void VuoApp_finiWindows(void)
{
	// Stop any window recordings currently in progress.
	// This prompts the user for the save destination,
	// so make sure these complete before shutting the composition down.
	SEL stopRecording = @selector(stopRecording);
	for (NSWindow *window in [NSApp windows])
		if ([window respondsToSelector:stopRecording])
			[window performSelector:stopRecording];

	// Explicitly restore the standard mouse cursor,
	// since occasionally the system doesn't do it automatically.
	// https://b33p.net/kosada/node/10769
	[[NSCursor arrowCursor] set];
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
	if (!NSApp)
		return;

	if (VuoApp_isMainThread())
		VuoApp_finiWindows();
	else
	{
		VUOLOG_PROFILE_BEGIN(mainQueue);
		dispatch_sync(dispatch_get_main_queue(), ^{
			VUOLOG_PROFILE_END(mainQueue);
			VuoApp_finiWindows();
		});
	}
}

void VuoWindowText_destroy(VuoWindowText w);

/**
 * Creates and displays a window containing a text edit widget.
 *
 * Creates a new NSApplication instance if one did not already exist.
 *
 * @threadNoMain
 */
VuoWindowText VuoWindowText_make(void)
{
	__block VuoWindowTextInternal *window = NULL;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  VuoApp_init();
					  window = [[VuoWindowTextInternal alloc] init];
					  [window makeKeyAndOrderFront:NSApp];
				   });
	VuoRegister(window, VuoWindowText_destroy);
	return (VuoWindowText *)window;
}

/**
 * Sets up the window to call the trigger functions when its text is edited.
 *
 * @threadNoMain
 */
void VuoWindowText_enableTriggers
(
		VuoWindowText w,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window enableTriggersWithTypedLine:typedLine typedWord:typedWord typedCharacter:typedCharacter];
				  });
}

/**
 * Stops the window from calling trigger functions when its text is edited.
 *
 * @threadNoMain
 */
void VuoWindowText_disableTriggers(VuoWindowText w)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window disableTriggers];
				  });
}

/**
 * Appends text and a linebreak to the text edit widget in the window.
 *
 * @threadAny
 */
void VuoWindowText_appendLine(VuoWindowText vw, const char *text)
{
	if (!text)
		return;

	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	char *textCopy = strdup(text);  // ... in case the caller frees text before the asynchronous block uses it.
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
					   [window appendLine:textCopy];
					   free(textCopy);
				   });
}

/**
 * Closes the window.
 *
 * @threadNoMain
 */
void VuoWindowText_close(VuoWindowText vw)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window close];
				  });
}

/**
 * Deallocates the window.
 *
 * @threadNoMain
 */
void VuoWindowText_destroy(VuoWindowText vw)
{
	VuoWindowTextInternal *window = (VuoWindowTextInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window release];
				  });
}


void VuoWindowOpenGl_destroy(VuoWindowOpenGl w);

/**
 * Creates and displays a window containing an OpenGL view.
 *
 * Creates a new NSApplication instance if one did not already exist.
 *
 * @threadNoMain
 */
VuoWindowOpenGl VuoWindowOpenGl_make
(
		bool useDepthBuffer,
		void (*initCallback)(VuoGlContext glContext, float backingScaleFactor, void *),
		void (*updateBackingCallback)(VuoGlContext glContext, void *, float backingScaleFactor),
		void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int, unsigned int),
		void (*drawCallback)(VuoGlContext glContext, void *),
		void *context
)
{
	__block VuoWindowOpenGLInternal *window = NULL;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  VuoApp_init();
					  window = [[VuoWindowOpenGLInternal alloc] initWithDepthBuffer:useDepthBuffer
						  initCallback:initCallback
						  updateBackingCallback:updateBackingCallback
						  resizeCallback:resizeCallback
						  drawCallback:drawCallback
						  drawContext:context];
					  [window makeKeyAndOrderFront:nil];
				  });
	VuoRegister(window, VuoWindowOpenGl_destroy);
	return (VuoWindowOpenGl *)window;
}

/**
 * Sets up the window to call the trigger functions when events occur.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_enableTriggers
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal)
)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window enableShowedWindowTrigger:showedWindow requestedFrameTrigger:requestedFrame];
				  });
}

/**
 * Stops the window from calling trigger functions when events occur.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window disableTriggers];
				  });
}

/**
 * Schedules the specified window to be redrawn.
 *
 * @threadAny
 */
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	[window scheduleRedraw];
}

/**
 * Applies a list of properties to a window.
 *
 * @threadAny
 */
void VuoWindowOpenGl_setProperties(VuoWindowOpenGl w, VuoList_VuoWindowProperty properties)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window setProperties:properties];
				  });
}

/**
 * Executes the specifed block on the window's OpenGL context, then returns.
 * Ensures that nobody else is using the OpenGL context at that time
 * (by synchronizing with the window's @c drawQueue).
 *
 * @threadAny
 */
void VuoWindowOpenGl_executeWithWindowContext(VuoWindowOpenGl w, void (^blockToExecute)(VuoGlContext glContext))
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	[window executeWithWindowContext:blockToExecute];
}

/**
 * Sets the window's current and preferred aspect ratio.
 *
 * If necessary, the window is resized to match the specified aspect ratio.
 * After calling this method, when the user resizes the window, its height is adjusted to match the specified aspect ratio.
 */
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
					   [window setAspectRatioToWidth:pixelsWide height:pixelsHigh];
				   });
}

/**
 * Removes the aspect ratio constraint set by @ref VuoWindowOpenGl_setAspectRatio.
 *
 * After calling this method, the user will be able to resizes the window freely.
 */
void VuoWindowOpenGl_unlockAspectRatio(VuoWindowOpenGl w)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)w;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_async(dispatch_get_main_queue(), ^{
					   VUOLOG_PROFILE_END(mainQueue);
					   [window unlockAspectRatio];
				   });
}

/**
 * Closes the window.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_close(VuoWindowOpenGl vw)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window close];
				  });
}

/**
 * Deallocates a @c VuoWindowOpenGl window.
 *
 * @threadNoMain
 */
void VuoWindowOpenGl_destroy(VuoWindowOpenGl vw)
{
	VuoWindowOpenGLInternal *window = (VuoWindowOpenGLInternal *)vw;
	VUOLOG_PROFILE_BEGIN(mainQueue);
	dispatch_sync(dispatch_get_main_queue(), ^{
					  VUOLOG_PROFILE_END(mainQueue);
					  [window release];

					  // -[NSWindow release] apparently doesn't actually dealloc the window; it adds the dealloc to the NSRunLoop.
					  // Make sure the dealloc actually happens now.
					  VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
				  });
}

/**
 * Replaces the top-level menus in the menu bar, except for application-wide menus,
 * with the given menus.
 *
 * `items` should be an `NSArray` of `NSMenuItem`s.
 *
 * Returns the old menu.
 */
void *VuoApp_setMenuItems(void *items)
{
	NSMenu *oldMenu = [NSApp mainMenu];

	NSMenu *menubar = [[NSMenu new] autorelease];

	// Application menu
	{
		NSMenu *appMenu = [[NSMenu new] autorelease];
		NSString *appName = [[NSProcessInfo processInfo] processName];

		NSString *aboutTitle = [@"About " stringByAppendingString:appName];
		NSMenuItem *aboutMenuItem = [[[NSMenuItem alloc] initWithTitle:aboutTitle action:@selector(displayAboutPanel:) keyEquivalent:@""] autorelease];

		static VuoApplicationAboutDialog *aboutDialog;
		static dispatch_once_t aboutDialogInitialized = 0;
		dispatch_once(&aboutDialogInitialized, ^{
			aboutDialog = [[VuoApplicationAboutDialog alloc] init];
		});
		[aboutMenuItem setTarget:aboutDialog];

		[appMenu addItem:aboutMenuItem];

		[appMenu addItem:[NSMenuItem separatorItem]];

		NSString *quitTitle = [@"Quit " stringByAppendingString:appName];
		NSMenuItem *quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
		[appMenu addItem:quitMenuItem];

		NSMenuItem *appMenuItem = [[NSMenuItem new] autorelease];
		[appMenuItem setSubmenu:appMenu];

		[menubar addItem:appMenuItem];
	}

	// Custom menus
	if (items)
	{
		NSArray *itemsArray = (NSArray *)items;
		for (NSMenuItem *item in itemsArray)
			[menubar addItem:item];
	}

	[NSApp setMainMenu:menubar];

	return oldMenu;
}

/**
 * Replaces the top-level menus in the menu bar with the given menu.
 *
 * `menu` should be an `NSMenu`.
 */
void VuoApp_setMenu(void *menu)
{
	[NSApp setMainMenu:menu];
}
