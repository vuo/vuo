/**
 * @file
 * vuo.layer.bounds.rendered node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRenderedLayers.h"

VuoModuleMetadata({
					 "title" : "Get Rendered Layer Bounds",
					 "keywords" : [ "box", "aabb", "axis", "collider", "collision", "size", "rectangle" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "StretchOvalWithBounds.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) layerName,
		VuoOutputData(VuoPoint2d) center,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoReal) height,
		VuoOutputData(VuoInteger) pixelsWide,
		VuoOutputData(VuoInteger) pixelsHigh
)
{
	VuoSceneObject layer;
	VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
	bool isLayerFound = VuoRenderedLayers_findLayer(renderedLayers, layerName, ancestorObjects, &layer);
	if (isLayerFound)
	{
		VuoPoint2d layerCorners[4];
		VuoRenderedLayers_getTransformedLayer(renderedLayers, ancestorObjects, layer, center, layerCorners);
		VuoRectangle bounds = VuoRenderedLayers_getBoundingBox(layerCorners);
		*width  = bounds.size.x;
		*height = bounds.size.y;
		*pixelsWide = renderedLayers.pixelsWide * bounds.size.x / 2.;
		*pixelsHigh = renderedLayers.pixelsWide * bounds.size.y / 2.;
	}
	VuoRetain(ancestorObjects);
	VuoRelease(ancestorObjects);
}
