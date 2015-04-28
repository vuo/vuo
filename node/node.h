/**
 * @file
 * Prototypes for node class implementations.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef NODE_H
#define NODE_H


#include "module.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

struct json_object;

// Types
#include "VuoAudioSamples.h"
#include "VuoBlendMode.h"
#include "VuoBoolean.h"
#include "VuoColor.h"
#include "VuoCurve.h"
#include "VuoCurveEasing.h"
#include "VuoHorizontalAlignment.h"
#include "VuoImage.h"
#include "VuoInteger.h"
#include "VuoModifierKey.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoReal.h"
#include "VuoSceneObject.h"
#include "VuoShader.h"
#include "VuoText.h"
#include "VuoTransform.h"
#include "VuoTransform2d.h"
#include "VuoVerticalAlignment.h"
#include "VuoVertices.h"
#include "VuoWave.h"
#include "VuoWindowReference.h"
#include "VuoWrapMode.h"

// List Types
#include "VuoList_VuoAudioSamples.h"
#include "VuoList_VuoBlendMode.h"
#include "VuoList_VuoBoolean.h"
#include "VuoList_VuoColor.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoInteger.h"
#include "VuoList_VuoModifierKey.h"
#include "VuoList_VuoPoint2d.h"
#include "VuoList_VuoPoint3d.h"
#include "VuoList_VuoPoint4d.h"
#include "VuoList_VuoReal.h"
#include "VuoList_VuoSceneObject.h"
#include "VuoList_VuoShader.h"
#include "VuoList_VuoText.h"
#include "VuoList_VuoTransform.h"
#include "VuoList_VuoTransform2d.h"
#include "VuoList_VuoVertices.h"
#include "VuoList_VuoWindowReference.h"
#include "VuoList_VuoWrapMode.h"


/**
 * @addtogroup DevelopingNodeClasses
 * @{
 */


/**
 * @defgroup VuoNodeParameters Node Parameters
 * Parameter decorations to be used by node classes.
 *
 * Each parameter of nodeEvent(), nodeInstanceEvent(), and other node class functions refers to
 * one of the node's ports or the node's instance data. This is indicated by its decoration.
 * @{
 */

/**
 * Options for input ports to block events.
 */
enum VuoPortEventBlocking
{
	/**
	 * An event received by this input port is never blocked. It always flows to all non-trigger output ports.
	 */
	VuoPortEventBlocking_None,

	/**
	 * An event received by this input port may or may not be blocked. Although it always flows to the "done" port,
	 * it may or may not flow to any other non-trigger output port.
	 */
	VuoPortEventBlocking_Door,

	/**
	 * An event received by this input port is always blocked. Although it always flows to the "done" port,
	 * it never flows to any other output port.
	 */
	VuoPortEventBlocking_Wall
};

/**
 * Options for trigger ports to throttle events when triggers are firing events faster than the composition
 * can process them.
 */
enum VuoPortEventThrottling
{
	/**
	 * An event fired by this port will eventually reach downstream nodes, waiting if necessary for previous events
	 * to flow through the composition.
	 */
	VuoPortEventThrottling_Enqueue,

	/**
	 * An event fired by this port will be dropped (not transmitted to any nodes downstream of the trigger port)
	 * if it would otherwise have to wait for previous events to flow through the composition.
	 */
	VuoPortEventThrottling_Drop
};

/**
 * Use this to decorate parameters referring to a stateful node's instance data.
 *
 * When this parameter's function is called, the argument passed will be
 * a pointer to the instance data created by nodeInstanceEvent().
 *
 * @hideinitializer
 *
 * @param type The instance data's C type.
 *
 * \eg{void nodeInstanceEvent(VuoInstanceData(struct nodeInstanceData) ctx);}
 */
#define VuoInstanceData(type) __attribute__((annotate("vuoInstanceData"))) type * const

/**
 * Use this to decorate parameters referring to the data part of a data-and-event input port.
 *
 * The parameter's name becomes the port name.
 *
 * When this parameter's function is called, the argument passed will be
 * a value of type @c type that holds the input port's data.
 *
 * If @c type is heap data (a pointer), the parameter's function _should not_ modify the heap data.
 * See @ref DevelopingNodeClassesImmutable "Developing Node Classes" for more information.
 *
 * @hideinitializer
 *
 * @param type The port type. See @ref VuoTypes.
 * @param ... Optionally, a JSON object specification containing additional details about the data, such as its default value.
 *		The "default" key is recognized for all port types, and should have the format accepted by the port type's
 *		MyType_valueFromJson() function. The "defaults" key is recognized for generic port types; its value should be
 *		a JSON object in which each key is a specialized port type name and each value has the format accepted by that
 *		port type's MyType_valueFromJson() function. Additional keys may be recognized by the port type's input editor (see
 *		@ref DevelopingInputEditors).
 *
 * \eg{void nodeEvent(VuoInputData(VuoInteger,{"default":60,"suggestedMin":0,"suggestedMax":127}) noteNumber);}
 */
#define VuoInputData(type, ...) __attribute__((annotate("vuoInputData"),annotate("vuoType:" #type),annotate("vuoInputDataDetails: " #__VA_ARGS__))) const type

/**
 * Use this to decorate parameters referring to an event-only input port or the event part of a data-and-event input port.
 *
 * When this parameter's function is called, the argument passed will be
 * a @c bool that is true if the port received an event.
 *
 * For an event-only port, the parameter's name becomes the port name.
 *
 * @hideinitializer
 *
 * @param eventBlocking A value of type VuoPortEventBlocking, indicating the port's event-blocking behavior.
 * @param name The identifier of the corresponding data port, or blank if this is intended to be an event-only input port.
 *
 * \eg{void nodeEvent(VuoInputData(VuoInteger,"0") seconds, VuoInputEvent(VuoPortEventBlocking_Wall,seconds) secondsEvent);}
 * \eg{void nodeEvent(VuoInputEvent(VuoPortEventBlocking_None,) start);}
 */
