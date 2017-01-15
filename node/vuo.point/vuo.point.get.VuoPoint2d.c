/**
 * @file
 * vuo.point.get.VuoPoint2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get 2D Point Values",
					 "keywords" : [ "xy", "cartesian", "euler", "coordinates" ],
					 "version" : "2.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}}) point,
		VuoOutputData(VuoReal) x,
		VuoOutputData(VuoReal) y
)
{
	*x = point.x;
	*y = point.y;
}
