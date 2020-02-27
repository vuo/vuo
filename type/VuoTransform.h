/**
 * @file
 * VuoTransform C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInteger.h"
#include <math.h>
#include <float.h>
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
#include "VuoRectangle.h"
#include "VuoTransform2d.h"
#include <stdio.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoTransform VuoTransform
 * A 3D transformation (scale, rotation, translation).
 *
 * @{
 */

/**
 * The type of rotation description used to construct this transform.
 */
enum VuoTransformType
{
	VuoTransformTypeEuler,
	VuoTransformTypeQuaternion,
	VuoTransformTypeTargeted
};

/**
 * A 3D transformation (scale, rotation, translation).
 *
 * Instead of a precalculated 4x4 matrix, it's stored as separate operations
 * in order to [minimize drift](http://bitsquid.blogspot.com/2012/07/matrices-rotation-scale-and-drifting.html).
 */
typedef struct
{
	VuoPoint3d translation;
	float rotation[9]; ///< Column-major 3x3 matrix
	VuoPoint3d scale;

	// The following values are stored so we can display a meaningful summary.
	enum VuoTransformType type;
	struct
	{
		VuoPoint3d euler; ///< Radians
		VuoPoint4d quaternion;
		struct
		{
			VuoPoint3d target;
			VuoPoint3d upDirection;
		};
	} rotationSource;
} VuoTransform;

void VuoTransform_getMatrix(const VuoTransform value, float *matrix) __attribute__((nonnull));
void VuoTransform_getBillboardMatrix(VuoInteger imageWidth, VuoInteger imageHeight, VuoReal imageScaleFactor, VuoBoolean preservePhysicalSize, VuoReal translationX, VuoReal translationY, VuoInteger viewportWidth, VuoInteger viewportHeight, VuoReal backingScaleFactor, VuoPoint2d mesh0, float *billboardMatrix)  __attribute__((nonnull));
VuoPoint3d VuoTransform_getEuler(const VuoTransform transform);
VuoPoint4d VuoTransform_getQuaternion(const VuoTransform transform);
VuoPoint3d VuoTransform_getDirection(const VuoTransform transform);
VuoTransform VuoTransform_makeIdentity(void);
VuoTransform VuoTransform_makeEuler(VuoPoint3d translation, VuoPoint3d rotation, VuoPoint3d scale);
VuoTransform VuoTransform_makeQuaternion(VuoPoint3d translation, VuoPoint4d rotation, VuoPoint3d scale);
void VuoTransform_rotationMatrixFromQuaternion(const VuoPoint4d quaternion, float *matrix) __attribute__((nonnull));
void VuoTransform_rotationMatrixFromEuler(const VuoPoint3d euler, float *matrix) __attribute__((nonnull));

VuoTransform VuoTransform_makeFrom2d(VuoTransform2d transform2d);
VuoTransform2d VuoTransform_get2d(VuoTransform transform);

VuoTransform VuoTransform_makeFromTarget(VuoPoint3d position, VuoPoint3d target, VuoPoint3d upDirection);
VuoTransform VuoTransform_makeFromMatrix4x4(const float *matrix) __attribute__((nonnull));
VuoTransform VuoTransform_composite(const VuoTransform a, const VuoTransform b);

/**
 * Returns the composite of quaternion @c a with quaternion @c b (i.e., the rotation described by @c a followed by the rotation described by @c b).
 */
