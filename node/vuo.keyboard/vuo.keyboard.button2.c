/**
 * @file
 * vuo.keyboard.button node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoKeyboard.h"

VuoModuleMetadata({
					  "title" : "Receive Keyboard Buttons",
					  "keywords" : [ "type" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoKeyboard" ],
					  "node": {
						  "exampleCompositions" : [ "RotateWithArrowKeys.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoKeyboard *buttonListener;

	VuoWindowReference window;
	VuoKey key;
	VuoModifierKey modifierKey;
	VuoBoolean repeatWhenHeld;
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
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoKey) key,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoBoolean) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->key = key;
	(*context)->modifierKey = modifierKey;
	(*context)->repeatWhenHeld = repeatWhenHeld;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoKeyboard_startListeningForButtons((*context)->buttonListener, pressed, released, (*context)->window, key, modifierKey, repeatWhenHeld);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoKey key, VuoModifierKey modifierKey, VuoBoolean repeatWhenHeld,
										   VuoOutputTrigger(pressed, void), VuoOutputTrigger(released, void))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || key != (*context)->key || modifierKey != (*context)->modifierKey || repeatWhenHeld != (*context)->repeatWhenHeld)
	{
		VuoKeyboard_stopListening((*context)->buttonListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->key = key;
		(*context)->modifierKey = modifierKey;
		(*context)->repeatWhenHeld = repeatWhenHeld;

		VuoKeyboard_startListeningForButtons((*context)->buttonListener, pressed, released, (*context)->window, key, modifierKey, repeatWhenHeld);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoKey) key,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoBoolean) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	restartTriggersIfInputsChanged(context, window, key, modifierKey, repeatWhenHeld, pressed, released);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoKey, {"default":"any"}) key,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoInputData(VuoBoolean, {"default":false, "name":"Repeat when Held"}) repeatWhenHeld,
		VuoOutputTrigger(pressed, void),
		VuoOutputTrigger(released, void)
)
{
	restartTriggersIfInputsChanged(context, window, key, modifierKey, repeatWhenHeld, pressed, released);
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
