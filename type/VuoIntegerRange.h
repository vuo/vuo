/**
 * @file
 * VuoIntegerRange C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInteger.h"

/// VuoIntegerRange.minimum's value when the range has no minimum.
#define VuoIntegerRange_NoMinimum INT64_MIN
/// VuoIntegerRange.maximum's value when the range has no maximum.
#define VuoIntegerRange_NoMaximum INT64_MAX

/**
 * Defines an int range with an optionally bound/unbound min/max.
 */
typedef struct
{
	// The minimum value of a range.  (VuoIntegerRange_min >= minimum >= maximum)
	VuoInteger minimum;
	// The maximum value of a range.  (minimum >= maximum >= VuoIntegerRange_max)
	VuoInteger maximum;
} VuoIntegerRange;

VuoIntegerRange VuoIntegerRange_makeFromJson(struct json_object * js);
struct json_object * VuoIntegerRange_getJson(const VuoIntegerRange value);
char * VuoIntegerRange_getSummary(const VuoIntegerRange value);

///@{
/**
 * Automatically generated function.
 */
VuoIntegerRange VuoIntegerRange_makeFromString(const char *str);
char * VuoIntegerRange_getString(const VuoIntegerRange value);
void VuoIntegerRange_retain(VuoIntegerRange value);
void VuoIntegerRange_release(VuoIntegerRange value);
///@}

/**
 * Creates an integer range.
 */
static inline VuoIntegerRange VuoIntegerRange_make(VuoInteger _minimum, VuoInteger _maximum) __attribute__((const));
static inline VuoIntegerRange VuoIntegerRange_make(VuoInteger _minimum, VuoInteger _maximum)
{
	return (VuoIntegerRange){_minimum, _maximum};
}

/**
 * Returns true if the range is inverted (min greater than max).
 */
static inline bool VuoIntegerRange_isInverted(VuoIntegerRange range) __attribute__((const));
static inline bool VuoIntegerRange_isInverted(VuoIntegerRange range)
{
	return range.minimum > range.maximum;
}

/**
 * Returns a range where the min is never greater than the max.
 */
static inline VuoIntegerRange VuoIntegerRange_getOrderedRange(VuoIntegerRange range) __attribute__((const));
static inline VuoIntegerRange VuoIntegerRange_getOrderedRange(VuoIntegerRange range)
{
	if (VuoIntegerRange_isInverted(range))
		return (VuoIntegerRange){range.maximum, range.minimum};
	else
		return range;
}
