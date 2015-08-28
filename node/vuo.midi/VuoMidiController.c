/**
 * @file
 * VuoMidiController implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoMidiController.h"
#include "VuoInteger.h"
#include "VuoBoolean.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "MIDI Controller",
					 "description" : "A music controller event sent via MIDI.",
					 "keywords" : [ "CC", "custom controller" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
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
VuoMidiController VuoMidiController_valueFromJson(json_object * js)
{
	VuoMidiController mn = {1,1,127};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "channel", &o))
		mn.channel = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "controllerNumber", &o))
		mn.controllerNumber = VuoInteger_valueFromJson(o);

	if (json_object_object_get_ex(js, "value", &o))
		mn.value = VuoInteger_valueFromJson(o);

	return mn;
}

/**
 * @ingroup VuoMidiController
 * Encodes @c value as a JSON object.
 */
json_object * VuoMidiController_jsonFromValue(const VuoMidiController mn)
{
	json_object *js = json_object_new_object();

	json_object *channelObject = VuoInteger_jsonFromValue(mn.channel);
	json_object_object_add(js, "channel", channelObject);

	json_object *controllerNumberObject = VuoInteger_jsonFromValue(mn.controllerNumber);
	json_object_object_add(js, "controllerNumber", controllerNumberObject);

	json_object *valueObject = VuoInteger_jsonFromValue(mn.value);
	json_object_object_add(js, "value", valueObject);

	return js;
}

/**
 * @ingroup VuoMidiController
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoMidiController_summaryFromValue(const VuoMidiController mn)
{
	return VuoText_format("Channel %d, controller %d (0x%02x): value %d",
						  mn.channel, mn.controllerNumber, mn.controllerNumber, mn.value);
}
