/**
 * @file
 * VuoWrapMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoWrapMode.h"
#include "VuoList_VuoWrapMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Wrap Mode",
					 "description" : "Controls what happens when a value exceeds a given range.",
					 "keywords" : [ "loop", "overflow", "saturate", "stay" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoWrapMode"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoWrapMode
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoWrapMode.
 */
VuoWrapMode VuoWrapMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoWrapMode value = VuoWrapMode_Wrap;

	if (! strcmp(valueAsString, "wrap")) {
		value = VuoWrapMode_Wrap;
	} else if (! strcmp(valueAsString, "saturate")) {
		value = VuoWrapMode_Saturate;
	}

	return value;
}

/**
 * @ingroup VuoWrapMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoWrapMode_getJson(const VuoWrapMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoWrapMode_Wrap:
			valueAsString = "wrap";
			break;
		case VuoWrapMode_Saturate:
			valueAsString = "saturate";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoWrapMode VuoWrapMode_getAllowedValues(void)
{
	VuoList_VuoWrapMode l = VuoListCreate_VuoWrapMode();
	VuoListAppendValue_VuoWrapMode(l, VuoWrapMode_Wrap);
	VuoListAppendValue_VuoWrapMode(l, VuoWrapMode_Saturate);
	return l;
}

/**
 * @ingroup VuoWrapMode
 * Same as @c %VuoWrapMode_getString()
 */
char * VuoWrapMode_getSummary(const VuoWrapMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoWrapMode_Wrap:
			valueAsString = "Wrap";
			break;
		case VuoWrapMode_Saturate:
			valueAsString = "Saturate";
			break;
	}

	return strdup(valueAsString);
}
