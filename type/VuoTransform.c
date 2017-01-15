/**
 * @file
 * VuoTransform implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "type.h"
#include "VuoTransform.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "3D Transform",
					 "description" : "A 3D transformation (scale, rotation, translation).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoInteger",
						"VuoPoint3d",
						"VuoPoint4d",
						"VuoText",
						"VuoTransform2d"
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
 * Outputs the inverse of `matrix`
 * (which is assumed to consist of a rotation followed by a scale followed by a translation,
 * like the output of @ref VuoTransform_getMatrix),
 * such that `outputInvertedMatrix * matrix = identityMatrix`.
 */
void VuoTransform_invertMatrix4x4(const float *matrix, float *outputInvertedMatrix)
{
	float inverseTranslation[16];
	VuoTransform_getMatrix(VuoTransform_makeEuler(
							   VuoPoint3d_make(-matrix[12], -matrix[13], -matrix[14]),
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_make(1,1,1)),
			inverseTranslation);

	// Transpose the rotation/scale part of the input matrix (which undoes the rotation), and set the last row/column to identity.
	float inverseRotation[16] = {
		matrix[0],
		matrix[4],
		matrix[8],
		0,

		matrix[1],
		matrix[5],
		matrix[9],
		0,

		matrix[2],
		matrix[6],
		matrix[10],
		0,

		0,
		0,
		0,
		1
	};

	float inverseScale[16];
	VuoTransform_getMatrix(VuoTransform_makeEuler(
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_make(0,0,0),
							   VuoPoint3d_make(
								   1/VuoPoint3d_magnitude(VuoPoint3d_make(matrix[0], matrix[1], matrix[2])),
								   1/VuoPoint3d_magnitude(VuoPoint3d_make(matrix[4], matrix[5], matrix[6])),
								   1/VuoPoint3d_magnitude(VuoPoint3d_make(matrix[8], matrix[9], matrix[10])))),
			inverseScale);

	VuoTransform_multiplyMatrices4x4(inverseTranslation, inverseRotation, outputInvertedMatrix);

	VuoTransform_multiplyMatrices4x4(outputInvertedMatrix, inverseScale, outputInvertedMatrix);

	// Apply inverseScale a second time, since inverseRotation includes forward scale.
	VuoTransform_multiplyMatrices4x4(outputInvertedMatrix, inverseScale, outputInvertedMatrix);
}

/**
 * Returns the transform's rotation, represented as euler angles in radians (see @ref VuoTransform_makeEuler).
 */
VuoPoint3d VuoTransform_getEuler(const VuoTransform transform)
{
	if (transform.type == VuoTransformTypeEuler)
		return transform.rotationSource.euler;

	// "Euler Angle Conversion" by Ken Shoemake.  Graphics Gems IV, pp. 222–229.
	VuoPoint3d ea;
	double cy = sqrt(transform.rotation[0]*transform.rotation[0] + transform.rotation[1]*transform.rotation[1]);
	if (cy > 16*FLT_EPSILON)
	{
		ea.z = atan2(transform.rotation[5], transform.rotation[8]);
		ea.y = atan2(-transform.rotation[2], cy);
		ea.x = atan2(transform.rotation[1], transform.rotation[0]);
	}
	else
	{
		ea.z = atan2(-transform.rotation[7], transform.rotation[4]);
		ea.y = atan2(-transform.rotation[2], cy);
		ea.x = 0;
	}

	return ea;
}

/**
 * Returns the transform's rotation, represented as a quaternion (see @ref VuoTransform_makeQuaternion).
 */
