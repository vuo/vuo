/**
 * @file
 * vuo.mouse.button node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Buttons",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "click", "tap" ],
					  "version" : "1.0.3",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "ShowMouseClicks.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *pressedListener;
	VuoMouse *releasedListener;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->pressedListener = VuoMouse_make();
	VuoRetain(context->pressedListener);
	context->releasedListener = VuoMouse_make();
	VuoRetain(context->releasedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	VuoMouse_startListeningForPresses((*context)->pressedListener, pressed, NULL, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->releasedListener, released, button, window, modifierKey, false);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->pressedListener);
	VuoMouse_stopListening((*context)->releasedListener);
	VuoMouse_startListeningForPresses((*context)->pressedListener, pressed, NULL, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->releasedListener, released, button, window, modifierKey, false);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(pressed, VuoPoint2d),
		VuoOutputTrigger(released, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->pressedListener);
	VuoMouse_stopListening((*context)->releasedListener);
	VuoMouse_startListeningForPresses((*context)->pressedListener, pressed, NULL, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->releasedListener, released, button, window, modifierKey, false);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->pressedListener);
	VuoMouse_stopListening((*context)->releasedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->pressedListener);
	VuoRelease((*context)->releasedListener);
}
