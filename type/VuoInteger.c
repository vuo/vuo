/**
 * @file
 * VuoInteger implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include <limits.h>
#include <string.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "Integer",
	"description": "A signed 64-bit integer.",
	"keywords": [ "number", "signed" ],
	"version": "1.0.0",
	"dependencies": [
		"VuoList_VuoInteger",
	],
});
#endif
/// @}

/**
 * @ingroup VuoInteger
 * Decodes the JSON object `js` to create a new value.
 */
VuoInteger VuoInteger_makeFromJson(json_object * js)
{
	if (!js)
		return 0;

	json_type t = json_object_get_type(js);
	if (t == json_type_int)
		return json_object_get_int64(js);
	else if (t == json_type_double)
	{
		double d = json_object_get_double(js);
		if (d == -INFINITY)
			return LLONG_MIN;
		if (d == INFINITY)
			return LLONG_MAX;
		return d;
	}
	else
	{
		// Use strtold() instead of json_object_get_int64(),
		// since the latter doesn't support JSON strings with scientific notation.
		// Use `long double strtold()` instead of `double atof()`
		// since `long double` can precisely represent the int64 max value, whereas `double` can't.
		const char *s = json_object_get_string(js);
		if (s)
			return strtold(s, NULL);
		else
			return 0;
	}
}

/**
 * @ingroup VuoInteger
 * Encodes @c value as a JSON object.
 */
json_object * VuoInteger_getJson(const VuoInteger value)
{
	if (value == LLONG_MIN)
		return json_object_new_double(-INFINITY);
	else if (value == LLONG_MAX)
		return json_object_new_double(INFINITY);
	else
		return json_object_new_int64(value);
}

/**
 * @ingroup VuoInteger
 * Always shows the full value, since it's guaranteed to be pretty short.
 */
char * VuoInteger_getSummary(const VuoInteger value)
{
	json_object *js = VuoInteger_getJson(value);
	char *summary = strdup(json_object_to_json_string_ext(js,JSON_C_TO_STRING_PLAIN));
	json_object_put(js);
	return summary;
}

/**
 * Returns the minimum of a list of terms, or 0 if the array is empty.
 */