static inline VuoPoint4d VuoTransform_quaternionComposite(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionComposite(VuoPoint4d a, VuoPoint4d b)
{
	VuoPoint4d q;
	// Hamilton product (a.w, a.x, a.y, a.z) * (b.w, b.x, b.y, b.z)
	q.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
	q.y = a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z;
	q.z = a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x;
	q.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
	return VuoPoint4d_normalize(q);
}

/**
 * Returns the quaternion describing the rotation of @c angle radians about @c axis.
 */
static inline VuoPoint4d VuoTransform_quaternionFromAxisAngle(VuoPoint3d axis, float angle) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionFromAxisAngle(VuoPoint3d axis, float angle)
{
	VuoPoint4d q;
	VuoPoint3d axisNormalized = VuoPoint3d_normalize(axis);
	q.x = axisNormalized.x * sinf(angle/2.f);
	q.y = axisNormalized.y * sinf(angle/2.f);
	q.z = axisNormalized.z * sinf(angle/2.f);
	q.w = cosf(angle/2.f);
	return VuoPoint4d_normalize(q);
}

VuoPoint4d VuoTransform_quaternionFromBasis(VuoPoint3d basis[3]);

/**
 * Returns the quaternion describing the rotation from direction @a from to @a to.
 *
 * If either @a from or @a to has magnitude 0, then returns the identity quaternion (no rotation).
 */
static inline VuoPoint4d VuoTransform_quaternionFromVectors(VuoPoint3d from, VuoPoint3d to) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionFromVectors(VuoPoint3d from, VuoPoint3d to)
{
	// http://books.google.com/books?id=hiBFUv_FT0wC&pg=PA214&lpg=PA214&dq=Stan+Melax's+article+in+Game+Programming+Gems&source=bl&ots=OCjDPwza1h&sig=6_bDSzTrnI3qCEG9vtVV_mDBgg8&hl=en&sa=X&ei=ffReUsSaBIrMqAGg6YD4Aw&ved=0CCsQ6AEwAA#v=onepage&q=Stan%20Melax's%20article%20in%20Game%20Programming%20Gems&f=false
	VuoPoint4d q = { 0, 0, 0, 1 };

	if (VuoPoint3d_magnitude(from) == 0 || VuoPoint3d_magnitude(to) == 0)
		return q;

	VuoPoint3d fromNormalized = VuoPoint3d_normalize(from);
	VuoPoint3d toNormalized = VuoPoint3d_normalize(to);

	VuoPoint3d 	c = VuoPoint3d_crossProduct(fromNormalized, toNormalized);
	float		d = VuoPoint3d_dotProduct(fromNormalized, toNormalized);
	float		s = sqrtf( (1+d)*2 );

	q.x = c.x / s;
	q.y = c.y / s;
	q.z = c.z / s;
	q.w = s / 2.f;

	return q;
}

/**
 * Create a unit quaternion from a rotation matrix (3x3).
 * https://web.archive.org/web/20170705120459/http://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
 */
static inline VuoPoint4d VuoTransform_quaternionFromMatrix(const float *rotation) __attribute__((nonnull));
static inline VuoPoint4d VuoTransform_quaternionFromMatrix(const float *rotation)
{
	float t;
	VuoPoint4d q;

	float m00 = rotation[0],
		m01 = rotation[1],
		m02 = rotation[2],
		m10 = rotation[3],
		m11 = rotation[4],
		m12 = rotation[5],
		m20 = rotation[6],
		m21 = rotation[7],
		m22 = rotation[8];

	if (m22 < 0)
	{
		if (m00 > m11)
		{
			t = 1 + m00 - m11 - m22;
			q = VuoPoint4d_make( t, m01+m10, m20+m02, m12-m21 );
		}
		else
		{
			t = 1 - m00 + m11 - m22;
			q = VuoPoint4d_make( m01+m10, t, m12+m21, m20-m02 );
		}
	}
	else
	{
		if (m00 < -m11)
		{
			t = 1 - m00 - m11 + m22;
			q = VuoPoint4d_make( m20+m02, m12+m21, t, m01-m10 );
		}
		else
		{
			t = 1 + m00 + m11 + m22;
			q = VuoPoint4d_make( m12-m21, m20-m02, m01-m10, t );
		}
	}

	return VuoPoint4d_multiply(q, 0.5f / sqrt(t));
}

/**
 * Convert an euler angle (radians) to a quaternion.
 * https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
 */
static inline VuoPoint4d VuoTransform_quaternionFromEuler(const VuoPoint3d euler) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionFromEuler(const VuoPoint3d euler)
{
	VuoPoint4d q;

	double t0 = cos(euler.z * 0.5);
	double t1 = sin(euler.z * 0.5);
	double t2 = cos(euler.x * 0.5);
	double t3 = sin(euler.x * 0.5);
	double t4 = cos(euler.y * 0.5);
	double t5 = sin(euler.y * 0.5);

	q.w = t0 * t2 * t4 + t1 * t3 * t5;
	q.x = t0 * t3 * t4 - t1 * t2 * t5;
	q.y = t0 * t2 * t5 + t1 * t3 * t4;
	q.z = t1 * t2 * t4 - t0 * t3 * t5;

	return VuoPoint4d_normalize(q);
}

/**
 * Convert a quaternion to an euler angle.
 * https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
 */
static inline VuoPoint3d VuoTransform_eulerFromQuaternion(const VuoPoint4d quaternion) __attribute__((const));
static inline VuoPoint3d VuoTransform_eulerFromQuaternion(const VuoPoint4d quaternion)
{
	VuoPoint3d ea;

	double ysqr = quaternion.y * quaternion.y;

	// roll (x-axis rotation)
	double t0 = + 2.0 * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
	double t1 = + 1.0 - 2.0 * (quaternion.x * quaternion.x + ysqr);
	ea.x = atan2(t0, t1);

	// pitch (y-axis rotation)
	double t2 = +2.0 * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
	t2 = t2 > 1.0 ? 1.0 : t2;
	t2 = t2 < -1.0 ? -1.0 : t2;
	ea.y = asin(t2);

	// yaw (z-axis rotation)
	double t3 = +2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
	double t4 = +1.0 - 2.0 * (ysqr + quaternion.z * quaternion.z);
	ea.z = atan2(t3, t4);

	return ea;
}

/**
 * Convert a rotation matrix (3x3) to an euler angle.
 */
static inline VuoPoint3d VuoTransform_eulerFromMatrix(const float *matrix) __attribute__((nonnull));
static inline VuoPoint3d VuoTransform_eulerFromMatrix(const float *matrix)
{
	// "Euler Angle Conversion" by Ken Shoemake.  Graphics Gems IV, pp. 222–229.
	VuoPoint3d ea;

	double cy = sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1]);

	if (cy > 16 * FLT_EPSILON)
	{
		ea.x = atan2f( matrix[5], matrix[8]);
		ea.y = atan2f(-matrix[2], cy);
		ea.z = atan2f( matrix[1], matrix[0]);
	}
	else
	{
		ea.x = atan2f(-matrix[7], matrix[4]);
		ea.y = atan2f(-matrix[2], cy);
		ea.z = 0;
	}

	return ea;
}

