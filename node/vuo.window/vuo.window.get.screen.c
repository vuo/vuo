/**
 * @file
 * vuo.window.get.screen node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Get Window Screen",
	"keywords": [
		"info",
		"display",
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ ],
	},
});

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
	bool first;
	bool priorFullscreen;
	VuoScreen priorScreen;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	context->first = true;
	return context;
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputEvent({"eventBlocking":"door","data":"window"}) windowEvent,

	VuoOutputData(VuoBoolean) fullscreen,
	VuoOutputEvent({"data":"fullscreen"}) fullscreenEvent,
	VuoOutputData(VuoScreen) screen,
	VuoOutputEvent({"data":"screen"}) screenEvent)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*context)->renderedLayers, window, &renderingDimensionsChanged);

	VuoWindowReference windowReference;
	if (!VuoRenderedLayers_getWindow((*context)->renderedLayers, &windowReference))
		return;

	bool isFullscreen = VuoWindowReference_isFullscreen(windowReference);
	if (isFullscreen != (*context)->priorFullscreen || (*context)->first)
	{
		*fullscreen = isFullscreen;
		*fullscreenEvent = true;
		(*context)->priorFullscreen = *fullscreen;
		(*context)->first = false;
	}

	VuoScreen s = VuoWindowReference_getScreen(windowReference);
	if (!VuoScreen_areEqual(s, (*context)->priorScreen))
	{
		*screen = s;
		*screenEvent = true;
		VuoScreen_release((*context)->priorScreen);
		VuoScreen_retain(*screen);
		(*context)->priorScreen = *screen;
	}
	else
	{
		VuoScreen_retain(s);
		VuoScreen_release(s);
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoScreen_release((*context)->priorScreen);
	VuoRenderedLayers_release((*context)->renderedLayers);
}
