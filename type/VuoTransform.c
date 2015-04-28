/**
 * @file
 * VuoTransform implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
 * Start with an object pointing rightward (increasing X axis).
 * This function returns a unit vector representing the direction
 * a rightward-pointing object (+x axis) would be pointing after being transformed by @c transform.
 */
VuoPoint3d VuoTransform_getDirection(const VuoTransform transform)
{
	// Make a new transform with only the rotational component.
	VuoTransform r;
	if (transform.type == VuoTransformTypeEuler)
		r = VuoTransform_makeEuler(VuoPoint3d_make(0,0,0), transform.rotationSource.euler, VuoPoint3d_make(1,1,1));
	else
		r = VuoTransform_makeQuaternion(VuoPoint3d_make(0,0,0), transform.rotationSource.quaternion, VuoPoint3d_make(1,1,1));

	float m[16];
	VuoTransform_getMatrix(r, m);
	return VuoTransform_transformPoint(m, VuoPoint3d_make(1,0,0));
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

	VuoPoint4d q = VuoPoint4d_normalize(rotation);
	t.rotationSource.quaternion = q;
	t.rotation[0] = 1. - 2.*(q.y*q.y + q.z*q.z);
	t.rotation[1] =      2.*(q.x*q.y + q.w*q.z);
	t.rotation[2] =      2.*(q.x*q.z - q.w*q.y);
	t.rotation[3] =      2.*(q.x*q.y - q.w*q.z);
	t.rotation[4] = 1. - 2.*(q.x*q.x + q.z*q.z);
	t.rotation[5] =      2.*(q.y*q.z + q.w*q.x);
	t.rotation[6] =      2.*(q.x*q.z + q.w*q.y);
	t.rotation[7] =      2.*(q.y*q.z - q.w*q.x);
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
 * Create a transform that translates to @c position and looks at @c target with roll determined by @c upDirection.
 *
 * Similar to @c gluLookAt.
 */
VuoTransform VuoTransform_makeFromTarget(VuoPoint3d position, VuoPoint3d target, VuoPoint3d upDirection)
{
	VuoPoint3d n = VuoPoint3d_normalize(VuoPoint3d_subtract(position, target));
	VuoPoint3d u = VuoPoint3d_normalize(VuoPoint3d_crossProduct(upDirection, n));
	VuoPoint3d v = VuoPoint3d_crossProduct(n, u);

	VuoTransform t;

	t.type = VuoTransformTypeTargeted;

	t.translation = position;

	t.rotationSource.target = target;
	t.rotationSource.upDirection = upDirection;
	t.rotation[0] = u.x;
	t.rotation[1] = v.x;
	t.rotation[2] = n.x;
	t.rotation[3] = u.y;
	t.rotation[4] = v.y;
	t.rotation[5] = n.y;
	t.rotation[6] = u.z;
	t.rotation[7] = v.z;
	t.rotation[8] = n.z;

	t.scale = VuoPoint3d_make(1,1,1);

	return t;
}

/**
 * @ingroup VuoTransform
 * Transforms @c point using @c matrix (a column-major matrix of 16 values), and returns the new point.
 *
 * @see VuoTransform_getMatrix
 */
VuoPoint3d VuoTransform_transformPoint(const float *matrix, VuoPoint3d point)
{
	VuoPoint3d transformedPoint = {
		point.x * matrix[0] + point.y * matrix[4] + point.z * matrix[ 8] + matrix[12],
		point.x * matrix[1] + point.y * matrix[5] + point.z * matrix[ 9] + matrix[13],
		point.x * matrix[2] + point.y * matrix[6] + point.z * matrix[10] + matrix[14]
	};
	return transformedPoint;
}

/**
 * Transforms @c rectangle using @c matrix (a column-major matrix of 16 values), and returns the new rectangle.
 *
 * If the matrix specifies a rotation, this function returns an axis-aligned rectangle fully enclosing the source rectangle.
 *
 * @see VuoTransform_getMatrix
 */
VuoRectangle VuoTransform_transformRectangle(const float *matrix, VuoRectangle rectangle)
{
	VuoReal left	= rectangle.center.x - rectangle.size.x/2.;
	VuoReal right	= rectangle.center.x + rectangle.size.x/2.;
	VuoReal bottom	= rectangle.center.y - rectangle.size.y/2.;
	VuoReal top		= rectangle.center.y + rectangle.size.y/2.;

	VuoPoint3d topLeft		= VuoTransform_transformPoint(matrix, VuoPoint3d_make(left, top, 0.));
	VuoPoint3d topRight		= VuoTransform_transformPoint(matrix, VuoPoint3d_make(right, top, 0.));
	VuoPoint3d bottomLeft	= VuoTransform_transformPoint(matrix, VuoPoint3d_make(left, bottom, 0.));
	VuoPoint3d bottomRight	= VuoTransform_transformPoint(matrix, VuoPoint3d_make(right, bottom, 0.));

	VuoReal transformedLeft		= MIN(MIN(MIN(topLeft.x, topRight.x), bottomLeft.x), bottomRight.x);
	VuoReal transformedRight	= MAX(MAX(MAX(topLeft.x, topRight.x), bottomLeft.x), bottomRight.x);
	VuoReal transformedBottom	= MIN(MIN(MIN(topLeft.y, topRight.y), bottomLeft.y), bottomRight.y);
	VuoReal transformedTop		= MAX(MAX(MAX(topLeft.y, topRight.y), bottomLeft.y), bottomRight.y);

	VuoRectangle transformedRectangle = VuoRectangle_make(
				(transformedLeft + transformedRight)/2.,
				(transformedBottom + transformedTop)/2.,
				transformedRight - transformedLeft,
				transformedTop - transformedBottom);

	return transformedRectangle;
}

/**
 * Returns a column-major matrix of 16 values that transforms a 1x1 quad so that it renders the specified image at real (pixel-perfect) size.
 */
void VuoTransform_getBillboardMatrix(VuoInteger imageWidth, VuoInteger imageHeight, VuoReal translationX, VuoReal translationY, VuoInteger viewportWidth, VuoInteger viewportHeight, float *billboardMatrix)
{
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), billboardMatrix);

	// Apply scale to make the image appear at real size (1:1).
	billboardMatrix[0] = 2. * imageWidth/viewportWidth;
	billboardMatrix[5] = billboardMatrix[0] * imageHeight/imageWidth;

	// Apply 2D translation.
		// Align the translation to pixel boundaries
		billboardMatrix[12] = floor((translationX+1.)/2.*viewportWidth) / ((float)viewportWidth) * 2. - 1.;
		billboardMatrix[13] = floor((translationY+1.)/2.*viewportWidth) / ((float)viewportWidth) * 2. - 1.;

		// Account for odd-dimensioned image
		billboardMatrix[12] += (imageWidth % 2 ? (1./viewportWidth) : 0);
		billboardMatrix[13] -= (imageHeight % 2 ? (1./viewportWidth) : 0);

		// Account for odd-dimensioned viewport
		billboardMatrix[13] += (viewportWidth  % 2 ? (1./viewportWidth) : 0);
		billboardMatrix[13] -= (viewportHeight % 2 ? (1./viewportWidth) : 0);
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

	if (json_object_object_get_ex(js, "target", &o))
	{
		VuoPoint3d target;
		target.x = json_object_get_double(json_object_array_get_idx(o,0));
		target.y = json_object_get_double(json_object_array_get_idx(o,1));
		target.z = json_object_get_double(json_object_array_get_idx(o,2));

		VuoPoint3d position = VuoPoint3d_make(0,0,0);
		if (json_object_object_get_ex(js, "translation", &o))
		{
			position.x = json_object_get_double(json_object_array_get_idx(o,0));
			position.y = json_object_get_double(json_object_array_get_idx(o,1));
			position.z = json_object_get_double(json_object_array_get_idx(o,2));
		}

		VuoPoint3d upDirection = VuoPoint3d_make(0,1,0);
		if (json_object_object_get_ex(js, "upDirection", &o))
		{
			upDirection.x = json_object_get_double(json_object_array_get_idx(o,0));
			upDirection.y = json_object_get_double(json_object_array_get_idx(o,1));
			upDirection.z = json_object_get_double(json_object_array_get_idx(o,2));
		}

		return VuoTransform_makeFromTarget(position, target, upDirection);
	}

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
	else if (value.type == VuoTransformTypeEuler)
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.x));
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.y));
		json_object_array_add(o,json_object_new_double(value.rotationSource.euler.z));
		json_object_object_add(js, "eulerRotation", o);
	}
	else
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.rotationSource.target.x));
		json_object_array_add(o,json_object_new_double(value.rotationSource.target.y));
		json_object_array_add(o,json_object_new_double(value.rotationSource.target.z));
		json_object_object_add(js, "target", o);

		o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(value.rotationSource.upDirection.x));
		json_object_array_add(o,json_object_new_double(value.rotationSource.upDirection.y));
		json_object_array_add(o,json_object_new_double(value.rotationSource.upDirection.z));
		json_object_object_add(js, "upDirection", o);
	}

	if (value.type != VuoTransformTypeTargeted)
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

	if (value.type == VuoTransformTypeTargeted)
	{
		const char *format = "position (%g, %g, %g)<br>target (%g, %g, %g)<br>up (%g, %g, %g)";
		int size = snprintf(NULL, 0, format, value.translation.x, value.translation.y, value.translation.z, value.rotationSource.target.x, value.rotationSource.target.y, value.rotationSource.target.z, value.rotationSource.upDirection.x, value.rotationSource.upDirection.y, value.rotationSource.upDirection.z);
		char *valueAsString = (char *)malloc(size+1);
		snprintf(valueAsString, size+1, format, value.translation.x, value.translation.y, value.translation.z, value.rotationSource.target.x, value.rotationSource.target.y, value.rotationSource.target.z, value.rotationSource.upDirection.x, value.rotationSource.upDirection.y, value.rotationSource.upDirection.z);
		return valueAsString;
	}

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
