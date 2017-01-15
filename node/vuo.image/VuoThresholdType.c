/**
 * @file
 * VuoThresholdType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoThresholdType.h"
#include "VuoList_VuoThresholdType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Threshold Type",
					 "description" : "VuoThreshold Type Enum.",
					 "keywords" : [ "mask", "color" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoThresholdType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoThresholdType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoThresholdType.
 */
VuoThresholdType VuoThresholdType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoThresholdType value = VuoThresholdType_Luminance;

	if( !strcmp(valueAsString, "red"))
		value = VuoThresholdType_Red;
	else if( !strcmp(valueAsString, "green"))
		value = VuoThresholdType_Green;
	else if (!strcmp(valueAsString, "blue"))
		value = VuoThresholdType_Blue;
	else if(!strcmp(valueAsString, "alpha"))
		value = VuoThresholdType_Alpha;
	else
		value = VuoThresholdType_Luminance;

	return value;
}

/**
 * @ingroup VuoThresholdType
 * Encodes @c value as a JSON object.
 */
json_object * VuoThresholdType_getJson(const VuoThresholdType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoThresholdType_Luminance:
			valueAsString = "luminance";
			break;

		case VuoThresholdType_Red:
			valueAsString = "red";
			break;

		case VuoThresholdType_Green:
			valueAsString = "green";
			break;

		case VuoThresholdType_Blue:
			valueAsString = "blue";
			break;

		case VuoThresholdType_Alpha:
			valueAsString = "alpha";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoThresholdType VuoThresholdType_getAllowedValues(void)
{
	VuoList_VuoThresholdType l = VuoListCreate_VuoThresholdType();
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Luminance);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Red);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Green);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Blue);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Alpha);
	return l;
}
/**
 * @ingroup VuoThresholdType
 * Same as @c %VuoThresholdType_getString()
 */
char * VuoThresholdType_getSummary(const VuoThresholdType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoThresholdType_Luminance:
			valueAsString = "Luminance";
			break;

		case VuoThresholdType_Red:
			valueAsString = "Red";
			break;

		case VuoThresholdType_Green:
			valueAsString = "Green";
			break;

		case VuoThresholdType_Blue:
			valueAsString = "Blue";
			break;

		case VuoThresholdType_Alpha:
			valueAsString = "Alpha";
			break;
	}

	return strdup(valueAsString);
}
