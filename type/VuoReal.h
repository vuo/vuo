/**
 * @file
 * VuoReal C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoReal_h
#define VuoReal_h

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include <math.h>

/**
 * @ingroup VuoTypes
 * @defgroup VuoReal VuoReal
 * A floating-point number.
 *
 * @{
 */

/**
 * A floating-point number.
 */
typedef double VuoReal;

#define VuoReal_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoReal.h"

VuoReal VuoReal_makeFromJson(struct json_object *js);
struct json_object * VuoReal_getJson(const VuoReal value);
char * VuoReal_getSummary(const VuoReal value);

VuoReal VuoReal_makeFromLocalizedText(const char *text);

VuoReal VuoReal_minList(VuoList_VuoReal values, VuoInteger *outputPosition);
VuoReal VuoReal_maxList(VuoList_VuoReal values, VuoInteger *outputPosition);
VuoReal VuoReal_average(VuoList_VuoReal values);

VuoReal VuoReal_wrap(VuoReal value, VuoReal minimum, VuoReal maximum);

VuoReal VuoReal_random(const VuoReal minimum, const VuoReal maximum);
VuoReal VuoReal_randomWithState(unsigned short state[3], const VuoReal minimum, const VuoReal maximum);

/// @{
/**
 * Automatically generated function.
 */
char * VuoReal_getString(const VuoReal value);
void VuoReal_retain(VuoReal value);
void VuoReal_release(VuoReal value);
/// @}

/**
 * @c a+b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoReal VuoReal_add(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_add(VuoReal a, VuoReal b)
{
	return a+b;
}

/**
 * @c a-b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoReal VuoReal_subtract(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_subtract(VuoReal a, VuoReal b)
{
	return a-b;
}

/**
 * @c a*b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoReal VuoReal_multiply(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_multiply(VuoReal a, VuoReal b)
{
	return a*b;
}

/**
 * Same as @ref VuoReal_multiply.
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoReal VuoReal_scale(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_scale(VuoReal a, VuoReal b)
{
	return a*b;
}

/**
 * @c a/b
 *
 * Provided for generic type equivalence with VuoPoints.
 */
static inline VuoReal VuoReal_divide(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_divide(VuoReal a, VuoReal b)
{
	return a/b;
}

/**
 * If the value is zero or very close to zero, moves the value further from zero (either 0.000001 or -0.000001).
 */
static inline VuoReal VuoReal_makeNonzero(VuoReal a) __attribute__((const));
static inline VuoReal VuoReal_makeNonzero(VuoReal a)
{
	if (fabs(a) < 0.000001)
		return copysign(0.000001, a);
	return a;
}

/**
 * Distance between @c a and @c b.
 */
static inline VuoReal VuoReal_distance(VuoReal a, VuoReal b) __attribute__((const));
static inline VuoReal VuoReal_distance(VuoReal a, VuoReal b)
{
	return sqrtf((a-b)*(a-b));
}

/**
 * Returns a linearly interpolated value between @c a and @c b using time @c t. @c t is between 0 and 1.
 */
static inline VuoReal VuoReal_lerp(VuoReal a, VuoReal b, float t) __attribute__((const));
static inline VuoReal VuoReal_lerp(VuoReal a, VuoReal b, float t)
{
	return a*(1-t) + b*t;
}

/**
 * Calculates a position along the path of an oscillating spring.
 */
static inline VuoReal VuoReal_spring(VuoReal timeSinceDrop, VuoReal dropPosition, VuoReal restingPosition, VuoReal period, VuoReal damping)
{
	// Based on https://en.wikibooks.org/wiki/Control_Systems/Examples/Second_Order_Systems#Task_2
	double t = 2*M_PI*timeSinceDrop/period;
	double d = sqrt(1-pow(damping,2));
	double p = exp(-damping * t) * sin(d * t + asin(d)) / d;
	return VuoReal_lerp(restingPosition, dropPosition, p);
}

#ifndef MIN
/**
 * Returns the smaller of @c a and @c b.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
/**
 * Returns the larger of @c a and @c b.
 */
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/**
 * Limits `value` to values between `limitA` and `limitB`, inclusive.
 * The limits can be in either order: (min,max) or (max,min).
 */
static inline VuoReal VuoReal_clamp(VuoReal value, VuoReal limitA, VuoReal limitB)
{
	return MIN(MAX(value, MIN(limitA, limitB)), MAX(limitA,limitB));
}

/**
 * Same as @ref VuoReal_clamp.
 * Provided for generic type compatibility with other `_clampn` functions.
 */
static inline VuoReal VuoReal_clampn(VuoReal value, VuoReal limitA, VuoReal limitB)
{
	return VuoReal_clamp(value, limitA, limitB);
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
static inline VuoReal VuoReal_bezier3(VuoReal p0, VuoReal p1, VuoReal p2, VuoReal p3, VuoReal time)
{
	return p0*pow(1-time,3) + 3.*p1*pow(1-time,2)*time + 3.*p2*(1-time)*pow(time,2) + p3*pow(time,3);
}

/**
 * Snap value a to the nearest increment of value snap.
 */
static inline VuoReal VuoReal_snap(VuoReal a, VuoReal center, VuoReal snap)
{
	VuoReal nonzeroSnap = VuoReal_makeNonzero(snap);
	return center + nonzeroSnap * (int)round( (a-center) / nonzeroSnap );
}

bool VuoReal_areEqual(const VuoReal value1, const VuoReal value2);
bool VuoReal_areEqualListWithinTolerance(VuoList_VuoReal values, VuoReal tolerance);
bool VuoReal_isLessThan(const VuoReal a, const VuoReal b);
bool VuoReal_isWithinRange(VuoReal value, VuoReal minimum, VuoReal maximum);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
