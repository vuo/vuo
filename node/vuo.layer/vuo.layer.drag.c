/**
 * @file
 * vuo.layer.drag node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
						  "exampleCompositions" : [ ]
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
	VuoPoint2d startCenter;
	VuoPoint2d startCorners[4];
	VuoPoint2d startUntransformedCenter;
	VuoPoint2d startUntransformedCorners[4];

	dispatch_queue_t layerQueue;
};

/**
 * Returns the intersection of the line through p1 and p2 and the line through p3 and p4.
 */
VuoPoint2d vuo_layer_drag_findIntersection(VuoPoint2d p1, VuoPoint2d p2, VuoPoint2d p3, VuoPoint2d p4)
{
	// http://paulbourke.net/geometry/pointlineplane/
	float ua = ((p4.x - p3.x)*(p1.y - p3.y) - (p4.y - p3.y)*(p1.x - p3.x)) /
			   ((p4.y - p3.y)*(p2.x - p1.x) - (p4.x - p3.x)*(p2.y - p1.y));
	float x = p1.x + ua*(p2.x - p1.x);
	float y = p1.y + ua*(p2.y - p1.y);

	return VuoPoint2d_make(x, y);
}

/**
 * Converts a drag in the transformed (rendered layers') coordinate space to a drag in the layer's coordinate space.
 * Returns the new center point that the layer would have after the drag.
 */
VuoPoint2d vuo_layer_drag_getUntransformedDraggedCenter(VuoPoint2d transformedCenter, VuoPoint2d transformedCorners[4],
														VuoPoint2d center, VuoPoint2d corners[4],
														VuoPoint2d startDrag, VuoPoint2d midDrag)
{
	if (startDrag.x == midDrag.x && startDrag.y == midDrag.y)
		return center;

	VuoPoint2d dragDelta = VuoPoint2d_subtract(midDrag, startDrag);
	VuoPoint2d transformedDraggedCenter = VuoPoint2d_add(transformedCenter, dragDelta);

	VuoPoint2d transformedEdges[4][2] = { transformedCorners[0], transformedCorners[1],
										  transformedCorners[1], transformedCorners[3],
										  transformedCorners[3], transformedCorners[2],
										  transformedCorners[2], transformedCorners[0] };
	VuoPoint2d edges[4][2] = { corners[0], corners[1],
							   corners[1], corners[3],
							   corners[3], corners[2],
							   corners[2], corners[0] };

	// Find the 2 points along the edges of the transformed layer that are intersected by the line
	// that passes through the transformed layer's center and the transformed dragged layer's center.
	VuoPoint2d transformedIntersections[2] = { VuoPoint2d_make(0,0), VuoPoint2d_make(0,0) };
	int edgeIndicesWithIntersections[2] = { 0, 0 };
	VuoPoint2d intersectionForTransformedEdge0 = vuo_layer_drag_findIntersection(transformedCenter, transformedDraggedCenter,
																				 transformedEdges[0][0], transformedEdges[0][1]);
	VuoPoint2d intersectionForTransformedEdge1 = vuo_layer_drag_findIntersection(transformedCenter, transformedDraggedCenter,
																				 transformedEdges[1][0], transformedEdges[1][1]);
	float transformedEdgeLengthViaIntersection0[] = {
		VuoPoint2d_distance(transformedEdges[0][0], intersectionForTransformedEdge0),
		VuoPoint2d_distance(transformedEdges[0][1], intersectionForTransformedEdge0) };
	float transformedEdgeLength0 = VuoPoint2d_distance(transformedEdges[0][0], transformedEdges[0][1]);
	float scaledDistanceThroughIntersection0 =
			(transformedEdgeLengthViaIntersection0[0] + transformedEdgeLengthViaIntersection0[1]) / transformedEdgeLength0;
	float transformedEdgeLengthViaIntersection1[] = {
		VuoPoint2d_distance(transformedEdges[1][0], intersectionForTransformedEdge1),
		VuoPoint2d_distance(transformedEdges[1][1], intersectionForTransformedEdge1) };
	float transformedEdgeLength1 = VuoPoint2d_distance(transformedEdges[1][0], transformedEdges[1][1]);
	float scaledDistanceThroughIntersection1 =
			(transformedEdgeLengthViaIntersection1[0] + transformedEdgeLengthViaIntersection1[1]) / transformedEdgeLength1;
	if (! isfinite(intersectionForTransformedEdge1.x) ||
			scaledDistanceThroughIntersection0 < scaledDistanceThroughIntersection1)
	{
		// intersectionForTransformedEdge0 is between the corner points of transformedEdges[0]
		transformedIntersections[0] = intersectionForTransformedEdge0;
		edgeIndicesWithIntersections[0] = 0;
		edgeIndicesWithIntersections[1] = 2;
	}
	else
	{
		// intersectionForTransformedEdge1 is between the corner points of transformedEdges[1]
		transformedIntersections[0] = intersectionForTransformedEdge1;
		edgeIndicesWithIntersections[0] = 1;
		edgeIndicesWithIntersections[1] = 3;
	}
	int secondIntersectionIndex = edgeIndicesWithIntersections[1];
	transformedIntersections[1] = vuo_layer_drag_findIntersection(transformedCenter, transformedDraggedCenter,
																  transformedEdges[secondIntersectionIndex][0],
																  transformedEdges[secondIntersectionIndex][1]);

	// Find the corresponding 2 points along the edges of the un-transformed layer.
	VuoPoint2d intersections[2] = { VuoPoint2d_make(0,0), VuoPoint2d_make(0,0) };
	for (int i = 0; i < 2; ++i)
	{
		int edgeIndex = edgeIndicesWithIntersections[i];
		float intersectionProportion =
				VuoPoint2d_distance(transformedEdges[edgeIndex][0], transformedIntersections[i]) /
				VuoPoint2d_distance(transformedEdges[edgeIndex][0], transformedEdges[edgeIndex][1]);
		VuoPoint2d edgeDelta = VuoPoint2d_subtract(edges[edgeIndex][1], edges[edgeIndex][0]);
		intersections[i] = VuoPoint2d_make(edges[edgeIndex][0].x + intersectionProportion * edgeDelta.x,
										   edges[edgeIndex][0].y + intersectionProportion * edgeDelta.y);
	}

	// Find the center point of the un-transformed dragged layer.
	// It falls along the line that passes through the 2 un-transformed intersection points.
	// The un-transformed dragged center's proportional distance between the 2 un-transformed intersection points is the same as
	// the transformed dragged center's proportional distance between the 2 transformed intersection points.
	VuoPoint2d transformedIntersectionDelta = VuoPoint2d_subtract(transformedIntersections[1], transformedIntersections[0]);
	VuoPoint2d transformedDraggedCenterDelta = VuoPoint2d_subtract(transformedDraggedCenter, transformedIntersections[0]);
	VuoPoint2d intersectionDelta = VuoPoint2d_subtract(intersections[1], intersections[0]);
	float draggedCenterProportion = VuoPoint2d_magnitude(transformedDraggedCenterDelta) /
									VuoPoint2d_magnitude(transformedIntersectionDelta);
	bool isDraggedCenterOnSameSideAsIntersection1 =
			(VuoPoint2d_dotProduct(transformedIntersectionDelta, transformedDraggedCenterDelta) > 0);
	int direction = isDraggedCenterOnSameSideAsIntersection1 ? 1 : -1;
	VuoPoint2d draggedCenter = VuoPoint2d_make(intersections[0].x + direction * draggedCenterProportion * intersectionDelta.x,
											   intersections[0].y + direction * draggedCenterProportion * intersectionDelta.y);
	return draggedCenter;
}

