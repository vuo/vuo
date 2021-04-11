/**
 * @file
 * VuoReal implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include <limits.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Real",
					 "description" : "A floating-point number.",
					 "keywords" : [ "double", "float", "number" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoReal",
						"VuoInteger",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoReal
 * Decodes the JSON object `js` to create a new value.
 */
VuoReal VuoReal_makeFromJson(json_object * js)
{
	if (!js)
		return 0;

	json_type t = json_object_get_type(js);
	if (t == json_type_double)
		return json_object_get_double(js);
	else if (t == json_type_int)
		return json_object_get_int64(js);
	else
	{
		// Use atof() instead of json_object_get_double(),
		// since the latter doesn't support JSON strings with scientific notation
		// or nonnumeric characters following the number.
		const char *s = json_object_get_string(js);
		if (s)
			return atof(s);
		else
			return 0;
	}
}

/**
 * @ingroup VuoReal
 * Encodes @c value as a JSON object.
 */
json_object * VuoReal_getJson(const VuoReal value)
{
	// json spec doesn't support inf or nan by default,
	// but VuoReal_makeFromJson does.
	if( !isfinite(value) )
	{
		if(isnan(value))
			return json_object_new_string("nan");
		else if(value == -INFINITY)
			return json_object_new_string("-inf");
		else
			return json_object_new_string("inf");
	}

	return json_object_new_double(value);
}

/**
 * @ingroup VuoReal
 * Returns a string representation of @c value (either decimal or scientific notation, whichever is shorter).
 */
char * VuoReal_getSummary(const VuoReal value)
{
	// See VuoDoubleSpinBox::textFromValue.
	return VuoText_format("%.11g", value);
}

/**
 * Returns the minimum of a list of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_minList(VuoList_VuoReal values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoReal(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoReal *reals = VuoListGetData_VuoReal(values);
	VuoReal min = DBL_MAX;
	for (unsigned long i = 0; i < count; ++i)
		if (reals[i] < min)
		{
			min = reals[i];
			*outputPosition = i + 1;
		}

	return min;
}

/**
 * Returns the maximum of a list of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_maxList(VuoList_VuoReal values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoReal(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoReal *reals = VuoListGetData_VuoReal(values);
	VuoReal max = -DBL_MAX;
	for (unsigned long i = 0; i < count; ++i)
		if (reals[i] > max)
		{
			max = reals[i];
			*outputPosition = i + 1;
		}

	return max;
}

/**
 * Returns the average of the values in the list, or 0 if the list is empty.
 */
VuoReal VuoReal_average(VuoList_VuoReal values)
{
	VuoInteger count = VuoListGetCount_VuoReal(values);
	if (count == 0)
		return 0;

	VuoReal sum = 0;
	for (VuoInteger i = 1; i <= count; ++i)
		sum += VuoListGetValue_VuoReal(values, i);

	return sum/count;
}

/**
 * Returns @a value if it is within the range of @a minimum (inclusive) to @a maximum (exclusive),
 * otherwise a value wrapped with modular arithmetic to be within the range.
 *
 * @version200Changed{Made `minimum` inclusive and `maximum` exclusive.}
 */
VuoReal VuoReal_wrap(VuoReal value, VuoReal minimum, VuoReal maximum)
{
	value -= minimum;
	value = fmod(value, maximum - minimum);
	if (value < 0)
		value += maximum - minimum;
	value += minimum;

	// Pretend IEEE 754 "signed zero" doesn't exist since it's unnecessarily confusing.
	if (value == -0)
		value = 0;

	return value;
}

/**
 * Returns a pseudorandom value between `minimum` and `maximum`.
 *
 * @see VuoInteger_random
 */
VuoReal VuoReal_random(const VuoReal minimum, const VuoReal maximum)
{
	return ((VuoReal)VuoInteger_random(0, INT_MAX) / (VuoReal)(INT_MAX)) * (maximum - minimum) + minimum;
}

/**
 * Returns a pseudorandom value between `minimum` and `maximum`.
 *
 * @see VuoInteger_randomWithState
 */
VuoReal VuoReal_randomWithState(unsigned short state[3], const VuoReal minimum, const VuoReal maximum)
{
	return ((VuoReal)VuoInteger_randomWithState(state, 0, INT_MAX) / (VuoReal)(INT_MAX)) * (maximum - minimum) + minimum;
}

/**
 * Returns true if the two values are equal (within a small tolerance).
 */
bool VuoReal_areEqual(const VuoReal value1, const VuoReal value2)
{
	// https://stackoverflow.com/questions/1565164/what-is-the-rationale-for-all-comparisons-returning-false-for-ieee754-nan-values
	if( isnan(value1) || isnan(value2) )
		return false;
	// when comparing inf or -inf don't use fuzzy values
	else if( !isfinite(value1) || !isfinite(value2) )
		return value1 == value2;
	else
		return fabs(value1 - value2) <= 0.00001;
}

/**
 * Returns true if the two values are equal within `tolerance`.
 */
bool VuoReal_areEqualListWithinTolerance(VuoList_VuoReal values, VuoReal tolerance)
{
	unsigned long count = VuoListGetCount_VuoReal(values);
	if (count <= 1)
		return true;

	VuoReal *reals = VuoListGetData_VuoReal(values);
	VuoReal min, max;
	min = max = reals[0];
	for (unsigned long i = 1; i < count; ++i)
	{
		min = MIN(min, reals[i]);
		max = MAX(max, reals[i]);
	}
	return (max - min) <= tolerance;
}

/**
 * Returns true if a < b.
 */
bool VuoReal_isLessThan(const VuoReal a, const VuoReal b)
{
	return a < b;
}

/**
 * Returns true if `value` is between `minimum` and `maximum`.
 */
bool VuoReal_isWithinRange(VuoReal value, VuoReal minimum, VuoReal maximum)
{
    return minimum <= value && value <= maximum;
}
