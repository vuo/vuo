/**
 * @file
 * vuo.point.make.VuoPoint3d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Point",
					 "keywords" : [ "xyz", "cartesian", "euler", "coordinates", "vector" ],
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
		VuoOutputData(VuoPoint3d) point
)
{
	point->x = x;
	point->y = y;
	point->z = z;
}
