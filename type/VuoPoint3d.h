/**
 * @file
 * VuoPoint3d C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoPoint3d_h
#define VuoPoint3d_h

#include "VuoReal.h"
#include "VuoBoolean.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

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
typedef float __attribute__((ext_vector_type(3))) VuoPoint3d;

#define VuoPoint3d_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoPoint3d.h"

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
bool VuoPoint3d_areEqualListWithinTolerance(VuoList_VuoPoint3d values, VuoPoint3d tolerance);
bool VuoPoint3d_isLessThan(const VuoPoint3d a, const VuoPoint3d b);
bool VuoPoint3d_isWithinRange(VuoPoint3d value, VuoPoint3d minimum, VuoPoint3d maximum);

VuoPoint3d VuoPoint3d_minList(VuoList_VuoPoint3d values, VuoInteger *outputPosition);
VuoPoint3d VuoPoint3d_maxList(VuoList_VuoPoint3d values, VuoInteger *outputPosition);
VuoPoint3d VuoPoint3d_average(VuoList_VuoPoint3d values);

VuoPoint3d VuoPoint3d_random(const VuoPoint3d minimum, const VuoPoint3d maximum);
VuoPoint3d VuoPoint3d_randomWithState(unsigned short state[3], const VuoPoint3d minimum, const VuoPoint3d maximum);

/// @{
/**
 * Automatically generated function.
 */
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
	return (VuoPoint3d){x, y, z};
}

/**
 * Returns a point using the first 3 elements in the specified array.
 *
 * @version200New
 */
static inline VuoPoint3d VuoPoint3d_makeFromArray(float *f)
{
    return (VuoPoint3d){ f[0], f[1], f[2] };
}

/**
 * Sets the first 3 elements in the specified array to the specified point.
 *
 * @version200New
 */
static inline void VuoPoint3d_setArray(float *f, VuoPoint3d p)
{
	f[0] = p.x;
	f[1] = p.y;
	f[2] = p.z;
}

/**
 * Returns a box with the specified center and size.
 */
static inline VuoBox VuoBox_make(VuoPoint3d center, VuoPoint3d size) __attribute__((const));
static inline VuoBox VuoBox_make(VuoPoint3d center, VuoPoint3d size)
{
	return (VuoBox){ center, size };
}

/**
 * Returns an axis aligned bounding box with the specified min and max coordinates.
 */
static inline VuoBox VuoBox_makeWithPoints(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax) __attribute__((const));
static inline VuoBox VuoBox_makeWithPoints(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
	return (VuoBox){
		(VuoPoint3d){ (xmin+xmax)/2.f, (ymin+ymax)/2.f, (zmin+zmax)/2.f },
		(VuoPoint3d){ xmax-xmin, ymax-ymin, zmax-zmin }
	};
}

/**
 * Returns the cross-product of @c u and @c v.
 */
static inline VuoPoint3d VuoPoint3d_crossProduct(VuoPoint3d u, VuoPoint3d v) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_crossProduct(VuoPoint3d u, VuoPoint3d v)
{
	return (VuoPoint3d){
		u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x
	};
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
	return a / sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

/**
 * @c a + @c b.
 */
static inline VuoPoint3d VuoPoint3d_add(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_add(VuoPoint3d a, VuoPoint3d b)
{
	return a + b;
}

/**
 * @c a - @c b.
 */
static inline VuoPoint3d VuoPoint3d_subtract(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_subtract(VuoPoint3d a, VuoPoint3d b)
{
	return a - b;
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
	return a / b;
}

/**
 * @c a * @c b
 */
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, float b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, float b)
{
	return a * b;
}

/**
 * Returns component-wise min.
 */
static inline VuoPoint3d VuoPoint3d_min(const VuoPoint3d l, const VuoPoint3d r) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_min(const VuoPoint3d l, const VuoPoint3d r)
{
	return (VuoPoint3d){
		fminf(l.x, r.x),
		fminf(l.y, r.y),
		fminf(l.z, r.z)};
}

/**
 * Returns component-wise max.
 */
static inline VuoPoint3d VuoPoint3d_max(const VuoPoint3d l, const VuoPoint3d r) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_max(const VuoPoint3d l, const VuoPoint3d r)
{
	return (VuoPoint3d){
		fmaxf(l.x, r.x),
		fmaxf(l.y, r.y),
		fmaxf(l.z, r.z)};
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
	return a * (1 - t) + b * t;
}

/**
 *	Returns component-wise multiplication of two VuoPoint3d vectors.
 */
static inline VuoPoint3d VuoPoint3d_scale(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_scale(VuoPoint3d a, VuoPoint3d b)
{
	return a * b;
}

/**
 * Returns the component-wise modulus of two VuoPoint3d vectors.
 *
 * Behavior for negative values matches
 * [GLSL's `mod` function](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/mod.xhtml).
 *
 * @version200New
 */
static inline VuoPoint3d VuoPoint3d_mod(VuoPoint3d a, VuoPoint3d b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_mod(VuoPoint3d a, VuoPoint3d b)
{
	return (VuoPoint3d){
		a.x - b.x * floorf(a.x / b.x),
		a.y - b.y * floorf(a.y / b.y),
		a.z - b.z * floorf(a.z / b.z)
	};
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
 * Limits `point` to values between `limitA` and `limitB`, inclusive.
 */
static inline VuoPoint3d VuoPoint3d_clamp(VuoPoint3d point, VuoReal limitA, VuoReal limitB)
{
	return (VuoPoint3d){
		(float)VuoReal_clamp(point.x, limitA, limitB),
		(float)VuoReal_clamp(point.y, limitA, limitB),
		(float)VuoReal_clamp(point.z, limitA, limitB)
	};
}

/**
 * Limits `point` to values between `limitA` and `limitB`, inclusive.
 */
static inline VuoPoint3d VuoPoint3d_clampn(VuoPoint3d point, VuoPoint3d limitA, VuoPoint3d limitB)
{
	return (VuoPoint3d){
		(float)VuoReal_clamp(point.x, limitA.x, limitB.x),
		(float)VuoReal_clamp(point.y, limitA.y, limitB.y),
		(float)VuoReal_clamp(point.z, limitA.z, limitB.z)
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
		(float)VuoReal_bezier3(p0.x,p1.x,p2.x,p3.x,time),
		(float)VuoReal_bezier3(p0.y,p1.y,p2.y,p3.y,time),
		(float)VuoReal_bezier3(p0.z,p1.z,p2.z,p3.z,time)
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
	VuoPoint3d exents = (VuoPoint3d){aabb.size.x / 2.f, aabb.size.y / 2.f};
	VuoPoint3d min = aabb.center - exents;
	VuoPoint3d max = aabb.center + exents;

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

#ifdef __cplusplus
}
#endif

#endif
