/**
 * @file
 * vuo.point.make.VuoPoint2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 2D Point",
					 "keywords" : [ "xy", "cartesian", "euler", "coordinates", "vector" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) x,
		VuoInputData(VuoReal, {"default":0.0}) y,
		VuoOutputData(VuoPoint2d) point
)
{
	point->x = x;
	point->y = y;
}
