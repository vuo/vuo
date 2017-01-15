/**
 * @file
 * vuo.mouse.click node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Clicks",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "tap", "press" ],
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
	VuoMouse *clickedListener;
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
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	VuoMouse_startListeningForClicks((*context)->clickedListener, singleClicked, doubleClicked, tripleClicked, button, window, modifierKey);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->clickedListener);
	VuoMouse_startListeningForClicks((*context)->clickedListener, singleClicked, doubleClicked, tripleClicked, button, window, modifierKey);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(singleClicked, VuoPoint2d),
		VuoOutputTrigger(doubleClicked, VuoPoint2d),
		VuoOutputTrigger(tripleClicked, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->clickedListener);
	VuoMouse_startListeningForClicks((*context)->clickedListener, singleClicked, doubleClicked, tripleClicked, button, window, modifierKey);
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
