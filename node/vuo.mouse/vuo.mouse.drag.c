/**
 * @file
 * vuo.mouse.drag node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Drags",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "move" ],
					  "version" : "1.0.2",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *dragStartedListener;
	VuoMouse *dragMovedToListener;
	VuoMouse *dragEndedListener;
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
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d),
		VuoOutputTrigger(dragEnded, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	VuoMouse_startListeningForPresses((*context)->dragStartedListener, dragStarted, button, window, modifierKey);
	VuoMouse_startListeningForDrags((*context)->dragMovedToListener, dragMovedTo, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->dragEndedListener, dragEnded, button, window, modifierKey);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d),
		VuoOutputTrigger(dragEnded, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->dragStartedListener);
	VuoMouse_stopListening((*context)->dragMovedToListener);
	VuoMouse_stopListening((*context)->dragEndedListener);
	VuoMouse_startListeningForPresses((*context)->dragStartedListener, dragStarted, button, window, modifierKey);
	VuoMouse_startListeningForDrags((*context)->dragMovedToListener, dragMovedTo, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->dragEndedListener, dragEnded, button, window, modifierKey);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(dragStarted, VuoPoint2d),
		VuoOutputTrigger(dragMovedTo, VuoPoint2d, {"eventThrottling":"drop"}),
		VuoOutputTrigger(dragEnded, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->dragStartedListener);
	VuoMouse_stopListening((*context)->dragMovedToListener);
	VuoMouse_stopListening((*context)->dragEndedListener);
	VuoMouse_startListeningForPresses((*context)->dragStartedListener, dragStarted, button, window, modifierKey);
	VuoMouse_startListeningForDrags((*context)->dragMovedToListener, dragMovedTo, button, window, modifierKey);
	VuoMouse_startListeningForReleases((*context)->dragEndedListener, dragEnded, button, window, modifierKey);
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
