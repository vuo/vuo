/**
 * @file
 * vuo.scene.make.light.point node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Point Light",
					  "keywords" : [ "draw", "opengl", "scenegraph", "graphics", "omnidirectional", "sunlight", "lighting", "source" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "CompareLights.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) color,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0., "suggestedMax":2., "suggestedStep":1.0}) brightness,
		VuoInputData(VuoPoint3d, {"default":{"x":-1.,"y":1.,"z":1.}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) position,
		VuoInputData(VuoReal, {"default":10., "suggestedMin":0.}, "suggestedStep":0.1) range,
		VuoInputData(VuoReal, {"default":0.9, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) sharpness,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makePointLight(color, brightness, position, range, sharpness);
}
