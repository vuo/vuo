/**
 * @file
 * vuo.layer.bounds.rendered2 node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"

VuoModuleMetadata({
	"title" : "Get Rendered Layer Bounds",
	"keywords": [
		"box", "rectangle",
		"aabb", "axis", "collider", "collision",
		"size", "dimensions", "measurements", "geometry", "bounding", "boundaries", "length",
	],
	"version" : "2.0.0",
	"node": {
		"exampleCompositions" : [ ]
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
	VuoInputEvent({"data":"layer","eventBlocking":"wall"}) layerEvent,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputEvent({"data":"window","eventBlocking":"door"}) windowEvent,
	VuoInputData(VuoBoolean, {"default":true}) includeChildren,
	VuoInputEvent({"data":"includeChildren","eventBlocking":"wall"}) includeChildrenEvent,

	VuoOutputData(VuoPoint2d) center,
	VuoOutputEvent({"data":"center"}) centerEvent,
	VuoOutputData(VuoReal) width,
	VuoOutputEvent({"data":"width"}) widthEvent,
	VuoOutputData(VuoReal) height,
	VuoOutputEvent({"data":"height"}) heightEvent,
	VuoOutputData(VuoInteger) pixelsWide,
	VuoOutputEvent({"data":"pixelsWide"}) pixelsWideEvent,
	VuoOutputData(VuoInteger) pixelsHigh,
	VuoOutputEvent({"data":"pixelsHigh"}) pixelsHighEvent
)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, window, &renderingDimensionsChanged);

	VuoSceneObject rootSceneObject = VuoRenderedLayers_getRootSceneObject((*context)->renderedLayers);
	if (!rootSceneObject)
		return;

	unsigned long int viewportWidth;
	unsigned long int viewportHeight;
	float backingScaleFactor;
	if (! VuoRenderedLayers_getRenderingDimensions((*context)->renderedLayers, &viewportWidth, &viewportHeight, &backingScaleFactor))
		return;

	VuoSceneObject foundObject;
	VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
	VuoLocal(ancestorObjects);
	bool isLayerFound = VuoSceneObject_findById(rootSceneObject, VuoLayer_getId(layer), ancestorObjects, &foundObject);
	if (isLayerFound)
	{
		VuoPoint2d layerCorners[4];
		bzero(layerCorners, sizeof(layerCorners));
		VuoPoint2d newCenter = (VuoPoint2d){0,0};
		if (!VuoRenderedLayers_getTransformedLayer((*context)->renderedLayers, ancestorObjects, foundObject, &newCenter, layerCorners, includeChildren))
			return;
		VuoRectangle bounds = VuoRenderedLayers_getBoundingBox(layerCorners);
		VuoInteger newPixelsWide = viewportWidth * bounds.size.x / 2.;
		VuoInteger newPixelsHigh = viewportWidth * bounds.size.y / 2.;

		if (!VuoPoint2d_areEqual(*center, newCenter)
		 || *width != bounds.size.x
		 || *height != bounds.size.y
		 || *pixelsWide != newPixelsWide
		 || *pixelsHigh != newPixelsHigh)
		{
			*center = newCenter;
			*width  = bounds.size.x;
			*height = bounds.size.y;
			*pixelsWide = newPixelsWide;
			*pixelsHigh = newPixelsHigh;

			*centerEvent = *widthEvent = *heightEvent = *pixelsWideEvent = *pixelsHighEvent = true;
		}
	}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
