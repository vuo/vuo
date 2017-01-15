/**
 * @file
 * VuoReal implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoText.h"
#include "VuoList_VuoReal.h"
#include <float.h>
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
 * @ingroup VuoSceneObject
 * Decodes the JSON object @c js, expected to contain a double, to create a new value.
 */
VuoReal VuoReal_makeFromJson(json_object * js)
{
	return json_object_get_double(js);
}

/**
 * @ingroup VuoReal
 * Encodes @c value as a JSON object.
 */
json_object * VuoReal_getJson(const VuoReal value)
{
	return json_object_new_double(value);
}

/**
 * @ingroup VuoReal
 * Returns a string representation of @c value (either decimal or scientific notation, whichever is shorter).
 */
char * VuoReal_getSummary(const VuoReal value)
{
	return VuoText_format("%g", value);
}

/**
 * Returns the minimum of an array of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_min(VuoReal *terms, unsigned long termsCount)
{
	if (termsCount == 0)
		return 0;

	VuoReal min = DBL_MAX;
	for (unsigned long i = 0; i < termsCount; ++i)
		min = MIN(min, terms[i]);

	return min;
}

/**
 * Returns the maximum of an array of terms, or 0 if the array is empty.
 */
VuoReal VuoReal_max(VuoReal *terms, unsigned long termsCount)
{
	if (termsCount == 0)
		return 0;

	VuoReal max = -DBL_MAX;
	for (unsigned long i = 0; i < termsCount; ++i)
		max = MAX(max, terms[i]);

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
 * Returns @a value if it is within the range of @a minimum to @a maximum (exclusive),
 * otherwise a value wrapped with modular arithmetic to be within the range.
 */
VuoReal VuoReal_wrap(VuoReal value, VuoReal minimum, VuoReal maximum)
{
	if (value > maximum)
		return minimum + fmod(value-maximum, maximum-minimum);
	else if (value < minimum)
		return maximum - fmod(minimum-value, maximum-minimum);
	else
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
