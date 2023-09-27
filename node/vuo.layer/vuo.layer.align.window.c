/**
 * @file
 * vuo.layer.align.window node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Align Layer to Window",
					  "keywords" : [ "anchor", "position", "top", "right", "bottom", "left", "arrange", "snap" ],
					  "version" : "1.1.0",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "AlignLayersToWindow.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoLayer) layer,
		VuoInputData(VuoHorizontalAlignment, {"default":"left"}) horizontalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) horizontalMargin,
		VuoInputData(VuoVerticalAlignment, {"default":"top"}) verticalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) verticalMargin,
		VuoOutputData(VuoLayer) alignedLayer
)
{
	*alignedLayer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)layer);

	if (!window)
		return;

	VuoInteger viewportWidth, viewportHeight;
	float backingScaleFactor;
	VuoWindowReference_getContentSize(window, &viewportWidth, &viewportHeight, &backingScaleFactor);
	VuoReal windowAspectRatio = (VuoReal)viewportWidth/(VuoReal)viewportHeight;

	VuoRectangle layerBoundingRectangle = VuoLayer_getBoundingRectangle(layer, viewportWidth, viewportHeight, backingScaleFactor);

	if (horizontalAlignment == VuoHorizontalAlignment_Left)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){-(1. - horizontalMargin - layerBoundingRectangle.size.x/2.), 0, 0});
	else if (horizontalAlignment == VuoHorizontalAlignment_Right)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){1. - horizontalMargin - layerBoundingRectangle.size.x/2., 0, 0});

	if (verticalAlignment == VuoVerticalAlignment_Top)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){0, 1./windowAspectRatio - verticalMargin - layerBoundingRectangle.size.y/2., 0});
	else if (verticalAlignment == VuoVerticalAlignment_Bottom)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){0, -(1./windowAspectRatio - verticalMargin - layerBoundingRectangle.size.y/2.)});
}
