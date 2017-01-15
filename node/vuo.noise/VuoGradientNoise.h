/**
 * @file
 * VuoGradientNoise C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

VuoGradientNoise VuoGradientNoise_makeFromJson(struct json_object * js);
struct json_object * VuoGradientNoise_getJson(const VuoGradientNoise value);
VuoList_VuoGradientNoise VuoGradientNoise_getAllowedValues(void);
char * VuoGradientNoise_getSummary(const VuoGradientNoise value);

/// @{
/**
 * Automatically generated function.
 */
VuoGradientNoise VuoGradientNoise_makeFromString(const char *str);
char * VuoGradientNoise_getString(const VuoGradientNoise value);
/// @}

/**
 * @}
*/

#endif
