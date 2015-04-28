/**
 * @file
 * vuo.mouse.scroll node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Scrolls",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "wheel" ],
					  "version" : "1.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *scrolledListener;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	context->isTriggerStopped = true;
	VuoRegister(context, free);
	context->scrolledListener = VuoMouse_make();
	VuoRetain(context->scrolledListener);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	VuoMouse_startListeningForScrolls((*context)->scrolledListener, scrolled, modifierKey);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->scrolledListener);
	VuoMouse_startListeningForScrolls((*context)->scrolledListener, scrolled, modifierKey);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(scrolled, VuoPoint2d, VuoPortEventThrottling_Drop)
)
{
	if ((*context)->isTriggerStopped)
		return;
	VuoMouse_stopListening((*context)->scrolledListener);
	VuoMouse_startListeningForScrolls((*context)->scrolledListener, scrolled, modifierKey);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	VuoMouse_stopListening((*context)->scrolledListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->scrolledListener);
}
