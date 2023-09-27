/**
 * @file
 * vuo.scene.make.camera.perspective node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Make Perspective Camera",
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
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0},
								  "suggestedMin":{"x":-180,"y":-180,"z":-180},
								  "suggestedMax":{"x":180,"y":180,"z":180},
								  "suggestedStep":{"x":15.0,"y":15.0,"z":15.0}}) rotation,
		VuoInputData(VuoReal, {"name":"Field of View", "default":90.0, "suggestedMin":0.01, "suggestedMax":179.9, "suggestedStep":1.0}) fieldOfView,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.01, "suggestedMax":20., "suggestedStep":0.1}) distanceMin,
		VuoInputData(VuoReal, {"default":10.0, "suggestedMin":0.01, "suggestedMax":20., "suggestedStep":0.1}) distanceMax,
		VuoOutputData(VuoSceneObject) object
)
{
	VuoTransform transform = VuoTransform_makeEuler(
				position,
				VuoPoint3d_multiply(rotation, M_PI/180.f),
				VuoPoint3d_make(1,1,1)
			);
	*object = VuoSceneObject_makePerspectiveCamera(
				name,
				transform,
				fieldOfView,
				distanceMin,
				distanceMax
				);
}
