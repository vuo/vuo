/**
 * @file
 * Implementations of functions available to modules (nodes, types, libraries).
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
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

static pthread_once_t VuoFrameworkPathsOnce = PTHREAD_ONCE_INIT;  ///< Ensures the framework paths are initialized just once.
static bool VuoFrameworkPathsInitialMatchCompleted = false;       ///< Stops checking dylibs after the initial scan.
static char *VuoFrameworkPath = NULL;                             ///< The absolute path of Vuo.framework.
static char *VuoRunnerFrameworkPath = NULL;                       ///< The absolute path of VuoRunner.framework.

/**
 * Helper for @ref VuoInitializeFrameworkPaths.
 *
 * This needs to be kept in sync with `VuoFileUtilities::dylibLoaded()`.
 */
void VuoFrameworkPaths_dylibLoaded(const struct mach_header *mh32, intptr_t vmaddr_slide)
{
	if (VuoFrameworkPathsInitialMatchCompleted)
		return;

	const struct mach_header_64 *mh = (const struct mach_header_64 *)mh32;

	// Ignore system libraries.
	if (mh->flags & MH_DYLIB_IN_CACHE)
		return;

	// Get the file path of the current dylib.
	Dl_info info = {"", NULL, "", NULL};
	dladdr((void *)vmaddr_slide, &info);

	// Check whether it's one of the dylibs we're looking for.

	char * (^getMatchingPath)(const char *) = ^(const char *fragment) {
		const char *found = strstr(info.dli_fname, fragment);
		if (found)
		{
			char *pathCandidate = strndup(info.dli_fname, found - info.dli_fname + strlen(fragment));
			if (access(pathCandidate, 0) == 0)
				return pathCandidate;
			else
				free(pathCandidate);
		}
		return (char *)NULL;
	};

	char *possibleVuoFramework = getMatchingPath("/Vuo.framework");
	if (possibleVuoFramework)
		VuoFrameworkPath = possibleVuoFramework;

	char *possibleVuoRunnerFramework = getMatchingPath("/VuoRunner.framework");
	if (possibleVuoRunnerFramework)
		VuoRunnerFrameworkPath = possibleVuoRunnerFramework;
}

/**
 * Helper for @ref VuoGetFrameworkPath and @ref VuoGetRunnerFrameworkPath.
 */
void VuoInitializeFrameworkPaths(void)
{
	// Check whether Vuo.framework is in the list of loaded dynamic libraries.
	_dyld_register_func_for_add_image(&VuoFrameworkPaths_dylibLoaded);
	VuoFrameworkPathsInitialMatchCompleted = true;
	// The above function invokes the callback for each already-loaded dylib, then returns.

	if (VuoFrameworkPath && !VuoRunnerFrameworkPath)
	{
		// Check for VuoRunner.framework alongside Vuo.framework.
		const char *runnerSuffix = "/../VuoRunner.framework";
		char *pathCandidate = malloc(strlen(VuoFrameworkPath) + strlen(runnerSuffix) + 1);
		strcpy(pathCandidate, VuoFrameworkPath);
		strcat(pathCandidate, runnerSuffix);
		if (access(pathCandidate, 0) == 0)
			VuoRunnerFrameworkPath = pathCandidate;
		else
			free(pathCandidate);
	}
}

/**
 * Returns the absolute path of Vuo.framework
 * (including the `Vuo.framework` directory name without a trailing slash),
 * or an empty string if Vuo.framework cannot be located.
 *
 * The returned string remains owned by libmodule;
 * the caller should not modify or free it.
 *
 * See also @ref VuoFileUtilities::getVuoFrameworkPath.
 */
const char *VuoGetFrameworkPath(void)
{
	pthread_once(&VuoFrameworkPathsOnce, VuoInitializeFrameworkPaths);
	return VuoFrameworkPath;
}

/**
 * Returns the absolute path of VuoRunner.framework
 * (including the `VuoRunner.framework` directory name without a trailing slash),
 * or an empty string if VuoRunner.framework cannot be located.
 *
 * The returned string remains owned by libmodule;
 * the caller should not modify or free it.
 *
 * See also @ref VuoFileUtilities::getVuoRunnerFrameworkPath.
 * @version200New
 */
const char *VuoGetRunnerFrameworkPath(void)
{
	pthread_once(&VuoFrameworkPathsOnce, VuoInitializeFrameworkPaths);
	return VuoRunnerFrameworkPath;
}

/**
 * Returns the absolute path of Vuo.framework (preferably)
 * or VuoRunner.framework (if Vuo.framework is unavailable),
 * or NULL (if neither is available).
 *
 * The returned string remains owned by libmodule;
 * the caller should not modify or free it.
 */
const char *VuoGetFrameworkOrRunnerFrameworkPath(void)
{
	pthread_once(&VuoFrameworkPathsOnce, VuoInitializeFrameworkPaths);
	if (VuoFrameworkPath)
		return VuoFrameworkPath;
	else
		return VuoRunnerFrameworkPath;
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

bool VuoIsPro(void)
{
	static bool proEnabledResult = false;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		bool *proEnabled = (bool *) dlsym(RTLD_SELF, "VuoProEnabled");
		if (!proEnabled)
			proEnabled = (bool *) dlsym(RTLD_DEFAULT, "VuoProEnabled");
		if (!proEnabled)
			return;
		proEnabledResult = *proEnabled;
	});
	return proEnabledResult;
}

/**
 * Returns true if the current (runtime) processor supports Intel AVX2.
 */
bool VuoProcessorSupportsAVX2(void)
{
#if __x86_64__
	// Based on https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
	uint32_t eax = 7, ebx, ecx = 0, edx;
	__asm__ ( "cpuid" : "+b" (ebx), "+a" (eax), "+c" (ecx), "=d" (edx) );
	uint32_t avx2_mask = 1 << 5;
	return (ebx & avx2_mask) == avx2_mask;
#else
	return false;
#endif
}
