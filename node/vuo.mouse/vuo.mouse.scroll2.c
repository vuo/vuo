/**
 * @file
 * vuo.mouse.scroll node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Scrolls",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "wheel" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.image/ShowNoiseImage.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *scrolledListener;

	VuoWindowReference window;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	context->isTriggerStopped = true;
	VuoRegister(context, free);
	context->scrolledListener = VuoMouse_make();
	VuoRetain(context->scrolledListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->modifierKey = modifierKey;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForScrolls((*context)->scrolledListener, scrolled, (*context)->window, modifierKey);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoModifierKey modifierKey,
										   VuoOutputTrigger(scrolled, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->scrolledListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->modifierKey = modifierKey;

		VuoMouse_startListeningForScrolls((*context)->scrolledListener, scrolled, (*context)->window, modifierKey);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, scrolled);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d, {"eventThrottling":"drop"})
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, scrolled);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->scrolledListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->scrolledListener);
}
