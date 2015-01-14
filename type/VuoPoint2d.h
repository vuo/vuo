/**
 * @file
 * VuoPoint2d C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT2D_H
#define VUOPOINT2D_H

#include <math.h>

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

VuoPoint2d VuoPoint2d_valueFromJson(struct json_object * js);
struct json_object * VuoPoint2d_jsonFromValue(const VuoPoint2d value);
char * VuoPoint2d_summaryFromValue(const VuoPoint2d value);

/// @{
/**
 * Automatically generated function.
 */
VuoPoint2d VuoPoint2d_valueFromString(const char *str);
char * VuoPoint2d_stringFromValue(const VuoPoint2d value);
/// @}

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
 * @c a / @c b
 */
static inline VuoPoint2d VuoPoint2d_divide(VuoPoint2d a, float b) __attribute__((const));
static inline VuoPoint2d VuoPoint2d_divide(VuoPoint2d a, float b)
{
	VuoPoint2d p =
	{
		a.x / b,
		a.y / b
	};
	return p;
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
 * @}
 */

#endif