VuoInteger VuoInteger_minList(VuoList_VuoInteger values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoInteger(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoInteger *integers = VuoListGetData_VuoInteger(values);
	VuoInteger min = LONG_MAX;
	for (unsigned long i = 0; i < count; ++i)
		if (integers[i] < min)
		{
			min = integers[i];
			*outputPosition = i + 1;
		}

	return min;
}

/**
 * Returns the maximum of a list of terms, or 0 if the array is empty.
 */
VuoInteger VuoInteger_maxList(VuoList_VuoInteger values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoInteger(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoInteger *integers = VuoListGetData_VuoInteger(values);
	VuoInteger max = LONG_MIN;
	for (unsigned long i = 0; i < count; ++i)
		if (integers[i] > max)
		{
			max = integers[i];
			*outputPosition = i + 1;
		}

	return max;
}

/**
 * Returns the average of the values in the list, or 0 if the list is empty.
 */
VuoInteger VuoInteger_average(VuoList_VuoInteger values)
{
	VuoInteger count = VuoListGetCount_VuoInteger(values);
	if (count == 0)
		return 0;

	VuoInteger sum = 0;
	for (VuoInteger i = 1; i <= count; ++i)
		sum += VuoListGetValue_VuoInteger(values, i);

	return sum/count;
}

/**
 * Returns @a value if it is within the range of @a minimum to @a maximum (inclusive),
 * otherwise a value wrapped with modular arithmetic to be within the range.
 */
VuoInteger VuoInteger_wrap(VuoInteger value, VuoInteger minimum, VuoInteger maximum)
{
	VuoInteger rectifiedMin = (minimum < maximum) ? minimum : maximum;
	VuoInteger rectifiedMax = (minimum < maximum) ? maximum : minimum;

	if (value > rectifiedMax)
		return rectifiedMin + ((value-rectifiedMax-1) % (rectifiedMax-rectifiedMin+1));
	else if (value < rectifiedMin)
		return rectifiedMax + ((value-rectifiedMin+1) % (rectifiedMax-rectifiedMin+1));
	else
		return value;
}

/**
 * Calculate a uniformly distributed random number less than upper_bound
 * avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers until the one
 * returned is outside the range [0, 2**32 % upper_bound).  This
 * guarantees the selected random number will be inside
 * [2**32 % upper_bound, 2**32) which maps back to [0, upper_bound)
 * after reduction modulo upper_bound.
 *
 * Copyright (c) 1999,2000,2004 Damien Miller <djm@mindrot.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
static u_int32_t VuoInteger_arc4random_uniform(u_int32_t upper_bound)
{
	u_int32_t r, min;

	if (upper_bound < 2)
		return 0;

#if (ULONG_MAX > 0xffffffffUL)
	min = 0x100000000UL % upper_bound;
#else
	/* Calculate (2**32 % upper_bound) avoiding 64-bit math */
	if (upper_bound > 0x80000000)
		min = 1 + ~upper_bound;		/* 2**32 - upper_bound */
	else {
		/* (2**32 - (x * 2)) % x == 2**32 % x when x <= 2**31 */
		min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
	}
#endif

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll.
	 */
	for (;;) {
		r = arc4random();
		if (r >= min)
			break;
	}

	return r % upper_bound;
}

/**
 * Returns a pseudorandom value between `minimum` and `maximum`, inclusive.
 *
 * This function uses `arc4random_uniform()` (the key stream generator employed by the arc4 cipher).  It is self-seeding.
 *
 * Both `maximum` and `maximum - minimum` must be `INT_MAX` (2^31) or less.
 */
VuoInteger VuoInteger_random(const VuoInteger minimum, const VuoInteger maximum)
{
	if (minimum == maximum)
		return minimum;

	VuoInteger actualMinimum = MIN(minimum,maximum);
	VuoInteger actualMaximum = MAX(minimum,maximum);

	return VuoInteger_arc4random_uniform(actualMaximum - actualMinimum + 1) + actualMinimum;
}

/**
 * Initializes `state` using the lower 48 bits of `seed`.
 */
void VuoInteger_setRandomState(unsigned short state[3], const VuoInteger seed)
{
	state[0] = seed & 0xffff;
	state[1] = (seed >> 16) & 0xffff;
	state[2] = (seed >> 32) & 0xffff;
}

/**
 * Returns a pseudorandom value between `minimum` and `maximum`, inclusive.
 *
 * This function uses `jrand48()` (a linear congruential algorithm).
 *
 * Seed it by providing 3 x 16-bit values to `state`, which is modified upon return.
 * Pass the modified `state` back to this function to retrieve the next value in the sequence.
 *
 * Both `maximum` and `maximum - minimum` must be `INT_MAX` (2^31) or less.
 */
VuoInteger VuoInteger_randomWithState(unsigned short state[3], const VuoInteger minimum, const VuoInteger maximum)
{
	if (minimum == maximum)
		return minimum;

	VuoInteger actualMinimum = MIN(minimum,maximum);
	VuoInteger actualMaximum = MAX(minimum,maximum);

	// Based on https://www.mikeash.com/pyblog/friday-qa-2011-03-18-random-numbers.html
	VuoInteger topPlusOne = actualMaximum - actualMinimum + 1;
	VuoInteger two31 = 1U << 31;
	VuoInteger maxUsable = (two31 / topPlusOne) * topPlusOne;

	while (1)
	{
		VuoInteger num = jrand48(state);
		if (num >= 0 && num < maxUsable)
			return num % topPlusOne + actualMinimum;
	}
}

/**
 * Convert between hex and decimal.
 */
static const char VuoInteger_hexToDec[256] =
{
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
 * If `byte` is a valid ASCII hex character (0–9, a–f, A–F), returns the equivalent numeric value.
 *
 * Otherwise returns 0.
 */
VuoInteger VuoInteger_makeFromHexByte(unsigned char byte)
{
	return VuoInteger_hexToDec[byte];
}

/**
 * Returns true if the two values are equal.
 */
bool VuoInteger_areEqual(const VuoInteger value1, const VuoInteger value2)
{
	return value1 == value2;
}

/**
 * Returns true if the two values are equal within `tolerance`.
 */
bool VuoInteger_areEqualListWithinTolerance(VuoList_VuoInteger values, VuoInteger tolerance)
{
	unsigned long count = VuoListGetCount_VuoInteger(values);
	if (count <= 1)
		return true;

	VuoInteger *integers = VuoListGetData_VuoInteger(values);
	VuoInteger min, max;
	min = max = integers[0];
	for (unsigned long i = 1; i < count; ++i)
	{
		min = MIN(min, integers[i]);
		max = MAX(max, integers[i]);
	}
	return (max - min) <= tolerance;
}

/**
 * Returns true if a < b.
 */
bool VuoInteger_isLessThan(const VuoInteger a, const VuoInteger b)
{
	return a < b;
}

/**
 * Returns true if `value` is between `minimum` and `maximum`.
 */
bool VuoInteger_isWithinRange(VuoInteger value, VuoInteger minimum, VuoInteger maximum)
{
    return minimum <= value && value <= maximum;
}
