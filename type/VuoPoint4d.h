/**
 * @file
 * VuoPoint4d C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOPOINT4D_H
#define VUOPOINT4D_H

#include <math.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoPoint4d VuoPoint4d
 * A floating-point 4-dimensional point.
 *
 * @{
 */

/**
 * A floating-point 4-dimensional point.
 */
typedef struct
{
	float x,y,z,w;
} VuoPoint4d;

VuoPoint4d VuoPoint4d_makeFromJson(struct json_object * js);
struct json_object * VuoPoint4d_getJson(const VuoPoint4d value);
char * VuoPoint4d_getSummary(const VuoPoint4d value);

bool VuoPoint4d_areEqual(const VuoPoint4d value1, const VuoPoint4d value2);
VuoPoint4d VuoPoint4d_random(const VuoPoint4d minimum, const VuoPoint4d maximum);
VuoPoint4d VuoPoint4d_randomWithState(unsigned short state[3], const VuoPoint4d minimum, const VuoPoint4d maximum);

/// @{
/**
 * Automatically generated function.
 */
VuoPoint4d VuoPoint4d_makeFromString(const char *str);
char * VuoPoint4d_getString(const VuoPoint4d value);
void VuoPoint4d_retain(VuoPoint4d value);
void VuoPoint4d_release(VuoPoint4d value);
/// @}

/**
 * Returns a @c VuoPoint4d with the specified coordinates.
 */
static inline VuoPoint4d VuoPoint4d_make(float x, float y, float z, float w) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_make(float x, float y, float z, float w)
{
	VuoPoint4d p = {x,y,z,w};
	return p;
}

/**
 * Returns the 3D cross-product of @c u and @c v (ignoring the @c w coordinate).
 */
static inline VuoPoint4d VuoPoint4d_crossProduct(VuoPoint4d u, VuoPoint4d v) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_crossProduct(VuoPoint4d u, VuoPoint4d v)
{
	VuoPoint4d p =
	{
		u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x,
		1.
	};
	return p;
}

/**
 * Returns the dot product of @c u, @c v.
 */
static inline float VuoPoint4d_dotProduct(VuoPoint4d u, VuoPoint4d v) __attribute__((const));
static inline float VuoPoint4d_dotProduct(VuoPoint4d u, VuoPoint4d v)
{
	return u.x*v.x + u.y*v.y + u.z*v.z + u.w*v.w;
}


/**
 * Returns the magnitude of the vector.
 */
static inline float VuoPoint4d_magnitude(VuoPoint4d a) __attribute__((const));
static inline float VuoPoint4d_magnitude(VuoPoint4d a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
}

/**
 * Returns the 3D normalization of @c a (ignoring the @c w coordinate).
 */
static inline VuoPoint4d VuoPoint4d_normalize3d(VuoPoint4d a) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_normalize3d(VuoPoint4d a)
{
	float length = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
	VuoPoint4d p =
	{
		a.x/length,
		a.y/length,
		a.z/length,
		a.w
	};
	return p;
}

/**
 * Returns the 4D normalization of @c a.
 */
static inline VuoPoint4d VuoPoint4d_normalize(VuoPoint4d a) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_normalize(VuoPoint4d a)
{
	float length = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
	VuoPoint4d p =
	{
		a.x/length,
		a.y/length,
		a.z/length,
		a.w/length
	};
	return p;
}

/**
 * @c a + @c b.
 */
static inline VuoPoint4d VuoPoint4d_add(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_add(VuoPoint4d a, VuoPoint4d b)
{
	VuoPoint4d p =
	{
		a.x + b.x,
		a.y + b.y,
		a.z + b.z,
		a.w + b.w
	};
	return p;
}

/**
 * @c a - @c b.
 */
static inline VuoPoint4d VuoPoint4d_subtract(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_subtract(VuoPoint4d a, VuoPoint4d b)
{
	VuoPoint4d p =
	{
		a.x - b.x,
		a.y - b.y,
		a.z - b.z,
		a.w - b.w
	};
	return p;
}

/**
 * Returns the magnitude of the vector.
 */
static inline float VuoPoint4d_squaredMagnitude(VuoPoint4d a) __attribute__((const));
static inline float VuoPoint4d_squaredMagnitude(VuoPoint4d a)
{
	return (a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
}


/**
 * Component-wise division.
 */
static inline VuoPoint4d VuoPoint4d_divide(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_divide(VuoPoint4d a, VuoPoint4d b)
{
	VuoPoint4d p =
	{
		a.x / b.x,
		a.y / b.y,
		a.z / b.z,
		a.w / b.w
	};
	return p;
}

/**
 * @c a * @c b
 */
static inline VuoPoint4d VuoPoint4d_multiply(VuoPoint4d a, float b) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_multiply(VuoPoint4d a, float b)
{
	VuoPoint4d p =
	{
		a.x * b,
		a.y * b,
		a.z * b,
		a.w * b
	};
	return p;
}

/**
 * Returns component-wise multiplication of two VuoPoint4d vectors.
 */
static inline VuoPoint4d VuoPoint4d_scale(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_scale(VuoPoint4d a, VuoPoint4d b)
{
	return (VuoPoint4d){ a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w };
}

/**
 * If any component of the value is zero or very close to zero, moves it further from zero (either 0.000001 or -0.000001).
 */
static inline VuoPoint4d VuoPoint4d_makeNonzero(VuoPoint4d a) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_makeNonzero(VuoPoint4d a)
{
	if (fabs(a.x) < 0.000001)
		a.x = copysign(0.000001, a.x);
	if (fabs(a.y) < 0.000001)
		a.y = copysign(0.000001, a.y);
	if (fabs(a.z) < 0.000001)
		a.z = copysign(0.000001, a.z);
	if (fabs(a.w) < 0.000001)
		a.w = copysign(0.000001, a.w);
	return a;
}

/**
 *	Distance between @c a and @c b.
 */
static inline float VuoPoint4d_distance(VuoPoint4d a, VuoPoint4d b) __attribute__((const));
static inline float VuoPoint4d_distance(VuoPoint4d a, VuoPoint4d b)
{
	return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z) + (a.w-b.w)*(a.w-b.w));
}

/**
 *	Returns a component-wise snap value using a center point and snap value.
 */
static inline VuoPoint4d VuoPoint4d_snap(VuoPoint4d a, VuoPoint4d center, VuoPoint4d snap) __attribute__((const));
static inline VuoPoint4d VuoPoint4d_snap(VuoPoint4d a, VuoPoint4d center, VuoPoint4d snap)
{
	VuoPoint4d nonzeroSnap = VuoPoint4d_makeNonzero(snap);
	return (VuoPoint4d) {
			center.x + nonzeroSnap.x * (int)round((a.x-center.x) / nonzeroSnap.x),
			center.y + nonzeroSnap.y * (int)round((a.y-center.y) / nonzeroSnap.y),
			center.z + nonzeroSnap.z * (int)round((a.z-center.z) / nonzeroSnap.z),
			center.w + nonzeroSnap.w * (int)round((a.w-center.w) / nonzeroSnap.w)
		};
}


/**
 * @}
 */

#endif
