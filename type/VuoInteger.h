/**
 * @file
 * vuo.integer C type definition.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINTEGER_H
#define VUOINTEGER_H

#include <stdint.h>
struct json_object;

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
typedef int64_t VuoInteger;

VuoInteger VuoInteger_makeFromJson(struct json_object * js);
struct json_object * VuoInteger_getJson(const VuoInteger value);
char * VuoInteger_getSummary(const VuoInteger value);

VuoInteger VuoInteger_min(VuoInteger *terms, unsigned long termsCount);
VuoInteger VuoInteger_max(VuoInteger *terms, unsigned long termsCount);

VuoInteger VuoInteger_wrap(VuoInteger value, VuoInteger minimum, VuoInteger maximum);

VuoInteger VuoInteger_random(const VuoInteger minimum, const VuoInteger maximum);

void VuoInteger_setRandomState(unsigned short state[3], const VuoInteger seed);
VuoInteger VuoInteger_randomWithState(unsigned short state[3], const VuoInteger minimum, const VuoInteger maximum);

VuoInteger VuoInteger_makeFromHexByte(unsigned char byte);

/**
 * @c a+b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoInteger VuoInteger_add(VuoInteger a, VuoInteger b) __attribute__((const));
static inline VuoInteger VuoInteger_add(VuoInteger a, VuoInteger b)
{
	return a+b;
}

/**
 * @c a-b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoInteger VuoInteger_subtract(VuoInteger a, VuoInteger b) __attribute__((const));
static inline VuoInteger VuoInteger_subtract(VuoInteger a, VuoInteger b)
{
	return a-b;
}

/**
 * If the value is zero, returns 1.  Otherwise returns the value.
 */
static inline VuoInteger VuoInteger_makeNonzero(VuoInteger a) __attribute__((const));
static inline VuoInteger VuoInteger_makeNonzero(VuoInteger a)
{
	return a ? a : 1;
}

/**
 *	Returns a snapped value using a center point and snap value.
 */
static inline VuoInteger VuoInteger_snap(VuoInteger a, VuoInteger center, VuoInteger snap) __attribute__((const));
static inline VuoInteger VuoInteger_snap(VuoInteger a, VuoInteger center, VuoInteger snap)
{
	if (snap == 0)
		return a;
	return center + snap * ((a-center) / snap);
}

/// @{
/**
 * Automatically generated function.
 */
VuoInteger VuoInteger_makeFromString(const char *str);
char * VuoInteger_getString(const VuoInteger value);
void VuoInteger_retain(VuoInteger value);
void VuoInteger_release(VuoInteger value);
/// @}

/**
 * @}
 */

#endif
