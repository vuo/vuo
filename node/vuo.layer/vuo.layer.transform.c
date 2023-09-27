/**
 * @file
 * vuo.layer.transform node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Transform Layer (Transform)",
					 "keywords" : [ "scenegraph", "composite", "rotate", "scale", "translate" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "vuo-example://vuo.layer/ShowArrangedLayers.vuo", "vuo-example://vuo.list/SelectLayerFromList.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoLayer) layer,
		VuoInputData(VuoTransform2d) transform,
		VuoOutputData(VuoLayer) transformedLayer
)
{
	VuoTransform t3d = VuoTransform_makeFrom2d(transform);
	*transformedLayer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)layer);
	VuoSceneObject_transform((VuoSceneObject)*transformedLayer, t3d);
}
