/**
 * @file
 * vuo.point.make.VuoPoint4d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 4D Point",
					 "keywords" : [ "homogenous", "xyzw", "coordinates", "vector" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) x,
		VuoInputData(VuoReal, {"default":0.0}) y,
		VuoInputData(VuoReal, {"default":0.0}) z,
		VuoInputData(VuoReal, {"default":0.0}) w,
		VuoOutputData(VuoPoint4d) point
)
{
	point->x = x;
	point->y = y;
	point->z = z;
	point->w = w;
}
