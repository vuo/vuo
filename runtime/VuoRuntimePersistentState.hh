/**
 * @file
 * VuoRuntimePersistentState interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompositionDiff;
class VuoNodeRegistry;
class VuoNodeSynchronization;
class VuoThreadManager;
class VuoRuntimeCommunicator;
class VuoRuntimeState;

#include "VuoCompositionState.h"

/**
 * Runtime state preserved across live-coding reloads.
 */
class VuoRuntimePersistentState
{
private:
	string workingDirectory;  ///< The directory that nodes should use to resolve relative paths.

	unsigned long lastEventId;  ///< The ID most recently assigned to any event, composition-wide. Used to generate a unique ID for each event.

	bool _isStopRequested;  ///< True if @ref VuoRuntimeState::stopCompositionAsOrderedByComposition() has been called.

	dispatch_group_t triggerWorkersScheduled;  ///< Keeps track of trigger workers that have been scheduled but have not yet launched an event into the composition.

public:
	/// @{
	/**
	 * Objects preserved across live-coding reloads.
	 */
	VuoCompositionDiff *compositionDiff;
	VuoNodeRegistry *nodeRegistry;
	VuoRuntimeCommunicator *communicator;
	VuoThreadManager *threadManager;
	VuoNodeSynchronization *nodeSynchronization;
	/// @}

	VuoRuntimeState *runtimeState;  ///< Reference to the parent VuoRuntimeState.

	VuoRuntimePersistentState(const char *workingDirectory);
	~VuoRuntimePersistentState(void);
	static string getCurrentWorkingDirectory(void);
	char * getWorkingDirectory(void);
	unsigned long getNextEventId(void);
	bool isStopRequested(void);
	void setStopRequested(bool isStopRequested);
	dispatch_group_t getTriggerWorkersScheduled(void);
};

extern "C"
{
unsigned long vuoGetNextEventId(VuoCompositionState *compositionState);
dispatch_group_t vuoGetTriggerWorkersScheduled(VuoCompositionState *compositionState);
}
