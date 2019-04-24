/**
 * @file
 * VuoColor C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoBoolean.h"
#include "VuoReal.h"
#include "VuoText.h"

/// @{
typedef const struct VuoList_VuoColor_struct { void *l; } * VuoList_VuoColor;
#define VuoList_VuoColor_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoColor VuoColor
 * A color, with component values specifying red, green, blue, and alpha (opacity).
 *
 * @{
 */

/**
 * A color, with component values specifying red, green, blue, and alpha (opacity).
 *
 * Each value is in the range [0,1], and specifies a color in the sRGB color space.
 *
 * RGB are not premultiplied by A.
 */
typedef struct
{
	float r,g,b,a;
} VuoColor;

VuoColor VuoColor_makeFromJson(struct json_object * js);
struct json_object * VuoColor_getJson(const VuoColor value);
char *VuoColor_getShortSummary(const VuoColor value);
char *VuoColor_getSummary(const VuoColor value);

#define VuoColor_SUPPORTS_COMPARISON
bool VuoColor_areEqual(const VuoColor a, const VuoColor b);
bool VuoColor_isLessThan(const VuoColor a, const VuoColor b);

bool VuoColor_areEqualWithinTolerance(const VuoColor a, const VuoColor b, const float tolerance);

/**
 * Returns a @c VuoColor with the given red, green, blue, alpha.
 *
 * Assumes each value is in the range [0,1], and specifies a color in the sRGB color space.
 *
 * You should not premultiply RGB by A.
 */
static inline VuoColor VuoColor_makeWithRGBA(VuoReal r, VuoReal g, VuoReal b, VuoReal a) __attribute__((const));
static inline VuoColor VuoColor_makeWithRGBA(VuoReal r, VuoReal g, VuoReal b, VuoReal a)
{
	VuoColor c = {(float)r,(float)g,(float)b,(float)a};
	return c;
}

/**
 * Gets the red, green, blue, alpha of a color.
 *
 * Each value is in the range [0,1], and specifies a color in the sRGB color space.
 *
 * RGB are not premultiplied by A.
 */
static inline void VuoColor_getRGBA(VuoColor color, VuoReal *r, VuoReal *g, VuoReal *b, VuoReal *a)
{
	*r = color.r;
	*g = color.g;
	*b = color.b;
	*a = color.a;
}

/**
 * Returns a @c VuoColor whose red, green, and blue values have been multiplied by its alpha.
 */
static inline VuoColor VuoColor_premultiply(VuoColor c) __attribute__((const));
static inline VuoColor VuoColor_premultiply(VuoColor c)
{
	return (VuoColor){ c.r*c.a, c.g*c.a, c.b*c.a, c.a };
}

VuoColor VuoColor_makeWithHSLA(VuoReal hue, VuoReal saturation, VuoReal luminosity, VuoReal alpha);
void VuoColor_getHSLA(VuoColor color, VuoReal *h, VuoReal *s, VuoReal *l, VuoReal *a);
VuoReal VuoColor_getLightness(VuoColor color);

VuoColor VuoColor_makeWithCMYKA(VuoReal c, VuoReal m, VuoReal y, VuoReal k, VuoReal a);
void VuoColor_getCMYKA(VuoColor color, VuoReal *c, VuoReal *m, VuoReal *y, VuoReal *k, VuoReal *a);

VuoText VuoColor_getHex(VuoColor color, VuoBoolean includeAlpha);

VuoColor VuoColor_average(VuoList_VuoColor colors);

/**
 * Returns a linearly-interpolated value between `a` and `b` using time `t` (between 0 and 1).
 */
static inline VuoColor VuoColor_lerp(VuoColor a, VuoColor b, float t) __attribute__((const));
static inline VuoColor VuoColor_lerp(VuoColor a, VuoColor b, float t)
{
	// Premultiply, then mix, then unpremultiply.
	VuoColor ap = VuoColor_makeWithRGBA(a.r*a.a, a.g*a.a, a.b*a.a, a.a);
	VuoColor bp = VuoColor_makeWithRGBA(b.r*b.a, b.g*b.a, b.b*b.a, b.a);

	VuoColor mixed = VuoColor_makeWithRGBA(
		ap.r*(1-t) + bp.r*t,
		ap.g*(1-t) + bp.g*t,
		ap.b*(1-t) + bp.b*t,
		ap.a*(1-t) + bp.a*t);

	float alpha = VuoReal_makeNonzero(mixed.a);
	return VuoColor_makeWithRGBA(mixed.r/alpha, mixed.g/alpha, mixed.b/alpha, mixed.a);
}

/// @{
/**
 * Automatically generated function.
 */
VuoColor VuoColor_makeFromString(const char *str);
char * VuoColor_getString(const VuoColor value);
void VuoColor_retain(VuoColor value);
void VuoColor_release(VuoColor value);
/// @}

/**
 * @}
 */
