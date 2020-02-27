/**
 * @file
 * Implementations of functions available to modules (nodes, types, libraries).
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <dlfcn.h>
#include <limits.h>
#include <mach-o/dyld.h>

#include "VuoCompositionState.h"

#include "module.h"

const int VuoGraphicsWindowDefaultWidth = 1024;

/**
 * Wrapper for @ref vuoCopyCompositionStateFromThreadLocalStorage().
 */
static void * copyCompositionStateFromThreadLocalStorage(void)
{
	typedef void * (*funcType)(void);
	funcType vuoCopyCompositionStateFromThreadLocalStorage = (funcType) dlsym(RTLD_SELF, "vuoCopyCompositionStateFromThreadLocalStorage");
	if (!vuoCopyCompositionStateFromThreadLocalStorage)
		vuoCopyCompositionStateFromThreadLocalStorage = (funcType) dlsym(RTLD_DEFAULT, "vuoCopyCompositionStateFromThreadLocalStorage");

	return vuoCopyCompositionStateFromThreadLocalStorage();
}

/**
 * Ensures the bulk of @ref VuoGetWorkingDirectory only executes once.
 *
 * This is a public symbol so TestVuoUrl can reset it.
 */
dispatch_once_t VuoGetWorkingDirectoryOnce = 0;

/**
 * Returns the POSIX path that this composition should use to resolve relative paths.
 *
 * The returned string remains owned by this function; the caller should not modify or free it.
 */
const char *VuoGetWorkingDirectory(void)
{
	static char *workingDirectoryResult = NULL;
	dispatch_once(&VuoGetWorkingDirectoryOnce, ^{
		typedef char * (*vuoGetWorkingDirectoryType)(struct VuoCompositionState *);
		vuoGetWorkingDirectoryType vuoGetWorkingDirectory = (vuoGetWorkingDirectoryType) dlsym(RTLD_SELF, "vuoGetWorkingDirectory");
		if (!vuoGetWorkingDirectory)
			vuoGetWorkingDirectory = (vuoGetWorkingDirectoryType) dlsym(RTLD_DEFAULT, "vuoGetWorkingDirectory");

		if (vuoGetWorkingDirectory)
		{
			void *compositionState = copyCompositionStateFromThreadLocalStorage();
			workingDirectoryResult = vuoGetWorkingDirectory((struct VuoCompositionState *)compositionState);
			free(compositionState);
		}
		else
		{
			// Keep consistent with VuoRuntimePersistentState::getCurrentWorkingDirectory().
			char currentWorkingDirectory[PATH_MAX+1];
			getcwd(currentWorkingDirectory, PATH_MAX+1);
			workingDirectoryResult = strdup(currentWorkingDirectory);
		}
	});
	return workingDirectoryResult;
}

/**
 * Returns the path of the folder containing `Vuo.framework`.
 *
 * See also @ref VuoFileUtilities::getVuoFrameworkPath.
 */
const char *VuoGetFrameworkPath(void)
{
	static char frameworkPath[PATH_MAX+1] = "";
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		uint32_t imageCount = _dyld_image_count();
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			const char *dylibPath = _dyld_get_image_name(i);
			char *pos;
			if ( (pos = strstr(dylibPath, "/Vuo.framework/")) )
			{
				strncpy(frameworkPath, dylibPath, pos-dylibPath);
				break;
			}
		}
	});
	return frameworkPath;
}

/**
 * Returns the path of the folder containing `VuoRunner.framework`.
 *
 * See also @ref VuoFileUtilities::getVuoRunnerFrameworkPath.
 * @version200New
 */
const char *VuoGetRunnerFrameworkPath(void)
{
	static char frameworkPath[PATH_MAX+1] = "";
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		uint32_t imageCount = _dyld_image_count();
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			const char *dylibPath = _dyld_get_image_name(i);
			char *pos;
			if ( (pos = strstr(dylibPath, "/VuoRunner.framework/")) )
			{
				strncpy(frameworkPath, dylibPath, pos-dylibPath);
				break;
			}
		}
	});
	return frameworkPath;
}

