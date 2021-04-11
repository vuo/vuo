/**
 * @file
 * VuoRuntimeContext interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

extern "C"
{

/**
 * Runtime information about a port.
 */
struct PortContext
{
	bool event;  ///< Whether this port has just received an event.
	void *data;  ///< A pointer to the port's data, or null if this port is event-only.
	bool dataRetained;  ///< A rudimentary retain system for `data`. If retained, `data` won't be freed when the port context is.
	dispatch_queue_t triggerQueue;  ///< A queue for synchronizing fired events, or null if this is not a trigger port.
	dispatch_semaphore_t triggerSemaphore;  ///< A semaphore for checking if events should be dropped, or null if this is not a trigger port.
	void *triggerFunction;  ///< A function pointer for the trigger scheduler function, or null if this is not a trigger port.
};

/**
 * Runtime information about a node.
 */
struct NodeContext
{
	struct PortContext **portContexts;  ///< An array of contexts for input and output ports, or null if this node is a subcomposition.
	unsigned long portContextCount;  ///< The number of elements in `portContexts`.
	void *instanceData;  ///< A pointer to the node's instance data, or null if this node is stateless.
	void *nodeMutex;  ///< A `std::mutex` that synchronizes access to @ref claimingEventId.
	void *nodeConditionVariable;  ///< A `std::condition_variable` that notifies threads waiting on @ref nodeMutex.
	unsigned long claimingEventId;  ///< The ID of the event that currently has exclusive claim on the node.
	dispatch_group_t executingGroup;  ///< A dispatch group used by the subcomposition's event function to wait for nodes to finish executing.
	void *executingEventIds;  ///< A `vector<unsigned long>` containing the ID of the event that most recently came in through the composition's published inputs and any events spun off from it.
	bool *outputEvents;  ///< An array used by the subcomposition's event function to track events to published output ports, or null if this is not a subcomposition.

	void *executingEventIdsSync;  ///< A `std::mutex` that synchronizes access to @ref executingEventIds.
};

struct PortContext * vuoCreatePortContext(void *data, bool isTrigger, const char *triggerQueueName);
struct NodeContext * vuoCreateNodeContext(bool hasInstanceData, bool isComposition, size_t outputEventCount);
void vuoFreePortContext(struct PortContext *portContext);
void vuoFreeNodeContext(struct NodeContext *nodeContext);
void vuoSetPortContextEvent(struct PortContext *portContext, bool event);
void vuoSetPortContextData(struct PortContext *portContext, void *data);
void vuoSetPortContextTriggerFunction(struct PortContext *portContext, void *triggerFunction);
bool vuoGetPortContextEvent(struct PortContext *portContext);
void * vuoGetPortContextData(struct PortContext *portContext);
dispatch_queue_t vuoGetPortContextTriggerQueue(struct PortContext *portContext);
dispatch_semaphore_t vuoGetPortContextTriggerSemaphore(struct PortContext *portContext);
void * vuoGetPortContextTriggerFunction(struct PortContext *portContext);
void vuoRetainPortContextData(struct PortContext *portContext);
void vuoSetNodeContextPortContexts(struct NodeContext *nodeContext, struct PortContext **portContexts, unsigned long portContextCount);
void vuoSetNodeContextInstanceData(struct NodeContext *nodeContext, void *instanceData);
void vuoSetNodeContextClaimingEventId(struct NodeContext *nodeContext, unsigned long claimingEventId);
void vuoSetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index, bool event);
struct PortContext * vuoGetNodeContextPortContext(struct NodeContext *nodeContext, size_t index);
void * vuoGetNodeContextInstanceData(struct NodeContext *nodeContext);
unsigned long vuoGetNodeContextClaimingEventId(struct NodeContext *nodeContext);
dispatch_group_t vuoGetNodeContextExecutingGroup(struct NodeContext *nodeContext);
bool vuoGetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index);
void vuoResetNodeContextEvents(struct NodeContext *nodeContext);
void vuoStartedExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId);
void vuoSpunOffExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId);
bool vuoFinishedExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId);
unsigned long vuoGetOneExecutingEvent(struct NodeContext *nodeContext);

}
