/**
 * @file
 * VuoNodeSynchronization interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <mutex>
#include "VuoCompositionState.h"
#include "VuoRuntimeContext.hh"

/**
 * Provides functions to ensure that nodes are executed thread-safely.
 */
class VuoNodeSynchronization
{
public:
	void lockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount, unsigned long eventId);
	void lockNode(VuoCompositionState *compositionState, unsigned long nodeIndex, unsigned long eventId);
	void unlockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount);
	void unlockNode(VuoCompositionState *compositionState, unsigned long nodeIndex);

private:
	void recordWaiting(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex);
	void recordLocked(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex);
	void recordUnlocked(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex);
	void print(void);

	/// @{
	/**
	 * For debugging.
	 */
	map<string, map<unsigned long, vector<unsigned long>>> statusLocked;
	map<string, map<unsigned long, vector<unsigned long>>> statusWaiting;
	std::mutex statusMutex;
	/// @}
};

extern "C"
{
void vuoLockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount, unsigned long eventId);
void vuoLockNode(VuoCompositionState *compositionState, unsigned long nodeIndex, unsigned long eventId);
void vuoUnlockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount);
void vuoUnlockNode(VuoCompositionState *compositionState, unsigned long nodeIndex);
}
