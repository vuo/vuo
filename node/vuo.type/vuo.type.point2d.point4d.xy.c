/**
 * @file
 * vuo.type.point2d.point4d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Point to 4D Point (X,Y,0,0)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}, "name":"(X,Y)"}) xy,
		VuoOutputData(VuoPoint4d, {"name":"(X,Y,0,0)"}) xyzw
)
{
	*xyzw = VuoPoint4d_make(xy.x, xy.y, 0, 0);
}
