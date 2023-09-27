/**
 * @file
 * vuo.layer.bounds.rendered node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRenderedLayers.h"

VuoModuleMetadata({
					 "title" : "Get Rendered Layer Bounds",
					 "keywords" : [ "box", "aabb", "axis", "collider", "collision", "size", "rectangle" ],
					 "version" : "1.0.2",
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "StretchOvalWithBounds.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) layerName,
		VuoInputData(VuoBoolean, {"default":true}) includeChildren,
		VuoOutputData(VuoPoint2d) center,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoReal) height,
		VuoOutputData(VuoInteger) pixelsWide,
		VuoOutputData(VuoInteger) pixelsHigh
)
{
	VuoSceneObject layer;
	VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestorObjects);
	bool isLayerFound = VuoRenderedLayers_findLayer(renderedLayers, layerName, ancestorObjects, &layer);
	if (isLayerFound)
	{
		VuoPoint2d layerCorners[4];
		if (!VuoRenderedLayers_getTransformedLayer(renderedLayers, ancestorObjects, layer, center, layerCorners, includeChildren))
			return;
		VuoRectangle bounds = VuoRenderedLayers_getBoundingBox(layerCorners);
		*width  = bounds.size.x;
		*height = bounds.size.y;

		unsigned long int rlPixelsWide, rlPixelsHigh;
		float backingScaleFactor;
		if (VuoRenderedLayers_getRenderingDimensions(renderedLayers, &rlPixelsWide, &rlPixelsHigh, &backingScaleFactor))
		{
			*pixelsWide = rlPixelsWide * bounds.size.x / 2.;
			*pixelsHigh = rlPixelsWide * bounds.size.y / 2.;
		}
	}
}
