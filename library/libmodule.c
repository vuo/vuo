/**
 * @file
 * Implementations of functions available to modules (nodes, types, libraries).
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dlfcn.h>
#include <limits.h>
#include "VuoCompositionState.h"

#include "module.h"

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

bool VuoIsTrial(void)
{
	static bool trialRestrictionsEnabledResult = true;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		bool **trialRestrictionsEnabled = (bool **) dlsym(RTLD_SELF, "VuoTrialRestrictionsEnabled");
		if (!trialRestrictionsEnabled)
			trialRestrictionsEnabled = (bool **) dlsym(RTLD_DEFAULT, "VuoTrialRestrictionsEnabled");
		if (!trialRestrictionsEnabled)
		{
//			VLog("Warning: Couldn't find symbol VuoTrialRestrictionsEnabled.");
			return;
		}
		if (!*trialRestrictionsEnabled)
		{
//			VLog("Warning: VuoTrialRestrictionsEnabled isn't allocated.");
			return;
		}
//		VLog("trialRestrictionsEnabled = %d",**trialRestrictionsEnabled);
		trialRestrictionsEnabledResult = **trialRestrictionsEnabled;
	});
	return trialRestrictionsEnabledResult;
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
		{
//			VLog("Warning: Couldn't find symbol VuoProEnabled.");
			return;
		}
		proEnabledResult = *proEnabled;
	});
	return proEnabledResult;
}
