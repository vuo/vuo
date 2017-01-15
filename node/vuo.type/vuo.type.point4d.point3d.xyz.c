/**
 * @file
 * vuo.type.point4d.point3d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 4D Point to 3D Point (X,Y,Z)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint4d, {"default":{"x":0, "y":0, "z":0, "w":0}, "name":"(X,Y,Z,W)"}) xyzw,
		VuoOutputData(VuoPoint3d, {"name":"(X,Y,Z)"}) xyz
)
{
	*xyz = VuoPoint3d_make(xyzw.x, xyzw.y, xyzw.z);
}
