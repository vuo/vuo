/**
 * @file
 * VuoThresholdType implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoThresholdType.h"
#include "VuoList_VuoThresholdType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Brightness Type",
					 "description" : "How to determine the brightness of a color",
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

	VuoThresholdType value = VuoThresholdType_Rec601;

	if (!strcmp(valueAsString, "rec709"))
		value = VuoThresholdType_Rec709;
	else if (!strcmp(valueAsString, "desaturate"))
		value = VuoThresholdType_Desaturate;
	else if (!strcmp(valueAsString, "rgb"))
		value = VuoThresholdType_RGB;
	else if (!strcmp(valueAsString, "rgb-average"))
		value = VuoThresholdType_RGBAverage;
	else if (!strcmp(valueAsString, "rgb-maximum"))
		value = VuoThresholdType_RGBMaximum;
	else if (!strcmp(valueAsString, "rgb-minimum"))
		value = VuoThresholdType_RGBMinimum;
	else if( !strcmp(valueAsString, "red"))
		value = VuoThresholdType_Red;
	else if( !strcmp(valueAsString, "green"))
		value = VuoThresholdType_Green;
	else if (!strcmp(valueAsString, "blue"))
		value = VuoThresholdType_Blue;
	else if(!strcmp(valueAsString, "alpha"))
		value = VuoThresholdType_Alpha;

	return value;
}

/**
 * @ingroup VuoThresholdType
 * Encodes @c value as a JSON object.
 */
json_object * VuoThresholdType_getJson(const VuoThresholdType value)
{
	char *valueAsString;

	switch (value)
	{
		case VuoThresholdType_Rec709:
			valueAsString = "rec709";
			break;

		case VuoThresholdType_Desaturate:
			valueAsString = "desaturate";
			break;

		case VuoThresholdType_RGB:
			valueAsString = "rgb";
			break;

		case VuoThresholdType_RGBAverage:
			valueAsString = "rgb-average";
			break;

		case VuoThresholdType_RGBMaximum:
			valueAsString = "rgb-maximum";
			break;

		case VuoThresholdType_RGBMinimum:
			valueAsString = "rgb-minimum";
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

		default:
			valueAsString = "rec601";
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoThresholdType VuoThresholdType_getAllowedValues(void)
{
	VuoList_VuoThresholdType l = VuoListCreate_VuoThresholdType();
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Rec601);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Rec709);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_Desaturate);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_RGB);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_RGBAverage);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_RGBMaximum);
	VuoListAppendValue_VuoThresholdType(l, VuoThresholdType_RGBMinimum);
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
		case VuoThresholdType_Rec601:
			valueAsString = "Perceptual (ITU Rec. 601 / NTSC CRT)";
			break;

		case VuoThresholdType_Rec709:
			valueAsString = "Perceptual (ITU Rec. 709 / HDTV)";
			break;

		case VuoThresholdType_Desaturate:
			valueAsString = "Desaturate (HSL)";
			break;

		case VuoThresholdType_RGB:
			valueAsString = "Individual Components (RGB)";
			break;

		case VuoThresholdType_RGBAverage:
			valueAsString = "Average Components (RGB)";
			break;

		case VuoThresholdType_RGBMaximum:
			valueAsString = "Lightest Components (RGB)";
			break;

		case VuoThresholdType_RGBMinimum:
			valueAsString = "Darkest Components (RGB)";
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
			valueAsString = "Opacity";
			break;
	}

	return strdup(valueAsString);
}
