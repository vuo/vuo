/**
 * @file
 * vuo.mouse.button node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Buttons",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "click", "tap",
						  "force touch", "pressure",
					  ],
					  "version" : "2.1.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "ShowMouseClicks.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *pressedListener;
	VuoMouse *pressureListener;
	VuoMouse *releasedListener;

	VuoWindowReference window;
	VuoMouseButton button;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->pressedListener = VuoMouse_make();
	VuoRetain(context->pressedListener);
	context->pressureListener = VuoMouse_make();
	VuoRetain(context->pressureListener);
	context->releasedListener = VuoMouse_make();
	VuoRetain(context->releasedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(forcePressed, VuoPoint2d),
		VuoOutputTrigger(pressureChanged, VuoReal),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->button = button;
	(*context)->modifierKey = modifierKey;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForPresses((*context)->pressedListener, pressed, forcePressed, button, (*context)->window, modifierKey);
	VuoMouse_startListeningForPressureChanges((*context)->pressureListener, pressureChanged, button, modifierKey);
	VuoMouse_startListeningForReleases((*context)->releasedListener, released, button, (*context)->window, modifierKey, false);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoMouseButton button, VuoModifierKey modifierKey,
	VuoOutputTrigger(pressed, VuoPoint2d),
	VuoOutputTrigger(forcePressed, VuoPoint2d),
	VuoOutputTrigger(pressureChanged, VuoReal),
	VuoOutputTrigger(released, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || button != (*context)->button || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->pressedListener);
		VuoMouse_stopListening((*context)->pressureListener);
		VuoMouse_stopListening((*context)->releasedListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->button = button;
		(*context)->modifierKey = modifierKey;

		VuoMouse_startListeningForPresses((*context)->pressedListener, pressed, forcePressed, button, (*context)->window, modifierKey);
		VuoMouse_startListeningForPressureChanges((*context)->pressureListener, pressureChanged, button, modifierKey);
		VuoMouse_startListeningForReleases((*context)->releasedListener, released, button, (*context)->window, modifierKey, false);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(forcePressed, VuoPoint2d),
		VuoOutputTrigger(pressureChanged, VuoReal),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, pressed, forcePressed, pressureChanged, released);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(forcePressed, VuoPoint2d),
		VuoOutputTrigger(pressureChanged, VuoReal),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, button, modifierKey, pressed, forcePressed, pressureChanged, released);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->pressedListener);
	VuoMouse_stopListening((*context)->pressureListener);
	VuoMouse_stopListening((*context)->releasedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->pressedListener);
	VuoRelease((*context)->pressureListener);
	VuoRelease((*context)->releasedListener);
}
