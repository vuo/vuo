/**
 * @file
 * VuoDmxColorMap implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDmxColorMap.h"
#include "VuoList_VuoDmxColorMap.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "DMX Color Map",
					  "description" : "How to convert between a VuoColor and a set of DMX channels.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoList_VuoDmxColorMap"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   "rgba"
 * }
 */
VuoDmxColorMap VuoDmxColorMap_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoDmxColorMap value = VuoDmxColorMap_RGB;

	if (strcmp(valueAsString, "rgba") == 0)
		value = VuoDmxColorMap_RGBA;
	else if (strcmp(valueAsString, "rgbaw") == 0)
		value = VuoDmxColorMap_RGBAW;
	else if (strcmp(valueAsString, "rgbw") == 0)
		value = VuoDmxColorMap_RGBW;
	else if (strcmp(valueAsString, "wwcw") == 0)
		value = VuoDmxColorMap_WWCW;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoDmxColorMap_getJson(const VuoDmxColorMap value)
{
	char *valueAsString = "rgb";

	if (value == VuoDmxColorMap_RGBA)
		valueAsString = "rgba";
	else if (value == VuoDmxColorMap_RGBAW)
		valueAsString = "rgbaw";
	else if (value == VuoDmxColorMap_RGBW)
		valueAsString = "rgbw";
	else if (value == VuoDmxColorMap_WWCW)
		valueAsString = "wwcw";

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoDmxColorMap VuoDmxColorMap_getAllowedValues(void)
{
	VuoList_VuoDmxColorMap l = VuoListCreate_VuoDmxColorMap();
	VuoListAppendValue_VuoDmxColorMap(l, VuoDmxColorMap_RGB);
	VuoListAppendValue_VuoDmxColorMap(l, VuoDmxColorMap_RGBA);
	VuoListAppendValue_VuoDmxColorMap(l, VuoDmxColorMap_RGBAW);
	VuoListAppendValue_VuoDmxColorMap(l, VuoDmxColorMap_RGBW);
	VuoListAppendValue_VuoDmxColorMap(l, VuoDmxColorMap_WWCW);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoDmxColorMap_getSummary(const VuoDmxColorMap value)
{
	char *valueAsString = "Red, Green, Blue";

	if (value == VuoDmxColorMap_RGBA)
		valueAsString = "Red, Green, Blue, Amber";
	else if (value == VuoDmxColorMap_RGBAW)
		valueAsString = "Red, Green, Blue, Amber, White";
	else if (value == VuoDmxColorMap_RGBW)
		valueAsString = "Red, Green, Blue, White";
	else if (value == VuoDmxColorMap_WWCW)
		valueAsString = "Warm white, Cool white";

	return strdup(valueAsString);
}
