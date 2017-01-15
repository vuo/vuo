/**
 * @file
 * vuo.transform.make.2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 2D Transform",
					 "keywords" : [ "translation", "rotation", "scale", "shift", "move", "position",
						 "angle", "roll", "axis", "size", "grow", "shrink" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0},"suggestedStep":{"x":0.1,"y":0.1}}) translation,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":360.0,"suggestedStep":15.0}) rotation,
		VuoInputData(VuoPoint2d, {"default":{"x":1.0,"y":1.0},"suggestedStep":{"x":0.1,"y":0.1}}) scale,
		VuoOutputData(VuoTransform2d) transform
)
{
	*transform = VuoTransform2d_make(translation, rotation * M_PI/180., scale);
}
