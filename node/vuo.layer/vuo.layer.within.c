/**
 * @file
 * vuo.layer.within node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRenderedLayers.h"

VuoModuleMetadata({
					  "title" : "Is Point within Layer",
					  "keywords" : [ "hit", "contains", "bounds" ],
					  "version" : "1.0.1",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "IsMouseWithinLayer.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}}) point,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputEvent({"eventBlocking":"wall","data":"renderedLayers"}) renderedLayersEvent,
		VuoInputData(VuoText) layerName,
		VuoOutputData(VuoBoolean) withinLayer
)
{
	*withinLayer = VuoRenderedLayers_isPointInLayer(renderedLayers, layerName, point);
}
