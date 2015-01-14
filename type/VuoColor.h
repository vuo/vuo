/**
 * @file
 * VuoColor C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOLOR_H
#define VUOCOLOR_H

#include "VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoColor VuoColor
 * A color, with component values specifying red, green, blue, and alpha (opacity).
 *
 * @{
 */

/**
 * A color, with component values specifying red, green, blue, and alpha (opacity).
 */
typedef struct
{
	float r,g,b,a;
} VuoColor;

VuoColor VuoColor_valueFromJson(struct json_object * js);
struct json_object * VuoColor_jsonFromValue(const VuoColor value);
char * VuoColor_summaryFromValue(const VuoColor value);

/**
 * Returns a @c VuoColor with the given red, green, blue, alpha.
 *
 * Assumes each value is in the range [0,1].
 */
static inline VuoColor VuoColor_makeWithRGBA(VuoReal r, VuoReal g, VuoReal b, VuoReal a) __attribute__((const));
static inline VuoColor VuoColor_makeWithRGBA(VuoReal r, VuoReal g, VuoReal b, VuoReal a)
{
	VuoColor c = {r,g,b,a};
	return c;
}

/**
 * Gets the red, green, blue, alpha of a color.
 *
 * Each value is in the range [0,1].
 */
static inline void VuoColor_getRGBA(VuoColor color, VuoReal *r, VuoReal *g, VuoReal *b, VuoReal *a)
{
	*r = color.r;
	*g = color.g;
	*b = color.b;
	*a = color.a;
}

VuoColor VuoColor_makeWithHSLA(VuoReal h, VuoReal s, VuoReal l, VuoReal a);
void VuoColor_getHSLA(VuoColor color, VuoReal *h, VuoReal *s, VuoReal *l, VuoReal *a);

/// @{
/**
 * Automatically generated function.
 */
VuoColor VuoColor_valueFromString(const char *str);
char * VuoColor_stringFromValue(const VuoColor value);
/// @}

/**
 * @}
 */

#endif
