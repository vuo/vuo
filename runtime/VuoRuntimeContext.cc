/**
 * @file
 * VuoRuntimeContext implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRuntimeContext.hh"
#include "VuoEventLoop.h"
#include <mutex>
#include <condition_variable>

/**
 * Creates a port context, initializing its fields to default values.
 */
struct PortContext * vuoCreatePortContext(void *data, bool isTrigger, const char *triggerQueueName)
{
	struct PortContext *portContext = (struct PortContext *)malloc(sizeof(struct PortContext));
	portContext->event = false;
	portContext->data = data;
	portContext->dataRetained = false;
	portContext->triggerFunction = NULL;

	if (isTrigger)
	{
		portContext->triggerQueue = dispatch_queue_create(triggerQueueName, VuoEventLoop_getDispatchInteractiveAttribute());
		portContext->triggerSemaphore = dispatch_semaphore_create(1);
	}
	else
	{
		portContext->triggerQueue = NULL;
		portContext->triggerSemaphore = NULL;
	}

	return portContext;
}

/**
 * Creates a node context, initializing its fields to default values.
 */
struct NodeContext * vuoCreateNodeContext(bool hasInstanceData, bool isComposition, size_t outputEventCount)
{
	struct NodeContext *nodeContext = (struct NodeContext *)malloc(sizeof(struct NodeContext));
	nodeContext->portContexts = NULL;
	nodeContext->portContextCount = 0;
	nodeContext->nodeMutex = new std::mutex();
	nodeContext->nodeConditionVariable = new std::condition_variable();
	nodeContext->claimingEventId = 0;

	if (isComposition)
	{
		nodeContext->executingEventIds = new vector<unsigned long>();
		nodeContext->executingGroup = dispatch_group_create();
		nodeContext->outputEvents = (bool *)calloc(outputEventCount, sizeof(bool));
		nodeContext->executingEventIdsSync = new std::mutex();
	}
	else
	{
		nodeContext->executingEventIds = NULL;
		nodeContext->executingGroup = NULL;
		nodeContext->outputEvents = NULL;
		nodeContext->executingEventIdsSync = NULL;
	}

	if (hasInstanceData)
	{
		void **instanceData = (void **)malloc(sizeof(void *));
		*instanceData = NULL;
		nodeContext->instanceData = (void *)instanceData;
	}
	else
		nodeContext->instanceData = NULL;

	return nodeContext;
}

/**
 * Frees the port context and its fields.
 */
void vuoFreePortContext(struct PortContext *portContext)
{
	if (! portContext->dataRetained)
		free(portContext->data);
	if (portContext->triggerQueue)
		dispatch_release(portContext->triggerQueue);
	if (portContext->triggerSemaphore)
		dispatch_release(portContext->triggerSemaphore);
	free(portContext);
}

/**
 * Frees the port context and its fields.
 */
void vuoFreeNodeContext(struct NodeContext *nodeContext)
{
	for (size_t i = 0; i < nodeContext->portContextCount; ++i)
		vuoFreePortContext(nodeContext->portContexts[i]);

	free(nodeContext->portContexts);
	free(nodeContext->instanceData);
	delete static_cast<std::mutex *>(nodeContext->nodeMutex);
	delete static_cast<std::condition_variable *>(nodeContext->nodeConditionVariable);
	delete static_cast< vector<unsigned long> *>(nodeContext->executingEventIds);
	if (nodeContext->executingGroup)
		dispatch_release(nodeContext->executingGroup);
	free(nodeContext->outputEvents);
	delete static_cast<std::mutex *>(nodeContext->executingEventIdsSync);
	free(nodeContext);
}

/**
 * Sets the port context's event field.
 */
void vuoSetPortContextEvent(struct PortContext *portContext, bool event)
{
	portContext->event = event;
}

/**
 * Sets the port context's data field.
 */
void vuoSetPortContextData(struct PortContext *portContext, void *data)
{
	portContext->data = data;
}

/**
 * Sets the port context's triggerFunction field.
 */
void vuoSetPortContextTriggerFunction(struct PortContext *portContext, void *triggerFunction)
{
	portContext->triggerFunction = triggerFunction;
}

/**
 * Gets the port context's event field.
 */
bool vuoGetPortContextEvent(struct PortContext *portContext)
{
	return portContext->event;
}

/**
 * Gets the port context's data field.
 */
void * vuoGetPortContextData(struct PortContext *portContext)
{
	return portContext->data;
}

/**
 * Gets the port context's triggerQueue field.
 */
dispatch_queue_t vuoGetPortContextTriggerQueue(struct PortContext *portContext)
{
	return portContext->triggerQueue;
}

/**
 * Gets the port context's triggerSemaphore field.
 */
dispatch_semaphore_t vuoGetPortContextTriggerSemaphore(struct PortContext *portContext)
{
	return portContext->triggerSemaphore;
}

