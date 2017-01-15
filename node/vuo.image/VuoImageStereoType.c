/**
 * @file
 * VuoImageStereoType implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoImageStereoType.h"
#include "VuoList_VuoImageStereoType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Stereo Image Type",
					 "description" : "Stereo Image Type Enum.",
					 "keywords" : [ "3d", "color" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoImageStereoType"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoImageStereoType
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoImageStereoType.
 */
VuoImageStereoType VuoImageStereoType_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoImageStereoType value = VuoImageStereoType_LeftRightHalf;

	if( !strcmp(valueAsString, "left-right-half"))
		value = VuoImageStereoType_LeftRightHalf;
	else if( !strcmp(valueAsString, "left-right-full"))
		value = VuoImageStereoType_LeftRightFull;
	else if (!strcmp(valueAsString, "top-bottom-half"))
		value = VuoImageStereoType_TopBottomHalf;
	else if(!strcmp(valueAsString, "top-bottom-full"))
		value = VuoImageStereoType_TopBottomFull;
	else if(!strcmp(valueAsString, "anaglyph-red-cyan"))
		value = VuoImageStereoType_AnaglyphRedCyan;
	else if(!strcmp(valueAsString, "anaglyph-amber-blue"))
		value = VuoImageStereoType_AnaglyphAmberBlue;
	else if(!strcmp(valueAsString, "anaglyph-green-magenta"))
		value = VuoImageStereoType_AnaglyphGreenMagenta;
	else if(!strcmp(valueAsString, "vertical-stripe"))
		value = VuoImageStereoType_VerticalStripe;
	else if(!strcmp(valueAsString, "horizontal-stripe"))
		value = VuoImageStereoType_HorizontalStripe;
	else if(!strcmp(valueAsString, "checkerboard"))
		value = VuoImageStereoType_Checkerboard;
	else if(!strcmp(valueAsString, "blend"))
		value = VuoImageStereoType_Blend;
	else if(!strcmp(valueAsString, "difference"))
		value = VuoImageStereoType_Difference;
	else if(!strcmp(valueAsString, "left-only"))
		value = VuoImageStereoType_LeftOnly;
	else if(!strcmp(valueAsString, "right-only"))
		value = VuoImageStereoType_RightOnly;

	return value;
}

/**
 * @ingroup VuoImageStereoType
 * Encodes @c value as a JSON object.
 */
json_object * VuoImageStereoType_getJson(const VuoImageStereoType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoImageStereoType_LeftRightHalf:
			valueAsString = "left-right-half";
			break;

		case VuoImageStereoType_LeftRightFull:
			valueAsString = "left-right-full";
			break;

		case VuoImageStereoType_TopBottomHalf:
			valueAsString = "top-bottom-half";
			break;

		case VuoImageStereoType_TopBottomFull:
			valueAsString = "top-bottom-full";
			break;

		case VuoImageStereoType_AnaglyphRedCyan:
			valueAsString = "anaglyph-red-cyan";
			break;

		case VuoImageStereoType_AnaglyphAmberBlue:
			valueAsString = "anaglyph-amber-blue";
			break;

		case VuoImageStereoType_AnaglyphGreenMagenta:
			valueAsString = "anaglyph-green-magenta";
			break;

		case VuoImageStereoType_VerticalStripe:
			valueAsString = "vertical-stripe";
			break;

		case VuoImageStereoType_HorizontalStripe:
			valueAsString = "horizontal-stripe";
			break;

		case VuoImageStereoType_Checkerboard:
			valueAsString = "checkerboard";
			break;

		case VuoImageStereoType_Blend:
			valueAsString = "blend";
			break;

		case VuoImageStereoType_Difference:
			valueAsString = "difference";
			break;

		case VuoImageStereoType_LeftOnly:
			valueAsString = "left-only";
			break;

		case VuoImageStereoType_RightOnly:
			valueAsString = "right-only";
			break;
	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoImageStereoType VuoImageStereoType_getAllowedValues(void)
{
	VuoList_VuoImageStereoType l = VuoListCreate_VuoImageStereoType();
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_LeftRightHalf);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_LeftRightFull);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_TopBottomHalf);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_TopBottomFull);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_AnaglyphRedCyan);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_AnaglyphAmberBlue);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_AnaglyphGreenMagenta);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_VerticalStripe);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_HorizontalStripe);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_Checkerboard);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_Blend);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_Difference);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_LeftOnly);
	VuoListAppendValue_VuoImageStereoType(l, VuoImageStereoType_RightOnly);

	return l;
}
/**
 * @ingroup VuoImageStereoType
 * Same as @c %VuoImageStereoType_getString()
 */
char * VuoImageStereoType_getSummary(const VuoImageStereoType value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoImageStereoType_LeftRightHalf:
			valueAsString = "Left by Right Half";
			break;

		case VuoImageStereoType_LeftRightFull:
			valueAsString = "Left by Right Full";
			break;

		case VuoImageStereoType_TopBottomHalf:
			valueAsString = "Top by Bottom Half";
			break;

		case VuoImageStereoType_TopBottomFull:
			valueAsString = "Top by Bottom Full";
			break;

		case VuoImageStereoType_AnaglyphRedCyan:
			valueAsString = "Anaglyph: Red / Cyan";
			break;
			
		case VuoImageStereoType_AnaglyphAmberBlue:
			valueAsString = "Anaglyph: Amber / Blue";
			break;

		case VuoImageStereoType_AnaglyphGreenMagenta:
			valueAsString = "Anaglyph: Green / Magenta";
			break;

		case VuoImageStereoType_VerticalStripe:
			valueAsString = "Vertical Stripe";
			break;

		case VuoImageStereoType_HorizontalStripe:
			valueAsString = "Horizontal Stripe";
			break;

		case VuoImageStereoType_Checkerboard:
			valueAsString = "Checkerboard";
			break;

		case VuoImageStereoType_Blend:
			valueAsString = "Blend";
			break;

		case VuoImageStereoType_Difference:
			valueAsString = "Difference";
			break;

		case VuoImageStereoType_LeftOnly:
			valueAsString = "Left Only";
			break;

		case VuoImageStereoType_RightOnly:
			valueAsString = "Right Only";
			break;
	}

	return strdup(valueAsString);
}
