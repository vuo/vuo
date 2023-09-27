/**
 * @file
 * VuoPoint3d implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "VuoPoint3d.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
    "title": "3D Point",
    "description": "A floating-point 3-dimensional Cartesian spatial location.",
    "keywords": [ "coordinate" ],
    "version": "1.0.0",
    "dependencies": [
        "VuoList_VuoPoint3d",
        "VuoBoolean",
        "VuoReal",
        "VuoText",
    ],
});
#endif
/// @}

/**
 * @ingroup VuoPoint3d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1,
 *     "z" : 0
 *   }
 * }
 */
VuoPoint3d VuoPoint3d_makeFromJson(json_object * js)
{
	VuoPoint3d point = {0,0,0};

	json_type t = json_object_get_type(js);
	if (t == json_type_string)
	{
		const char *s = json_object_get_string(js);
		float x, y, z;
		sscanf(s, "%20g, %20g, %20g", &x, &y, &z);
		return (VuoPoint3d){x, y, z};
	}
	else if (t == json_type_array)
	{
		int len = json_object_array_length(js);
		if (len >= 1)
			point.x = json_object_get_double(json_object_array_get_idx(js, 0));
		if (len >= 2)
			point.y = json_object_get_double(json_object_array_get_idx(js, 1));
		if (len >= 3)
			point.z = json_object_get_double(json_object_array_get_idx(js, 2));
		return point;
	}

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = json_object_get_double(o);
	else if (json_object_object_get_ex(js, "X", &o))
		point.x = json_object_get_double(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = json_object_get_double(o);
	else if (json_object_object_get_ex(js, "Y", &o))
		point.y = json_object_get_double(o);

	if (json_object_object_get_ex(js, "z", &o))
		point.z = json_object_get_double(o);
	else if (json_object_object_get_ex(js, "Z", &o))
		point.z = json_object_get_double(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint3d_getJson(const VuoPoint3d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = json_object_new_double(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = json_object_new_double(value.y);
	json_object_object_add(js, "y", yObject);

	json_object *zObject = json_object_new_double(value.z);
	json_object_object_add(js, "z", zObject);

	return js;
}

/**
 * @ingroup VuoPoint3d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint3d_getSummary(const VuoPoint3d value)
{
	return VuoText_format("%g, %g, %g", value.x, value.y, value.z);
}

/**
 * Returns true if the two points are equal (within tolerance).
 */
bool VuoPoint3d_areEqual(const VuoPoint3d value1, const VuoPoint3d value2)
{
	return VuoReal_areEqual(value1.x, value2.x)
		&& VuoReal_areEqual(value1.y, value2.y)
		&& VuoReal_areEqual(value1.z, value2.z);
}

/**
 * Returns true if the two values are equal within component-wise `tolerance`.
 */
bool VuoPoint3d_areEqualListWithinTolerance(VuoList_VuoPoint3d values, VuoPoint3d tolerance)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(values);
	if (count <= 1)
		return true;

	VuoPoint3d *points = VuoListGetData_VuoPoint3d(values);
	VuoPoint3d min, max;
	min = max = points[0];
	for (unsigned long i = 1; i < count; ++i)
	{
		min = VuoPoint3d_min(min, points[i]);
		max = VuoPoint3d_max(max, points[i]);
	}
	VuoPoint3d diff = max - min;
	return diff.x <= tolerance.x
		&& diff.y <= tolerance.y
		&& diff.z <= tolerance.z;
}

/**
 * Compares `a` to `b` primarily by `x`-value, secondarily by `y`-value,
 * and finally by `z`-value, returning true if `a` is less than `b`.
 *
 * @version200New
 */
bool VuoPoint3d_isLessThan(const VuoPoint3d a, const VuoPoint3d b)
{
	VuoType_returnInequality(VuoReal, a.x, b.x);
	VuoType_returnInequality(VuoReal, a.y, b.y);
	VuoType_returnInequality(VuoReal, a.z, b.z);
	return false;
}

/**
 * Returns true if each component of `value` is between `minimum` and `maximum`.
 */
bool VuoPoint3d_isWithinRange(VuoPoint3d value, VuoPoint3d minimum, VuoPoint3d maximum)
{
	return minimum.x <= value.x && value.x <= maximum.x
		&& minimum.y <= value.y && value.y <= maximum.y
		&& minimum.z <= value.z && value.z <= maximum.z;
}

/**
 * Returns the minimum of a list of terms, or 0 if the array is empty.
 */
VuoPoint3d VuoPoint3d_minList(VuoList_VuoPoint3d values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoPoint3d *points = VuoListGetData_VuoPoint3d(values);
	VuoPoint3d min = FLT_MAX;
	for (unsigned long i = 0; i < count; ++i)
		if (VuoPoint3d_isLessThan(points[i], min))
		{
			min = points[i];
			*outputPosition = i + 1;
		}

	return min;
}

/**
 * Returns the maximum of a list of terms, or 0 if the array is empty.
 */
VuoPoint3d VuoPoint3d_maxList(VuoList_VuoPoint3d values, VuoInteger *outputPosition)
{
	unsigned long count = VuoListGetCount_VuoPoint3d(values);
	if (count == 0)
	{
		*outputPosition = 0;
		return 0;
	}

	VuoPoint3d *points = VuoListGetData_VuoPoint3d(values);
	VuoPoint3d max = -FLT_MAX;
	for (unsigned long i = 0; i < count; ++i)
		if (VuoPoint3d_isLessThan(max, points[i]))
		{
			max = points[i];
			*outputPosition = i + 1;
		}

	return max;
}

/**
 * Returns the average of the values in the list, or 0 if the list is empty.
 */
VuoPoint3d VuoPoint3d_average(VuoList_VuoPoint3d values)
{
	VuoInteger count = VuoListGetCount_VuoPoint3d(values);
	if (count == 0)
		return 0;

	VuoPoint3d sum = 0;
	for (VuoInteger i = 1; i <= count; ++i)
		sum += VuoListGetValue_VuoPoint3d(values, i);

	VuoPoint3d divisor = count;
	return sum / divisor;
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_random
 */
VuoPoint3d VuoPoint3d_random(const VuoPoint3d minimum, const VuoPoint3d maximum)
{
	return VuoPoint3d_make(
				VuoReal_random(minimum.x, maximum.x),
				VuoReal_random(minimum.y, maximum.y),
				VuoReal_random(minimum.z, maximum.z));
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_randomWithState
 */
VuoPoint3d VuoPoint3d_randomWithState(unsigned short state[3], const VuoPoint3d minimum, const VuoPoint3d maximum)
{
	return VuoPoint3d_make(
				VuoReal_randomWithState(state, minimum.x, maximum.x),
				VuoReal_randomWithState(state, minimum.y, maximum.y),
				VuoReal_randomWithState(state, minimum.z, maximum.z));
}
