/**
 * @file
 * VuoPoint3d C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT3D_H
#define VUOPOINT3D_H

#include "VuoReal.h"
#include <math.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoPoint3d VuoPoint3d
 * A floating-point 3-dimensional Cartesian spatial location.
 *
 * @{
 */

/**
 * A floating-point 3-dimensional Cartesian spatial location.
 */
typedef struct
{
	float x,y,z;
} VuoPoint3d;

VuoPoint3d VuoPoint3d_valueFromJson(struct json_object * js);
struct json_object * VuoPoint3d_jsonFromValue(const VuoPoint3d value);
char * VuoPoint3d_summaryFromValue(const VuoPoint3d value);

/// @{
/**
 * Automatically generated function.
 */
VuoPoint3d VuoPoint3d_valueFromString(const char *str);
char * VuoPoint3d_stringFromValue(const VuoPoint3d value);
/// @}

/**
 * Returns a point with the specified coordinates.
 */
static inline VuoPoint3d VuoPoint3d_make(float x, float y, float z) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_make(float x, float y, float z)
{
	VuoPoint3d p = {x,y,z};
	return p;
}

/**
 * Returns the cross-product of @c u and @c v.
 */
static inline VuoPoint3d VuoPoint3d_crossProduct(VuoPoint3d u, VuoPoint3d v) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_crossProduct(VuoPoint3d u, VuoPoint3d v)
{
	VuoPoint3d p =
	{
		u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x
	};
	return p;
}

/**
 * Returns the dot product of @c u, @c v.
 */
static inline float VuoPoint3d_dotProduct(VuoPoint3d u, VuoPoint3d v) __attribute__((const));
static inline float VuoPoint3d_dotProduct(VuoPoint3d u, VuoPoint3d v)
{
	return u.x*v.x+u.y*v.y+u.z*v.z;
}

/**
 * Returns the magnitude of the vector.
 */
static inline float VuoPoint3d_magnitude(VuoPoint3d a) __attribute__((const));
static inline float VuoPoint3d_magnitude(VuoPoint3d a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

/**
 * Returns the normalization of @c a.
 */
static inline VuoPoint3d VuoPoint3d_normalize(VuoPoint3d a) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_normalize(VuoPoint3d a)
{
	float length = VuoPoint3d_magnitude(a);
	VuoPoint3d p =
	{
		a.x/length,
		a.y/length,
		a.z/length
	};
	return p;
}

/**
 * @c a + @c b.
 */
static inline VuoPoint3d VuoPoint3d_add(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_add(VuoPoint3d a, VuoPoint3d b)
{
	VuoPoint3d p =
	{
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
	return p;
}

/**
 * @c a - @c b.
 */
static inline VuoPoint3d VuoPoint3d_subtract(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_subtract(VuoPoint3d a, VuoPoint3d b)
{
	VuoPoint3d p =
	{
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	};
	return p;
}


/**
 * Returns the magnitude of the vector.
 */
static inline float VuoPoint3d_squaredMagnitude(VuoPoint3d a) __attribute__((const));
static inline float VuoPoint3d_squaredMagnitude(VuoPoint3d a)
{
	return (a.x*a.x + a.y*a.y + a.z*a.z);
}

/**
 * @c a / @c b
 */
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, float b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, float b)
{
	VuoPoint3d p =
	{
		a.x / b,
		a.y / b,
		a.z / b
	};
	return p;
}

/**
 * @c a * @c b
 */
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, float b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, float b)
{
	VuoPoint3d p =
	{
		a.x * b,
		a.y * b,
		a.z * b
	};
	return p;
}

/**
 *	Distance between @c a and @c b.
 */
static inline float VuoPoint3d_distance(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline float VuoPoint3d_distance(VuoPoint3d a, VuoPoint3d b)
{
	return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z));
}

/**
 * Returns a linearly interpolated value between @c a and @c b using time @c t.  @c t is between 0 and 1.
 */
static inline VuoPoint3d VuoPoint3d_lerp(VuoPoint3d a, VuoPoint3d b, float t) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_lerp(VuoPoint3d a, VuoPoint3d b, float t)
{
	return VuoPoint3d_add( VuoPoint3d_multiply(a, (1-t)), VuoPoint3d_multiply(b, t) );
}

/**
 *	Returns component-wise multiplication of two VuoPoint3d vectors.
 */
static inline VuoPoint3d VuoPoint3d_scale(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_scale(VuoPoint3d a, VuoPoint3d b)
{
	return (VuoPoint3d) { a.x*b.x, a.y*b.y, a.z*b.z };
}

/**
 * Calculates a position along the path of an oscillating spring.
 */
static inline VuoPoint3d VuoPoint3d_spring(VuoReal timeSinceDrop, VuoPoint3d dropPosition, VuoPoint3d restingPosition, VuoReal period, VuoReal damping)
{
	VuoPoint3d p;
	p.x = VuoReal_spring(timeSinceDrop, dropPosition.x, restingPosition.x, period, damping);
	p.y = VuoReal_spring(timeSinceDrop, dropPosition.y, restingPosition.y, period, damping);
	p.z = VuoReal_spring(timeSinceDrop, dropPosition.z, restingPosition.z, period, damping);
	return p;
}

/**
 * Limits @c point to values between @c min and @c max, inclusive.
 */
static inline VuoPoint3d VuoPoint3d_clamp(VuoPoint3d point, VuoReal min, VuoReal max)
{
	return (VuoPoint3d) {
		fmin(fmax(point.x,min),max),
		fmin(fmax(point.y,min),max),
		fmin(fmax(point.z,min),max)
	};
}

/**
 * Calculates a position along a cubic bezier curve.
 *
 * @param p0	The curve's starting position.
 * @param p1	The control point for the curve's starting position.
 * @param p2	The control point for the curve's ending position.
 * @param p3	The curve's ending position.
 * @param time	Which value along the curve should be returned. 0 = starting position, 1 = ending position.
 */
static inline VuoPoint3d VuoPoint3d_bezier3(VuoPoint3d p0, VuoPoint3d p1, VuoPoint3d p2, VuoPoint3d p3, VuoReal time)
{
	return (VuoPoint3d) {
		VuoReal_bezier3(p0.x,p1.x,p2.x,p3.x,time),
		VuoReal_bezier3(p0.y,p1.y,p2.y,p3.y,time),
		VuoReal_bezier3(p0.z,p1.z,p2.z,p3.z,time)
	};
}

/**
 * Snap value a to the nearest increment of value snap.
 */
static inline VuoPoint3d VuoPoint3d_snap(VuoPoint3d a, VuoPoint3d center, VuoPoint3d snap)
{
	return (VuoPoint3d) {
		center.x + snap.x * (int)round( (a.x-center.x) / snap.x ),
		center.y + snap.y * (int)round( (a.y-center.y) / snap.y ),
		center.z + snap.z * (int)round( (a.z-center.z) / snap.z )
	};
}


/**
 * @}
 */

#endif