/**
 * Gets the port context's triggerFunction field.
 */
void * vuoGetPortContextTriggerFunction(struct PortContext *portContext)
{
	return portContext->triggerFunction;
}

/**
 * Prevents the port context's data field from being freed by `vuoFreePortContext()`.
 */
void vuoRetainPortContextData(struct PortContext *portContext)
{
	portContext->dataRetained = true;
}

/**
 * Sets the node context's portContexts and portContextCount fields.
 */
void vuoSetNodeContextPortContexts(struct NodeContext *nodeContext, struct PortContext **portContexts, unsigned long portContextCount)
{
	nodeContext->portContexts = portContexts;
	nodeContext->portContextCount = portContextCount;
}

/**
 * Sets the node context's instanceData field, freeing the previous value.
 */
void vuoSetNodeContextInstanceData(struct NodeContext *nodeContext, void *instanceData)
{
	free(nodeContext->instanceData);
	nodeContext->instanceData = instanceData;
}

/**
 * Sets the node context's claimingEventId field.
 */
void vuoSetNodeContextClaimingEventId(struct NodeContext *nodeContext, unsigned long claimingEventId)
{
	nodeContext->claimingEventId = claimingEventId;
}

/**
 * Sets the array element at the given index of the node context's outputEvents field.
 */
void vuoSetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index, bool event)
{
	nodeContext->outputEvents[index] = event;
}

/**
 * Gets the array element at the given index of the node context's portContexts field.
 */
struct PortContext * vuoGetNodeContextPortContext(struct NodeContext *nodeContext, size_t index)
{
	return nodeContext->portContexts[index];
}

/**
 * Gets the node context's instanceData field.
 */
void * vuoGetNodeContextInstanceData(struct NodeContext *nodeContext)
{
	return nodeContext->instanceData;
}

/**
 * Gets the node context's claimingEventId field.
 */
unsigned long vuoGetNodeContextClaimingEventId(struct NodeContext *nodeContext)
{
	return nodeContext->claimingEventId;
}

/**
 * Gets the node context's executingGroup field.
 */
dispatch_group_t vuoGetNodeContextExecutingGroup(struct NodeContext *nodeContext)
{
	return nodeContext->executingGroup;
}

/**
 * Gets the array element at the given index of the node context's outputEvents field.
 */
bool vuoGetNodeContextOutputEvent(struct NodeContext *nodeContext, size_t index)
{
	return nodeContext->outputEvents[index];
}

/**
 * Sets the event field to false for all of the port contexts in this node context.
 */
void vuoResetNodeContextEvents(struct NodeContext *nodeContext)
{
	for (size_t i = 0; i < nodeContext->portContextCount; ++i)
		nodeContext->portContexts[i]->event = false;
}

/**
 * Sets the node context's executingEventIds field to a list containing the given value, clearing out any old values.
 */
void vuoStartedExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId)
{
	std::lock_guard<std::mutex> guard(*static_cast<std::mutex *>(nodeContext->executingEventIdsSync));

	vector<unsigned long> *executingEventIds = static_cast< vector<unsigned long> *>(nodeContext->executingEventIds);
	executingEventIds->clear();
	executingEventIds->push_back(eventId);
}

/**
 * Adds the given value to the list in the node context's executingEventIds field.
 */
void vuoSpunOffExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId)
{
	std::lock_guard<std::mutex> guard(*static_cast<std::mutex *>(nodeContext->executingEventIdsSync));

	vector<unsigned long> *executingEventIds = static_cast< vector<unsigned long> *>(nodeContext->executingEventIds);
	if (! executingEventIds->empty())
		executingEventIds->push_back(eventId);
}

/**
 * Removes the given value from the list in the node context's executingEventIds field if present.
 * Returns true if it was present and was the last item in the list.
 */
bool vuoFinishedExecutingEvent(struct NodeContext *nodeContext, unsigned long eventId)
{
	std::lock_guard<std::mutex> guard(*static_cast<std::mutex *>(nodeContext->executingEventIdsSync));

	vector<unsigned long> *executingEventIds = static_cast< vector<unsigned long> *>(nodeContext->executingEventIds);
	auto found = find(executingEventIds->begin(), executingEventIds->end(), eventId);
	if (found != executingEventIds->end())
	{
		executingEventIds->erase(found);
		return executingEventIds->empty();
	}
	return false;
}

/**
 * Returns the first list item in the node context's executingEventIds field.
 * This is useful for when you know the list contains exactly one item.
 */
unsigned long vuoGetOneExecutingEvent(struct NodeContext *nodeContext)
{
	std::lock_guard<std::mutex> guard(*static_cast<std::mutex *>(nodeContext->executingEventIdsSync));

	vector<unsigned long> *executingEventIds = static_cast< vector<unsigned long> *>(nodeContext->executingEventIds);
	return executingEventIds->at(0);
}