pid_t VuoGetRunnerPid(void)
{
	typedef pid_t (*vuoGetRunnerPidType)(struct VuoCompositionState *);
	vuoGetRunnerPidType vuoGetRunnerPid = (vuoGetRunnerPidType) dlsym(RTLD_SELF, "vuoGetRunnerPid");
	if (!vuoGetRunnerPid)
		vuoGetRunnerPid = (vuoGetRunnerPidType) dlsym(RTLD_DEFAULT, "vuoGetRunnerPid");

	void *compositionState = copyCompositionStateFromThreadLocalStorage();
	pid_t runnerPid = vuoGetRunnerPid((struct VuoCompositionState *)compositionState);
	free(compositionState);
	return runnerPid;
}

void VuoStopComposition(void)
{
	typedef void (*vuoStopCompositionType)(struct VuoCompositionState *);
	vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_SELF, "vuoStopComposition");
	if (!vuoStopComposition)
		vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_DEFAULT, "vuoStopComposition");

	void *compositionState = copyCompositionStateFromThreadLocalStorage();
	vuoStopComposition((struct VuoCompositionState *)compositionState);
	free(compositionState);
}

void VuoStopCurrentComposition(void)
{
	typedef void (*vuoStopCompositionType)(struct VuoCompositionState *);
	vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_SELF, "vuoStopComposition");
	if (!vuoStopComposition)
		vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_DEFAULT, "vuoStopComposition");

	vuoStopComposition(NULL);
}

void VuoAddCompositionFiniCallback(VuoCompositionFiniCallback fini)
{
	typedef void (*vuoAddCompositionFiniCallbackType)(struct VuoCompositionState *, VuoCompositionFiniCallback);
	vuoAddCompositionFiniCallbackType vuoAddCompositionFiniCallback = (vuoAddCompositionFiniCallbackType) dlsym(RTLD_SELF, "vuoAddCompositionFiniCallback");
	if (!vuoAddCompositionFiniCallback)
		vuoAddCompositionFiniCallback = (vuoAddCompositionFiniCallbackType) dlsym(RTLD_DEFAULT, "vuoAddCompositionFiniCallback");

	if (vuoAddCompositionFiniCallback)
	{
		void *compositionState = copyCompositionStateFromThreadLocalStorage();
		vuoAddCompositionFiniCallback((struct VuoCompositionState *)compositionState, fini);
		free(compositionState);
	}
}

void VuoDisableTermination(void)
{
	typedef void (*vuoDisableTerminationType)(struct VuoCompositionState *);
	vuoDisableTerminationType vuoDisableTermination = (vuoDisableTerminationType) dlsym(RTLD_SELF, "vuoDisableTermination");
	if (!vuoDisableTermination)
		vuoDisableTermination = (vuoDisableTerminationType) dlsym(RTLD_DEFAULT, "vuoDisableTermination");

	void *compositionState = copyCompositionStateFromThreadLocalStorage();
	vuoDisableTermination((struct VuoCompositionState *)compositionState);
	free(compositionState);
}

void VuoEnableTermination(void)
{
	typedef void (*vuoEnableTerminationType)(struct VuoCompositionState *);
	vuoEnableTerminationType vuoEnableTermination = (vuoEnableTerminationType) dlsym(RTLD_SELF, "vuoEnableTermination");
	if (!vuoEnableTermination)
		vuoEnableTermination = (vuoEnableTerminationType) dlsym(RTLD_DEFAULT, "vuoEnableTermination");

	void *compositionState = copyCompositionStateFromThreadLocalStorage();
	vuoEnableTermination((struct VuoCompositionState *)compositionState);
	free(compositionState);
}

bool VuoShouldShowSplashWindow(void)
{
	typedef bool (*vuoShouldShowSplashWindowType)(void);
	vuoShouldShowSplashWindowType vuoShouldShowSplashWindow = (vuoShouldShowSplashWindowType) dlsym(RTLD_DEFAULT, "vuoShouldShowSplashWindow");
	if (!vuoShouldShowSplashWindow)
	{
//		VLog("Warning: Couldn't find symbol VuoShouldShowSplashWindow.");
		return false;
	}
//	VLog("shouldShowSplashWindow = %d",vuoShouldShowSplashWindow());
	return vuoShouldShowSplashWindow();
}

/**
 * Returns true if the current (runtime) processor supports Intel AVX2.
 */
bool VuoProcessorSupportsAVX2(void)
{
	// Based on https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
	uint32_t eax = 7, ebx, ecx = 0, edx;
	__asm__ ( "cpuid" : "+b" (ebx), "+a" (eax), "+c" (ecx), "=d" (edx) );
	uint32_t avx2_mask = 1 << 5;
	return (ebx & avx2_mask) == avx2_mask;
}
