/**
 * @file
 * vuo.layer.align.window node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Align Layer to Window",
					  "keywords" : [ "anchor", "position", "top", "right", "bottom", "left", "arrange", "snap" ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions" : [ "AlignLayersToWindow.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoLayer) layer,
		VuoInputData(VuoHorizontalAlignment, {"default":"left"}) horizontalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedStep":0.1}) horizontalMargin,
		VuoInputData(VuoVerticalAlignment, {"default":"top"}) verticalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedStep":0.1}) verticalMargin,
		VuoOutputData(VuoLayer) alignedLayer
)
{
	*alignedLayer = layer;

	if (!window)
		return;

	VuoInteger viewportWidth, viewportHeight;
	float backingScaleFactor;
	VuoWindowReference_getContentSize(window, &viewportWidth, &viewportHeight, &backingScaleFactor);
	VuoReal windowAspectRatio = (VuoReal)viewportWidth/(VuoReal)viewportHeight;

	VuoRectangle layerBoundingRectangle = VuoLayer_getBoundingRectangle(layer, viewportWidth, viewportHeight, backingScaleFactor);

	if (horizontalAlignment == VuoHorizontalAlignment_Left)
		(*alignedLayer).sceneObject.transform.translation.x -= 1. - horizontalMargin + (layerBoundingRectangle.center.x - layerBoundingRectangle.size.x/2.);
	else if (horizontalAlignment == VuoHorizontalAlignment_Right)
		(*alignedLayer).sceneObject.transform.translation.x += 1. - horizontalMargin + (layerBoundingRectangle.center.x - layerBoundingRectangle.size.x/2.);

	if (verticalAlignment == VuoVerticalAlignment_Top)
		(*alignedLayer).sceneObject.transform.translation.y += 1./windowAspectRatio - verticalMargin - (layerBoundingRectangle.center.y + layerBoundingRectangle.size.y/2.);
	else if (verticalAlignment == VuoVerticalAlignment_Bottom)
		(*alignedLayer).sceneObject.transform.translation.y -= 1./windowAspectRatio - verticalMargin - (layerBoundingRectangle.center.y + layerBoundingRectangle.size.y/2.);
}
