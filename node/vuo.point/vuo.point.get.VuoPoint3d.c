/**
 * @file
 * vuo.point.get.VuoPoint3d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get 3D Point Values",
					 "keywords" : [ "xyz", "cartesian", "euler", "coordinates" ],
					 "version" : "2.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) point,
		VuoOutputData(VuoReal) x,
		VuoOutputData(VuoReal) y,
		VuoOutputData(VuoReal) z
)
{
	*x = point.x;
	*y = point.y;
	*z = point.z;
}
