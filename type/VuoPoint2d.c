/**
 * @file
 * VuoPoint2d implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "2D Point",
					 "description" : "A floating-point 2-dimensional Cartesian spatial location.",
					 "keywords" : [ "coordinate" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoReal",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoPoint2d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1
 *   }
 * }
 */
VuoPoint2d VuoPoint2d_makeFromJson(json_object * js)
{
	VuoPoint2d point = {0,0};

	json_type t = json_object_get_type(js);
	if (t == json_type_string)
	{
		const char *s = json_object_get_string(js);
		float x, y;
		sscanf(s, "%20g, %20g", &x, &y);
		return (VuoPoint2d){x, y};
	}
	else if (t == json_type_array)
	{
		int len = json_object_array_length(js);
		if (len >= 1)
			point.x = json_object_get_double(json_object_array_get_idx(js, 0));
		if (len >= 2)
			point.y = json_object_get_double(json_object_array_get_idx(js, 1));
		return point;
	}

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = VuoReal_makeFromJson(o);
	else if (json_object_object_get_ex(js, "X", &o))
		point.x = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = VuoReal_makeFromJson(o);
	else if (json_object_object_get_ex(js, "Y", &o))
		point.y = VuoReal_makeFromJson(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint2d_getJson(const VuoPoint2d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = VuoReal_getJson(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = VuoReal_getJson(value.y);
	json_object_object_add(js, "y", yObject);

	return js;
}

/**
 * @ingroup VuoPoint2d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint2d_getSummary(const VuoPoint2d value)
{
	return VuoText_format("%g, %g", value.x, value.y);
}

/**
 * Returns true if the two points are equal (within tolerance).
 */
bool VuoPoint2d_areEqual(const VuoPoint2d value1, const VuoPoint2d value2)
{
	return fabs(value1.x - value2.x) < 0.00001
		&& fabs(value1.y - value2.y) < 0.00001;
}

/**
 * Compares `a` to `b` primarily by `x`-value and secondarily by `y`-value,
 * returning true if `a` is less than `b`.
 *
 * @version200New
 */
bool VuoPoint2d_isLessThan(const VuoPoint2d a, const VuoPoint2d b)
{
	VuoType_returnInequality(VuoReal, a.x, b.x);
	VuoType_returnInequality(VuoReal, a.y, b.y);
	return false;
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_random
 */
VuoPoint2d VuoPoint2d_random(const VuoPoint2d minimum, const VuoPoint2d maximum)
{
	return VuoPoint2d_make(
				VuoReal_random(minimum.x, maximum.x),
				VuoReal_random(minimum.y, maximum.y));
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_randomWithState
 */
VuoPoint2d VuoPoint2d_randomWithState(unsigned short state[3], const VuoPoint2d minimum, const VuoPoint2d maximum)
{
	return VuoPoint2d_make(
				VuoReal_randomWithState(state, minimum.x, maximum.x),
				VuoReal_randomWithState(state, minimum.y, maximum.y));
}
