/**
 * @file
 * vuo.scene.make.camera.perspective.target node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Targeted Perspective Camera",
					 "keywords" : [ "frustum", "projection", "draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "CompareCameras.vuo", "SwitchCameras.vuo" ]
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
		VuoInputData(VuoReal, {"name":"Field of View", "default":90.0, "suggestedMin":0.01, "suggestedMax":179.9, "suggestedStep":1.0}) fieldOfView,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.01, "suggestedMax":20., "suggestedStep":0.1}) distanceMin,
		VuoInputData(VuoReal, {"default":10.0, "suggestedMin":0.01, "suggestedMax":20., "suggestedStep":0.1}) distanceMax,
		VuoOutputData(VuoSceneObject) object
)
{
	VuoTransform transform = VuoTransform_makeFromTarget(position, target, upDirection);
	*object = VuoSceneObject_makePerspectiveCamera(
				name,
				transform,
				fieldOfView,
				distanceMin,
				distanceMax
				);
}
