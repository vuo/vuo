/**
 * @file
 * VuoBlendMode C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOBLENDMODE_H
#define VUOBLENDMODE_H

/// @{
typedef const struct VuoList_VuoBlendMode_struct { void *l; } * VuoList_VuoBlendMode;
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
	VuoBlendMode_Normal,			///< Standard alpha compositing.  Foreground colors replace background colors, unless they're alpha-transparent.
	VuoBlendMode_Multiply,			///< Multiplies each component of the foreground and background colors.  Makes the composite image darker.
	VuoBlendMode_DarkerComponent,	///< Chooses the darker of each of the foreground and background color's RGB components.
	VuoBlendMode_DarkerColor,		///< Chooses the darker color by comparing the luminance of each color.
	VuoBlendMode_LinearBurn,		///< Linear burn
	VuoBlendMode_ColorBurn,			///< Color burn
	VuoBlendMode_Screen,			///< Screen
	VuoBlendMode_LighterComponent,	///< Chooses the lighter of each of the foreground and background color's RGB components.
	VuoBlendMode_LighterColor,		///< Chooses the lighter color by comparing the luminance of each color.
	VuoBlendMode_LinearDodge,		///< Adds each component of the foreground and background colors.  Makes the composite image lighter.
	VuoBlendMode_ColorDodge,		///< Color dodge
	VuoBlendMode_Overlay,			///< Overlay
	VuoBlendMode_SoftLight,			///< Soft light
	VuoBlendMode_HardLight,			///< Hard light
	VuoBlendMode_VividLight,		///< Vivid light
	VuoBlendMode_LinearLight,		///< Linear light
	VuoBlendMode_PinLight,			///< Pin light
	VuoBlendMode_HardMix,			///< Hard mix
	VuoBlendMode_Difference,		///< Difference
	VuoBlendMode_Exclusion,			///< Exclusion
	VuoBlendMode_Subtract,			///< Subtracts each component of the foreground color from the background color.  Makes the composite image darker.
	VuoBlendMode_Divide,			///< Divides each component of the background color by the foreground color.
	VuoBlendMode_Hue,				///< Hue
	VuoBlendMode_Saturation,		///< Saturation
	VuoBlendMode_Color,				///< Color
	VuoBlendMode_Luminosity			///< Luminosity
} VuoBlendMode;

VuoBlendMode VuoBlendMode_makeFromJson(struct json_object * js);
struct json_object * VuoBlendMode_getJson(const VuoBlendMode value);
VuoList_VuoBlendMode VuoBlendMode_getAllowedValues(void);
char * VuoBlendMode_getSummary(const VuoBlendMode value);

/// @{
/**
 * Automatically generated function.
 */
VuoBlendMode VuoBlendMode_makeFromString(const char *str);
char * VuoBlendMode_getString(const VuoBlendMode value);
void VuoBlendMode_retain(VuoBlendMode value);
void VuoBlendMode_release(VuoBlendMode value);
/// @}

/**
 * @}
*/

#endif
