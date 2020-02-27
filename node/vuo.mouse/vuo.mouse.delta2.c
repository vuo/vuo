/**
 * @file
 * vuo.mouse.delta node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Deltas",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "move" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.image/RotateMovie.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *deltaListener;

	VuoWindowReference window;
	VuoModifierKey modifierKey;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->isTriggerStopped = true;
	context->deltaListener = VuoMouse_make();
	VuoRetain(context->deltaListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->modifierKey = modifierKey;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForDeltas((*context)->deltaListener, movedBy, (*context)->window, modifierKey);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoModifierKey modifierKey,
										   VuoOutputTrigger(movedBy, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window) || modifierKey != (*context)->modifierKey)
	{
		VuoMouse_stopListening((*context)->deltaListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->modifierKey = modifierKey;

		VuoMouse_startListeningForDeltas((*context)->deltaListener, movedBy, (*context)->window, modifierKey);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, movedBy);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d, {"eventThrottling":"drop"})
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, movedBy);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->deltaListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->deltaListener);
}
