/**
 * @file
 * VuoPoint3d C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT3D_H
#define VUOPOINT3D_H

#include "VuoReal.h"
#include "VuoBoolean.h"
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

/**
 *	Defines a bounding box.
 */
typedef struct
{
	VuoPoint3d center;
	VuoPoint3d size;
} VuoBox;

VuoPoint3d VuoPoint3d_makeFromJson(struct json_object * js);
struct json_object * VuoPoint3d_getJson(const VuoPoint3d value);
char * VuoPoint3d_getSummary(const VuoPoint3d value);

bool VuoPoint3d_areEqual(const VuoPoint3d value1, const VuoPoint3d value2);
VuoPoint3d VuoPoint3d_random(const VuoPoint3d minimum, const VuoPoint3d maximum);
VuoPoint3d VuoPoint3d_randomWithState(unsigned short state[3], const VuoPoint3d minimum, const VuoPoint3d maximum);

/// @{
/**
 * Automatically generated function.
 */
VuoPoint3d VuoPoint3d_makeFromString(const char *str);
char * VuoPoint3d_getString(const VuoPoint3d value);
void VuoPoint3d_retain(VuoPoint3d value);
void VuoPoint3d_release(VuoPoint3d value);
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
 * Returns a box with the specified center and size.
 */
static inline VuoBox VuoBox_make(VuoPoint3d center, VuoPoint3d size) __attribute__((const));
static inline VuoBox VuoBox_make(VuoPoint3d center, VuoPoint3d size)
{
	VuoBox box = { center, size };
	return box;
}

/**
 * Returns an axis aligned bounding box with the specified min and max coordinates.
 */
static inline VuoBox VuoBox_makeWithPoints(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax) __attribute__((const));
static inline VuoBox VuoBox_makeWithPoints(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
	VuoBox box =
	{
		(VuoPoint3d){ (xmin+xmax)/2., (ymin+ymax)/2., (zmin+zmax)/2. },
		(VuoPoint3d){ xmax-xmin, ymax-ymin, zmax-zmin }
	};
	return box;
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
 * Component-wise division.
 */
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, VuoPoint3d b)
{
	VuoPoint3d p =
	{
		a.x / b.x,
		a.y / b.y,
		a.z / b.z
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
 * If any component of the value is zero or very close to zero, moves it further from zero (either 0.000001 or -0.000001).
 */
static inline VuoPoint3d VuoPoint3d_makeNonzero(VuoPoint3d a) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_makeNonzero(VuoPoint3d a)
{
	if (fabs(a.x) < 0.000001)
		a.x = copysign(0.000001, a.x);
	if (fabs(a.y) < 0.000001)
		a.y = copysign(0.000001, a.y);
	if (fabs(a.z) < 0.000001)
		a.z = copysign(0.000001, a.z);
	return a;
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
	VuoPoint3d nonzeroSnap = VuoPoint3d_makeNonzero(snap);
	return (VuoPoint3d) {
		center.x + nonzeroSnap.x * (int)round( (a.x-center.x) / nonzeroSnap.x ),
		center.y + nonzeroSnap.y * (int)round( (a.y-center.y) / nonzeroSnap.y ),
		center.z + nonzeroSnap.z * (int)round( (a.z-center.z) / nonzeroSnap.z )
	};
}

/**
 *	Grow the VuoBox to encapsulate @c b
 */
static inline VuoBox VuoBox_encapsulate(VuoBox a, VuoBox b)
{
	float xmin, ymin, zmin, xmax, ymax, zmax;

	VuoPoint3d extents_a = VuoPoint3d_multiply(a.size, .5);
	VuoPoint3d extents_b = VuoPoint3d_multiply(b.size, .5);

	xmin = fmin(a.center.x - extents_a.x, b.center.x - extents_b.x);
	xmax = fmax(a.center.x + extents_a.x, b.center.x + extents_b.x);

	ymin = fmin(a.center.y - extents_a.y, b.center.y - extents_b.y);
	ymax = fmax(a.center.y + extents_a.y, b.center.y + extents_b.y);

	zmin = fmin(a.center.z - extents_a.z, b.center.z - extents_b.z);
	zmax = fmax(a.center.z + extents_a.z, b.center.z + extents_b.z);

	return VuoBox_makeWithPoints( xmin, xmax, ymin, ymax, zmin, zmax );
}

/**
 *	Check if a point is contained within a bounding box.
 */
static inline VuoBoolean VuoBox_contains(VuoBox aabb, VuoPoint3d point)
{
	VuoPoint3d exents = VuoPoint3d_multiply(aabb.size, .5);
	VuoPoint3d min = VuoPoint3d_subtract(aabb.center, exents);
	VuoPoint3d max = VuoPoint3d_add(aabb.center, exents);

	return 	point.x > min.x && point.x < max.x &&
			point.y > min.y && point.y < max.y &&
			point.z > min.z && point.z < max.z;
}

/**
 *	Check if two boxes interesect with one another.
 */
static inline VuoBoolean VuoBox_intersects(VuoBox a, VuoBox b)
{
	VuoPoint3d dist = VuoPoint3d_subtract(a.center, b.center);
	VuoPoint3d size = VuoPoint3d_add(a.size, b.size);

	return 	fabs(dist.x) * 2 < size.x &&
			fabs(dist.y) * 2 < size.y &&
			fabs(dist.z) * 2 < size.z;
}

/**
 * @}
 */

#endif
