/**
 * @file
 * VuoNoise C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUONOISE_H
#define VUONOISE_H

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

VuoNoise VuoNoise_valueFromJson(struct json_object * js);
struct json_object * VuoNoise_jsonFromValue(const VuoNoise value);
char * VuoNoise_summaryFromValue(const VuoNoise value);

/// @{
/**
 * Automatically generated function.
 */
VuoNoise VuoNoise_valueFromString(const char *str);
char * VuoNoise_stringFromValue(const VuoNoise value);
/// @}

/**
 * @}
*/

#endif