#define VuoInputEvent(eventBlocking,name) __attribute__((annotate("vuoInputEventBlocking:" #eventBlocking),annotate("vuoInputEvent:" #name))) const bool

/**
 * Use this to decorate parameters referring to the data part of a data-and-event output port.
 *
 * When this parameter's function is called, the argument passed will be
 * a value of type @c type* that points to the output port's data. Set this data to set the output port's value.
 *
 * The parameter's name becomes the port name.
 *
 * If @c type is heap data (a pointer), the node _should not_ modify the heap data after the node function returns.
 * See @ref DevelopingNodeClassesImmutable "Developing Node Classes" for more information.
 *
 * @hideinitializer
 *
 * @param type The port type. See @ref VuoTypes.
 *
 * \eg{void nodeEvent(VuoOutputData(VuoInteger) seconds);}
 */
#define VuoOutputData(type) __attribute__((annotate("vuoOutputData"), annotate("vuoType:" # type))) type * const

/**
 * Use this to decorate parameters referring to an event-only output port or the event part of a data-and-event output port.
 *
 * When this parameter's function is called, the argument passed will be
 * a @c bool*. Set this to true if an event should be sent through the output port.
 * However, if any input port with the @ref VuoPortEventBlocking_None option received an event, then an
 * event will be sent through the output port regardless of this parameter's value.
 *
 * For an event-only port, the parameter's name becomes the port name.
 *
 * @hideinitializer
 *
 * @param name The identifier of the corresponding data port, or blank if this is intended to be an event-only output port.
 *
 * \eg{void nodeEvent(VuoOutputData(VuoInteger) seconds, VuoOutputEvent(seconds) secondsEvent);}
 * \eg{void nodeEvent(VuoOutputEvent() started);}
 */
#define VuoOutputEvent(name) __attribute__((annotate("vuoOutputEvent:" #name))) bool * const

/**
 * Use this to decorate parameters referring to a trigger output port.
 *
 * When this parameter's function is called, the argument passed will be
 * a callback function. Call the function to fire an event through the trigger port.
 *
 * @hideinitializer
 *
 * @param name The name of the trigger port.
 * @param type The port type, or `void` for an event-only trigger port. See @ref VuoTypes.
 * @param ... Optionally, a VuoPortEventThrottling value.
 *
 * \eg{void nodeEvent(VuoOutputTrigger(started,void))
 * {
 *     // Fire an event without any data.
 *     started();
 * }}
 * \eg{void nodeEvent(VuoOutputTrigger(didSomething,VuoInteger))
 * {
 *     // Fire an event with stack data.
 *     didSomething(5);
 * }}
 * \eg{void nodeEvent(VuoOutputTrigger(didSomething,VuoText))
 * {
 *     // Fire an event with heap data.
 *     VuoText t = VuoText_make("hello");
 *     didSomething(t);
 * }}
 */
#define VuoOutputTrigger(name,type,...) __attribute__((annotate("vuoOutputTrigger:" #name), annotate("vuoType:" #type), annotate("vuoInputEventThrottling:" #__VA_ARGS__))) void (* const name)(type)

/**
 * @}
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
 *		@arg @ref VuoInputData
 *		@arg @ref VuoInputEvent
 *		@arg @ref VuoOutputTrigger
 *		@arg @ref VuoOutputData
 *		@arg @ref VuoOutputEvent
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
 *		@arg @ref VuoInputData — This argument has the initial (constant or default) value of the input port, before it has received any events.
 */
struct nodeInstanceData *nodeInstanceInit(...);

/**
 * This function is called just after nodeInstanceInit() and each time the running composition is updated (e.g. when
 * a node is added). It's responsible for starting any threads, dispatch sources, or event loops that call @c VuoOutputTrigger functions.
 *
 * This function is optional.
 *
 * Parameter decorations must include:
 *		@arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *		@arg @ref VuoInputData — This argument has the current value of the input port.
 *		@arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
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
 *		@arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *		@arg @ref VuoInputData
 *		@arg @ref VuoInputEvent
 *		@arg @ref VuoOutputTrigger
 *		@arg @ref VuoOutputData
 *		@arg @ref VuoOutputEvent
 *
 * This function _should not_ modify heap data provided via @ref VuoInputData,
 * and it _should not_ keep references to heap data sent via @ref VuoOutputData.
 * See @ref DevelopingNodeClassesImmutable "Developing Node Classes" for more information.
 */
void nodeInstanceEvent(...);

/**
 * This function is called each time an input port constant on this node is updated while the composition is running.
 * It's responsible for adjusting all @c VuoOutputTrigger callbacks to use the new input port values.
 *
 * This function is optional.
 *
 * Parameter decorations must include:
 *		@arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *		@arg @ref VuoInputData — This argument has the current value of the input port.
 *		@arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
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
 *		@arg @ref VuoInstanceData
 *
 * Parameter decorations may include:
 *		@arg @ref VuoOutputTrigger — Stateful nodes may store references to trigger functions in their instance data, in order to fire events at any time.
 */
void nodeInstanceTriggerStop(...);

/**
 * This function is called once for the node, either when node is removed from the running composition or the composition stops.
 * It's responsible for closing any services opened in nodeInstanceInit() or elsewhere.
 *
 * Parameter decorations must include:
 *		@arg @ref VuoInstanceData
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
