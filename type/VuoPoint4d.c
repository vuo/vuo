/**
 * @file
 * VuoPoint4d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoPoint4d.h"
#include "VuoReal.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "4D Point",
					 "description" : "A floating-point 4-dimensional Cartesian spatial location.",
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
 * @ingroup VuoPoint3d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1,
 *     "z" : 0,
 *     "w" : 0
 *   }
 * }
 */
VuoPoint4d VuoPoint4d_makeFromJson(json_object * js)
{
	VuoPoint4d point = {0,0,0,0};

	if (json_object_get_type(js) == json_type_string)
	{
		const char *s = json_object_get_string(js);
		sscanf(s, "%20g, %20g, %20g, %20g", &point.x, &point.y, &point.z, &point.w);
		return point;
	}

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "z", &o))
		point.z = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "w", &o))
		point.w = VuoReal_makeFromJson(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint4d_getJson(const VuoPoint4d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = VuoReal_getJson(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = VuoReal_getJson(value.y);
	json_object_object_add(js, "y", yObject);

	json_object *zObject = VuoReal_getJson(value.z);
	json_object_object_add(js, "z", zObject);

	json_object *wObject = VuoReal_getJson(value.w);
	json_object_object_add(js, "w", wObject);

	return js;
}

/**
 * @ingroup VuoPoint4d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint4d_getSummary(const VuoPoint4d value)
{
	return VuoText_format("%g, %g, %g, %g", value.x, value.y, value.z, value.w);
}

/**
 * Returns true if the two points are equal (within tolerance).
 */
bool VuoPoint4d_areEqual(const VuoPoint4d value1, const VuoPoint4d value2)
{
	return VuoReal_areEqual(value1.x, value2.x)
		&& VuoReal_areEqual(value1.y, value2.y)
		&& VuoReal_areEqual(value1.z, value2.z)
		&& VuoReal_areEqual(value1.w, value2.w);
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_randomWithState
 */
VuoPoint4d VuoPoint4d_random(const VuoPoint4d minimum, const VuoPoint4d maximum)
{
	return VuoPoint4d_make(
				VuoReal_random(minimum.x, maximum.x),
				VuoReal_random(minimum.y, maximum.y),
				VuoReal_random(minimum.z, maximum.z),
				VuoReal_random(minimum.w, maximum.w));
}

/**
 * Returns a pseudorandom value where each component is between `minimum` and `maximum`.
 *
 * @see VuoInteger_randomWithState
 */
VuoPoint4d VuoPoint4d_randomWithState(unsigned short state[3], const VuoPoint4d minimum, const VuoPoint4d maximum)
{
	return VuoPoint4d_make(
				VuoReal_randomWithState(state, minimum.x, maximum.x),
				VuoReal_randomWithState(state, minimum.y, maximum.y),
				VuoReal_randomWithState(state, minimum.z, maximum.z),
				VuoReal_randomWithState(state, minimum.w, maximum.w));
}
