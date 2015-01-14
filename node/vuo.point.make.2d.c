/**
 * @file
 * vuo.point.make.2d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 2D Point",
					 "description" :
						"<p>Creates a 2D point from coordinates.</p>",
					 "keywords" : [ "xy", "cartesian", "euler", "coordinates", "vector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0}) x,
		VuoInputData(VuoReal, {"default":0}) y,
		VuoOutputData(VuoPoint2d) point
)
{
	point->x = x;
	point->y = y;
}
