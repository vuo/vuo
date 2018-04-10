/**
 * @file
 * VuoColorSample implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoColorSample.h"
#include "VuoList_VuoColorSample.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Color Sample",
					  "description" : "How to sample a color.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
					  "VuoList_VuoColorSample"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "darker-component"
 * }
 */
VuoColorSample VuoColorSample_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoColorSample value = VuoColorSample_Average;

	if (strcmp(valueAsString, "darkest-components") == 0)
		value = VuoColorSample_DarkestComponents;
	else if (strcmp(valueAsString, "darkest-color") == 0)
		value = VuoColorSample_DarkestColor;
	else if (strcmp(valueAsString, "lightest-components") == 0)
		value = VuoColorSample_LightestComponents;
	else if (strcmp(valueAsString, "lightest-color") == 0)
		value = VuoColorSample_LightestColor;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoColorSample_getJson(const VuoColorSample value)
{
	char *valueAsString = "average";

	if (value == VuoColorSample_DarkestComponents)
		valueAsString = "darkest-components";
	else if (value == VuoColorSample_DarkestColor)
		valueAsString = "darkest-color";
	else if (value == VuoColorSample_LightestComponents)
		valueAsString = "lightest-components";
	else if (value == VuoColorSample_LightestColor)
		valueAsString = "lightest-color";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoColorSample VuoColorSample_getAllowedValues(void)
{
	VuoList_VuoColorSample l = VuoListCreate_VuoColorSample();
	VuoListAppendValue_VuoColorSample(l, VuoColorSample_Average);
	VuoListAppendValue_VuoColorSample(l, VuoColorSample_DarkestComponents);
	VuoListAppendValue_VuoColorSample(l, VuoColorSample_DarkestColor);
	VuoListAppendValue_VuoColorSample(l, VuoColorSample_LightestComponents);
	VuoListAppendValue_VuoColorSample(l, VuoColorSample_LightestColor);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoColorSample_getSummary(const VuoColorSample value)
{
	char *valueAsString = "Average";

	if (value == VuoColorSample_DarkestComponents)
		valueAsString = "Darkest Components";
	else if (value == VuoColorSample_DarkestColor)
		valueAsString = "Darkest Color";
	else if (value == VuoColorSample_LightestComponents)
		valueAsString = "Lightest Components";
	else if (value == VuoColorSample_LightestColor)
		valueAsString = "Lightest Color";

	return strdup(valueAsString);
}

/**
 * Returns true if the two values are equal.
 */
bool VuoColorSample_areEqual(const VuoColorSample valueA, const VuoColorSample valueB)
{
	return valueA == valueB;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoColorSample_isLessThan(const VuoColorSample valueA, const VuoColorSample valueB)
{
	return valueA < valueB;
}
