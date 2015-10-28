/**
 * @file
 * VuoGradientNoise C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGRADIENTNOISE_H
#define VUOGRADIENTNOISE_H

/// @{
typedef const struct VuoList_VuoGradientNoise_struct { void *l; } * VuoList_VuoGradientNoise;
#define VuoList_VuoGradientNoise_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoGradientNoise VuoGradientNoise
 * An enum defining different types of noise.
 *
 * @{
 */

/**
 * An enum defining different types of noise.
 */
typedef enum {
	VuoGradientNoise_Perlin,
	VuoGradientNoise_Simplex
} VuoGradientNoise;

VuoGradientNoise VuoGradientNoise_valueFromJson(struct json_object * js);
struct json_object * VuoGradientNoise_jsonFromValue(const VuoGradientNoise value);
VuoList_VuoGradientNoise VuoGradientNoise_allowedValues(void);
char * VuoGradientNoise_summaryFromValue(const VuoGradientNoise value);

/// @{
/**
 * Automatically generated function.
 */
VuoGradientNoise VuoGradientNoise_valueFromString(const char *str);
char * VuoGradientNoise_stringFromValue(const VuoGradientNoise value);
/// @}

/**
 * @}
*/

#endif
