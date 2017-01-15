/**
 * @file
 * VuoBlendMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoBlendMode.h"
#include "VuoList_VuoBlendMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Blend Mode",
					 "description" : "Blend Mode Enum.",
					 "keywords" : [ "blend, mix" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoBlendMode"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoBlendMode
 * Decodes the JSON object @c js, expected to contain a string, to create a new @c VuoBlendMode.
 */
VuoBlendMode VuoBlendMode_makeFromJson(json_object *js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoBlendMode value = VuoBlendMode_Normal;

	if( !strcmp(valueAsString, "normal") ) {
		value = VuoBlendMode_Normal;
	}	else
	if( !strcmp(valueAsString, "multiply") ) {
		value = VuoBlendMode_Multiply;
	}	else
	if( !strcmp(valueAsString, "darker-component") ) {
		value = VuoBlendMode_DarkerComponent;
	}	else
	if( !strcmp(valueAsString, "darker-color") ) {
		value = VuoBlendMode_DarkerColor;
	}	else
	if( !strcmp(valueAsString, "linear-burn") ) {
		value = VuoBlendMode_LinearBurn;
	}	else
	if( !strcmp(valueAsString, "color-burn") ) {
		value = VuoBlendMode_ColorBurn;
	}	else
	if( !strcmp(valueAsString, "screen") ) {
		value = VuoBlendMode_Screen;
	}	else
	if( !strcmp(valueAsString, "lighter-component") ) {
		value = VuoBlendMode_LighterComponent;
	}	else
	if( !strcmp(valueAsString, "lighter-color") ) {
		value = VuoBlendMode_LighterColor;
	}	else
	if( !strcmp(valueAsString, "linear-dodge") ) {
		value = VuoBlendMode_LinearDodge;
	}	else
	if( !strcmp(valueAsString, "color-dodge") ) {
		value = VuoBlendMode_ColorDodge;
	}	else
	if( !strcmp(valueAsString, "overlay") ) {
		value = VuoBlendMode_Overlay;
	}	else
	if( !strcmp(valueAsString, "soft-light") ) {
		value = VuoBlendMode_SoftLight;
	}	else
	if( !strcmp(valueAsString, "hard-light") ) {
		value = VuoBlendMode_HardLight;
	}	else
	if( !strcmp(valueAsString, "vivid-light") ) {
		value = VuoBlendMode_VividLight;
	}	else
	if( !strcmp(valueAsString, "linear-light") ) {
		value = VuoBlendMode_LinearLight;
	}	else
	if( !strcmp(valueAsString, "pin-light") ) {
		value = VuoBlendMode_PinLight;
	}	else
	if( !strcmp(valueAsString, "hard-mix") ) {
		value = VuoBlendMode_HardMix;
	}	else
	if( !strcmp(valueAsString, "difference") ) {
		value = VuoBlendMode_Difference;
	}	else
	if( !strcmp(valueAsString, "exclusion") ) {
		value = VuoBlendMode_Exclusion;
	}	else
	if( !strcmp(valueAsString, "subtract") ) {
		value = VuoBlendMode_Subtract;
	}	else
	if( !strcmp(valueAsString, "divide") ) {
		value = VuoBlendMode_Divide;
	}	else
	if( !strcmp(valueAsString, "hue") ) {
		value = VuoBlendMode_Hue;
	}	else
	if( !strcmp(valueAsString, "saturation") ) {
		value = VuoBlendMode_Saturation;
	}	else
	if( !strcmp(valueAsString, "color") ) {
		value = VuoBlendMode_Color;
	}	else
	if( !strcmp(valueAsString, "luminosity") ) {
		value = VuoBlendMode_Luminosity;
	}

	return value;
}

/**
 * @ingroup VuoBlendMode
 * Encodes @c value as a JSON object.
 */
