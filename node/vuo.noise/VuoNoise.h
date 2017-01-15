/**
 * @file
 * VuoNoise C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONOISE_H
#define VUONOISE_H

/// @{
typedef const struct VuoList_VuoNoise_struct { void *l; } * VuoList_VuoNoise;
#define VuoList_VuoNoise_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoNoise VuoNoise
 * An enum defining different types of noise.
 *
 * @{
 */

/**
 * An enum defining different types of noise.
 */
typedef enum {
	VuoNoise_White,
	VuoNoise_Grey,
	VuoNoise_Pink,
	VuoNoise_Brown,
	VuoNoise_Blue,
	VuoNoise_Violet
} VuoNoise;

VuoNoise VuoNoise_makeFromJson(struct json_object * js);
struct json_object * VuoNoise_getJson(const VuoNoise value);
VuoList_VuoNoise VuoNoise_getAllowedValues(void);
char * VuoNoise_getSummary(const VuoNoise value);

/// @{
/**
 * Automatically generated function.
 */
VuoNoise VuoNoise_makeFromString(const char *str);
char * VuoNoise_getString(const VuoNoise value);
/// @}

/**
 * @}
*/

#endif