/**
 * Rotates 3D vector `v` by quaternion `q`.
 */
static inline VuoPoint3d VuoTransform_rotateVectorWithQuaternion(const VuoPoint3d v, const VuoPoint4d q) __attribute__((const));
static inline VuoPoint3d VuoTransform_rotateVectorWithQuaternion(const VuoPoint3d v, const VuoPoint4d q)
{
	// https://math.stackexchange.com/questions/40164/how-do-you-rotate-a-vector-by-a-unit-quaternion/535223
	VuoPoint4d vQuaternion = (VuoPoint4d){v.x, v.y, v.z, 0};
	VuoPoint4d qConjugate = (VuoPoint4d){-q.x, -q.y, -q.z, q.w};
	VuoPoint4d result = VuoTransform_quaternionComposite(VuoTransform_quaternionComposite(q, vQuaternion), qConjugate);
	return (VuoPoint3d){result.x, result.y, result.z};
}

/**
 * Returns true if the transform is an identity (i.e., causes no change).
 */
static inline bool VuoTransform_isIdentity(const VuoTransform transform)
{
	const float tolerance = 0.00001f;
	return     fabs(transform.translation.x) < tolerance
			&& fabs(transform.translation.y) < tolerance
			&& fabs(transform.translation.z) < tolerance
			&& fabs(transform.scale.x - 1.) < tolerance
			&& fabs(transform.scale.y - 1.) < tolerance
			&& fabs(transform.scale.z - 1.) < tolerance
			&& (
				(transform.type == VuoTransformTypeEuler
				 && fabs(transform.rotationSource.euler.x) < tolerance
				 && fabs(transform.rotationSource.euler.y) < tolerance
				 && fabs(transform.rotationSource.euler.z) < tolerance
				)
				||
				(transform.type == VuoTransformTypeQuaternion
				 && fabs(transform.rotationSource.quaternion.x) < tolerance
				 && fabs(transform.rotationSource.quaternion.y) < tolerance
				 && fabs(transform.rotationSource.quaternion.z) < tolerance
				 && (fabs(transform.rotationSource.quaternion.w - 1.) < tolerance || fabs(transform.rotationSource.quaternion.w + 1.) < tolerance)
				 )
				||
				(transform.type == VuoTransformTypeTargeted
				 && fabs(transform.rotationSource.target.x - 1.) < tolerance
				 && fabs(transform.rotationSource.target.y) < tolerance
				 && fabs(transform.rotationSource.target.z) < tolerance
				 )
				);
}