VuoPoint4d VuoTransform_getQuaternion(const VuoTransform transform)
{
	if (transform.type == VuoTransformTypeQuaternion)
		return transform.rotationSource.quaternion;

	// Convert the euler angles to a quaternion.
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/
	VuoPoint3d eu = transform.rotationSource.euler;
	double c1 = cos(eu.y/2);
	double s1 = sin(eu.y/2);
	double c2 = cos(eu.x/2);
	double s2 = sin(eu.x/2);
	double c3 = cos(eu.z/2);
	double s3 = sin(eu.z/2);
	double c1c2 = c1*c2;
	double s1s2 = s1*s2;
	return VuoPoint4d_make(
		c1c2*s3 + s1s2*c3,
		s1*c2*c3 + c1*s2*s3,
		c1*s2*c3 - s1*c2*s3,
		c1c2*c3 - s1s2*s3);
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
	VuoTransform t;

	t.type = VuoTransformTypeEuler;

	t.translation = (VuoPoint3d){0,0,0};

	t.rotationSource.euler = (VuoPoint3d){0,0,0};
	t.rotation[0] = 1;
	t.rotation[1] = 0;
	t.rotation[2] = 0;
	t.rotation[3] = 0;
	t.rotation[4] = 1;
	t.rotation[5] = 0;
	t.rotation[6] = 0;
	t.rotation[7] = 0;
	t.rotation[8] = 1;

	t.scale = (VuoPoint3d){1,1,1};

	return t;
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
 *
 * `VuoPoint4d_make(0,0,0,1)` (w=1, x=y=z=0) is the rotational identity.
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
	t.rotation[3] = v.x;
	t.rotation[6] = n.x;
	t.rotation[1] = u.y;
	t.rotation[4] = v.y;
	t.rotation[7] = n.y;
	t.rotation[2] = u.z;
	t.rotation[5] = v.z;
	t.rotation[8] = n.z;

	t.scale = VuoPoint3d_make(1,1,1);

	return t;
}

/**
 * Creates a transform from the specified `matrix` (assumed to consist of affine rotation, scale, and translation).
 */
VuoTransform VuoTransform_makeFromMatrix4x4(const float *matrix)
{
	VuoTransform t;

	t.scale = VuoPoint3d_make(
				VuoPoint3d_magnitude(VuoPoint3d_make(matrix[0], matrix[1], matrix[2])),
				VuoPoint3d_magnitude(VuoPoint3d_make(matrix[4], matrix[5], matrix[6])),
				VuoPoint3d_magnitude(VuoPoint3d_make(matrix[8], matrix[9], matrix[10])));

	t.rotation[0] = matrix[ 0] / t.scale.x;
	t.rotation[1] = matrix[ 1] / t.scale.x;
	t.rotation[2] = matrix[ 2] / t.scale.x;

	t.rotation[3] = matrix[ 4] / t.scale.y;
	t.rotation[4] = matrix[ 5] / t.scale.y;
	t.rotation[5] = matrix[ 6] / t.scale.y;

	t.rotation[6] = matrix[ 8] / t.scale.z;
	t.rotation[7] = matrix[ 9] / t.scale.z;
	t.rotation[8] = matrix[10] / t.scale.z;

	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
	t.type = VuoTransformTypeQuaternion;
	t.rotationSource.quaternion.w = sqrt( fmax( 0, 1 + t.rotation[0] + t.rotation[4] + t.rotation[8] ) ) / 2;
	t.rotationSource.quaternion.x = sqrt( fmax( 0, 1 + t.rotation[0] - t.rotation[4] - t.rotation[8] ) ) / 2;
	t.rotationSource.quaternion.y = sqrt( fmax( 0, 1 - t.rotation[0] + t.rotation[4] - t.rotation[8] ) ) / 2;
	t.rotationSource.quaternion.z = sqrt( fmax( 0, 1 - t.rotation[0] - t.rotation[4] + t.rotation[8] ) ) / 2;
	t.rotationSource.quaternion.x = copysign( t.rotationSource.quaternion.x, t.rotation[5] - t.rotation[4] );
	t.rotationSource.quaternion.y = copysign( t.rotationSource.quaternion.y, t.rotation[6] - t.rotation[2] );
	t.rotationSource.quaternion.z = copysign( t.rotationSource.quaternion.z, t.rotation[1] - t.rotation[3] );

	t.translation = VuoPoint3d_make(matrix[12], matrix[13], matrix[14]);

	return t;
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
 * Returns a composite transformation, consisting of `a` followed by `b`.
 */
VuoTransform VuoTransform_composite(const VuoTransform a, const VuoTransform b)
{
	float aMatrix[16];
	VuoTransform_getMatrix(a, aMatrix);

	float bMatrix[16];
	VuoTransform_getMatrix(b, bMatrix);

	float compositeMatrix[16];
	VuoTransform_multiplyMatrices4x4(aMatrix, bMatrix, compositeMatrix);

	return VuoTransform_makeFromMatrix4x4(compositeMatrix);
}

/**
 * Returns a quaternion representing the rotation of the specified basis matrix.
 */
VuoPoint4d VuoTransform_quaternionFromBasis(VuoPoint3d basis[3])
{
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
	VuoPoint4d q;
	q.w = sqrt(1 + basis[0].x + basis[1].y + basis[2].z) / 2;
	q.x = (basis[2].y - basis[1].z) / (4 * q.w);
	q.y = (basis[0].z - basis[2].x) / (4 * q.w);
	q.z = (basis[1].x - basis[0].y) / (4 * q.w);
	return q;
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
VuoTransform VuoTransform_makeFromJson(json_object *js)
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
 * If the float is negative or positive zero, return positive zero.
 */
static inline float cook(float f)
{
	if (fabs(f) < FLT_EPSILON)
		return 0;

	return f;
}

/**
 * @ingroup VuoTransform
 * Encodes @c value as a JSON object.
 */
json_object * VuoTransform_getJson(const VuoTransform value)
{
	if (VuoTransform_isIdentity(value))
		return json_object_new_string("identity");

	json_object *js = json_object_new_object();

	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.translation.x)));
		json_object_array_add(o,json_object_new_double(cook(value.translation.y)));
		json_object_array_add(o,json_object_new_double(cook(value.translation.z)));
		json_object_object_add(js, "translation", o);
	}

	// Don't store value.rotation, since we can calculate it from the source rotation.

	if (value.type == VuoTransformTypeQuaternion)
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.quaternion.x)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.quaternion.y)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.quaternion.z)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.quaternion.w)));
		json_object_object_add(js, "quaternionRotation", o);
	}
	else if (value.type == VuoTransformTypeEuler)
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.euler.x)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.euler.y)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.euler.z)));
		json_object_object_add(js, "eulerRotation", o);
	}
	else
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.target.x)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.target.y)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.target.z)));
		json_object_object_add(js, "target", o);

		o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.upDirection.x)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.upDirection.y)));
		json_object_array_add(o,json_object_new_double(cook(value.rotationSource.upDirection.z)));
		json_object_object_add(js, "upDirection", o);
	}

	if (value.type != VuoTransformTypeTargeted)
	{
		json_object * o = json_object_new_array();
		json_object_array_add(o,json_object_new_double(cook(value.scale.x)));
		json_object_array_add(o,json_object_new_double(cook(value.scale.y)));
		json_object_array_add(o,json_object_new_double(cook(value.scale.z)));
		json_object_object_add(js, "scale", o);
	}

	return js;
}


