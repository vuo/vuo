/**
 * @file
 * VuoPoint2d C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoReal.h"
#include <math.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoPoint2d VuoPoint2d
 * A floating-point 2-dimensional Cartesian spatial location.
 *
 * @{
 */

/**
 * A floating-point 2-dimensional Cartesian spatial location.
 */
typedef float __attribute__((ext_vector_type(2))) VuoPoint2d;

/// @{ List type.
typedef const struct VuoList_VuoPoint2d_struct { void *l; } * VuoList_VuoPoint2d;
#define VuoList_VuoPoint2d_TYPE_DEFINED
/// @}

VuoPoint2d VuoPoint2d_makeFromJson(struct json_object * js);
struct json_object * VuoPoint2d_getJson(const VuoPoint2d value);
char * VuoPoint2d_getSummary(const VuoPoint2d value);

#define VuoPoint2d_SUPPORTS_COMPARISON
bool VuoPoint2d_areEqual(const VuoPoint2d value1, const VuoPoint2d value2);
bool VuoPoint2d_areEqualListWithinTolerance(VuoList_VuoPoint2d values, VuoPoint2d tolerance);
bool VuoPoint2d_isLessThan(const VuoPoint2d a, const VuoPoint2d b);
bool VuoPoint2d_isWithinRange(VuoPoint2d value, VuoPoint2d minimum, VuoPoint2d maximum);

VuoPoint2d VuoPoint2d_minList(VuoList_VuoPoint2d values, VuoInteger *outputPosition);
VuoPoint2d VuoPoint2d_maxList(VuoList_VuoPoint2d values, VuoInteger *outputPosition);
VuoPoint2d VuoPoint2d_average(VuoList_VuoPoint2d values);

VuoPoint2d VuoPoint2d_random(const VuoPoint2d minimum, const VuoPoint2d maximum);
VuoPoint2d VuoPoint2d_randomWithState(unsigned short state[3], const VuoPoint2d minimum, const VuoPoint2d maximum);

/// @{
/**
 * Automatically generated function.
 */
VuoPoint2d VuoPoint2d_makeFromString(const char *str);
char * VuoPoint2d_getString(const VuoPoint2d value);
void VuoPoint2d_retain(VuoPoint2d value);
void VuoPoint2d_release(VuoPoint2d value);
/// @}

/**
 * Returns a point with the specified coordinates.
 *
 * @version200New
 */
static inline VuoPoint2d VuoPoint2d_make(float x, float y) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_make(float x, float y)
{
	return (VuoPoint2d){x, y};
}

/**
 * Returns a point using the first 2 elements in the specified array.
 *
 * @version200New
 */
static inline VuoPoint2d VuoPoint2d_makeFromArray(float *f)
{
    return (VuoPoint2d){ f[0], f[1] };
}

/**
 * Sets the first 2 elements in the specified array to the specified point.
 */
static inline void VuoPoint2d_setArray(float *f, VuoPoint2d p)
{
	f[0] = p.x;
	f[1] = p.y;
}

/**
 * @c a + @c b.
 */
