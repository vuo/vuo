/**
 * @file
 * vuo.mouse.delta node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Deltas",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "move" ],
					  "version" : "1.0.3",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "vuo-example://vuo.image/RotateImage.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *deltaListener;
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
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	VuoMouse_startListeningForDeltas((*context)->deltaListener, movedBy, window, modifierKey);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->deltaListener);
	VuoMouse_startListeningForDeltas((*context)->deltaListener, movedBy, window, modifierKey);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(movedBy, VuoPoint2d, {"eventThrottling":"drop"})
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->deltaListener);
	VuoMouse_startListeningForDeltas((*context)->deltaListener, movedBy, window, modifierKey);
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
