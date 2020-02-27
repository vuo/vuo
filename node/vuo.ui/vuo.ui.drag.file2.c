/**
 * @file
 * vuo.ui.drag.file node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoDragEvent.h"

VuoModuleMetadata({
					  "title" : "Receive File Drags",
					  "keywords" : [
						  "mouse", "trackpad", "trackball", "touchpad", "cursor", "pointer", "window",
						  "Finder", "move", "copy", "reference", "folder", "and drop"
					  ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "exampleCompositions" : [ "ShowDraggedImages.vuo" ]
					  }
				  });

struct nodeInstanceData
{
	VuoWindowReference wr;
	void (*dragEntered)(VuoDragEvent e);
	void (*dragMovedTo)(VuoDragEvent e);
	void (*dragCompleted)(VuoDragEvent e);
	void (*dragExited)(VuoDragEvent e);
	bool triggersEnabled;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoOutputTrigger(dragEntered, VuoDragEvent),
		VuoOutputTrigger(dragMovedTo, VuoDragEvent),
		VuoOutputTrigger(dragCompleted, VuoDragEvent),
		VuoOutputTrigger(dragExited, VuoDragEvent)
)
{
	(*context)->triggersEnabled = true;
	(*context)->dragEntered = dragEntered;
	(*context)->dragMovedTo = dragMovedTo;
	(*context)->dragCompleted = dragCompleted;
	(*context)->dragExited = dragExited;

	if (!VuoRenderedLayers_getWindow(window, &(*context)->wr))
		return;

	VuoWindowReference_addDragCallbacks((*context)->wr, dragEntered, dragMovedTo, dragCompleted, dragExited);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) window,
		VuoOutputTrigger(dragEntered, VuoDragEvent),
		VuoOutputTrigger(dragMovedTo, VuoDragEvent),
		VuoOutputTrigger(dragCompleted, VuoDragEvent),
		VuoOutputTrigger(dragExited, VuoDragEvent)
)
{
	if (!(*context)->triggersEnabled)
		return;

	VuoWindowReference wr;
	if (! VuoRenderedLayers_getWindow(window, &wr))
		return;

	if (wr != (*context)->wr)
	{
		VuoWindowReference_removeDragCallbacks((*context)->wr, (*context)->dragEntered, (*context)->dragMovedTo, (*context)->dragCompleted, (*context)->dragExited);

		(*context)->wr = wr;
		(*context)->dragEntered = dragEntered;
		(*context)->dragMovedTo = dragMovedTo;
		(*context)->dragCompleted = dragCompleted;
		(*context)->dragExited = dragExited;

		VuoWindowReference_addDragCallbacks(wr, dragEntered, dragMovedTo, dragCompleted, dragExited);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowReference_removeDragCallbacks((*context)->wr, (*context)->dragEntered, (*context)->dragMovedTo, (*context)->dragCompleted, (*context)->dragExited);
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
