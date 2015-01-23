/**
 * @file
 * vuo.type.point2d.point4d node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Point to 4D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}}) xy,
		VuoOutputData(VuoPoint4d) xyzw
)
{
	*xyzw = VuoPoint4d_make(xy.x, xy.y, 0, 0);
}
