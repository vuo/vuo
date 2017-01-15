/**
 * @file
 * VuoNoise implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoNoise.h"
#include "VuoList_VuoNoise.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Noise",
					 "description" : "Type of noise.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoNoise"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoNoise
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoNoise.
 */
VuoNoise VuoNoise_makeFromJson(json_object *js)
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
json_object * VuoNoise_getJson(const VuoNoise value)
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
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoNoise VuoNoise_getAllowedValues(void)
{
	VuoList_VuoNoise l = VuoListCreate_VuoNoise();
	VuoListAppendValue_VuoNoise(l, VuoNoise_White);
	VuoListAppendValue_VuoNoise(l, VuoNoise_Grey);
	VuoListAppendValue_VuoNoise(l, VuoNoise_Pink);
	VuoListAppendValue_VuoNoise(l, VuoNoise_Brown);
	VuoListAppendValue_VuoNoise(l, VuoNoise_Blue);
	VuoListAppendValue_VuoNoise(l, VuoNoise_Violet);
	return l;
}

/**
 * @ingroup VuoNoise
 * Same as @c %VuoNoise_getString()
 */
char * VuoNoise_getSummary(const VuoNoise value)
{
	return VuoNoise_getString(value);
}
