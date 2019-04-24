/**
 * @file
 * vuo.mouse.move node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Moves",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer" ],
					  "version" : "1.0.5",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "vuo-example://vuo.image/ShowNoiseImage.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *movedListener;

	VuoWindowReference window;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	context->isTriggerStopped = true;
	VuoRegister(context, free);
	context->movedListener = VuoMouse_make();
	VuoRetain(context->movedListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedTo, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->window = window;
	(*context)->modifierKey = modifierKey;
	VuoMouse_startListeningForMoves((*context)->movedListener, movedTo, window, modifierKey);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedTo, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;

	if (window != (*context)->window
	 || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->movedListener);
		(*context)->window = window;
		(*context)->modifierKey = modifierKey;
		VuoMouse_startListeningForMoves((*context)->movedListener, movedTo, window, modifierKey);
	}
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(movedTo, VuoPoint2d, {"eventThrottling":"drop"})
)
{
	if ((*context)->isTriggerStopped)
		return;

	if ((*context)->window != window
	 || (*context)->modifierKey != modifierKey)
	{
		VuoMouse_stopListening((*context)->movedListener);
		(*context)->window = window;
		(*context)->modifierKey = modifierKey;
		VuoMouse_startListeningForMoves((*context)->movedListener, movedTo, window, modifierKey);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	(*context)->window = NULL;
	(*context)->modifierKey = VuoModifierKey_None;
	VuoMouse_stopListening((*context)->movedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->movedListener);
}
