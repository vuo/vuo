/**
 * @file
 * VuoRange C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

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

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoRange;

VuoRange VuoRange_makeFromJson(struct json_object * js);
struct json_object * VuoRange_getJson(const VuoRange value);
char * VuoRange_getSummary(const VuoRange value);

///@{
/**
 * Automatically generated function.
 */
VuoRange VuoRange_makeFromString(const char *str);
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
	VuoRange range = { _minimum, _maximum, "" };
	return range;
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
		return (VuoRange){range.maximum, range.minimum, ""};
	else
		return range;
}
