/**
 * @file
 * vuo.window.get.dimensions3 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Get Window Dimensions",
	"keywords": [
		"info",
		"position", "location", "origin",
		"frame", "bounds", "size",
	],
	"version": "3.0.0",
	"node": {
		"exampleCompositions": [ "ShowWindowInfo.vuo" ],
	},
});

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
	VuoCoordinateUnit priorUnit;
	VuoPoint2d priorPosition;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	return context;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputEvent({"eventBlocking":"door","data":"window"}) windowEvent,
	VuoInputData(VuoCoordinateUnit, {"default":"points"}) unit,
	VuoInputEvent({"eventBlocking":"wall","data":"unit"}) unitEvent,

	VuoOutputData(VuoPoint2d) topLeft,
	VuoOutputEvent({"data":"topLeft"}) topLeftEvent,
	VuoOutputData(VuoPoint2d) bottomRight,
	VuoOutputEvent({"data":"bottomRight"}) bottomRightEvent,
	VuoOutputData(VuoReal) width,
	VuoOutputEvent({"data":"width"}) widthEvent,
	VuoOutputData(VuoReal) height,
	VuoOutputEvent({"data":"height"}) heightEvent,
	VuoOutputData(VuoReal) aspectRatio,
	VuoOutputEvent({"data":"aspectRatio"}) aspectRatioEvent)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, window, &renderingDimensionsChanged);

	unsigned long int pixelsWide;
	unsigned long int pixelsHigh;
	float backingScaleFactor;
	if (!VuoRenderedLayers_getRenderingDimensions((*context)->renderedLayers, &pixelsWide, &pixelsHigh, &backingScaleFactor))
		return;

	VuoWindowReference windowReference;
	if (!VuoRenderedLayers_getWindow((*context)->renderedLayers, &windowReference))
		return;

	VuoPoint2d position = VuoWindowReference_getPosition(windowReference);

	if (!renderingDimensionsChanged && (*context)->priorUnit == unit && VuoPoint2d_areEqual(position, (*context)->priorPosition))
		return;

	*aspectRatio = (VuoReal)pixelsWide / pixelsHigh;

	if (unit == VuoCoordinateUnit_Pixels)
	{
		*width  = pixelsWide;
		*height = pixelsHigh;
		*topLeft     = position * backingScaleFactor;
		*bottomRight = position * backingScaleFactor + (VuoPoint2d){ pixelsWide, pixelsHigh };
	}
	else if (unit == VuoCoordinateUnit_Points)
	{
		*width  = pixelsWide / backingScaleFactor;
		*height = pixelsHigh / backingScaleFactor;
		*topLeft     = position;
		*bottomRight = position + (VuoPoint2d){ *width, *height };
	}
	else // unit == VuoCoordinateUnit_VuoCoordinates
	{
		*width  = 2.;
		*height = 2. / *aspectRatio;
		*topLeft     = (VuoPoint2d){ -1., *height /  2. };
		*bottomRight = (VuoPoint2d){  1., *height / -2. };
	}

	*topLeftEvent = true;
	*bottomRightEvent = true;
	*widthEvent = true;
	*heightEvent = true;
	*aspectRatioEvent = true;

	(*context)->priorUnit = unit;
	(*context)->priorPosition = position;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
