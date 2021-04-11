/**
 * @file
 * vuo.mouse.touch node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
	"title" : "Receive Touches",
	"keywords" : [
		"trackpad", "touchpad", "finger", "pointer", "cursor",
		"gesture",
		// "pinch",
		"drag", "flick", "next", "previous", "forward", "backward",
	],
	"version" : "1.0.0",
	"dependencies" : [ "VuoMouse" ],
	"node": {
		"exampleCompositions" : [ ]
	}
});

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *touchListener;
	VuoWindowReference window;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->touchListener = VuoMouse_make();
	VuoRetain(context->touchListener);
	return context;
}

void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoOutputTrigger(touchesMoved, VuoList_VuoPoint2d),
	VuoOutputTrigger(zoomed, VuoReal))
{
	(*context)->isTriggerStopped = false;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForTouches((*context)->touchListener, touchesMoved, zoomed, NULL, NULL, (*context)->window);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window,
	VuoOutputTrigger(touchesMoved, VuoList_VuoPoint2d),
	VuoOutputTrigger(zoomed, VuoReal))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if (hasWindow && w != (*context)->window)
	{
		VuoMouse_stopListening((*context)->touchListener);

		if (hasWindow)
			(*context)->window = w;

		VuoMouse_startListeningForTouches((*context)->touchListener, touchesMoved, zoomed, NULL, NULL, (*context)->window);
	}
}

void nodeInstanceTriggerUpdate(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoOutputTrigger(touchesMoved, VuoList_VuoPoint2d),
	VuoOutputTrigger(zoomed, VuoReal))
{
	restartTriggersIfInputsChanged(context, window, touchesMoved, zoomed);
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoOutputTrigger(touchesMoved, VuoList_VuoPoint2d),
	VuoOutputTrigger(zoomed, VuoReal))
{
	restartTriggersIfInputsChanged(context, window, touchesMoved, zoomed);
}

void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData *) context)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->touchListener);
}

void nodeInstanceFini(
	VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoRelease((*context)->touchListener);
}
