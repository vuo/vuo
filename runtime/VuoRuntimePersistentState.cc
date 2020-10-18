/**
 * @file
 * VuoRuntimePersistentState implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
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

	finiCallbackQueue = dispatch_queue_create("org.vuo.runtime.finiCallback", NULL);

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
	dispatch_release(finiCallbackQueue);
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
	// Keep consistent with VuoGetWorkingDirectory().
	char currentWorkingDirectory[PATH_MAX+1];
	getcwd(currentWorkingDirectory, PATH_MAX+1);
	return currentWorkingDirectory;
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
 * Registers a callback to be invoked when the composition is shutting down, after all nodes have been fini'ed.
 */
void VuoRuntimePersistentState::addFiniCallback(VuoCompositionFiniCallback fini)
{
	dispatch_sync(finiCallbackQueue, ^{
					  finiCallbacks.push_back(fini);
				  });
}

/**
 * Calls all fini callbacks that have been registered.
 */
void VuoRuntimePersistentState::callFiniCallbacks(void)
{
	dispatch_sync(finiCallbackQueue, ^{
					  VuoCompositionState compositionState = { (void *)runtimeState, "" };
					  vuoAddCompositionStateToThreadLocalStorage(&compositionState);

					  for (vector<VuoCompositionFiniCallback>::iterator i = finiCallbacks.begin(); i != finiCallbacks.end(); ++i)
						  (*i)();

					  vuoRemoveCompositionStateFromThreadLocalStorage();
				  });
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
