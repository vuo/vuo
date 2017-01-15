/**
 * @file
 * VuoPoint3d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoPoint3d.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "3D Point",
					 "description" : "A floating-point 3-dimensional Cartesian spatial location.",
					 "keywords" : [ "coordinate" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoBoolean",
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
 *     "z" : 0
 *   }
 * }
 */
VuoPoint3d VuoPoint3d_makeFromJson(json_object * js)
{
	VuoPoint3d point = {0,0,0};

	if (json_object_get_type(js) == json_type_string)
	{
		const char *s = json_object_get_string(js);
		sscanf(s, "%20g, %20g, %20g", &point.x, &point.y, &point.z);
		return point;
	}

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = json_object_get_double(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = json_object_get_double(o);

	if (json_object_object_get_ex(js, "z", &o))
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
