/**
 * @file
 * vuo.scene.transform.trs node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Transform 3D Object (TRS)",
	"keywords": [
		"scenegraph", "composite",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions" : [ ],
	}
});

void nodeEvent(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},
							  "suggestedMin":{"x":-2,"y":-2,"z":-2},
							  "suggestedMax":{"x":2,"y":2,"z":2},
							  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) translation,
	VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},
							  "suggestedMin":{"x":-180,"y":-180,"z":-180},
							  "suggestedMax":{"x":180,"y":180,"z":180},
							  "suggestedStep":{"x":15,"y":15,"z":15}}) rotation,
	VuoInputData(VuoPoint3d, {"default":{"x":1,"y":1,"z":1},
							  "suggestedMin":{"x":0,"y":0,"z":0},
							  "suggestedMax":{"x":2,"y":2,"z":2},
							  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) scale,
	VuoOutputData(VuoSceneObject) transformedObject)
{
	VuoTransform t = VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale);
	*transformedObject = VuoSceneObject_copy(object);
	VuoSceneObject_transform(*transformedObject, t);
}
