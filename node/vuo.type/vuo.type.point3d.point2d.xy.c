/**
 * @file
 * vuo.type.point3d.point2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Point to 2D Point (X,Y)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}, "name":"(X,Y,Z)"}) xyz,
		VuoOutputData(VuoPoint2d, {"name":"(X,Y)"}) xy
)
{
	*xy = VuoPoint2d_make(xyz.x, xyz.y);
}
