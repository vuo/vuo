/**
 * @file
 * VuoTransform C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTRANSFORM_H
#define VUOTRANSFORM_H

#include "VuoPoint3d.h"
#include "VuoPoint4d.h"
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
	VuoTransformTypeQuaternion
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
	union
	{
		VuoPoint3d euler; ///< Radians
		VuoPoint4d quaternion;
	} rotationSource;
} VuoTransform;

void VuoTransform_getMatrix(const VuoTransform value, float *matrix);
VuoTransform VuoTransform_makeIdentity(void);
VuoTransform VuoTransform_makeEuler(VuoPoint3d translation, VuoPoint3d rotation, VuoPoint3d scale);
VuoTransform VuoTransform_makeQuaternion(VuoPoint3d translation, VuoPoint4d rotation, VuoPoint3d scale);
VuoTransform VuoTransform_makeFrom2d(VuoTransform2d transform2d);

/**
 * Returns the composite of quaternion @c a with quaternion @c b (i.e., the rotation described by @c a followed by the rotation described by @c b).
 */
static inline VuoPoint4d VuoTransform_quaternionComposite(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionComposite(VuoPoint4d a, VuoPoint4d b)
{
	VuoPoint4d q;
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

/**
 * Returns the quaternion describing the rotation from direction a to b.
 */
static inline VuoPoint4d VuoTransform_quaternionFromVectors(VuoPoint3d from, VuoPoint3d to) __attribute__((const));
static inline VuoPoint4d VuoTransform_quaternionFromVectors(VuoPoint3d from, VuoPoint3d to)
{
	// http://books.google.com/books?id=hiBFUv_FT0wC&pg=PA214&lpg=PA214&dq=Stan+Melax's+article+in+Game+Programming+Gems&source=bl&ots=OCjDPwza1h&sig=6_bDSzTrnI3qCEG9vtVV_mDBgg8&hl=en&sa=X&ei=ffReUsSaBIrMqAGg6YD4Aw&ved=0CCsQ6AEwAA#v=onepage&q=Stan%20Melax's%20article%20in%20Game%20Programming%20Gems&f=false
	VuoPoint4d q;

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
				);
}

/**
 * Multiplies the specified matrices.
 */
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
static inline void VuoTransform_copyMatrix4x4(const float *sourceMatrix, float *destMatrix)
{
	for (int i=0; i<16; ++i)
		destMatrix[i] = sourceMatrix[i];
}

/**
 * Prints the specified column-major matrix.
 */
static inline void VuoTransform_printMatrix4x4(const float *matrix)
{
	for (int i=0; i<4; ++i)
		fprintf(stderr, "[\t%.4g\t%.4g\t%.4g\t%.4g\t]\n",matrix[i+0*4],matrix[i+1*4],matrix[i+2*4],matrix[i+3*4]);
}

VuoTransform VuoTransform_valueFromJson(struct json_object *js);
struct json_object * VuoTransform_jsonFromValue(const VuoTransform value);
char * VuoTransform_summaryFromValue(const VuoTransform value);

/// @{
/**
 * Automatically generated function.
 */
VuoTransform VuoTransform_valueFromString(const char *str);
char * VuoTransform_stringFromValue(const VuoTransform value);
/// @}

/**
 * @}
 */

#endif
