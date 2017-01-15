/**
 * @file
 * VuoPoint2d C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT2D_H
#define VUOPOINT2D_H

#include "VuoReal.h"
#include <math.h>
#include <stdbool.h>

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
typedef struct
{
	float x,y;
} VuoPoint2d;

VuoPoint2d VuoPoint2d_makeFromJson(struct json_object * js);
struct json_object * VuoPoint2d_getJson(const VuoPoint2d value);
char * VuoPoint2d_getSummary(const VuoPoint2d value);

bool VuoPoint2d_areEqual(const VuoPoint2d value1, const VuoPoint2d value2);
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
 * A rectangular area.
 */
typedef struct
{
	VuoPoint2d center;
	VuoPoint2d size;
} VuoRectangle;

/**
 * Returns a point with the specified coordinates.
 */
static inline VuoPoint2d VuoPoint2d_make(float x, float y) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_make(float x, float y)
{
	VuoPoint2d p = {x,y};
	return p;
}

/**
 * Returns a rectangle with the specified coordinates.
 */
static inline VuoRectangle VuoRectangle_make(float centerX, float centerY, float width, float height) __attribute__((const));
static inline VuoRectangle VuoRectangle_make(float centerX, float centerY, float width, float height)
{
	VuoRectangle r = {{centerX,centerY},{width,height}};
	return r;
}

/**
 * Returns true if the rectangles have the same position and size.
 */
static inline bool VuoRectangle_areEqual(const VuoRectangle a, const VuoRectangle b)
{
	return VuoPoint2d_areEqual(a.center, b.center) && VuoPoint2d_areEqual(a.size, b.size);
}

/**
 * Returns a rectangle with the specified coordinates.
 */
static inline VuoRectangle VuoRectangle_makeTopLeft(float leftX, float topY, float width, float height) __attribute__((const));
static inline VuoRectangle VuoRectangle_makeTopLeft(float leftX, float topY, float width, float height)
{
	VuoRectangle r = {{leftX+width/2.,topY+height/2.},{width,height}};
	return r;
}

/**
 * @c a + @c b.
 */
static inline VuoPoint2d VuoPoint2d_add(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_add(VuoPoint2d a, VuoPoint2d b)
{
	VuoPoint2d p =
	{
		a.x + b.x,
		a.y + b.y
	};
	return p;
}

/**
 * @c a - @c b.
 */
static inline VuoPoint2d VuoPoint2d_subtract(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_subtract(VuoPoint2d a, VuoPoint2d b)
{
	VuoPoint2d p =
	{
		a.x - b.x,
		a.y - b.y
	};
	return p;
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
	VuoPoint2d p =
	{
		a.x / b.x,
		a.y / b.y
	};
	return p;
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
	float length = VuoPoint2d_magnitude(a);
	VuoPoint2d p =
	{
		a.x/length,
		a.y/length
	};
	return p;
}

/**
 * @c a * @c b
 */
static inline VuoPoint2d VuoPoint2d_multiply(VuoPoint2d a, float b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_multiply(VuoPoint2d a, float b)
{
	VuoPoint2d p =
	{
		a.x * b,
		a.y * b
	};
	return p;
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
 * Returns a linearly interpolated value between @c a and @c b using time @c t.  @c t is between 0 and 1.
 */
static inline VuoPoint2d VuoPoint2d_lerp(VuoPoint2d a, VuoPoint2d b, float t) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_lerp(VuoPoint2d a, VuoPoint2d b, float t)
{
	return VuoPoint2d_add( VuoPoint2d_multiply(a, (1-t)), VuoPoint2d_multiply(b, t) );
}

/**
 *	Returns component-wise multiplication of two VuoPoint2d vectors.
 */
static inline VuoPoint2d VuoPoint2d_scale(VuoPoint2d a, VuoPoint2d b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_scale(VuoPoint2d a, VuoPoint2d b)
{
	return (VuoPoint2d) { a.x*b.x, a.y*b.y };
}

VuoRectangle VuoPoint2d_rectangleIntersection(VuoRectangle rectangleA, VuoRectangle rectangleB);
VuoRectangle VuoPoint2d_rectangleUnion(VuoRectangle rectangleA, VuoRectangle rectangleB);

/**
 * Calculates a position along the path of an oscillating spring.
 */
static inline VuoPoint2d VuoPoint2d_spring(VuoReal timeSinceDrop, VuoPoint2d dropPosition, VuoPoint2d restingPosition, VuoReal period, VuoReal damping)
{
	VuoPoint2d p;
	p.x = VuoReal_spring(timeSinceDrop, dropPosition.x, restingPosition.x, period, damping);
	p.y = VuoReal_spring(timeSinceDrop, dropPosition.y, restingPosition.y, period, damping);
	return p;
}

/**
 * Limits @c point to values between @c min and @c max, inclusive.
 */
static inline VuoPoint2d VuoPoint2d_clamp(VuoPoint2d point, VuoReal min, VuoReal max)
{
	return (VuoPoint2d) {
		fmin(fmax(point.x,min),max),
		fmin(fmax(point.y,min),max)
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
		VuoReal_bezier3(p0.x,p1.x,p2.x,p3.x,time),
		VuoReal_bezier3(p0.y,p1.y,p2.y,p3.y,time)
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

#endif
