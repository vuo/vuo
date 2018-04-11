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
 * Wrapper for @ref vuoGetCompositionStateFromThreadLocalStorage().
 */
static void * getCompositionStateFromThreadLocalStorage(void)
{
	typedef void * (*funcType)(void);
	funcType vuoGetCompositionStateFromThreadLocalStorage = (funcType) dlsym(RTLD_SELF, "vuoGetCompositionStateFromThreadLocalStorage");
	if (!vuoGetCompositionStateFromThreadLocalStorage)
		vuoGetCompositionStateFromThreadLocalStorage = (funcType) dlsym(RTLD_DEFAULT, "vuoGetCompositionStateFromThreadLocalStorage");

	return vuoGetCompositionStateFromThreadLocalStorage();
}

char * VuoGetWorkingDirectory(void)
{
	typedef char * (*vuoGetWorkingDirectoryType)(struct VuoCompositionState *);
	vuoGetWorkingDirectoryType vuoGetWorkingDirectory = (vuoGetWorkingDirectoryType) dlsym(RTLD_SELF, "vuoGetWorkingDirectory");
	if (!vuoGetWorkingDirectory)
		vuoGetWorkingDirectory = (vuoGetWorkingDirectoryType) dlsym(RTLD_DEFAULT, "vuoGetWorkingDirectory");

	if (vuoGetWorkingDirectory)
	{
		void *compositionState = getCompositionStateFromThreadLocalStorage();
		return vuoGetWorkingDirectory((struct VuoCompositionState *)compositionState);
	}
	else
	{
		// Keep consistent with VuoRuntimePersistentState::getCurrentWorkingDirectory().
		char currentWorkingDirectory[PATH_MAX+1];
		getcwd(currentWorkingDirectory, PATH_MAX+1);
		return strdup(currentWorkingDirectory);
	}
}

pid_t VuoGetRunnerPid(void)
{
	typedef pid_t (*vuoGetRunnerPidType)(struct VuoCompositionState *);
	vuoGetRunnerPidType vuoGetRunnerPid = (vuoGetRunnerPidType) dlsym(RTLD_SELF, "vuoGetRunnerPid");
	if (!vuoGetRunnerPid)
		vuoGetRunnerPid = (vuoGetRunnerPidType) dlsym(RTLD_DEFAULT, "vuoGetRunnerPid");

	void *compositionState = getCompositionStateFromThreadLocalStorage();

	return vuoGetRunnerPid((struct VuoCompositionState *)compositionState);
}

void VuoStopComposition(void)
{
	typedef void (*vuoStopCompositionType)(struct VuoCompositionState *);
	vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_SELF, "vuoStopComposition");
	if (!vuoStopComposition)
		vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_DEFAULT, "vuoStopComposition");

	void *compositionState = getCompositionStateFromThreadLocalStorage();

	vuoStopComposition((struct VuoCompositionState *)compositionState);
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

	void *compositionState = getCompositionStateFromThreadLocalStorage();

	vuoAddCompositionFiniCallback((struct VuoCompositionState *)compositionState, fini);
}

void VuoDisableTermination(void)
{
	typedef void (*vuoDisableTerminationType)(struct VuoCompositionState *);
	vuoDisableTerminationType vuoDisableTermination = (vuoDisableTerminationType) dlsym(RTLD_SELF, "vuoDisableTermination");
	if (!vuoDisableTermination)
		vuoDisableTermination = (vuoDisableTerminationType) dlsym(RTLD_DEFAULT, "vuoDisableTermination");

	void *compositionState = getCompositionStateFromThreadLocalStorage();

	vuoDisableTermination((struct VuoCompositionState *)compositionState);
}

void VuoEnableTermination(void)
{
	typedef void (*vuoEnableTerminationType)(struct VuoCompositionState *);
	vuoEnableTerminationType vuoEnableTermination = (vuoEnableTerminationType) dlsym(RTLD_SELF, "vuoEnableTermination");
	if (!vuoEnableTermination)
		vuoEnableTermination = (vuoEnableTerminationType) dlsym(RTLD_DEFAULT, "vuoEnableTermination");

	void *compositionState = getCompositionStateFromThreadLocalStorage();

	vuoEnableTermination((struct VuoCompositionState *)compositionState);
}

bool VuoIsTrial(void)
{
	bool **trialRestrictionsEnabled = (bool **) dlsym(RTLD_SELF, "VuoTrialRestrictionsEnabled");
	if (!trialRestrictionsEnabled)
		trialRestrictionsEnabled = (bool **) dlsym(RTLD_DEFAULT, "VuoTrialRestrictionsEnabled");
	if (!trialRestrictionsEnabled)
	{
//		VLog("Warning: Couldn't find symbol VuoTrialRestrictionsEnabled.");
		return true;
	}
	if (!*trialRestrictionsEnabled)
	{
//		VLog("Warning: VuoTrialRestrictionsEnabled isn't allocated.");
		return true;
	}
//	VLog("trialRestrictionsEnabled = %d",**trialRestrictionsEnabled);
	return **trialRestrictionsEnabled;
}

bool VuoIsPro(void)
{
	bool *proEnabled = (bool *) dlsym(RTLD_SELF, "VuoProEnabled");
	if (!proEnabled)
		proEnabled = (bool *) dlsym(RTLD_DEFAULT, "VuoProEnabled");
	if (!proEnabled)
	{
//		VLog("Warning: Couldn't find symbol VuoProEnabled.");
		return true;
	}
	return *proEnabled;
}
