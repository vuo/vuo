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
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ "AlignLayersToWindow.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoLayer) layer,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputEvent({"data":"window","eventBlocking":"door"}) windowEvent,
		VuoInputData(VuoHorizontalAlignment, {"default":"left"}) horizontalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) horizontalMargin,
		VuoInputData(VuoVerticalAlignment, {"default":"top"}) verticalAlignment,
		VuoInputData(VuoReal, {"default":0.0, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) verticalMargin,
		VuoOutputData(VuoLayer) alignedLayer,
		VuoOutputEvent({"data":"alignedLayer"}) alignedLayerEvent
)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, window, &renderingDimensionsChanged);
	if (renderingDimensionsChanged)
		*alignedLayerEvent = true;

	unsigned long int viewportWidth;
	unsigned long int viewportHeight;
	float backingScaleFactor;
	if (! VuoRenderedLayers_getRenderingDimensions((*context)->renderedLayers, &viewportWidth, &viewportHeight, &backingScaleFactor))
		return;

	*alignedLayer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)layer);

	VuoReal windowAspectRatio = (VuoReal)viewportWidth/(VuoReal)viewportHeight;

	VuoRectangle layerBoundingRectangle = VuoLayer_getBoundingRectangle(layer, viewportWidth, viewportHeight, backingScaleFactor);

	if (horizontalAlignment == VuoHorizontalAlignment_Left)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){-(1. - horizontalMargin - layerBoundingRectangle.size.x/2.), 0, 0});
	else if (horizontalAlignment == VuoHorizontalAlignment_Right)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){1. - horizontalMargin - layerBoundingRectangle.size.x/2., 0, 0});

	if (verticalAlignment == VuoVerticalAlignment_Top)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){0, 1./windowAspectRatio - verticalMargin - layerBoundingRectangle.size.y/2., 0});
	else if (verticalAlignment == VuoVerticalAlignment_Bottom)
		VuoSceneObject_translate((VuoSceneObject)*alignedLayer, (VuoPoint3d){0, -(1./windowAspectRatio - verticalMargin - layerBoundingRectangle.size.y/2.), 0});
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