static inline VuoPoint2d VuoPoint2d_add(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_add(VuoPoint2d a, VuoPoint2d b)
{
	return a + b;
}

/**
 * @c a - @c b.
 */
static inline VuoPoint2d VuoPoint2d_subtract(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_subtract(VuoPoint2d a, VuoPoint2d b)
{
	return a - b;
}


/**
 * Returns the squared magnitude of the vector.
 */
static inline float VuoPoint2d_squaredMagnitude(VuoPoint2d a) __attribute__((const));
static inline float VuoPoint2d_squaredMagnitude(VuoPoint2d a)
{
	return (a.x*a.x + a.y*a.y);
}

/**
 * Component-wise division.
 */
static inline VuoPoint2d VuoPoint2d_divide(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_divide(VuoPoint2d a, VuoPoint2d b)
{
	return a / b;
}

/**
 * If any component of the value is zero or very close to zero, moves it further from zero (either 0.000001 or -0.000001).
 */
static inline VuoPoint2d VuoPoint2d_makeNonzero(VuoPoint2d a) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_makeNonzero(VuoPoint2d a)
{
	if (fabs(a.x) < 0.000001)
		a.x = copysign(0.000001, a.x);
	if (fabs(a.y) < 0.000001)
		a.y = copysign(0.000001, a.y);
	return a;
}

/**
 * Returns the magnitude of the vector.
 */
static inline float VuoPoint2d_magnitude(VuoPoint2d a) __attribute__((const));
static inline float VuoPoint2d_magnitude(VuoPoint2d a)
{
	return sqrtf(a.x*a.x + a.y*a.y);
}

/**
 * Returns the normalization of @c a.
 */
static inline VuoPoint2d VuoPoint2d_normalize(VuoPoint2d a) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_normalize(VuoPoint2d a)
{
	return a / sqrtf(a.x * a.x + a.y * a.y);
}

/**
 * @c a * @c b
 */
static inline VuoPoint2d VuoPoint2d_multiply(VuoPoint2d a, float b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_multiply(VuoPoint2d a, float b)
{
	return a * b;
}

/**
 * Returns the dot product of @c u, @c v.
 */
static inline float VuoPoint2d_dotProduct(VuoPoint2d u, VuoPoint2d v) __attribute__((const));
static inline float VuoPoint2d_dotProduct(VuoPoint2d u, VuoPoint2d v)
{
	return u.x*v.x+u.y*v.y;
}

/**
 *	Distance between @c a and @c b.
 */
static inline float VuoPoint2d_distance(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline float VuoPoint2d_distance(VuoPoint2d a, VuoPoint2d b)
{
	return sqrtf( (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) );
}

/**
 * Returns component-wise min.
 */
static inline VuoPoint2d VuoPoint2d_min(const VuoPoint2d l, const VuoPoint2d r) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_min(const VuoPoint2d l, const VuoPoint2d r)
{
	return (VuoPoint2d){
		fminf(l.x, r.x),
		fminf(l.y, r.y)};
}

/**
 * Returns component-wise max.
 */
static inline VuoPoint2d VuoPoint2d_max(const VuoPoint2d l, const VuoPoint2d r) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_max(const VuoPoint2d l, const VuoPoint2d r)
{
	return (VuoPoint2d){
		fmaxf(l.x, r.x),
		fmaxf(l.y, r.y)};
}

/**
 * Returns a linearly interpolated value between @c a and @c b using time @c t.  @c t is between 0 and 1.
 */
static inline VuoPoint2d VuoPoint2d_lerp(VuoPoint2d a, VuoPoint2d b, float t) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_lerp(VuoPoint2d a, VuoPoint2d b, float t)
{
	return a * (1 - t) + b * t;
}

/**
 *	Returns component-wise multiplication of two VuoPoint2d vectors.
 */
static inline VuoPoint2d VuoPoint2d_scale(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_scale(VuoPoint2d a, VuoPoint2d b)
{
	return a * b;
}

/**
 * Calculates a position along the path of an oscillating spring.
 */
static inline VuoPoint2d VuoPoint2d_spring(VuoReal timeSinceDrop, VuoPoint2d dropPosition, VuoPoint2d restingPosition, VuoReal period, VuoReal damping)
{
	return (VuoPoint2d){
		(float)VuoReal_spring(timeSinceDrop, dropPosition.x, restingPosition.x, period, damping),
		(float)VuoReal_spring(timeSinceDrop, dropPosition.y, restingPosition.y, period, damping)
	};
}

/**
 * Limits `point` to values between `limitA` and `limitB`, inclusive.
 */
static inline VuoPoint2d VuoPoint2d_clamp(VuoPoint2d point, VuoReal limitA, VuoReal limitB)
{
	return (VuoPoint2d){
		(float)VuoReal_clamp(point.x, limitA, limitB),
		(float)VuoReal_clamp(point.y, limitA, limitB)
	};
}

/**
 * Limits `point` to values between `limitA` and `limitB`, inclusive.
 */
static inline VuoPoint2d VuoPoint2d_clampn(VuoPoint2d point, VuoPoint2d limitA, VuoPoint2d limitB)
{
	return (VuoPoint2d){
		(float)VuoReal_clamp(point.x, limitA.x, limitB.x),
		(float)VuoReal_clamp(point.y, limitA.y, limitB.y)
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
static inline VuoPoint2d VuoPoint2d_bezier3(VuoPoint2d p0, VuoPoint2d p1, VuoPoint2d p2, VuoPoint2d p3, VuoReal time)
{
	return (VuoPoint2d) {
		(float)VuoReal_bezier3(p0.x,p1.x,p2.x,p3.x,time),
		(float)VuoReal_bezier3(p0.y,p1.y,p2.y,p3.y,time)
	};
}

/**
 * Snap value a to the nearest increment of value snap.
 */
static inline VuoPoint2d VuoPoint2d_snap(VuoPoint2d a, VuoPoint2d center, VuoPoint2d snap)
{
	VuoPoint2d nonzeroSnap = VuoPoint2d_makeNonzero(snap);
	return (VuoPoint2d) {
		center.x + nonzeroSnap.x * (int)round( (a.x-center.x) / nonzeroSnap.x ),
		center.y + nonzeroSnap.y * (int)round( (a.y-center.y) / nonzeroSnap.y )
	};
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
