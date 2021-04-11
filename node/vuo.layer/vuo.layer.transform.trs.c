/**
 * @file
 * vuo.layer.transform.trs node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
	"title": "Transform Layer (TRS)",
	"keywords": [
		"scenegraph", "composite",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	}
});

void nodeEvent(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0},
							  "suggestedMin":{"x":-1,"y":-1},
							  "suggestedMax":{"x":1,"y":1},
							  "suggestedStep":{"x":0.1,"y":0.1}}) translation,
	VuoInputData(VuoReal,    {"default":0.0,
							  "suggestedMin":-180.0,
							  "suggestedMax":180.0,
							  "suggestedStep":15.0}) rotation,
	VuoInputData(VuoPoint2d, {"default":{"x":1.0,"y":1.0},
							  "suggestedMin":{"x":0,"y":0},
							  "suggestedMax":{"x":2,"y":2},
							  "suggestedStep":{"x":0.1,"y":0.1}}) scale,
	VuoOutputData(VuoLayer) transformedLayer)
{
	VuoTransform t3d = VuoTransform_makeFrom2d(VuoTransform2d_make(translation, rotation * M_PI/180., scale));
	*transformedLayer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)layer);
	VuoSceneObject_transform((VuoSceneObject)*transformedLayer, t3d);
}
