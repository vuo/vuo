/**
 * @file
 * vuo.window.get.dimensions node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Get Window Dimensions",
					 "keywords" : [ "width", "height", "size" ],
					 "version" : "2.0.0",
					 "node" : {
						  "isDeprecated" : true,
						  "exampleCompositions" : [ ]
					 }
				 });

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
	bool isFullscreen;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *instance = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);
	instance->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(instance->renderedLayers);
	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputEvent({"eventBlocking":"door","data":"window"}) windowEvent,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoReal) height,
		VuoOutputData(VuoInteger) pixelsWide,
		VuoOutputData(VuoInteger) pixelsHigh,
		VuoOutputData(VuoReal) aspectRatio,
		VuoOutputData(VuoReal) top,
		VuoOutputData(VuoReal) right,
		VuoOutputData(VuoReal) bottom,
		VuoOutputData(VuoReal) left,
		VuoOutputData(VuoBoolean) isFullscreen,
		VuoOutputEvent({"data":"width"}) widthEvent,
		VuoOutputEvent({"data":"height"}) heightEvent,
		VuoOutputEvent({"data":"pixelsWide"}) pixelsWideEvent,
		VuoOutputEvent({"data":"pixelsHigh"}) pixelsHighEvent,
		VuoOutputEvent({"data":"aspectRatio"}) aspectRatioEvent,
		VuoOutputEvent({"data":"top"}) topEvent,
		VuoOutputEvent({"data":"right"}) rightEvent,
		VuoOutputEvent({"data":"bottom"}) bottomEvent,
		VuoOutputEvent({"data":"left"}) leftEvent,
		VuoOutputEvent({"data":"isFullscreen"}) isFullscreenEvent
)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, window, &renderingDimensionsChanged);

	unsigned long int pw;
	unsigned long int ph;
	float backingScaleFactor;
	if (! VuoRenderedLayers_getRenderingDimensions((*instance)->renderedLayers, &pw, &ph, &backingScaleFactor))
		return;

	*pixelsWide = pw;
	*pixelsHigh = ph;

	VuoWindowReference w;
	if (! VuoRenderedLayers_getWindow((*instance)->renderedLayers, &w))
		return;

	bool fullscreenChanged = false;
	*isFullscreen = VuoWindowReference_isFullscreen(w);
	if (*isFullscreen != (*instance)->isFullscreen)
	{
		fullscreenChanged = true;
		(*instance)->isFullscreen = *isFullscreen;
	}

	if (! (renderingDimensionsChanged || fullscreenChanged))
		return;

	*width = 2.;
	*left = -1.;
	*right = 1.;

	*aspectRatio = (VuoReal)*pixelsWide / *pixelsHigh;
	*height = *width / *aspectRatio;
	*top = *height / 2.;
	*bottom = -*top;

	*widthEvent = true;
	*heightEvent = true;
	*pixelsWideEvent = true;
	*pixelsHighEvent = true;
	*aspectRatioEvent = true;
	*topEvent = true;
	*rightEvent = true;
	*bottomEvent = true;
	*leftEvent = true;
	*isFullscreenEvent = true;
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) instance
)
{
	VuoRenderedLayers_release((*instance)->renderedLayers);
}
