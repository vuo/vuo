/**
 * @file
 * VuoRectangle implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "Rectangle",
	"description" : "A 2-dimensional axis-aligned box.",
	"keywords" : [
		"coordinate",
		"point", "size",
		"box", "square",
	],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoPoint2d",
		"VuoReal",
		"VuoText"
	]
});
#endif
/// @}

/**
 * @ingroup VuoRectangle
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "center" : [0,0],
 *     "size" : [1,1]
 *   }
 * }
 *
 * @version200New
 */
VuoRectangle VuoRectangle_makeFromJson(json_object *js)
{
	return (VuoRectangle){
		VuoJson_getObjectValue(VuoPoint2d, js, "center", {0,0}),
		VuoJson_getObjectValue(VuoPoint2d, js, "size",   {0,0})
	};
}

/**
 * @ingroup VuoRectangle
 * Encodes @c r as a JSON object.
 *
 * @version200New
 */
json_object *VuoRectangle_getJson(const VuoRectangle r)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "center", VuoPoint2d_getJson(r.center));
	json_object_object_add(js, "size",   VuoPoint2d_getJson(r.size));
	return js;
}

/**
 * @ingroup VuoRectangle
 * Returns a compact string representation of @c r (comma-separated coordinates).
 *
 * @version200New
 */
char *VuoRectangle_getSummary(const VuoRectangle r)
{
	return VuoText_format("%g×%g @ %g,%g", r.size.x, r.size.y, r.center.x, r.center.y);
}

/**
 * Returns true if the two rectangles are equal (within tolerance).
 *
 * @version200New
 */
bool VuoRectangle_areEqual(const VuoRectangle a, const VuoRectangle b)
{
	return VuoPoint2d_areEqual(a.center, b.center)
		&& VuoPoint2d_areEqual(a.size,   b.size);
}

/**
 * Compares `a` to `b` primarily by `center`-value and secondarily by `size`-value,
 * returning true if `a` is less than `b`.
 *
 * @version200New
 */
bool VuoRectangle_isLessThan(const VuoRectangle a, const VuoRectangle b)
{
	VuoType_returnInequality(VuoPoint2d, a.center, b.center);
	VuoType_returnInequality(VuoPoint2d, a.size, b.size);
	return false;
}

/**
 * Returns true if `p` is inside `r`.
 *
 * @version200New
 */
bool VuoRectangle_isPointInside(VuoRectangle r, VuoPoint2d p)
{
	return p.x >= r.center.x - r.size.x/2
		&& p.x <= r.center.x + r.size.x/2
		&& p.y >= r.center.y - r.size.y/2
		&& p.y <= r.center.y + r.size.y/2;
}

/**
 * Returns the intersecting area of @c rectangleA and @c rectangleB.
 *
 * Edges that touch (but don't overlap) are not considered to be overlapping.
 *
 * If the rectangles do not intersect, returns rectangle @c {{0,0},{0,0}}.
 *
 * @version200New
 */
VuoRectangle VuoRectangle_intersection(VuoRectangle rectangleA, VuoRectangle rectangleB)
{
	float rectangleALeft   = rectangleA.center.x - rectangleA.size.x/2.;
	float rectangleARight  = rectangleA.center.x + rectangleA.size.x/2.;
	float rectangleABottom = rectangleA.center.y - rectangleA.size.y/2.;
	float rectangleATop    = rectangleA.center.y + rectangleA.size.y/2.;

	float rectangleBLeft   = rectangleB.center.x - rectangleB.size.x/2.;
	float rectangleBRight  = rectangleB.center.x + rectangleB.size.x/2.;
	float rectangleBBottom = rectangleB.center.y - rectangleB.size.y/2.;
	float rectangleBTop    = rectangleB.center.y + rectangleB.size.y/2.;

	float tolerance = 0.00001;

	if (rectangleARight < rectangleBLeft   + tolerance
	 || rectangleBRight < rectangleALeft   + tolerance
	 || rectangleATop   < rectangleBBottom + tolerance
	 || rectangleBTop   < rectangleABottom + tolerance)
		return (VuoRectangle){ { 0, 0 }, { 0, 0 } };
	else
	{
		float intersectionLeft   = MAX(rectangleALeft,   rectangleBLeft);
		float intersectionRight  = MIN(rectangleARight,  rectangleBRight);
		float intersectionBottom = MAX(rectangleABottom, rectangleBBottom);
		float intersectionTop    = MIN(rectangleATop,    rectangleBTop);

		return (VuoRectangle){
			{ (intersectionLeft + intersectionRight) / 2.,
			  (intersectionBottom + intersectionTop) / 2. },
			{ intersectionRight - intersectionLeft,
			  intersectionTop - intersectionBottom }
		};
	}
}

/**
 * Returns the union area of @c rectangleA and @c rectangleB.
 *
 * @version200New
 */
VuoRectangle VuoRectangle_union(VuoRectangle rectangleA, VuoRectangle rectangleB)
{
	VuoReal left   = MIN(rectangleA.center.x - rectangleA.size.x / 2., rectangleB.center.x - rectangleB.size.x / 2.);
	VuoReal right  = MAX(rectangleA.center.x + rectangleA.size.x / 2., rectangleB.center.x + rectangleB.size.x / 2.);
	VuoReal bottom = MIN(rectangleA.center.y - rectangleA.size.y / 2., rectangleB.center.y - rectangleB.size.y / 2.);
	VuoReal top    = MAX(rectangleA.center.y + rectangleA.size.y / 2., rectangleB.center.y + rectangleB.size.y / 2.);

	return (VuoRectangle){
		{ (left + right) / 2.,
		  (bottom + top) / 2. },
		{ right - left,
		  top - bottom }
	};
}
