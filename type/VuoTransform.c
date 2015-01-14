/**
 * @file
 * VuoTransform implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "type.h"
#include "VuoTransform.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "3D Transform",
					 "description" : "A 3D transformation (scale, rotation, translation).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoTransform
 * Converts @c value into a column-major matrix of 16 values,
 * composed as a rotation followed by a scale followed by a translation.
 */
void VuoTransform_getMatrix(const VuoTransform value, float *matrix)
{
	matrix[ 0] = value.rotation[0] * value.scale.x;
	matrix[ 1] = value.rotation[1] * value.scale.x;
	matrix[ 2] = value.rotation[2] * value.scale.x;
	matrix[ 3] = 0;

	matrix[ 4] = value.rotation[3] * value.scale.y;
	matrix[ 5] = value.rotation[4] * value.scale.y;
	matrix[ 6] = value.rotation[5] * value.scale.y;
	matrix[ 7] = 0;

	matrix[ 8] = value.rotation[6] * value.scale.z;
	matrix[ 9] = value.rotation[7] * value.scale.z;
	matrix[10] = value.rotation[8] * value.scale.z;
	matrix[11] = 0;

	matrix[12] = value.translation.x;
	matrix[13] = value.translation.y;
	matrix[14] = value.translation.z;
	matrix[15] = 1;
}

/**
 * @ingroup VuoTransform
 * Creates a @c VuoTransform with no effect.
 */
VuoTransform VuoTransform_makeIdentity(void)
{
	return VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1));
}

/**
 * @ingroup VuoTransform
 * Creates a @c VuoTransform from translation, rotation (Euler angles, in radians), and scale values.
 */
VuoTransform VuoTransform_makeEuler(VuoPoint3d translation, VuoPoint3d rotation, VuoPoint3d scale)
{
	VuoTransform t;

	t.type = VuoTransformTypeEuler;

	t.translation = translation;

	t.rotationSource.euler = rotation;
	// Rotate along the x axis, followed by y, follwed by z.
	t.rotation[0] = cos(rotation.y)*cos(rotation.z);
	t.rotation[1] = cos(rotation.y)*sin(rotation.z);
	t.rotation[2] = -sin(rotation.y);
	t.rotation[3] = cos(rotation.z)*sin(rotation.x)*sin(rotation.y) - cos(rotation.x)*sin(rotation.z);
	t.rotation[4] = cos(rotation.x)*cos(rotation.z) + sin(rotation.x)*sin(rotation.y)*sin(rotation.z);
	t.rotation[5] = cos(rotation.y)*sin(rotation.x);
	t.rotation[6] = cos(rotation.x)*cos(rotation.z)*sin(rotation.y) + sin(rotation.x)*sin(rotation.z);
	t.rotation[7] = -cos(rotation.z)*sin(rotation.x) + cos(rotation.x)*sin(rotation.y)*sin(rotation.z);
	t.rotation[8] = cos(rotation.x)*cos(rotation.y);

	t.scale = scale;

	return t;
}

/**
 * @ingroup VuoTransform
 * Creates a @c VuoTransform from translation, rotation (quaternion), and scale values.
 */
VuoTransform VuoTransform_makeQuaternion(VuoPoint3d translation, VuoPoint4d rotation, VuoPoint3d scale)
{
	VuoTransform t;

	t.type = VuoTransformTypeQuaternion;

	t.translation = translation;

	t.rotationSource.quaternion = rotation;
	VuoPoint4d q = VuoPoint4d_normalize4d(rotation);
	t.rotation[0] = 1. - 2.*(q.y*q.y + q.z*q.z);
	t.rotation[1] =      2.*(q.x*q.y - q.w*q.z);
	t.rotation[2] =      2.*(q.x*q.z + q.w*q.y);
	t.rotation[3] =      2.*(q.x*q.y + q.w*q.z);
	t.rotation[4] = 1. - 2.*(q.x*q.x + q.z*q.z);
	t.rotation[5] =      2.*(q.y*q.z - q.w*q.x);
	t.rotation[6] =      2.*(q.x*q.z - q.w*q.y);
	t.rotation[7] =      2.*(q.y*q.z + q.w*q.x);
	t.rotation[8] = 1. - 2.*(q.x*q.x + q.y*q.y);

	t.scale = scale;

	return t;
}

/**
 * Creates a 3D transform from a 2D transform.
 */
VuoTransform VuoTransform_makeFrom2d(VuoTransform2d transform2d)
{
	VuoPoint3d center3d = VuoPoint3d_make(transform2d.translation.x, transform2d.translation.y, 0);
	VuoPoint3d rotation3d = VuoPoint3d_make(0, 0, transform2d.rotation);
	VuoPoint3d scale3d = VuoPoint3d_make(transform2d.scale.x, transform2d.scale.y, 1);
	return VuoTransform_makeEuler(center3d, rotation3d, scale3d);
}

