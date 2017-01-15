/**
 * @file
 * VuoMidiController implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiController.h"
#include "VuoBoolean.h"
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
	VuoMidiController mn = {1,1,127};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "channel", &o))
		mn.channel = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "controllerNumber", &o))
		mn.controllerNumber = VuoInteger_makeFromJson(o);

	if (json_object_object_get_ex(js, "value", &o))
		mn.value = VuoInteger_makeFromJson(o);

	return mn;
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
