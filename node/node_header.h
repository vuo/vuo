/**
 * @file
 * Prototypes for node class headers.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
 * @param ... Optionally, a JSON object specification containing additional details about the data. Supported JSON keys include:
 *          - "default" — The default constant value for the port. It should have the format accepted by the port type's
 *            MyType_makeFromJson() function.
 *          - "defaults" — For generic ports, the default constant values for data types to which the port can be specialized.
 *            The value for this JSON key should be a JSON object in which each key is a specialized port type name and each
 *            value has the format accepted by that port type's MyType_makeFromJson() function.
 *          - "name" (string) — Overrides the default heuristics for creating the port's displayed name in rendered compositions.
 *            This is usually not necessary.
 *          - "menuItems" (array) — For VuoInteger ports with a fixed set of values, a list of menu items.
 *            The port's input editor will be a menu instead of a numerical input editor.  The array elements can be:
 *             - an object with 2 keys: `value` (integer port value) and `name` (display name)
 *             - the string `---` — a menu separator line
 *             - any other string — a non-selectable menu label, for labeling multiple sections within the menu
 *          - "includeValues" (array of strings) — Enum types by default display all `_allowedValues()` in a menu.
 *            When this detail is present, only the values listed will be displayed in the menu.
 *            The values should be string keys — the output of `_getJson()`.
 *          - "auto" — For VuoInteger and VuoReal ports, a special port value that signifies that the node should calculate
 *            the value for this port automatically instead of using the set value.
 *          - "autoSupersedesDefault" (boolean) — For ports with an "auto" value, true if the port should have the "auto" value instead of
 *            the "default" value when the node is instantiated. This is useful if you want the port to start with the "auto"
 *            value but default to the "default" value if the user deselects the "auto" value in the port's input editor.
 *
 *            Additional keys may be recognized by the port type's input editor (see @ref DevelopingInputEditors).
 *
 * \eg{void nodeEvent(VuoInputData(VuoInteger,{"default":60,"suggestedMin":0,"suggestedMax":127}) noteNumber);}
 *
 * @eg{
 * {
 *     "menuItems":[
 *         "RGB",
 *         {"value":0, "name":"    Red"    },
 *         {"value":1, "name":"    Green"  },
 *         {"value":2, "name":"    Blue"   },
 *         "---",
 *         "CMYK",
 *         {"value":3, "name":"    Cyan"   },
 *         {"value":4, "name":"    Magenta"},
 *         {"value":5, "name":"    Yellow" },
 *         {"value":6, "name":"    Black"  }
 *     ]
 * }
 * }
 */
#define VuoInputData(type, ...) __attribute__((annotate("vuoInputData"),annotate("vuoType:" #type),annotate("vuoDetails: " #__VA_ARGS__))) const type

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
 * @param ... Optionally, a JSON object specification containing additional details about the event. Supported JSON keys are:
 *          - "data" (string) — For data-and-event ports, the variable name for the VuoInputData parameter that provides the
 *            data part of the port.
 *          - "eventBlocking" (string) — The port's policy for blocking events. Defaults to "none".
 *              - "none" — An event received by this input port is never blocked. It always flows to all non-trigger output ports.
 *              - "door" — An event received by this input port may or may not be blocked. It may or may not flow to any
 *                 non-trigger output port.
 *              - "wall" — An event received by this input port is always blocked. It never flows to any output port.
 *          - "hasPortAction" (boolean) — Overrides the default heuristics for determining whether the port has a port action
 *            (does something special when it receives an event). This is usually not necessary.
 *          - "name" (string) — For event-only ports, overrides the default heuristics for creating the port's displayed name
 *            in rendered compositions. This is usually not necessary.
 *
 * \eg{void nodeEvent(VuoInputData(VuoInteger,"0") seconds, VuoInputEvent({"data":"seconds","eventBlocking":"wall"}) secondsEvent);}
 * \eg{void nodeEvent(VuoInputEvent() start);}
 */
#define VuoInputEvent(...) __attribute__((annotate("vuoInputEvent"),annotate("vuoDetails: " #__VA_ARGS__))) const bool

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
 * @param ... Optionally, a JSON object specification containing additional details about the data. Supported JSON keys are:
 *          - "name" (string) — Overrides the default heuristics for creating the port's displayed name in rendered compositions.
 *            This is usually not necessary.
 *
 * \eg{void nodeEvent(VuoOutputData(VuoInteger) seconds);}
 */
#define VuoOutputData(type, ...) __attribute__((annotate("vuoOutputData"), annotate("vuoType:" # type),annotate("vuoDetails: " #__VA_ARGS__))) type * const

/**
 * Use this to decorate parameters referring to an event-only output port or the event part of a data-and-event output port.
 *
 * When this parameter's function is called, the argument passed will be
 * a @c bool*. Set this to true if an event should be sent through the output port.
 * However, if any input port with the "eventBlocking" option "none" received an event, then an
 * event will be sent through the output port regardless of this parameter's value.
 *
 * For an event-only port, the parameter's name becomes the port name.
 *
 * @hideinitializer
 *
 * @param ... Optionally, a JSON object specification containing additional details about the event. Supported JSON keys are:
 *          - "data" (string) — For data-and-event ports, the variable name for the VuoOutputData parameter that provides the
 *            data part of the port.
 *          - "name" (string) — For event-only ports, overrides the default heuristics for creating the port's displayed name
 *            in rendered compositions. This is usually not necessary.
 *
 * \eg{void nodeEvent(VuoOutputData(VuoInteger) seconds, VuoOutputEvent({"data":"seconds"}) secondsEvent);}
 * \eg{void nodeEvent(VuoOutputEvent() started);}
 */
#define VuoOutputEvent(...) __attribute__((annotate("vuoOutputEvent"),annotate("vuoDetails: " #__VA_ARGS__))) bool * const

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
 * @param ... Optionally, a JSON object specification containing additional details about the event. Supported JSON keys are:
 *          - "eventThrottling" (string) — How the trigger should handle events when triggers are firing events faster than the
 *            composition can process them. Defaults to "enqueue".
 *              - "enqueue" — An event fired by this port will eventually reach downstream nodes, waiting if necessary for
 *                previous events to flow through the composition.
 *              - "drop" — An event fired by this port will be dropped (not transmitted to any nodes downstream of the trigger
 *                port) if it would otherwise have to wait for previous events to flow through the composition.
 *          - "name" (string) — Overrides the default heuristics for creating the port's displayed name in rendered compositions.
 *            This is usually not necessary.
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
#define VuoOutputTrigger(name,type,...) __attribute__((annotate("vuoOutputTrigger:" #name), annotate("vuoType:" #type), annotate("vuoDetails: " #__VA_ARGS__))) void (* const name)(type)

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
