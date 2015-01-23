/**
 * @file
 * vuo.scene.make.image node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Object from Image",
					 "keywords" : [ "billboard", "sprite", "projector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "FlipCoin.vuo", "RippleImageOfSphere.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMax":{"x":360.0,"y":360.0,"z":360.0}, "suggestedStep":{"x":1.0,"y":1.0,"z":1.0}}) rotation,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) alpha,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeImage(image, center, rotation, width, alpha);
}
