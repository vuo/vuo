/**
 * @file
 * vuo.mouse.move node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Moves",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.image/ShowNoiseImage.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *movedListener;

	VuoWindowReference window;
	VuoModifierKey modifierKey;
	VuoInteger appFocus;
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
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoInteger) appFocus,
		VuoOutputTrigger(movedTo, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	(*context)->modifierKey = modifierKey;
	(*context)->appFocus = appFocus;

	VuoWindowReference w = NULL;
	if (VuoRenderedLayers_getWindow(window, &w))
		(*context)->window = w;

	VuoMouse_startListeningForMoves((*context)->movedListener, movedTo, (*context)->window, modifierKey, appFocus);
}

static void restartTriggersIfInputsChanged(struct nodeInstanceData **context, VuoRenderedLayers window, VuoModifierKey modifierKey, VuoInteger appFocus,
										   VuoOutputTrigger(movedTo, VuoPoint2d))
{
	if ((*context)->isTriggerStopped)
		return;

	VuoWindowReference w = NULL;
	bool hasWindow = VuoRenderedLayers_getWindow(window, &w);
	if ((hasWindow && w != (*context)->window)
	 || modifierKey != (*context)->modifierKey
	 || appFocus != (*context)->appFocus)
	{
		VuoMouse_stopListening((*context)->movedListener);

		if (hasWindow)
			(*context)->window = w;
		(*context)->modifierKey = modifierKey;
		(*context)->appFocus = appFocus;

		VuoMouse_startListeningForMoves((*context)->movedListener, movedTo, (*context)->window, modifierKey, appFocus);
	}
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoInputData(VuoInteger) appFocus,
		VuoOutputTrigger(movedTo, VuoPoint2d)
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, appFocus, movedTo);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoInputData(VuoInteger, {"menuItems":[
									  {"value":0, "name":"Foreground"},
									  {"value":1, "name":"Any"},
								  ], "default":0}) appFocus,
		VuoOutputTrigger(movedTo, VuoPoint2d, {"eventThrottling":"drop"})
)
{
	restartTriggersIfInputsChanged(context, window, modifierKey, appFocus, movedTo);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	(*context)->window = NULL;
	(*context)->modifierKey = VuoModifierKey_None;
	(*context)->appFocus = 0;
	VuoMouse_stopListening((*context)->movedListener);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->movedListener);
}