json_object * VuoBlendMode_getJson(const VuoBlendMode value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoBlendMode_Normal:
			valueAsString = "normal";
			break;
		case VuoBlendMode_Multiply:
			valueAsString = "multiply";
			break;
		case VuoBlendMode_DarkerComponent:
			valueAsString = "darker-component";
			break;
		case VuoBlendMode_DarkerColor:
			valueAsString = "darker-color";
			break;
		case VuoBlendMode_LinearBurn:
			valueAsString = "linear-burn";
			break;
		case VuoBlendMode_ColorBurn:
			valueAsString = "color-burn";
			break;
		case VuoBlendMode_Screen:
			valueAsString = "screen";
			break;
		case VuoBlendMode_LighterComponent:
			valueAsString = "lighter-component";
			break;
		case VuoBlendMode_LighterColor:
			valueAsString = "lighter-color";
			break;
		case VuoBlendMode_LinearDodge:
			valueAsString = "linear-dodge";
			break;
		case VuoBlendMode_ColorDodge:
			valueAsString = "color-dodge";
			break;
		case VuoBlendMode_Overlay:
			valueAsString = "overlay";
			break;
		case VuoBlendMode_SoftLight:
			valueAsString = "soft-light";
			break;
		case VuoBlendMode_HardLight:
			valueAsString = "hard-light";
			break;
		case VuoBlendMode_VividLight:
			valueAsString = "vivid-light";
			break;
		case VuoBlendMode_LinearLight:
			valueAsString = "linear-light";
			break;
		case VuoBlendMode_PinLight:
			valueAsString = "pin-light";
			break;
		case VuoBlendMode_HardMix:
			valueAsString = "hard-mix";
			break;
		case VuoBlendMode_Difference:
			valueAsString = "difference";
			break;
		case VuoBlendMode_Exclusion:
			valueAsString = "exclusion";
			break;
		case VuoBlendMode_Subtract:
			valueAsString = "subtract";
			break;
		case VuoBlendMode_Divide:
			valueAsString = "divide";
			break;
		case VuoBlendMode_Hue:
			valueAsString = "hue";
			break;
		case VuoBlendMode_Saturation:
			valueAsString = "saturation";
			break;
		case VuoBlendMode_Color:
			valueAsString = "color";
			break;
		case VuoBlendMode_Luminosity:
			valueAsString = "luminosity";
			break;

	}
	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoBlendMode VuoBlendMode_getAllowedValues(void)
{
	VuoList_VuoBlendMode l = VuoListCreate_VuoBlendMode();
	for (VuoBlendMode b = VuoBlendMode_Normal; b <= VuoBlendMode_Luminosity; ++b)
		VuoListAppendValue_VuoBlendMode(l, b);
	return l;
}

/**
 * @ingroup VuoBlendMode
 * Same as @c %VuoBlendMode_getString()
 */
char * VuoBlendMode_getSummary(const VuoBlendMode value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoBlendMode_Normal:
			valueAsString = "Normal — Alpha";
			break;
		case VuoBlendMode_Multiply:
			valueAsString = "Multiply — b•f";
			break;
		case VuoBlendMode_DarkerComponent:
			valueAsString = "Darker Component — min(b,f)";
			break;
		case VuoBlendMode_DarkerColor:
			valueAsString = "Darker Color — min(b,f)";
			break;
		case VuoBlendMode_LinearBurn:
			valueAsString = "Linear Burn — b+f-1";
			break;
		case VuoBlendMode_ColorBurn:
			valueAsString = "Color Burn — 1-(1-b)/f";
			break;
		case VuoBlendMode_Screen:
			valueAsString = "Screen — 1-(1-b)•(1-f)";
			break;
		case VuoBlendMode_LighterComponent:
			valueAsString = "Lighter Component — max(b,f)";
			break;
		case VuoBlendMode_LighterColor:
			valueAsString = "Lighter Color — max(b,f)";
			break;
		case VuoBlendMode_LinearDodge:
			valueAsString = "Linear Dodge (Add) — b+f";
			break;
		case VuoBlendMode_ColorDodge:
			valueAsString = "Color Dodge — b/(1-f)";
			break;
		case VuoBlendMode_Overlay:
			valueAsString = "Overlay";
			break;
		case VuoBlendMode_SoftLight:
			valueAsString = "Soft Light";
			break;
		case VuoBlendMode_HardLight:
			valueAsString = "Hard Light";
			break;
		case VuoBlendMode_VividLight:
			valueAsString = "Vivid Light";
			break;
		case VuoBlendMode_LinearLight:
			valueAsString = "Linear Light";
			break;
		case VuoBlendMode_PinLight:
			valueAsString = "Pin Light";
			break;
		case VuoBlendMode_HardMix:
			valueAsString = "Hard Mix";
			break;
		case VuoBlendMode_Difference:
			valueAsString = "Difference — abs(b-f)";
			break;
		case VuoBlendMode_Exclusion:
			valueAsString = "Exclusion — b+f-2•b•f";
			break;
		case VuoBlendMode_Subtract:
			valueAsString = "Subtract — b-f";
			break;
		case VuoBlendMode_Divide:
			valueAsString = "Divide — b/f";
			break;
		case VuoBlendMode_Hue:
			valueAsString = "Hue";
			break;
		case VuoBlendMode_Saturation:
			valueAsString = "Saturation";
			break;
		case VuoBlendMode_Color:
			valueAsString = "Color";
			break;
		case VuoBlendMode_Luminosity:
			valueAsString = "Luminosity";
			break;
	}
	return strdup(valueAsString);
}
