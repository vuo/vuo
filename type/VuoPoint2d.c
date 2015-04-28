/**
 * @file
 * VuoPoint2d implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoPoint2d.h"
#include "VuoReal.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "2D Point",
					 "description" : "A floating-point 2-dimensional Cartesian spatial location.",
					 "keywords" : [ "coordinate" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c"
					 ]
				 });
#endif
/// @}

/**
 * Returns the intersecting area of @c rectangleA and @c rectangleB.
 *
 * Edges that touch (but don't overlap) are not considered to be overlapping.
 *
 * If the rectangles do not intersect, returns rectangle @c {{0,0},{0,0}}.
 */
VuoRectangle VuoPoint2d_rectangleIntersection(VuoRectangle rectangleA, VuoRectangle rectangleB)
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

	if (       rectangleARight < rectangleBLeft   + tolerance
			|| rectangleBRight < rectangleALeft   + tolerance
			|| rectangleATop   < rectangleBBottom + tolerance
			|| rectangleBTop   < rectangleABottom + tolerance)
		return VuoRectangle_make(0,0,0,0);
	else
	{
		float intersectionLeft   = MAX(rectangleALeft,   rectangleBLeft);
		float intersectionRight  = MIN(rectangleARight,  rectangleBRight);
		float intersectionBottom = MAX(rectangleABottom, rectangleBBottom);
		float intersectionTop    = MIN(rectangleATop,    rectangleBTop);

		return VuoRectangle_make(
					(intersectionLeft   + intersectionRight)/2.,
					(intersectionBottom + intersectionTop)/2.,
					intersectionRight   - intersectionLeft,
					intersectionTop     - intersectionBottom);
	}
}

/**
 * Returns the union area of @c rectangleA and @c rectangleB.
 */
VuoRectangle VuoPoint2d_rectangleUnion(VuoRectangle rectangleA, VuoRectangle rectangleB)
{
	VuoReal left	= MIN(rectangleA.center.x - rectangleA.size.x/2., rectangleB.center.x - rectangleB.size.x/2.);
	VuoReal right	= MAX(rectangleA.center.x + rectangleA.size.x/2., rectangleB.center.x + rectangleB.size.x/2.);
	VuoReal bottom	= MIN(rectangleA.center.y - rectangleA.size.y/2., rectangleB.center.y - rectangleB.size.y/2.);
	VuoReal top		= MAX(rectangleA.center.y + rectangleA.size.y/2., rectangleB.center.y + rectangleB.size.y/2.);

	return VuoRectangle_make(
				(left + right)/2.,
				(bottom + top)/2.,
				right - left,
				top - bottom);
}

/**
 * @ingroup VuoPoint2d
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "x" : 0.5,
 *     "y" : 1
 *   }
 * }
 */
VuoPoint2d VuoPoint2d_valueFromJson(json_object * js)
{
	VuoPoint2d point = {0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "x", &o))
		point.x = VuoReal_valueFromJson(o);

	if (json_object_object_get_ex(js, "y", &o))
		point.y = VuoReal_valueFromJson(o);

	return point;
}

/**
 * @ingroup VuoPoint2d
 * Encodes @c value as a JSON object.
 */
json_object * VuoPoint2d_jsonFromValue(const VuoPoint2d value)
{
	json_object *js = json_object_new_object();

	json_object *xObject = VuoReal_jsonFromValue(value.x);
	json_object_object_add(js, "x", xObject);

	json_object *yObject = VuoReal_jsonFromValue(value.y);
	json_object_object_add(js, "y", yObject);

	return js;
}

/**
 * @ingroup VuoPoint2d
 * Returns a compact string representation of @c value (comma-separated coordinates).
 */
char * VuoPoint2d_summaryFromValue(const VuoPoint2d value)
{
	const char *format = "%g, %g";
	int size = snprintf(NULL,0,format,value.x,value.y);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString,size+1,format,value.x,value.y);
	return valueAsString;
}
