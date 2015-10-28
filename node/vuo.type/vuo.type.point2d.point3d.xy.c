/**
 * @file
 * vuo.type.point2d.point3d node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Point to 3D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}}) xy,
		VuoOutputData(VuoPoint3d) xyz
)
{
	*xyz = VuoPoint3d_make(xy.x, xy.y, 0);
}
