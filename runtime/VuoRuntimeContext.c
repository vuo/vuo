/**
 * @file
 * VuoRuntimeContext implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRuntimeContext.h"

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
		portContext->triggerQueue = dispatch_queue_create(triggerQueueName, NULL);
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
	nodeContext->semaphore = dispatch_semaphore_create(1);
	nodeContext->claimingEventId = 0;
	nodeContext->executingEventId = 0;

	if (isComposition)
	{
		nodeContext->executingGroup = dispatch_group_create();
		nodeContext->outputEvents = (bool *)calloc(outputEventCount, sizeof(bool));
	}
	else
	{
		nodeContext->executingGroup = NULL;
		nodeContext->outputEvents = NULL;
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
	if (nodeContext->semaphore)
		dispatch_release(nodeContext->semaphore);
	if (nodeContext->executingGroup)
		dispatch_release(nodeContext->executingGroup);
	free(nodeContext->outputEvents);
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
 * Sets the node context's executingEventId field.
 */
void vuoSetNodeContextExecutingEventId(struct NodeContext *nodeContext, unsigned long executingEventId)
{
	nodeContext->executingEventId = executingEventId;
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
 * Gets the node context's semaphore field.
 */
dispatch_semaphore_t vuoGetNodeContextSemaphore(struct NodeContext *nodeContext)
{
	return nodeContext->semaphore;
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
 * Gets the node context's executingEventId field.
 */
unsigned long vuoGetNodeContextExecutingEventId(struct NodeContext *nodeContext)
{
	return nodeContext->executingEventId;
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
