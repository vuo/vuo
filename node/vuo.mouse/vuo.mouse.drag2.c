/**
 * @file
 * vuo.mouse.drag node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Drags",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "move" ],
					  "version" : "2.0.1",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *dragStartedListener;
	VuoMouse *dragMovedToListener;
	VuoMouse *dragEndedListener;

	VuoWindowReference window;
	VuoMouseButton button;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->dragStartedListener = VuoMouse_make();
	VuoRetain(context->dragStartedListener);
	context->dragMovedToListener = VuoMouse_make();
	VuoRetain(context->dragMovedToListener);
	context->dragEndedListener = VuoMouse_make();
	VuoRetain(context->dragEndedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d),
		VuoOutputTrigger(dragEnded, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->button = button;
	(*context)->modifierKey = modifierKey;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForPresses((*context)->dragStartedListener, dragStarted, NULL, button, (*context)->window, modifierKey);
	VuoMouse_startListeningForDrags((*context)->dragMovedToListener, dragMovedTo, button, (*context)->window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->dragEndedListener, dragEnded, button, (*context)->window, modifierKey, true);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoMouseButton button, VuoModifierKey modifierKey,
										   VuoOutputTrigger(dragStarted, VuoPoint2d), VuoOutputTrigger(dragMovedTo, VuoPoint2d), VuoOutputTrigger(dragEnded, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || button != (*context)->button || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->dragStartedListener);
		VuoMouse_stopListening((*context)->dragMovedToListener);
		VuoMouse_stopListening((*context)->dragEndedListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->button = button;
		(*context)->modifierKey = modifierKey;

		VuoMouse_startListeningForPresses((*context)->dragStartedListener, dragStarted, NULL, button, (*context)->window, modifierKey);
		VuoMouse_startListeningForDrags((*context)->dragMovedToListener, dragMovedTo, button, (*context)->window, modifierKey);
		VuoMouse_startListeningForReleases((*context)->dragEndedListener, dragEnded, button, (*context)->window, modifierKey, true);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d),
		VuoOutputTrigger(dragEnded, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, dragStarted, dragMovedTo, dragEnded);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d, {"name":"Started Drag"}),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d, {"name":"Dragged To", "eventThrottling":"drop"}),
		VuoOutputTrigger(dragEnded, VuoPoint2d, {"name":"Ended Drag"})
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, dragStarted, dragMovedTo, dragEnded);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->dragStartedListener);
	VuoMouse_stopListening((*context)->dragMovedToListener);
	VuoMouse_stopListening((*context)->dragEndedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->dragStartedListener);
	VuoRelease((*context)->dragMovedToListener);
	VuoRelease((*context)->dragEndedListener);
}
