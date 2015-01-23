/**
 * @file
 * VuoNoise implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoNoise.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Noise",
					 "description" : "Type of noise.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoNoise
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoNoise.
 */
VuoNoise VuoNoise_valueFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoNoise noise = VuoNoise_White;

	if (strcmp(valueAsString, "white") == 0) {
		noise = VuoNoise_White;
	} else if (strcmp(valueAsString, "grey") == 0) {
		noise = VuoNoise_Grey;
	} else if (strcmp(valueAsString, "pink") == 0) {
		noise = VuoNoise_Pink;
	} else if (strcmp(valueAsString, "brown") == 0) {
		noise = VuoNoise_Brown;
	} else if (strcmp(valueAsString, "blue") == 0) {
		noise = VuoNoise_Blue;
	} else if (strcmp(valueAsString, "violet") == 0) {
		noise = VuoNoise_Violet;
	}

	return noise;
}

/**
 * @ingroup VuoNoise
 * Encodes @c value as a JSON object.
 */
json_object * VuoNoise_jsonFromValue(const VuoNoise value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoNoise_White:
			valueAsString = "white";
			break;
		case VuoNoise_Grey:
			valueAsString = "grey";
			break;
		case VuoNoise_Pink:
			valueAsString = "pink";
			break;
		case VuoNoise_Brown:
			valueAsString = "brown";
			break;
		case VuoNoise_Blue:
			valueAsString = "blue";
			break;
		case VuoNoise_Violet:
			valueAsString = "violet";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * @ingroup VuoNoise
 * Same as @c %VuoNoise_stringFromValue()
 */
char * VuoNoise_summaryFromValue(const VuoNoise value)
{
	return VuoNoise_stringFromValue(value);
}
