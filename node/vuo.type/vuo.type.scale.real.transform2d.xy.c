/**
 * @file
 * vuo.type.scale.real.transform2d.xy node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to Scale (XY)",
					 "keywords" : [ "matrix", "trs", "size", "angle", "axis", "grow", "shrink", "stretch" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0,"suggestedStep":0.1}) scale,
		VuoOutputData(VuoTransform2d) transform
)
{
	*transform = VuoTransform2d_make(VuoPoint2d_make(0,0), 0, VuoPoint2d_make(scale,scale));
}
