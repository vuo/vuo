/**
 * @file
 * VuoRuntimeContext implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

/**
 * Runtime information about a port.
 */
struct PortContext
{
	bool event;  ///< Whether this port has just received an event.
	void *data;  ///< A pointer to the port's data, or null if this port is event-only.
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
	void *instanceData;  ///< A pointer to the node's instance data, or null if this node is stateless.
	dispatch_semaphore_t semaphore;  ///< A semaphore to wait on while a node's event function is executing.
	unsigned long claimingEventId;  ///< The ID of the event that currently has exclusive claim on the node.
	dispatch_group_t executingGroup;  ///< A dispatch group used by the subcomposition's event function to wait for nodes to finish executing.
	unsigned long executingEventId;  ///< The ID of the most recent event that came from the composition's event function.
	bool *outputEvents;  ///< An array used by the subcomposition's event function to track events to published output ports, or null if this is not a subcomposition.
};

/**
 * Creates a port context, initializing its fields to default values.
 */
struct PortContext * vuoCreatePortContext(void *data, bool isTrigger, const char *triggerQueueName)
{
	struct PortContext *portContext = (struct PortContext *)malloc(sizeof(struct PortContext));
	portContext->event = false;
	portContext->data = data;
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
void vuoFreeNodeContext(struct NodeContext *nodeContext, size_t portContextCount)
{
	for (size_t i = 0; i < portContextCount; ++i)
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
 * Sets the node context's portContexts field.
 */
void vuoSetNodeContextPortContexts(struct NodeContext *nodeContext, struct PortContext **portContexts)
{
	nodeContext->portContexts = portContexts;
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
void vuoResetNodeContextEvents(struct NodeContext *nodeContext, size_t portCount)
{
	for (size_t i = 0; i < portCount; ++i)
		nodeContext->portContexts[i]->event = false;
}
