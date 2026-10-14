/**
 * @file
 * VuoRuntimePersistentState implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRuntimePersistentState.hh"

#include "VuoCompositionDiff.hh"
#include "VuoNodeRegistry.hh"
#include "VuoNodeSynchronization.hh"
#include "VuoRuntimeCommunicator.hh"
#include "VuoRuntimeState.hh"
#include "VuoThreadManager.hh"

/**
 * Constructor.
 */
VuoRuntimePersistentState::VuoRuntimePersistentState(const char *workingDirectory)
{
	this->workingDirectory = (strlen(workingDirectory) > 0 ? workingDirectory : getCurrentWorkingDirectory());

	lastEventId = 0;

	_isStopRequested = false;

	triggerWorkersScheduled = dispatch_group_create();

	compositionDiff = new VuoCompositionDiff();
	nodeRegistry = new VuoNodeRegistry(this);
	communicator = new VuoRuntimeCommunicator(this);
	threadManager = new VuoThreadManager();
	nodeSynchronization = new VuoNodeSynchronization();
}

/**
 * Destructor
 */
VuoRuntimePersistentState::~VuoRuntimePersistentState(void)
{
	dispatch_release(triggerWorkersScheduled);

	delete compositionDiff;
	delete nodeRegistry;
	delete communicator;
	delete threadManager;
	delete nodeSynchronization;
}

/**
 * Returns the current working directory of the process.
 */
string VuoRuntimePersistentState::getCurrentWorkingDirectory(void)
{
	// See explanation in VuoFileUtilities::getWorkingDirectory().
	// Keep consistent with VuoGetWorkingDirectory() and VuoFileUtilities::getWorkingDirectory().

	const int oexec = 0x40000000;
	int fd = open(".", oexec | O_DIRECTORY);
	if (fd > 0)
	{
		char filePath[PATH_MAX] = "(unset)";
		int ret = fcntl(fd, F_GETPATH, filePath);
		if (ret == 0 && strcmp(filePath, "(unset)") != 0)
			return filePath;
		else
		{
			VUserLog("Error: Couldn't get the process's current directory name.  Relative pathnames may not be resolved correctly.");
			return "/";
		}
	}
	else
	{
		VUserLog("Error: Couldn't get the process's current directory.  Relative pathnames may not be resolved correctly.");
		return "/";
	}
}

/**
 * Returns the directory that nodes should use to resolve relative paths.
 */
char * VuoRuntimePersistentState::getWorkingDirectory(void)
{
	return strdup(workingDirectory.c_str());
}

/**
 * Returns a unique event ID.
 */
unsigned long VuoRuntimePersistentState::getNextEventId(void)
{
	return __sync_add_and_fetch(&lastEventId, 1);
}

/**
 * Returns true if something in the composition (nodes/libraries/runtime) has requested the composition to stop.
 */
bool VuoRuntimePersistentState::isStopRequested(void)
{
	return _isStopRequested;
}

/**
 * Sets a flag indicating that something in the composition (nodes/libraries/runtime) has requested the composition to stop.
 */
void VuoRuntimePersistentState::setStopRequested(bool isStopRequested)
{
	_isStopRequested = isStopRequested;
}

/**
 * Returns the dispatch group that tracks trigger workers that have been scheduled but have not yet launched an event
 * into the composition.
 */
dispatch_group_t VuoRuntimePersistentState::getTriggerWorkersScheduled(void)
{
	return triggerWorkersScheduled;
}

extern "C"
{

/**
 * C wrapper for VuoRuntimePersistentState::getNextEventId().
 */
unsigned long vuoGetNextEventId(VuoCompositionState *compositionState)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->getNextEventId();
}

/**
 * C wrapper for VuoRuntimePersistentState::getTriggerWorkersScheduled().
 */
dispatch_group_t vuoGetTriggerWorkersScheduled(VuoCompositionState *compositionState)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->getTriggerWorkersScheduled();
}

}
