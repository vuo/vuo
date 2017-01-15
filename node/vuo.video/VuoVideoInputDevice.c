/**
 * @file
 * VuoVideoInputDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoVideoInputDevice.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Video Input Device",
					  "description" : "Information about a video input device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : "",
 *     "name" : ""
 *   }
 * }
 */
VuoVideoInputDevice VuoVideoInputDevice_makeFromJson(json_object *js)
{
	VuoVideoInputDevice value = {"",""};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "id", &o))
		value.id = VuoText_makeFromJson(o);
	else
		value.id = VuoText_make("");

	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	else
		value.name = VuoText_make("");

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoVideoInputDevice_getJson(const VuoVideoInputDevice value)
{
	json_object *js = json_object_new_object();

	json_object *idObject = VuoText_getJson(value.id);
	json_object_object_add(js, "id", idObject);

	json_object *nameObject = VuoText_getJson(value.name);
	json_object_object_add(js, "name", nameObject);

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoVideoInputDevice_getSummary(const VuoVideoInputDevice value)
{
	return VuoText_format("Video input device: \"%s\"", value.name);
}

/**
 * Returns true if the two audio input device specifications are identical.
 */
bool VuoVideoInputDevice_areEqual(VuoVideoInputDevice value1, VuoVideoInputDevice value2)
{
	return ( VuoText_areEqual(value1.id, value2.id) && VuoText_areEqual(value1.name, value2.name) );
}
