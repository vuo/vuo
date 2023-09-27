/**
 * @file
 * VuoMidiController implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidiController.h"
#include "VuoInteger.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Controller",
					 "description" : "A music controller event sent via MIDI.",
					 "keywords" : [ "CC", "custom controller" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBoolean",
						"VuoInteger",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoMidiController
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "channel" : 1,
 *     "controllerNumber" : 1,
 *     "value" : 127
 *   }
 * }
 */
VuoMidiController VuoMidiController_makeFromJson(json_object * js)
{
	return (VuoMidiController){
		VuoJson_getObjectValue(VuoInteger, js, "channel", 1),
		VuoJson_getObjectValue(VuoInteger, js, "controllerNumber", 1),
		VuoJson_getObjectValue(VuoInteger, js, "value", 127),
	};
}

/**
 * @ingroup VuoMidiController
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiController_getJson(const VuoMidiController mn)
{
	json_object *js = json_object_new_object();

	json_object *channelObject = VuoInteger_getJson(mn.channel);
	json_object_object_add(js, "channel", channelObject);

	json_object *controllerNumberObject = VuoInteger_getJson(mn.controllerNumber);
	json_object_object_add(js, "controllerNumber", controllerNumberObject);

	json_object *valueObject = VuoInteger_getJson(mn.value);
	json_object_object_add(js, "value", valueObject);

	return js;
}

/**
 * @ingroup VuoMidiController
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiController_getSummary(const VuoMidiController mn)
{
	return VuoText_format("Channel %d, controller %d (0x%02x): value %d",
						  mn.channel, mn.controllerNumber, mn.controllerNumber, mn.value);
}

/**
 * Returns true if the channel, controller number, and value all match.
 */
bool VuoMidiController_areEqual(const VuoMidiController value1, const VuoMidiController value2)
{
	return (value1.channel          == value2.channel
		 && value1.controllerNumber == value2.controllerNumber
		 && value1.value            == value2.value);
}

/**
 * Returns true if `a < b`.
 * @version200New
 */
bool VuoMidiController_isLessThan(const VuoMidiController a, const VuoMidiController b)
{
	VuoType_returnInequality(VuoInteger, a.channel, b.channel);
	VuoType_returnInequality(VuoInteger, a.controllerNumber, b.controllerNumber);
	VuoType_returnInequality(VuoInteger, a.value, b.value);
	return false;
}
