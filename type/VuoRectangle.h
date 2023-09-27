/**
 * @file
 * VuoRectangle C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoRectangle_h
#define VuoRectangle_h

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoPoint2d.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoRectangle VuoRectangle
 * A 2-dimensional axis-aligned box.
 *
 * @{
 */

/**
 * A 2-dimensional axis-aligned box.
 */
typedef struct
{
	VuoPoint2d center;
	VuoPoint2d size;
} VuoRectangle;

#define VuoRectangle_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.

VuoRectangle VuoRectangle_makeFromJson(struct json_object *js);
struct json_object *VuoRectangle_getJson(const VuoRectangle r);
char *VuoRectangle_getSummary(const VuoRectangle r);

bool VuoRectangle_areEqual(const VuoRectangle a, const VuoRectangle b);
bool VuoRectangle_isLessThan(const VuoRectangle a, const VuoRectangle b);

/// @{
/**
 * Automatically generated function.
 */
char *VuoRectangle_getString(const VuoRectangle r);
void VuoRectangle_retain(VuoRectangle r);
void VuoRectangle_release(VuoRectangle r);
/// @}

/**
 * Returns a rectangle with the specified coordinates.
 */
static inline VuoRectangle VuoRectangle_make(float centerX, float centerY, float width, float height) __attribute__((const));
static inline VuoRectangle VuoRectangle_make(float centerX, float centerY, float width, float height)
{
	return (VuoRectangle){ { centerX, centerY }, { width, height } };
}

/**
 * Returns a rectangle with the specified coordinates.
 */
static inline VuoRectangle VuoRectangle_makeTopLeft(float leftX, float topY, float width, float height) __attribute__((const));
static inline VuoRectangle VuoRectangle_makeTopLeft(float leftX, float topY, float width, float height)
{
	return (VuoRectangle){ { leftX + width / 2.f, topY + height / 2.f }, { width, height } };
}

bool VuoRectangle_isPointInside(VuoRectangle r, VuoPoint2d p);

VuoRectangle VuoRectangle_intersection(VuoRectangle rectangleA, VuoRectangle rectangleB);
VuoRectangle VuoRectangle_union(VuoRectangle rectangleA, VuoRectangle rectangleB);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
