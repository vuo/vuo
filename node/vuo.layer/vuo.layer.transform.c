/**
 * @file
 * vuo.layer.transform node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Transform Layer",
					 "keywords" : [ "scenegraph", "composite", "rotate", "scale", "translate" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "vuo-example://vuo.layer/ShowArrangedLayers.vuo", "vuo-example://vuo.list/SelectLayerFromList.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform2d) transform,
		VuoInputData(VuoLayer) layer,
		VuoOutputData(VuoLayer) transformedLayer
)
{
	VuoTransform t3d = VuoTransform_makeFrom2d(transform);
	(*transformedLayer) = layer;
	(*transformedLayer).sceneObject.transform = VuoTransform_composite(layer.sceneObject.transform, t3d);
}