/**
 * @ingroup VuoTransform
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{"identity"}
 * @eg{
 *   {
 *     "quaternionRotation" = [0,0,0,1],
 *     "translation" = [0,0,0],
 *     "scale" = [1,1,1]
 *   }
 * }
 * @eg{
 *   {
 *     "eulerRotation" = [0,0,0],
 *     "translation" = [0,0,0],
 *     "scale" = [1,1,1]
 *   }
 * }
 */
VuoTransform VuoTransform_valueFromJson(json_object *js)
{
	VuoTransform t = VuoTransform_makeIdentity();
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "quaternionRotation", &o))
	{
		t.type = VuoTransformTypeQuaternion;
		VuoPoint4d q;
		q.x = json_object_get_double(json_object_array_get_idx(o,0));
		q.y = json_object_get_double(json_object_array_get_idx(o,1));
		q.z = json_object_get_double(json_object_array_get_idx(o,2));
		q.w = json_object_get_double(json_object_array_get_idx(o,3));
		t = VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0), q, VuoPoint3d_make(0,0,0));
	}
	else if (json_object_object_get_ex(js, "eulerRotation", &o))
	{
		t.type = VuoTransformTypeEuler;
		VuoPoint3d e;
		e.x = json_object_get_double(json_object_array_get_idx(o,0));
		e.y = json_object_get_double(json_object_array_get_idx(o,1));
		e.z = json_object_get_double(json_object_array_get_idx(o,2));
		t = VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), e, VuoPoint3d_make(0,0,0));
	}

	if (json_object_object_get_ex(js, "translation", &o))
	{
		t.translation.x = json_object_get_double(json_object_array_get_idx(o,0));
		t.translation.y = json_object_get_double(json_object_array_get_idx(o,1));
		t.translation.z = json_object_get_double(json_object_array_get_idx(o,2));
	}

	if (json_object_object_get_ex(js, "scale", &o))
	{
		t.scale.x = json_object_get_double(json_object_array_get_idx(o,0));
		t.scale.y = json_object_get_double(json_object_array_get_idx(o,1));
		t.scale.z = json_object_get_double(json_object_array_get_idx(o,2));
	}

	return t;
}

/**
 * @ingroup VuoTransform
 * Encodes @c value as a JSON object.
 */
json_object * VuoTransform_jsonFromValue(const VuoTransform value)
{
	if (VuoTransform_isIdentity(value))
		return json_object_new_string("identity");

	json_object *js = json_object_new_object();

	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.translation.x));
		json_object_array_add(o,json_object_new_double(value.translation.y));
		json_object_array_add(o,json_object_new_double(value.translation.z));
		json_object_object_add(js, "translation", o);
	}

	// Don't store value.rotation, since we can calculate it from the source rotation.

	if (value.type == VuoTransformTypeQuaternion)
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.rotationSource.quaternion.x));
		json_object_array_add(o,json_object_new_double(value.rotationSource.quaternion.y));
		json_object_array_add(o,json_object_new_double(value.rotationSource.quaternion.z));
		json_object_array_add(o,json_object_new_double(value.rotationSource.quaternion.w));
		json_object_object_add(js, "quaternionRotation", o);
	}
	else
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.x));
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.y));
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.z));
		json_object_object_add(js, "eulerRotation", o);
	}

	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.scale.x));
		json_object_array_add(o,json_object_new_double(value.scale.y));
		json_object_array_add(o,json_object_new_double(value.scale.z));
		json_object_object_add(js, "scale", o);
	}

	return js;
}


/**
 * @ingroup VuoTransform
 * Produces a brief human-readable summary of @c value.
 */
char * VuoTransform_summaryFromValue(const VuoTransform value)
{
	if (VuoTransform_isIdentity(value))
		return strdup("identity transform (no change)");

	const char *format = "translation (%g, %g, %g)<br>rotation %s<br>scale (%g, %g, %g)";

	char *rotation;
	if (value.type == VuoTransformTypeQuaternion)
	{
		const char *format = "(%g, %g, %g, %g) quaternion";
		int size = snprintf(NULL, 0, format, value.rotationSource.quaternion.x, value.rotationSource.quaternion.y, value.rotationSource.quaternion.z, value.rotationSource.quaternion.w);
		rotation = (char *)malloc(size+1);
		snprintf(rotation, size+1, format, value.rotationSource.quaternion.x, value.rotationSource.quaternion.y, value.rotationSource.quaternion.z, value.rotationSource.quaternion.w);
	}
	else
	{
		const char *format = "(%g°, %g°, %g°) euler";
		VuoPoint3d r = VuoPoint3d_multiply(value.rotationSource.euler, 180./M_PI);
		int size = snprintf(NULL, 0, format, r.x, r.y, r.z);
		rotation = (char *)malloc(size+1);
		snprintf(rotation, size+1, format, r.x, r.y, r.z);
	}

	int size = snprintf(NULL, 0, format, value.translation.x, value.translation.y, value.translation.z, rotation, value.scale.x, value.scale.y, value.scale.z);
	char * valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString, size+1, format, value.translation.x, value.translation.y, value.translation.z, rotation, value.scale.x, value.scale.y, value.scale.z);
	free(rotation);
	return valueAsString;
}
