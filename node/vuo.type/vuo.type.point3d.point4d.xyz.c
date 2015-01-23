/**
 * @file
 * vuo.type.point3d.point4d node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Point to 4D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) xyz,
		VuoOutputData(VuoPoint4d) xyzw
)
{
	*xyzw = VuoPoint4d_make(xyz.x, xyz.y, xyz.z, 0);
}
