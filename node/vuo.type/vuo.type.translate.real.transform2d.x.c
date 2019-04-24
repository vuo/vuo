/**
 * @file
 * vuo.type.translate.real.transform2d.x node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to Translation (X)",
					 "keywords" : [ "position", "matrix", "trs", "angle", "axis" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":-1.0,"suggestedMax":1.0,"suggestedStep":0.1}) xTranslation,
		VuoOutputData(VuoTransform2d) transform
)
{
	*transform = VuoTransform2d_make(VuoPoint2d_make(xTranslation,0), 0, VuoPoint2d_make(1,1));
}
