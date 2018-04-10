/**
 * @file
 * VuoApp implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"

#include "VuoApp.h"

/// Disable NS_RETURNS_INNER_POINTER (new in Mac OS 10.10's system headers), since Clang 3.2 doesn't support it.
/// https://b33p.net/kosada/node/9140
#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>
#undef NS_RETURNS_INNER_POINTER

#include <pthread.h>
#include <libproc.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoApp",
					"dependencies" : [
						"AppKit.framework",
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
		if (strncmp(*dylibPath, "/tmp/", 5) == 0)
		{
			char *name = strdup(*dylibPath + 5);
			name[strlen(name) - strlen("-XXXXXX.dylib")] = 0;

			if (strcmp(name, "VuoComposition") == 0)
			{
				free(name);
				return strdup("Vuo Composition");
			}

			return name;
		}

	pid_t *runnerPid = (pid_t *)dlsym(RTLD_SELF, "VuoApp_runnerPid");
	if (!runnerPid)
		runnerPid = (pid_t *)dlsym(RTLD_DEFAULT, "VuoApp_runnerPid");
	if (runnerPid && *runnerPid)
	{
		char *runnerName = (char *)malloc(2*MAXCOMLEN);
		proc_name(*runnerPid, runnerName, 2*MAXCOMLEN);
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
