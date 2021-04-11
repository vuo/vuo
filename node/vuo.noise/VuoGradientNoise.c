/**
 * @file
 * VuoGradientNoise implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoGradientNoise.h"
#include "VuoList_VuoGradientNoise.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Gradient Noise",
					 "description" : "A method for generating gradient noise.",
					 "keywords" : [ "perlin", "simplex" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoGradientNoise"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoGradientNoise
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoGradientNoise.
 */
VuoGradientNoise VuoGradientNoise_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoGradientNoise gn = VuoGradientNoise_Rectangular;

	if (strcmp(valueAsString, "rectangular") == 0)
		gn = VuoGradientNoise_Rectangular;
	else if (strcmp(valueAsString, "triangular") == 0)
		gn = VuoGradientNoise_Triangular;
	else if (strcmp(valueAsString, "perlin") == 0)
		gn = VuoGradientNoise_Rectangular;
	else if (strcmp(valueAsString, "simplex") == 0)
		gn = VuoGradientNoise_Triangular;

	return gn;
}

/**
 * @ingroup VuoGradientNoise
 * Encodes @c value as a JSON object.
 */
json_object * VuoGradientNoise_getJson(const VuoGradientNoise value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoGradientNoise_Rectangular:
			valueAsString = "rectangular";
			break;
		case VuoGradientNoise_Triangular:
			valueAsString = "triangular";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoGradientNoise VuoGradientNoise_getAllowedValues(void)
{
	VuoList_VuoGradientNoise l = VuoListCreate_VuoGradientNoise();
	VuoListAppendValue_VuoGradientNoise(l, VuoGradientNoise_Rectangular);
	VuoListAppendValue_VuoGradientNoise(l, VuoGradientNoise_Triangular);
	return l;
}

/**
 * @ingroup VuoGradientNoise
 * Same as @c %VuoGradientnoise_getString()
 */
char * VuoGradientNoise_getSummary(const VuoGradientNoise value)
{
	char * valueAsString = "";

	switch (value) {
		case VuoGradientNoise_Rectangular:
			valueAsString = "Rectangular (Perlin)";
			break;
		case VuoGradientNoise_Triangular:
			valueAsString = "Triangular (Simplex)";
			break;
	}

	return strdup(valueAsString);
}
