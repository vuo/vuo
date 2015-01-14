/**
 * @file
 * vuo.point.make.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Point",
					 "description" :
						"<p>Creates a 3D point from coordinates.</p>",
					 "keywords" : [ "xyz", "cartesian", "euler", "coordinates", "vector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0}) x,
		VuoInputData(VuoReal, {"default":0}) y,
		VuoInputData(VuoReal, {"default":0}) z,
		VuoOutputData(VuoPoint3d) point
)
{
	point->x = x;
	point->y = y;
	point->z = z;
}
