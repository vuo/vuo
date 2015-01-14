/**
 * @file
 * VuoCountWrapMode implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoCountWrapMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "CountWrapMode",
					 "description" : "Wrap Mode Enum.",
					 "keywords" : [ "wrap" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoCountWrapMode
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoCountWrapMode.
 */
VuoCountWrapMode VuoCountWrapMode_valueFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoCountWrapMode value = VuoCountWrapMode_Wrap;

	if (! strcmp(valueAsString, "wrap")) {
		value = VuoCountWrapMode_Wrap;
	} else if (! strcmp(valueAsString, "saturate")) {
		value = VuoCountWrapMode_Saturate;
	}

	return value;
}

/**
 * @ingroup VuoCountWrapMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoCountWrapMode_jsonFromValue(const VuoCountWrapMode value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoCountWrapMode_Wrap:
			valueAsString = "wrap";
			break;
		case VuoCountWrapMode_Saturate:
			valueAsString = "saturate";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoCountWrapMode
 * Same as @c %VuoCountWrapMode_stringFromValue()
 */
char * VuoCountWrapMode_summaryFromValue(const VuoCountWrapMode value)
{
	return VuoCountWrapMode_stringFromValue(value);
}