void vuo_layer_drag_dragStarted(VuoPoint2d startDragPoint, VuoOutputTrigger(startedDrag, VuoPoint2d), struct nodeInstanceData *context)
{
	dispatch_sync(context->layerQueue, ^{
					  VuoSceneObject layer;
					  VuoList_VuoSceneObject ancestorObjects = VuoListCreate_VuoSceneObject();
					  bool isLayerFound = VuoRenderedLayers_findLayer(context->renderedLayers, context->layerName, ancestorObjects, &layer);
					  if (isLayerFound)
					  {
						  VuoList_VuoSceneObject noObjects = VuoListCreate_VuoSceneObject();
						  VuoRenderedLayers_getTransformedLayer(context->renderedLayers, noObjects, layer, &context->startUntransformedCenter, context->startUntransformedCorners);
						  VuoRenderedLayers_getTransformedLayer(context->renderedLayers, ancestorObjects, layer, &context->startCenter, context->startCorners);
						  context->isDraggingLayer = VuoRenderedLayers_isPointInQuad(context->startCorners, startDragPoint);
						  VuoRetain(noObjects);
						  VuoRelease(noObjects);
					  }
					  VuoRetain(ancestorObjects);
					  VuoRelease(ancestorObjects);
				  });
	if (context->isDraggingLayer)
	{
		context->startDrag = startDragPoint;
		startedDrag(context->startUntransformedCenter);
	}
}

void vuo_layer_drag_dragMovedTo(VuoPoint2d midDragPoint, VuoOutputTrigger(draggedCenterTo, VuoPoint2d), struct nodeInstanceData *context)
{
	if (context->isDraggingLayer)
	{
		VuoPoint2d center = vuo_layer_drag_getUntransformedDraggedCenter(context->startCenter, context->startCorners,
																		 context->startUntransformedCenter, context->startUntransformedCorners,
																		 context->startDrag, midDragPoint);
		draggedCenterTo(center);
	}
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
	context->startCenter = VuoPoint2d_make(0,0);
	context->startUntransformedCenter = VuoPoint2d_make(0,0);
	for (int i = 0; i < 4; ++i)
	{
		context->startCorners[i] = VuoPoint2d_make(0,0);
		context->startUntransformedCorners[i] = VuoPoint2d_make(0,0);
	}

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
		VuoOutputTrigger(draggedCenterTo, VuoPoint2d, {"eventThrottling":"drop"}),
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
