/**
 * @file
 * vuo.list.process node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Process List",
					 "keywords" : [ "iterate", "repeat", "multiple", "many", "foreach", "each", "loop", "while", "cycle" ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ "DisplayGridOfImages.vuo", "WobbleEggs.vuo", "AddEffectToInstantReplay.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoInteger count;
	VuoInteger total;
	VuoList_VuoGenericType1 inputList;
	VuoList_VuoGenericType2 processedList;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoList_VuoGenericType1) fire,
		VuoInputEvent({"data":"fire", "eventBlocking":"none"}) fireEvent,
		VuoInputData(VuoGenericType2) processedItem,
		VuoInputEvent({"data":"processedItem", "eventBlocking":"none"}) processedItemEvent,
		VuoOutputTrigger(processedList, VuoList_VuoGenericType2),
		VuoOutputTrigger(processItem, VuoGenericType1),
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	// Only allow starting a new list if we've received back _all_ the fired events,
	// to prevent outputting partially-complete lists
	// when `fire` receives frequent events but the feedback loop is slow.
	if (fireEvent && !(*context)->processedList)
	{
		(*context)->total = VuoListGetCount_VuoGenericType1(fire);
		if ((*context)->total < 1)
		{
			processedList(VuoListCreate_VuoGenericType2());
			return;
		}

		(*context)->inputList = fire;
		VuoRetain((*context)->inputList);

		(*context)->processedList = VuoListCreate_VuoGenericType2();
		VuoRetain((*context)->processedList);

		(*context)->count = 1;
		processItem(VuoListGetValue_VuoGenericType1(fire, (*context)->count));
	}

	if (processedItemEvent && (*context)->processedList && (*context)->count <= (*context)->total)
	{
		VuoListAppendValue_VuoGenericType2((*context)->processedList, processedItem);

		++(*context)->count;
		if ((*context)->count <=  (*context)->total)
			processItem(VuoListGetValue_VuoGenericType1((*context)->inputList, (*context)->count));
		else
		{
			processedList((*context)->processedList);
			VuoRelease((*context)->processedList);
			(*context)->processedList = NULL;

			VuoRelease((*context)->inputList);
			(*context)->inputList = NULL;

			(*context)->count = 0;
		}
	}
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->inputList)
		VuoRelease((*context)->inputList);
	if ((*context)->processedList)
		VuoRelease((*context)->processedList);
}
