/**
 * @file
 * VuoWave implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoWave.h"
#include "VuoList_VuoWave.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Wave",
					 "description" : "Type of wave.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoWave"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoWave
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoWave.
 */
VuoWave VuoWave_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoWave wave = VuoWave_Sine;

	if (! strcmp(valueAsString, "sine"))
		wave = VuoWave_Sine;
	else if (! strcmp(valueAsString, "triangle"))
		wave = VuoWave_Triangle;
	else if (! strcmp(valueAsString, "sawtooth"))
		wave = VuoWave_Sawtooth;

	return wave;
}

/**
 * @ingroup VuoWave
 * Encodes @c value as a JSON object.
 */
json_object * VuoWave_getJson(const VuoWave value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoWave_Sine:
			valueAsString = "sine";
			break;
		case VuoWave_Triangle:
			valueAsString = "triangle";
			break;
		case VuoWave_Sawtooth:
			valueAsString = "sawtooth";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoWave VuoWave_getAllowedValues(void)
{
	VuoList_VuoWave l = VuoListCreate_VuoWave();
	VuoListAppendValue_VuoWave(l, VuoWave_Sine);
	VuoListAppendValue_VuoWave(l, VuoWave_Triangle);
	VuoListAppendValue_VuoWave(l, VuoWave_Sawtooth);
	return l;
}

/**
 * @ingroup VuoWave
 * Same as @c %VuoWave_getString()
 */
char * VuoWave_getSummary(const VuoWave value)
{
	char *valueAsString = "";

	switch (value) {
		case VuoWave_Sine:
			valueAsString = "Sine";
			break;
		case VuoWave_Triangle:
			valueAsString = "Triangle";
			break;
		case VuoWave_Sawtooth:
			valueAsString = "Sawtooth";
			break;
	}

	return strdup(valueAsString);
}
