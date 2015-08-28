/**
 * @file
 * VuoBlendMode C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOBLENDMODE_H
#define VUOBLENDMODE_H

/// @{
typedef void * VuoList_VuoBlendMode;
#define VuoList_VuoBlendMode_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlendMode VuoBlendMode
 * An enum defining different types of blend shaders.
 *
 * @{
 */

/**
 * An enum defining different types of blend shaders.
 */
typedef enum {
	VuoBlendMode_Normal,
	VuoBlendMode_Multiply,
	VuoBlendMode_DarkerComponent,
	VuoBlendMode_DarkerColor,
	VuoBlendMode_LinearBurn,
	VuoBlendMode_ColorBurn,
	VuoBlendMode_Screen,
	VuoBlendMode_LighterComponent,
	VuoBlendMode_LighterColor,
	VuoBlendMode_LinearDodge,
	VuoBlendMode_ColorDodge,
	VuoBlendMode_Overlay,
	VuoBlendMode_SoftLight,
	VuoBlendMode_HardLight,
	VuoBlendMode_VividLight,
	VuoBlendMode_LinearLight,
	VuoBlendMode_PinLight,
	VuoBlendMode_HardMix,
	VuoBlendMode_Difference,
	VuoBlendMode_Exclusion,
	VuoBlendMode_Subtract,
	VuoBlendMode_Divide,
	VuoBlendMode_Hue,
	VuoBlendMode_Saturation,
	VuoBlendMode_Color,
	VuoBlendMode_Luminosity
} VuoBlendMode;

VuoBlendMode VuoBlendMode_valueFromJson(struct json_object * js);
struct json_object * VuoBlendMode_jsonFromValue(const VuoBlendMode value);
VuoList_VuoBlendMode VuoBlendMode_allowedValues(void);
char * VuoBlendMode_summaryFromValue(const VuoBlendMode value);

/// @{
/**
 * Automatically generated function.
 */
VuoBlendMode VuoBlendMode_valueFromString(const char *str);
char * VuoBlendMode_stringFromValue(const VuoBlendMode value);
void VuoBlendMode_retain(VuoBlendMode value);
void VuoBlendMode_release(VuoBlendMode value);
/// @}

/**
 * @}
*/

#endif
