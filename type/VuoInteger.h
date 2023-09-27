/**
 * @file
 * VuoInteger C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoInteger_h
#define VuoInteger_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

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

#define VuoInteger_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoInteger.h"

VuoInteger VuoInteger_makeFromJson(struct json_object * js);
struct json_object * VuoInteger_getJson(const VuoInteger value);
char * VuoInteger_getSummary(const VuoInteger value);

VuoInteger VuoInteger_minList(VuoList_VuoInteger values, VuoInteger *outputPosition);
VuoInteger VuoInteger_maxList(VuoList_VuoInteger values, VuoInteger *outputPosition);
VuoInteger VuoInteger_average(VuoList_VuoInteger values);

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
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoInteger VuoInteger_multiply(VuoInteger a, VuoInteger b) __attribute__((const));
static inline VuoInteger VuoInteger_multiply(VuoInteger a, VuoInteger b)
{
	return a*b;
}

/**
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoInteger VuoInteger_scale(VuoInteger a, VuoInteger b) __attribute__((const));
static inline VuoInteger VuoInteger_scale(VuoInteger a, VuoInteger b)
{
	return a*b;
}

/**
 * @c a/b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoInteger VuoInteger_divide(VuoInteger a, VuoInteger b) __attribute__((const));
static inline VuoInteger VuoInteger_divide(VuoInteger a, VuoInteger b)
{
    return a/b;
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

bool VuoInteger_areEqual(const VuoInteger value1, const VuoInteger value2);
bool VuoInteger_areEqualListWithinTolerance(VuoList_VuoInteger values, VuoInteger tolerance);
bool VuoInteger_isLessThan(const VuoInteger a, const VuoInteger b);
bool VuoInteger_isWithinRange(VuoInteger value, VuoInteger minimum, VuoInteger maximum);

#ifndef MIN
/**
 * Returns the smaller of @c a and @c b.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
/**
 * Returns the larger of @c a and @c b.
 */
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/**
 * Limits `value` to values between `min` and `max`, inclusive.
 */
static inline VuoInteger VuoInteger_clamp(VuoInteger value, VuoInteger limitA, VuoInteger limitB)
{
	return MIN(MAX(value, MIN(limitA, limitB)), MAX(limitA,limitB));
}

/**
 * Same as @ref VuoInteger_clamp.
 * Provided for generic type compatibility with other `_clampn` functions.
 */
static inline VuoInteger VuoInteger_clampn(VuoInteger value, VuoInteger limitA, VuoInteger limitB)
{
	return VuoInteger_clamp(value, limitA, limitB);
}

/// @{
/**
 * Automatically generated function.
 */
char * VuoInteger_getString(const VuoInteger value);
void VuoInteger_retain(VuoInteger value);
void VuoInteger_release(VuoInteger value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
