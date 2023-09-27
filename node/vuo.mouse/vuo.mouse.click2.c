/**
 * @file
 * vuo.mouse.click node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMouse.h"

VuoModuleMetadata({
	"title": "Receive Mouse Clicks",
	"keywords": [
		"trackpad", "trackball", "touchpad", "cursor", "pointer", "tap", "press",
		"control", "GUI", "UI", "user interface", "interact",
	],
	"version": "2.0.0",
	"dependencies": [ "VuoMouse" ],
	"node": {
		"exampleCompositions": [ "ShowMouseClicks.vuo" ]
	}
});

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *clickedListener;

	VuoWindowReference window;
	VuoMouseButton button;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->clickedListener = VuoMouse_make();
	VuoRetain(context->clickedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->button = button;
	(*context)->modifierKey = modifierKey;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForClicks((*context)->clickedListener, singleClicked, doubleClicked, tripleClicked, button, (*context)->window, modifierKey);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoMouseButton button, VuoModifierKey modifierKey,
										   VuoOutputTrigger(singleClicked, VuoPoint2d), VuoOutputTrigger(doubleClicked, VuoPoint2d), VuoOutputTrigger(tripleClicked, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || button != (*context)->button || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->clickedListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->button = button;
		(*context)->modifierKey = modifierKey;

		VuoMouse_startListeningForClicks((*context)->clickedListener, singleClicked, doubleClicked, tripleClicked, button, (*context)->window, modifierKey);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, singleClicked, doubleClicked, tripleClicked);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, singleClicked, doubleClicked, tripleClicked);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->clickedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->clickedListener);
}
