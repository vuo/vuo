/**
 * @file
 * vuo.layer.drag2 node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoMouse.h"

VuoModuleMetadata({
	"title" : "Receive Mouse Drags on Layer",
	"keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer", "hit test" ],
	"version" : "2.0.0",
	"dependencies" : [ "VuoMouse" ],
	"node": {
		"exampleCompositions" : [
			"vuo-example://vuo.motion/SpringBack.vuo",
			"vuo-example://vuo.window/DragWithHandCursor.vuo"
		]
	}
});

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *dragStartedListener;
	VuoMouse *dragMovedToListener;
	VuoMouse *dragEndedListener;

	VuoRenderedLayers renderedLayers;
	uint64_t layerId;
	VuoMouseButton button;
	VuoModifierKey modifierKey;

	bool isDraggingLayer;

	VuoPoint2d startDrag;
	VuoPoint2d dragOffset;

	dispatch_queue_t layerQueue;
};

static void vuo_layer_drag2_dragStarted(VuoPoint2d startDragPoint, VuoOutputTrigger(startedDrag, VuoPoint2d), struct nodeInstanceData *context)
{
	dispatch_async(context->layerQueue, ^{
					context->isDraggingLayer = VuoRenderedLayers_isPointInLayerId(context->renderedLayers, context->layerId, startDragPoint);

	if (context->isDraggingLayer)
	{
		VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
		VuoLocal(ancestors);
		VuoSceneObject target;
		VuoSceneObject rootSceneObject = VuoRenderedLayers_getRootSceneObject(context->renderedLayers);
		if (rootSceneObject && VuoSceneObject_findById(rootSceneObject, context->layerId, ancestors, &target))
		{
			VuoPoint2d localStartDrag = VuoPoint2d_make(0,0);
			VuoSceneObject dummy = VuoSceneObject_makeEmpty();

			if (VuoRenderedLayers_getInverseTransformedPoint(context->renderedLayers, ancestors, dummy, startDragPoint, &localStartDrag))
			{
				context->startDrag = localStartDrag;
				VuoPoint2d p = VuoSceneObject_getTranslation(target).xy;;
				context->dragOffset = VuoPoint2d_subtract(p, context->startDrag);
				startedDrag(VuoPoint2d_add(context->startDrag, context->dragOffset));
			}
		}
	}
	});
}

static void vuo_layer_drag2_dragMovedTo(VuoPoint2d midDragPoint, VuoOutputTrigger(draggedCenterTo, VuoPoint2d), struct nodeInstanceData *context, bool ended)
{
	dispatch_async(context->layerQueue, ^{
	if (context->isDraggingLayer)
	{
		VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
		VuoLocal(ancestors);
		VuoSceneObject target;
		VuoSceneObject rootSceneObject = VuoRenderedLayers_getRootSceneObject(context->renderedLayers);
		if (rootSceneObject && VuoSceneObject_findById(rootSceneObject, context->layerId, ancestors, &target))
		{
			VuoPoint2d localMidDrag = VuoPoint2d_make(0,0);

			VuoSceneObject dummy = VuoSceneObject_makeEmpty();

			if (VuoRenderedLayers_getInverseTransformedPoint(context->renderedLayers, ancestors, dummy, midDragPoint, &localMidDrag))
				draggedCenterTo( VuoPoint2d_add(localMidDrag, context->dragOffset) );
		}
		if (ended)
			context->isDraggingLayer = false;
	}
	});
}

static void vuo_layer_drag2_updateLayers(struct nodeInstanceData * context,
										 VuoRenderedLayers renderedLayers,
										 uint64_t layerId)
{
	dispatch_sync(context->layerQueue, ^{
					  bool renderingDimensionsChanged;
					  VuoRenderedLayers_update(context->renderedLayers, renderedLayers, &renderingDimensionsChanged);

					  context->layerId = layerId;
				  });
}

static void vuo_layer_drag2_startTriggers(struct nodeInstanceData * context,
										  VuoMouseButton button,
										  VuoModifierKey modifierKey,
										  VuoOutputTrigger(startedDrag, VuoPoint2d),
										  VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
										  VuoOutputTrigger(endedDrag, VuoPoint2d))
{
	VuoWindowReference window;
	if (! VuoRenderedLayers_getWindow(context->renderedLayers, &window))
		return;

	context->button = button;
	context->modifierKey = modifierKey;

	VuoMouse_startListeningForPressesWithCallback(context->dragStartedListener,
												  ^(VuoPoint2d point){ vuo_layer_drag2_dragStarted(point, startedDrag, context); },
												  NULL,
												  button, window, modifierKey);
	VuoMouse_startListeningForDragsWithCallback(context->dragMovedToListener,
												^(VuoPoint2d point){ vuo_layer_drag2_dragMovedTo(point, draggedCenterTo, context, false); },
												button, window, modifierKey, true);
	VuoMouse_startListeningForReleasesWithCallback(context->dragEndedListener,
												   ^(VuoPoint2d point){ vuo_layer_drag2_dragMovedTo(point, endedDrag, context, true); },
												   button, window, modifierKey, true);
}

static void vuo_layer_drag2_stopTriggers(struct nodeInstanceData *context)
{
	VuoWindowReference window;
	if (! VuoRenderedLayers_getWindow(context->renderedLayers, &window))
		return;

	VuoMouse_stopListening(context->dragStartedListener);
	VuoMouse_stopListening(context->dragMovedToListener);
	VuoMouse_stopListening(context->dragEndedListener);
}

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

	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	context->layerId = -1;
	context->button = VuoMouseButton_Any;
	context->modifierKey = VuoModifierKey_Any;

	context->isDraggingLayer = false;
	context->startDrag = VuoPoint2d_make(0,0);
	context->layerQueue = dispatch_queue_create("vuo.layer.drag", NULL);

	return context;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoMouseButton) button,
	VuoInputData(VuoModifierKey) modifierKey,
	VuoOutputTrigger(startedDrag, VuoPoint2d),
	VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
	VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	vuo_layer_drag2_updateLayers(*context, window, VuoLayer_getId(layer));
	vuo_layer_drag2_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceTriggerUpdate
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoMouseButton) button,
	VuoInputData(VuoModifierKey) modifierKey,
	VuoOutputTrigger(startedDrag, VuoPoint2d),
	VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
	VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	vuo_layer_drag2_stopTriggers(*context);
	vuo_layer_drag2_updateLayers(*context, window, VuoLayer_getId(layer));
	vuo_layer_drag2_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,

	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputData(VuoMouseButton, {"default":"left"}) button,
	VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,

	VuoOutputTrigger(startedDrag, VuoPoint2d),
	VuoOutputTrigger(draggedCenterTo, VuoPoint2d, {"eventThrottling":"drop", "name":"Dragged Position To"}),
	VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;

	bool windowChanged = VuoRenderedLayers_windowChanged((*context)->renderedLayers, window);
	bool buttonChanged = (button != (*context)->button);
	bool modifierKeyChanged = (modifierKey != (*context)->modifierKey);

	if (buttonChanged || modifierKeyChanged || windowChanged)
		vuo_layer_drag2_stopTriggers(*context);

	vuo_layer_drag2_updateLayers(*context, window, VuoLayer_getId(layer));

	if (buttonChanged || modifierKeyChanged || windowChanged)
		vuo_layer_drag2_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceTriggerStop
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	vuo_layer_drag2_stopTriggers(*context);
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->dragStartedListener);
	VuoRelease((*context)->dragMovedToListener);
	VuoRelease((*context)->dragEndedListener);

	VuoRenderedLayers_release((*context)->renderedLayers);

	dispatch_release((*context)->layerQueue);
}
