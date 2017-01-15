/**
 * @file
 * vuo.transform.make node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Transform",
					 "keywords" : [ "Euler", "translation", "rotation", "scale", "shift", "move", "position",
						 "angle", "yaw", "pitch", "roll", "axis", "size", "grow", "shrink" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) translation,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedMin":{"x":0,"y":0,"z":0},"suggestedMax":{"x":360,"y":360,"z":360},"suggestedStep":{"x":15,"y":15,"z":15}}) rotation,
		VuoInputData(VuoPoint3d, {"default":{"x":1,"y":1,"z":1},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) scale,
		VuoOutputData(VuoTransform) transform
)
{
	*transform = VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale);
}
