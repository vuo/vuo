/**
 * @file
 * VuoRuntimeContext interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

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
	dispatch_semaphore_t semaphore;  ///< A semaphore to wait on while a node's event function is executing.
	unsigned long claimingEventId;  ///< The ID of the event that currently has exclusive claim on the node.
	dispatch_group_t executingGroup;  ///< A dispatch group used by the subcomposition's event function to wait for nodes to finish executing.
	unsigned long executingEventId;  ///< The ID of the most recent event that came from the composition's event function.
	bool *outputEvents;  ///< An array used by the subcomposition's event function to track events to published output ports, or null if this is not a subcomposition.
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
void vuoSetNodeContextExecutingEventId(struct NodeContext *nodeContext, unsigned long executingEventId);
void vuoSetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index, bool event);
struct PortContext * vuoGetNodeContextPortContext(struct NodeContext *nodeContext, size_t index);
void * vuoGetNodeContextInstanceData(struct NodeContext *nodeContext);
dispatch_semaphore_t vuoGetNodeContextSemaphore(struct NodeContext *nodeContext);
unsigned long vuoGetNodeContextClaimingEventId(struct NodeContext *nodeContext);
dispatch_group_t vuoGetNodeContextExecutingGroup(struct NodeContext *nodeContext);
unsigned long vuoGetNodeContextExecutingEventId(struct NodeContext *nodeContext);
bool vuoGetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index);
void vuoResetNodeContextEvents(struct NodeContext *nodeContext);