/**
 * @ingroup VuoTransform
 * Produces a brief human-readable summary of @c value.
 */
char * VuoTransform_getSummary(const VuoTransform value)
{
	if (VuoTransform_isIdentity(value))
		return strdup("identity transform (no change)");

	if (value.type == VuoTransformTypeTargeted)
		return VuoText_format("position (%g, %g, %g)<br>target (%g, %g, %g)<br>up (%g, %g, %g)",
							  value.translation.x, value.translation.y, value.translation.z, value.rotationSource.target.x, value.rotationSource.target.y, value.rotationSource.target.z, value.rotationSource.upDirection.x, value.rotationSource.upDirection.y, value.rotationSource.upDirection.z);

	char *rotation;
	if (value.type == VuoTransformTypeQuaternion)
		rotation = VuoText_format("(%g, %g, %g, %g) quaternion",
								  value.rotationSource.quaternion.x, value.rotationSource.quaternion.y, value.rotationSource.quaternion.z, value.rotationSource.quaternion.w);
	else
	{
		VuoPoint3d r = VuoPoint3d_multiply(value.rotationSource.euler, 180./M_PI);
		rotation = VuoText_format("(%g°, %g°, %g°) euler",
								  r.x, r.y, r.z);
	}

	char *valueAsString = VuoText_format("translation (%g, %g, %g)<br>rotation %s<br>scale (%g, %g, %g)",
										 value.translation.x, value.translation.y, value.translation.z, rotation, value.scale.x, value.scale.y, value.scale.z);
	free(rotation);
	return valueAsString;
}
