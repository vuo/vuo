/**
 * @file
 * VuoPoint3d C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT3D_H
#define VUOPOINT3D_H

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

/// @todo add divide / multiply etc
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
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, double b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_divide(VuoPoint3d a, double b)
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
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, double b) __attribute__((const));
static inline VuoPoint3d VuoPoint3d_multiply(VuoPoint3d a, double b)
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
 * @}
 */

#endif
