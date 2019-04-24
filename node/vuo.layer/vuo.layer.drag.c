/**
 * @file
 * vuo.layer.drag node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoRenderedLayers.h"
#include "VuoMouse.h"

VuoModuleMetadata({
					  "title" : "Receive Mouse Drags on Layer",
					  "keywords" : [ "trackpad", "trackball", "touchpad", "cursor", "pointer" ],
					  "version" : "1.0.1",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true,
					    "exampleCompositions" : [ "vuo-example://vuo.motion/SpringBack.vuo",
								      "vuo-example://vuo.window/DragWithHandCursor.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	bool isTriggerStopped;
	VuoMouse *dragStartedListener;
	VuoMouse *dragMovedToListener;
	VuoMouse *dragEndedListener;

	VuoRenderedLayers renderedLayers;
	VuoText layerName;
	VuoMouseButton button;
	VuoModifierKey modifierKey;

	bool isDraggingLayer;

	VuoPoint2d startDrag;
	VuoPoint2d dragOffset;

	dispatch_queue_t layerQueue;
};

void vuo_layer_drag_dragStarted(VuoPoint2d startDragPoint, VuoOutputTrigger(startedDrag, VuoPoint2d), struct nodeInstanceData *context)
{
	dispatch_sync(context->layerQueue, ^{
					context->isDraggingLayer = VuoRenderedLayers_isPointInLayer(context->renderedLayers, context->layerName, startDragPoint);

	if (context->isDraggingLayer)
	{
		VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
		VuoLocal(ancestors);
		VuoSceneObject target;

		if( VuoRenderedLayers_findLayer(context->renderedLayers, context->layerName, ancestors, &target) )
		{
			VuoPoint2d localStartDrag = VuoPoint2d_make(0,0);
			VuoSceneObject dummy = VuoSceneObject_makeEmpty();

			if(VuoRenderedLayers_getInverseTransformedPoint(context->renderedLayers, ancestors, dummy, startDragPoint, &localStartDrag))
			{
				context->startDrag = localStartDrag;
				VuoPoint2d p = VuoPoint2d_make(target.transform.translation.x, target.transform.translation.y);
				context->dragOffset = VuoPoint2d_subtract(p, context->startDrag);
				startedDrag(VuoPoint2d_add(context->startDrag, context->dragOffset));
			}
		}
	}
	});
}

void vuo_layer_drag_dragMovedTo(VuoPoint2d midDragPoint, VuoOutputTrigger(draggedCenterTo, VuoPoint2d), struct nodeInstanceData *context)
{
	dispatch_sync(context->layerQueue, ^{
	if (context->isDraggingLayer)
	{
		VuoList_VuoSceneObject ancestors = VuoListCreate_VuoSceneObject();
		VuoLocal(ancestors);
		VuoSceneObject target;

		if( VuoRenderedLayers_findLayer(context->renderedLayers, context->layerName, ancestors, &target) )
		{
			VuoPoint2d localMidDrag = VuoPoint2d_make(0,0);

			VuoSceneObject dummy = VuoSceneObject_makeEmpty();

			if(VuoRenderedLayers_getInverseTransformedPoint(context->renderedLayers, ancestors, dummy, midDragPoint, &localMidDrag))
			{
				draggedCenterTo( VuoPoint2d_add(localMidDrag, context->dragOffset) );
			}
		}
	}
	});
}

void vuo_layer_drag_dragEnded(VuoPoint2d endDragPoint, VuoOutputTrigger(endedDrag, VuoPoint2d), struct nodeInstanceData *context)
{
	vuo_layer_drag_dragMovedTo(endDragPoint, endedDrag, context);
	context->isDraggingLayer = false;
}


void vuo_layer_drag_updateLayers(struct nodeInstanceData * context,
								 VuoRenderedLayers renderedLayers,
								 VuoText layerName)
{
	dispatch_sync(context->layerQueue, ^{
					  VuoRenderedLayers_release(context->renderedLayers);
					  context->renderedLayers = renderedLayers;
					  VuoRenderedLayers_retain(renderedLayers);

					  VuoRelease(context->layerName);
					  context->layerName = layerName;
					  VuoRetain(layerName);
				  });
}

void vuo_layer_drag_startTriggers(struct nodeInstanceData * context,
								  VuoMouseButton button,
								  VuoModifierKey modifierKey,
								  VuoOutputTrigger(startedDrag, VuoPoint2d),
								  VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
								  VuoOutputTrigger(endedDrag, VuoPoint2d))
{
	if (! context->renderedLayers.window)
		return;

	context->button = button;
	context->modifierKey = modifierKey;

	VuoMouse_startListeningForPressesWithCallback(context->dragStartedListener,
												  ^(VuoPoint2d point){ vuo_layer_drag_dragStarted(point, startedDrag, context); },
												  button, context->renderedLayers.window, modifierKey);
	VuoMouse_startListeningForDragsWithCallback(context->dragMovedToListener,
												^(VuoPoint2d point){ vuo_layer_drag_dragMovedTo(point, draggedCenterTo, context); },
												button, context->renderedLayers.window, modifierKey, true);
	VuoMouse_startListeningForReleasesWithCallback(context->dragEndedListener,
												   ^(VuoPoint2d point){ vuo_layer_drag_dragEnded(point, endedDrag, context); },
												   button, context->renderedLayers.window, modifierKey, true);
}

void vuo_layer_drag_stopTriggers(struct nodeInstanceData *context)
{
	if (! context->renderedLayers.window)
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
	context->layerName = NULL;
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
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) layerName,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(startedDrag, VuoPoint2d),
		VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
		VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	(*context)->isTriggerStopped = false;
	vuo_layer_drag_updateLayers(*context, renderedLayers, layerName);
	vuo_layer_drag_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) layerName,
		VuoInputData(VuoMouseButton) button,
		VuoInputData(VuoModifierKey) modifierKey,
		VuoOutputTrigger(startedDrag, VuoPoint2d),
		VuoOutputTrigger(draggedCenterTo, VuoPoint2d),
		VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;
	vuo_layer_drag_stopTriggers(*context);
	vuo_layer_drag_updateLayers(*context, renderedLayers, layerName);
	vuo_layer_drag_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) layerName,
		VuoInputData(VuoMouseButton, {"default":"left"}) button,
		VuoInputData(VuoModifierKey, {"default":"any"}) modifierKey,
		VuoOutputTrigger(startedDrag, VuoPoint2d),
		VuoOutputTrigger(draggedCenterTo, VuoPoint2d, {"eventThrottling":"drop", "name":"Dragged Position To"}),
		VuoOutputTrigger(endedDrag, VuoPoint2d)
)
{
	if ((*context)->isTriggerStopped)
		return;

	bool windowChanged = (renderedLayers.window != (*context)->renderedLayers.window);
	bool buttonChanged = (button != (*context)->button);
	bool modifierKeyChanged = (modifierKey != (*context)->modifierKey);

	if (buttonChanged || modifierKeyChanged || windowChanged)
		vuo_layer_drag_stopTriggers(*context);

	vuo_layer_drag_updateLayers(*context, renderedLayers, layerName);

	if (buttonChanged || modifierKeyChanged || windowChanged)
		vuo_layer_drag_startTriggers(*context, button, modifierKey, startedDrag, draggedCenterTo, endedDrag);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	(*context)->isTriggerStopped = true;
	vuo_layer_drag_stopTriggers(*context);
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
	VuoRelease((*context)->layerName);

	dispatch_release((*context)->layerQueue);
}
