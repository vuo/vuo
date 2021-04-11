/**
 * @file
 * vuo.scene.make.camera.orthographic.target node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Targeted Orthographic Camera",
					 "keywords" : [
						"axonometric", "elevation", "isometric", "orthogonal", "parallel", "projection", "flat",
						"draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "CompareCameras.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"camera"}) name,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":1.0},
								  "suggestedMin":{"x":-2,"y":-2,"z":-2},
								  "suggestedMax":{"x":2,"y":2,"z":2},
								  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) position,
		VuoInputData(VuoPoint3d, {"default":{"x":0.,"y":0.,"z":-1.},
								  "suggestedMin":{"x":-2,"y":-2,"z":-2},
								  "suggestedMax":{"x":2,"y":2,"z":2},
								  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) target,
		VuoInputData(VuoPoint3d, {"default":{"x":0.,"y":1.,"z":0.},
								  "suggestedMin":{"x":-2,"y":-2,"z":-2},
								  "suggestedMax":{"x":2,"y":2,"z":2},
								  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) upDirection,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.01, "suggestedMax":2., "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":-10.0, "suggestedMin":-20.0, "suggestedMax":20., "suggestedStep":0.1}) distanceMin,  // The orthographic camera can have zero or negative clip planes (unlike the perspective camera).
		VuoInputData(VuoReal, {"default":10.0, "suggestedMin":-20.0, "suggestedMax":20., "suggestedStep":0.1}) distanceMax,
		VuoOutputData(VuoSceneObject) object
)
{
	VuoTransform transform = VuoTransform_makeFromTarget(position, target, upDirection);
	*object = VuoSceneObject_makeOrthographicCamera(
				name,
				transform,
				width,
				distanceMin,
				distanceMax
				);
}