/**
 * Multiplies the specified matrices.
 */
static inline void VuoTransform_multiplyMatrices4x4(const float *a, const float *b, float *outputMatrix) __attribute__((nonnull));
static inline void VuoTransform_multiplyMatrices4x4(const float *a, const float *b, float *outputMatrix)
{
	outputMatrix[0*4+0] = a[0*4+0] * b[0*4+0] + a[0*4+1] * b[1*4+0] + a[0*4+2] * b[2*4+0] + a[0*4+3] * b[3*4+0];
	outputMatrix[0*4+1] = a[0*4+0] * b[0*4+1] + a[0*4+1] * b[1*4+1] + a[0*4+2] * b[2*4+1] + a[0*4+3] * b[3*4+1];
	outputMatrix[0*4+2] = a[0*4+0] * b[0*4+2] + a[0*4+1] * b[1*4+2] + a[0*4+2] * b[2*4+2] + a[0*4+3] * b[3*4+2];
	outputMatrix[0*4+3] = a[0*4+0] * b[0*4+3] + a[0*4+1] * b[1*4+3] + a[0*4+2] * b[2*4+3] + a[0*4+3] * b[3*4+3];
	outputMatrix[1*4+0] = a[1*4+0] * b[0*4+0] + a[1*4+1] * b[1*4+0] + a[1*4+2] * b[2*4+0] + a[1*4+3] * b[3*4+0];
	outputMatrix[1*4+1] = a[1*4+0] * b[0*4+1] + a[1*4+1] * b[1*4+1] + a[1*4+2] * b[2*4+1] + a[1*4+3] * b[3*4+1];
	outputMatrix[1*4+2] = a[1*4+0] * b[0*4+2] + a[1*4+1] * b[1*4+2] + a[1*4+2] * b[2*4+2] + a[1*4+3] * b[3*4+2];
	outputMatrix[1*4+3] = a[1*4+0] * b[0*4+3] + a[1*4+1] * b[1*4+3] + a[1*4+2] * b[2*4+3] + a[1*4+3] * b[3*4+3];
	outputMatrix[2*4+0] = a[2*4+0] * b[0*4+0] + a[2*4+1] * b[1*4+0] + a[2*4+2] * b[2*4+0] + a[2*4+3] * b[3*4+0];
	outputMatrix[2*4+1] = a[2*4+0] * b[0*4+1] + a[2*4+1] * b[1*4+1] + a[2*4+2] * b[2*4+1] + a[2*4+3] * b[3*4+1];
	outputMatrix[2*4+2] = a[2*4+0] * b[0*4+2] + a[2*4+1] * b[1*4+2] + a[2*4+2] * b[2*4+2] + a[2*4+3] * b[3*4+2];
	outputMatrix[2*4+3] = a[2*4+0] * b[0*4+3] + a[2*4+1] * b[1*4+3] + a[2*4+2] * b[2*4+3] + a[2*4+3] * b[3*4+3];
	outputMatrix[3*4+0] = a[3*4+0] * b[0*4+0] + a[3*4+1] * b[1*4+0] + a[3*4+2] * b[2*4+0] + a[3*4+3] * b[3*4+0];
	outputMatrix[3*4+1] = a[3*4+0] * b[0*4+1] + a[3*4+1] * b[1*4+1] + a[3*4+2] * b[2*4+1] + a[3*4+3] * b[3*4+1];
	outputMatrix[3*4+2] = a[3*4+0] * b[0*4+2] + a[3*4+1] * b[1*4+2] + a[3*4+2] * b[2*4+2] + a[3*4+3] * b[3*4+2];
	outputMatrix[3*4+3] = a[3*4+0] * b[0*4+3] + a[3*4+1] * b[1*4+3] + a[3*4+2] * b[2*4+3] + a[3*4+3] * b[3*4+3];
}

/**
 * Copies @c sourceMatrix to @c destMatrix.
 */
static inline void VuoTransform_copyMatrix4x4(const float *sourceMatrix, float *destMatrix) __attribute__((nonnull));
static inline void VuoTransform_copyMatrix4x4(const float *sourceMatrix, float *destMatrix)
{
	for (int i=0; i<16; ++i)
		destMatrix[i] = sourceMatrix[i];
}

