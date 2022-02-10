/**
 * @file
 * VuoTransform2d implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <string.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "2D Transform",
					 "description" : "A 2D transformation (scale, rotation, translation).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoPoint2d",
						"VuoReal",
						"VuoText",
						"VuoTransform",
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTransform2d
 * Creates a @c VuoTransform2d with no effect.
 */
VuoTransform2d VuoTransform2d_makeIdentity(void)
{
	return VuoTransform2d_make(VuoPoint2d_make(0,0), 0, VuoPoint2d_make(1,1));
}

/**
 * @ingroup VuoTransform2d
 * Creates a @c VuoTransform2d from translation, rotation (in radians), and scale values.
 */
VuoTransform2d VuoTransform2d_make(VuoPoint2d translation, VuoReal rotation, VuoPoint2d scale)
{
	VuoTransform2d t;
	t.translation = translation;
	t.rotation = rotation;
	t.scale = scale;
	return t;
}

/**
 * @ingroup VuoTransform2d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{"identity"}
 * @eg{
 *   {
 *     "translation" = [0,0],
 *     "rotation" = 1,
 *     "scale" = [1,1]
 *   }
 * }
 */
VuoTransform2d VuoTransform2d_makeFromJson(json_object *js)
{
	VuoTransform2d t = VuoTransform2d_makeIdentity();
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "rotation", &o))
		t.rotation = json_object_get_double(o);

	if (json_object_object_get_ex(js, "translation", &o))
	{
		t.translation.x = json_object_get_double(json_object_array_get_idx(o,0));
		t.translation.y = json_object_get_double(json_object_array_get_idx(o,1));
	}

	if (json_object_object_get_ex(js, "scale", &o))
	{
		t.scale.x = json_object_get_double(json_object_array_get_idx(o,0));
		t.scale.y = json_object_get_double(json_object_array_get_idx(o,1));
	}

	return t;
}

/**
 * @ingroup VuoTransform2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoTransform2d_getJson(const VuoTransform2d value)
{
	if (VuoTransform2d_isIdentity(value))
		return json_object_new_string("identity");

	json_object *js = json_object_new_object();

	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.translation.x));
		json_object_array_add(o,json_object_new_double(value.translation.y));
		json_object_object_add(js, "translation", o);
	}

	json_object_object_add(js, "rotation", json_object_new_double(value.rotation));

	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.scale.x));
		json_object_array_add(o,json_object_new_double(value.scale.y));
		json_object_object_add(js, "scale", o);
	}

	return js;
}


/**
 * @ingroup VuoTransform2d
 * Produces a brief human-readable summary of @c value.
 */
char * VuoTransform2d_getSummary(const VuoTransform2d value)
{
	if (VuoTransform2d_isIdentity(value))
		return strdup("identity transform (no change)");

	VuoReal rotationInDegrees = value.rotation * 180./M_PI;
	return VuoText_format("<div>translation (%g, %g)</div>\n<div>rotation %g°</div>\n<div>scale (%g, %g)</div>",
						  value.translation.x, value.translation.y, rotationInDegrees, value.scale.x, value.scale.y);
}

/**
 * Applies `transform` to `point`, using input Z coordinate 0 and discarding the transformed Z coordinate.
 */
VuoPoint2d VuoTransform2d_transform_VuoPoint2d(VuoTransform2d transform, VuoPoint2d point)
{
    return VuoTransform_transform_VuoPoint2d(VuoTransform_makeFrom2d(transform), point);
}

/**
 * Applies `transform` to `point`.
 */
VuoPoint3d VuoTransform2d_transform_VuoPoint3d(VuoTransform2d transform, VuoPoint3d point)
{
    return VuoTransform_transform_VuoPoint3d(VuoTransform_makeFrom2d(transform), point);
}
