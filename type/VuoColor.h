/**
 * @file
 * VuoColor C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOLOR_H
#define VUOCOLOR_H

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
 */
typedef struct
{
	float r,g,b,a;
} VuoColor;

VuoColor VuoColor_makeFromJson(struct json_object * js);
struct json_object * VuoColor_getJson(const VuoColor value);
char * VuoColor_getSummary(const VuoColor value);

#define VuoColor_SUPPORTS_COMPARISON
bool VuoColor_areEqual(const VuoColor a, const VuoColor b);
bool VuoColor_isLessThan(const VuoColor a, const VuoColor b);

/**
 * Returns a @c VuoColor with the given red, green, blue, alpha.
 *
 * Assumes each value is in the range [0,1], and specifies a color in the sRGB color space.
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
 */
static inline void VuoColor_getRGBA(VuoColor color, VuoReal *r, VuoReal *g, VuoReal *b, VuoReal *a)
{
	*r = color.r;
	*g = color.g;
	*b = color.b;
	*a = color.a;
}

VuoColor VuoColor_makeWithHSLA(VuoReal hue, VuoReal saturation, VuoReal luminosity, VuoReal alpha);
void VuoColor_getHSLA(VuoColor color, VuoReal *h, VuoReal *s, VuoReal *l, VuoReal *a);

VuoText VuoColor_getHex(VuoColor color, VuoBoolean includeAlpha);

VuoColor VuoColor_average(VuoList_VuoColor colors);

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

#endif
