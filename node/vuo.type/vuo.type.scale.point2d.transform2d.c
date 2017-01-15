/**
 * @file
 * vuo.type.scale.point2d.transform2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Point to Scale",
					 "keywords" : [ "matrix", "trs", "size", "angle", "axis", "grow", "shrink", "stretch" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":1, "y":1}}) scale,
		VuoOutputData(VuoTransform2d) transform
)
{
	*transform = VuoTransform2d_make(VuoPoint2d_make(0,0), 0., scale);
}
