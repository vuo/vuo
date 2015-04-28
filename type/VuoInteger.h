/**
 * @file
 * vuo.integer C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINTEGER_H
#define VUOINTEGER_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoInteger VuoInteger
 * A signed 64-bit integer.
 *
 * @{
 */

/**
 * A signed 64-bit integer.
 */
typedef signed long VuoInteger;

VuoInteger VuoInteger_valueFromJson(struct json_object * js);
struct json_object * VuoInteger_jsonFromValue(const VuoInteger value);
char * VuoInteger_summaryFromValue(const VuoInteger value);

VuoInteger VuoInteger_min(VuoInteger *terms, unsigned long termsCount);
VuoInteger VuoInteger_max(VuoInteger *terms, unsigned long termsCount);

/**
 *	Returns a snapped value using a center point and snap value.
 */
static inline VuoInteger VuoInteger_snap(VuoInteger a, VuoInteger center, VuoInteger snap) __attribute__((const));
static inline VuoInteger VuoInteger_snap(VuoInteger a, VuoInteger center, VuoInteger snap)
{
	return center + snap * ((a-center) / snap);
}

/// @{
/**
 * Automatically generated function.
 */
VuoInteger VuoInteger_valueFromString(const char *str);
char * VuoInteger_stringFromValue(const VuoInteger value);
/// @}

/**
 * @}
 */

#endif
