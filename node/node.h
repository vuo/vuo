/**
 * @file
 * Prototypes for node class implementations.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef NODE_H
#define NODE_H

#include "module.h"
#include "node_header.h"
#include "coreTypes.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#pragma clang diagnostic push
#ifdef VUO_CLANG_32_OR_LATER
	#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include "json-c/json.h"
#pragma clang diagnostic pop

/**
 * @addtogroup DevelopingNodeClasses
 * @{
 */

// These prototypes should only appear to Doxygen, since individual node classes' parameter lists can vary.
#ifdef DOXYGEN

/**
 * @defgroup VuoNodeMethodsStateless Node Methods: Stateless
 * Event handler method to be implemented by node classes.
 *
 * @{
 */

/**
 * This function is called each time the node receives an event. It's responsible for performing whatever task the node does;
 * it takes any input data and events, processes them, and sets or fires any output data and events.
 *
 * This function is called once per event received by the node (even if the event is received through multiple input ports).
 *
 * Parameter decorations may include:
 *      @arg @ref VuoInputData
 *      @arg @ref VuoInputEvent
 *      @arg @ref VuoOutputTrigger
 *      @arg @ref VuoOutputData
 *      @arg @ref VuoOutputEvent
 *
 * This function _should not_ modify heap data provided via @ref VuoInputData.
 * See @ref DevelopingNodeClassesImmutable "Developing Node Classes" for more information.
 */
void nodeEvent(...);

/**
 * @}
 */



/**
 * @defgroup VuoNodeMethodsStateful Node Methods: Stateful
 * Setup, event handler, and teardown methods to be implemented by stateful node classes.
 *
 * These functions are always called sequentially (not concurrently), so you don't need to make them mutually thread-safe.
 *
 * @{
 */

/**
 * This function is called once for the node, either when the composition starts or when the node is added to a running
 * composition. It's responsible for creating the instance data, and may open services (file, network port, ...).
 *
 * If this function allocates memory on the heap for the instance data, it should register that memory. See @ref ManagingMemory.
 *
 * Parameter decorations may include:
 *      @arg @ref VuoInputData — This argument has the initial (constant or default) value of the input port, before it has received any events.
 */
struct nodeInstanceData *nodeInstanceInit(...);

/**
 * This function is called just after nodeInstanceInit() and each time the running composition is updated (e.g. when
 * a node is added). It's responsible for starting any threads, dispatch sources, or event loops that call @c VuoOutputTrigger functions.
 *
 * This function is optional.
 *
 * Parameter decorations must include:
 *      @arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *      @arg @ref VuoInputData — This argument has the current value of the input port.
 *      @arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
 */
void nodeInstanceTriggerStart(...);

/**
 * This function is called each time this node receives an event. Typically, it's responsible for performing whatever task the
 * node does; it takes any input data and events, processes them, and sets or fires any output data and events. For node classes
 * whose primary task is firing events through trigger ports, this function may not be as important.
 *
 * This function is called once per event received by the node (even if the event is received through multiple input ports).
 *
 * Parameter decorations must include:
 *      @arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *      @arg @ref VuoInputData
 *      @arg @ref VuoInputEvent
 *      @arg @ref VuoOutputTrigger
 *      @arg @ref VuoOutputData
 *      @arg @ref VuoOutputEvent
 *
 * This function _should not_ modify heap data provided via @ref VuoInputData.
 * The node may retain references to heap data sent via @ref VuoOutputData,
 * but it _should not_ modify the heap data once it has been sent via @ref VuoOutputData.
 *
 * See @ref DevelopingNodeClassesImmutable "Developing Node Classes" for more information.
 *
 * @note This function may be called even after @ref nodeInstanceTriggerStop has been called.
 * For example, when a composition is stopping, the runtime may call this node's `nodeInstanceTriggerStop`,
 * then another node (whose triggers haven't yet been stopped) may fire an event,
 * which may call this node's `nodeInstanceEvent`.
 */
void nodeInstanceEvent(...);

/**
 * This function is called each time an input port constant on this node is updated while the composition is running.
 * It's responsible for adjusting all @c VuoOutputTrigger callbacks to use the new input port values.
 *
 * This function is optional.
 *
 * Parameter decorations must include:
 *      @arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *      @arg @ref VuoInputData — This argument has the current value of the input port.
 *      @arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
 */
void nodeInstanceTriggerUpdate(...);

/**
 * This function is called each time the running composition is updated (e.g. when a node is added) and just before nodeInstanceFini().
 * It's responsible for stopping all threads, dispatch sources, and event loops started in nodeInstanceTriggerStart() or elsewhere.
 *
 * Any VuoOutputTrigger functions stored in instance data become invalid after this function returns.
 * No VuoOutputTrigger functions should fire from the time this function returns to the next time that nodeInstanceTriggerStart()
 * is called.
 *
 * This function is optional.
 *
 * Parameter decorations must include:
 *      @arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *      @arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
 */
void nodeInstanceTriggerStop(...);

/**
 * This function is called once for the node, either when node is removed from the running composition or the composition stops.
 * It's responsible for closing any services opened in nodeInstanceInit() or elsewhere.
 *
 * Parameter decorations must include:
 *      @arg @ref VuoInstanceData
 */
void nodeInstanceFini(...);

/**
 * @}
 */

#endif

/**
 * @}
 */

#endif
