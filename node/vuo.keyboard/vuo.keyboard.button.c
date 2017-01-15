/**
 * @file
 * vuo.keyboard.button node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoKeyboard.h"

VuoModuleMetadata({
					  "title" : "Receive Keyboard Buttons",
					  "keywords" : [ "type" ],
					  "version" : "1.0.1",
					  "dependencies" : [ "VuoKeyboard" ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "RotateWithArrowKeys.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoKeyboard *buttonListener;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->buttonListener = VuoKeyboard_make();
	VuoRetain(context->buttonListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoKey) key,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoBoolean) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	(*context)->isTriggerStopped = false;
	VuoKeyboard_startListeningForButtons((*context)->buttonListener, pressed, released, window, key, modifierKey, repeatWhenHeld);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoKey) key,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoBoolean) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoKeyboard_stopListening((*context)->buttonListener);
	VuoKeyboard_startListeningForButtons((*context)->buttonListener, pressed, released, window, key, modifierKey, repeatWhenHeld);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoKey, {"default":"any"}) key,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoInputData(VuoBoolean, {"default":false}) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoKeyboard_stopListening((*context)->buttonListener);
	VuoKeyboard_startListeningForButtons((*context)->buttonListener, pressed, released, window, key, modifierKey, repeatWhenHeld);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoKeyboard_stopListening((*context)->buttonListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->buttonListener);
}
