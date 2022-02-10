/**
 * @file
 * VuoNodeSynchronization implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>
#include "VuoNodeSynchronization.hh"
#include "VuoNodeRegistry.hh"
#include "VuoRuntimeState.hh"
#include "VuoRuntimePersistentState.hh"

/**
 * Waits until @a eventId has claimed exclusive access to all of the given nodes.
 * The nodes should be passed in the same order as VuoCompilerBitcodeGenerator::orderedNodes.
 */
void VuoNodeSynchronization::lockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount, unsigned long eventId)
{
	for (unsigned long i = 0; i < nodeCount; ++i)
		lockNode(compositionState, nodeIndices[i], eventId);
}

/**
 * Waits until @a eventId has claimed exclusive access to the node.
 */
void VuoNodeSynchronization::lockNode(VuoCompositionState *compositionState, unsigned long nodeIndex, unsigned long eventId)
{
	recordWaiting(compositionState, eventId, nodeIndex);

	{
		NodeContext *nodeContext = vuoGetNodeContext(compositionState, nodeIndex);

		std::unique_lock<std::mutex> lock(* static_cast<std::mutex *>(nodeContext->nodeMutex));
		static_cast<std::condition_variable *>(nodeContext->nodeConditionVariable)->wait(lock, [nodeContext, eventId]()
		{
			if (nodeContext->claimingEventId == 0)
				nodeContext->claimingEventId = eventId;

			return nodeContext->claimingEventId == eventId;
		});
	}

	recordLocked(compositionState, eventId, nodeIndex);
}

/**
 * Relinquishes exclusive access to all of the given nodes.
 */
void VuoNodeSynchronization::unlockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount)
{
	for (unsigned long i = 0; i < nodeCount; ++i)
		unlockNode(compositionState, nodeIndices[i]);
}

/**
 * Relinquishes exclusive access to the node.
 */
void VuoNodeSynchronization::unlockNode(VuoCompositionState *compositionState, unsigned long nodeIndex)
{
	unsigned long eventId = 0;

	{
		NodeContext *nodeContext = vuoGetNodeContext(compositionState, nodeIndex);

		std::unique_lock<std::mutex> lock(* static_cast<std::mutex *>(nodeContext->nodeMutex));

		eventId = nodeContext->claimingEventId;
		nodeContext->claimingEventId = 0;

		static_cast<std::condition_variable *>(nodeContext->nodeConditionVariable)->notify_all();
	}

	recordUnlocked(compositionState, eventId, nodeIndex);
}

/**
 * If debugging is enabled, records the node's change in status.
 */
void VuoNodeSynchronization::recordWaiting(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex)
{
	if (! VuoIsDebugEnabled())
		return;

	std::lock_guard<std::mutex> lock(statusMutex);

	auto i = statusWaiting.find(compositionState->compositionIdentifier);
	if (i != statusWaiting.end())
	{
		auto j = i->second.find(eventId);
		if (j != i->second.end())
		{
			auto k = std::find(j->second.begin(), j->second.end(), nodeIndex);
			if (k != j->second.end())
				return;
		}
	}

	statusWaiting[compositionState->compositionIdentifier][eventId].push_back(nodeIndex);
}

/**
 * If debugging is enabled, records the node's change in status.
 */
void VuoNodeSynchronization::recordLocked(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex)
{
	if (! VuoIsDebugEnabled())
		return;

	std::lock_guard<std::mutex> lock(statusMutex);

	auto i = statusWaiting.find(compositionState->compositionIdentifier);
	if (i != statusWaiting.end())
	{
		auto j = i->second.find(eventId);
		if (j != i->second.end())
		{
			auto k = std::find(j->second.begin(), j->second.end(), nodeIndex);
			if (k != j->second.end())
			{
				j->second.erase(k);
				if (j->second.empty())
					i->second.erase(j);
				if (i->second.empty())
					statusWaiting.erase(i);
			}
		}
	}

	i = statusLocked.find(compositionState->compositionIdentifier);
	if (i != statusLocked.end())
	{
		auto j = i->second.find(eventId);
		if (j != i->second.end())
		{
			auto k = std::find(j->second.begin(), j->second.end(), nodeIndex);
			if (k != j->second.end())
				return;
		}
	}

	statusLocked[compositionState->compositionIdentifier][eventId].push_back(nodeIndex);
}

/**
 * If debugging is enabled, records the node's change in status.
 */
void VuoNodeSynchronization::recordUnlocked(VuoCompositionState *compositionState, unsigned long eventId, unsigned long nodeIndex)
{
	if (! VuoIsDebugEnabled())
		return;

	std::lock_guard<std::mutex> lock(statusMutex);

	auto i = statusLocked.find(compositionState->compositionIdentifier);
	if (i != statusLocked.end())
	{
		auto j = i->second.find(eventId);
		if (j != i->second.end())
		{
			auto k = std::find(j->second.begin(), j->second.end(), nodeIndex);
			if (k != j->second.end())
			{
				j->second.erase(k);
				if (j->second.empty())
					i->second.erase(j);
				if (i->second.empty())
					statusLocked.erase(i);

				return;
			}
		}
	}

	VDebugLog("Error: Node %lu was unlocked without having been locked", nodeIndex);
}

/**
 * If debugging is enabled, logs the status of nodes claimed by an event and those that an event is waiting on.
 */
void VuoNodeSynchronization::print(void)
{
	if (! VuoIsDebugEnabled())
	{
		VUserLog("Enable debug mode to record status.");
		return;
	}

	std::lock_guard<std::mutex> lock(statusMutex);

	ostringstream oss;
	oss << "=== locked ===" << endl << endl;
	for (auto i : statusLocked)
	{
		oss << i.first << ":" << endl;
		for (auto j : i.second)
		{
			oss << "   eventId " << j.first << ":";
			for (unsigned long nodeIndex : j.second)
				oss << " " << nodeIndex;
			oss << endl;
		}
		oss << endl;
	}
	oss << endl;

	oss << "=== waiting to lock ===" << endl << endl;
	for (auto i : statusWaiting)
	{
		oss << i.first << ":" << endl;
		for (auto j : i.second)
		{
			oss << "   eventId " << j.first << ":";
			for (unsigned long nodeIndex : j.second)
				oss << " " << nodeIndex;
			oss << endl;
		}
		oss << endl;
	}
	oss << endl;

	VUserLog("\n%s", oss.str().c_str());
}

extern "C"
{

/**
 * C wrapper for VuoNodeSynchronization::lockNodes().
 */
void vuoLockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount, unsigned long eventId)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->nodeSynchronization->lockNodes(compositionState, nodeIndices, nodeCount, eventId);
}

/**
 * C wrapper for VuoNodeSynchronization::lockNode().
 */
void vuoLockNode(VuoCompositionState *compositionState, unsigned long nodeIndex, unsigned long eventId)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->nodeSynchronization->lockNode(compositionState, nodeIndex, eventId);
}

/**
 * C wrapper for VuoNodeSynchronization::unlockNodes().
 */
void vuoUnlockNodes(VuoCompositionState *compositionState, unsigned long *nodeIndices, unsigned long nodeCount)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->nodeSynchronization->unlockNodes(compositionState, nodeIndices, nodeCount);
}

/**
 * C wrapper for VuoNodeSynchronization::unlockNode().
 */
void vuoUnlockNode(VuoCompositionState *compositionState, unsigned long nodeIndex)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->nodeSynchronization->unlockNode(compositionState, nodeIndex);
}

}
