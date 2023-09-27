/**
 * @file
 * VuoRange C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoRange_h
#define VuoRange_h

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoReal.h"
#include <float.h>

/// VuoRange.minimum's value when the range has no minimum.
#define VuoRange_NoMinimum -INFINITY

/// VuoRange.maximum's value when the range has no maximum.
#define VuoRange_NoMaximum INFINITY

/**
 * Defines a range with an optionally bound/unbound min/max.
 */
typedef struct
{
	// The minimum value of a range.  (VuoRange_NoMinimum >= minimum >= maximum)
	VuoReal minimum;

	// The maximum value of a range.  (minimum >= maximum >= VuoRange_NoMaximum)
	VuoReal maximum;
} VuoRange;

#define VuoRange_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.

VuoRange VuoRange_makeFromJson(struct json_object * js);
struct json_object * VuoRange_getJson(const VuoRange value);
char * VuoRange_getSummary(const VuoRange value);

bool VuoRange_areEqual(const VuoRange a, const VuoRange b);
bool VuoRange_isLessThan(const VuoRange a, const VuoRange b);

///@{
/**
 * Automatically generated function.
 */
char * VuoRange_getString(const VuoRange value);
void VuoRange_retain(VuoRange value);
void VuoRange_release(VuoRange value);
///@}

/**
 * Creates a range.
 */
static inline VuoRange VuoRange_make(VuoReal _minimum, VuoReal _maximum) __attribute__((const));
static inline VuoRange VuoRange_make(VuoReal _minimum, VuoReal _maximum)
{
	return (VuoRange){_minimum, _maximum};
}

/**
 * Returns true if the range is inverted (min greater than max).
 */
static inline bool VuoRange_isInverted(VuoRange range) __attribute__((const));
static inline bool VuoRange_isInverted(VuoRange range)
{
	return range.minimum > range.maximum;
}

/**
 * Returns a range where the min is never greater than the max.
 */
static inline VuoRange VuoRange_getOrderedRange(VuoRange range) __attribute__((const));
static inline VuoRange VuoRange_getOrderedRange(VuoRange range)
{
	if (VuoRange_isInverted(range))
		return (VuoRange){range.maximum, range.minimum};
	else
		return range;
}

/**
 * If the range is empty (minimum == maximum), increases the maximum.
 */
static inline VuoRange VuoRange_makeNonzero(VuoRange a) __attribute__((const));
static inline VuoRange VuoRange_makeNonzero(VuoRange a)
{
	if (fabs(a.maximum - a.minimum) < FLT_EPSILON)
		a.maximum = a.minimum + FLT_EPSILON;
	return a;
}

/**
 * Returns a value clamped to range.
 *
 * @version200New
 */
static inline VuoReal VuoRange_clamp(VuoRange range, VuoReal value) __attribute__((const));
static inline VuoReal VuoRange_clamp(VuoRange range, VuoReal value)
{
	VuoReal lower = fmin(range.minimum, range.maximum);
	VuoReal upper = fmax(range.minimum, range.maximum);
	return fmin(fmax(value, lower), upper);
}

/**
 * Returns a value scaled to range.
 * If value is outside of `from` bounds it will be clamped. If either range
 * contains an infinite bound value is returned unmodified.
 *
 * @version200New
 */
static inline VuoReal VuoRange_scale(VuoRange from, VuoRange to, VuoReal value) __attribute__((const));
static inline VuoReal VuoRange_scale(VuoRange from, VuoRange to, VuoReal value)
{
	VuoReal from_lower = fmin(from.minimum, from.maximum);
	VuoReal from_upper = fmax(from.minimum, from.maximum);
	VuoReal to_lower = fmin(to.minimum, to.maximum);
	VuoReal to_upper = fmax(to.minimum, to.maximum);

	if( from_lower == VuoRange_NoMinimum || from_upper == VuoRange_NoMaximum ||
		to_lower == VuoRange_NoMinimum || to_upper == VuoRange_NoMaximum )
		return value;

	VuoReal from_range = from_upper - from_lower;
	VuoReal to_range = to_upper - to_lower;
	VuoReal v = fmax(fmin(value, from_upper), from_lower);
	VuoReal n = (v - from_lower) / from_range;

	return to_lower + (n * to_range);
}

#ifdef __cplusplus
}
#endif

#endif
