/**
 * @file
 * vuo.ui.drag.file node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDragEvent.h"

VuoModuleMetadata({
					  "title" : "Receive File Drags",
					  "keywords" : [
						  "mouse", "trackpad", "trackball", "touchpad", "cursor", "pointer", "window",
						  "Finder", "move", "copy", "reference", "folder", "and drop"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [ "VuoMouse" ],
					  "node": {
						  "isInterface" : true,
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
		VuoInputData(VuoWindowReference) window,
		VuoOutputTrigger(dragEntered, VuoDragEvent),
		VuoOutputTrigger(dragMovedTo, VuoDragEvent),
		VuoOutputTrigger(dragCompleted, VuoDragEvent),
		VuoOutputTrigger(dragExited, VuoDragEvent)
)
{
	(*context)->wr = window;
	(*context)->dragEntered = dragEntered;
	(*context)->dragMovedTo = dragMovedTo;
	(*context)->dragCompleted = dragCompleted;
	(*context)->dragExited = dragExited;
	VuoWindowReference_addDragCallbacks(window, dragEntered, dragMovedTo, dragCompleted, dragExited);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoWindowReference) window,
		VuoOutputTrigger(dragEntered, VuoDragEvent),
		VuoOutputTrigger(dragMovedTo, VuoDragEvent),
		VuoOutputTrigger(dragCompleted, VuoDragEvent),
		VuoOutputTrigger(dragExited, VuoDragEvent)
)
{
	if (window != (*context)->wr)
	{
		VuoWindowReference_removeDragCallbacks((*context)->wr, (*context)->dragEntered, (*context)->dragMovedTo, (*context)->dragCompleted, (*context)->dragExited);

		(*context)->wr = window;
		(*context)->dragEntered = dragEntered;
		(*context)->dragMovedTo = dragMovedTo;
		(*context)->dragCompleted = dragCompleted;
		(*context)->dragExited = dragExited;
		VuoWindowReference_addDragCallbacks(window, dragEntered, dragMovedTo, dragCompleted, dragExited);
	}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoWindowReference_removeDragCallbacks((*context)->wr, (*context)->dragEntered, (*context)->dragMovedTo, (*context)->dragCompleted, (*context)->dragExited);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