/**
 * Returns the translation specified in `matrix`.
 * @version200New
 */
static inline VuoPoint3d VuoTransform_getMatrix4x4Translation(const float *matrix) __attribute__((nonnull));
static inline VuoPoint3d VuoTransform_getMatrix4x4Translation(const float *matrix)
{
	return (VuoPoint3d){matrix[12], matrix[13], matrix[14]};
}

/**
 * Returns the scale specified in `matrix`.
 * @version200New
 */
static inline VuoPoint3d VuoTransform_getMatrix4x4Scale(const float *matrix) __attribute__((nonnull));
static inline VuoPoint3d VuoTransform_getMatrix4x4Scale(const float *matrix)
{
	return (VuoPoint3d){
		VuoPoint3d_magnitude((VuoPoint3d){matrix[0], matrix[1], matrix[ 2]}),
		VuoPoint3d_magnitude((VuoPoint3d){matrix[4], matrix[5], matrix[ 6]}),
		VuoPoint3d_magnitude((VuoPoint3d){matrix[8], matrix[9], matrix[10]})
	};
}

/**
 * Prints the specified column-major matrix.
 */
static inline void VuoTransform_printMatrix4x4(const float *matrix) __attribute__((nonnull));
static inline void VuoTransform_printMatrix4x4(const float *matrix)
{
	for (int i=0; i<4; ++i)
		fprintf(stderr, "[ %11.6f %11.6f %11.6f %11.6f ]\n",matrix[i+0*4],matrix[i+1*4],matrix[i+2*4],matrix[i+3*4]);
}

void VuoTransform_invertMatrix4x4(const float *matrix, float *outputInvertedMatrix) __attribute__((nonnull));

/**
 * @ingroup VuoTransform
 * Transforms @c point using @c matrix (a column-major matrix of 16 values), and returns the new point.
 *
 * @see VuoTransform_getMatrix
 */
static inline VuoPoint3d VuoTransform_transformPoint(const float *matrix, VuoPoint3d point) __attribute__((nonnull));
static inline VuoPoint3d VuoTransform_transformPoint(const float *matrix, VuoPoint3d point)
{
	return (VuoPoint3d){
		point.x * matrix[0] + point.y * matrix[4] + point.z * matrix[ 8] + matrix[12],
		point.x * matrix[1] + point.y * matrix[5] + point.z * matrix[ 9] + matrix[13],
		point.x * matrix[2] + point.y * matrix[6] + point.z * matrix[10] + matrix[14]
	};
}

VuoPoint2d VuoTransform_transform_VuoPoint2d(VuoTransform transform, VuoPoint2d point);
VuoPoint3d VuoTransform_transform_VuoPoint3d(VuoTransform transform, VuoPoint3d point);

/**
 * @ingroup VuoTransform
 * Transforms `vector` using `matrix` (a column-major matrix of 16 values), and returns the new vector.
 *
 * Ignores the matrix's translation.  Useful for adjusting normals.
 *
 * @see VuoTransform_getMatrix
 */
static inline VuoPoint3d VuoTransform_transformVector(const float *matrix, VuoPoint3d point) __attribute__((nonnull));
static inline VuoPoint3d VuoTransform_transformVector(const float *matrix, VuoPoint3d point)
{
	return (VuoPoint3d){
		point.x * matrix[0] + point.y * matrix[4] + point.z * matrix[ 8],
		point.x * matrix[1] + point.y * matrix[5] + point.z * matrix[ 9],
		point.x * matrix[2] + point.y * matrix[6] + point.z * matrix[10]
	};
}


VuoRectangle VuoTransform_transformRectangle(const float *matrix, VuoRectangle rectangle) __attribute__((nonnull));

VuoTransform VuoTransform_makeFromJson(struct json_object *js);
struct json_object * VuoTransform_getJson(const VuoTransform value);
char * VuoTransform_getSummary(const VuoTransform value);

/// @{
/**
 * Automatically generated function.
 */
VuoTransform VuoTransform_makeFromString(const char *str);
char * VuoTransform_getString(const VuoTransform value);
void VuoTransform_retain(VuoTransform value);
void VuoTransform_release(VuoTransform value);
/// @}

/**
 * @}
 */
