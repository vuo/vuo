/**
 * @file
 * vuo.list.build node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Build List",
					 "keywords" : [ "iterate", "repeat", "multiple", "many", "foreach", "each", "loop", "while", "cycle" ],
					 "version" : "1.0.1",
					 "node": {
						  "exampleCompositions" : [ "DisplayRainbowOvals.vuo", "DisplayMovieFrames.vuo" ],
					 }
				 });

struct nodeInstanceData
{
	VuoInteger count;
	VuoList_VuoGenericType1 list;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceEvent
(
		VuoInputData(VuoInteger, {"default":10, "suggestedMin":1}) fire,
		VuoInputEvent({"data":"fire", "eventBlocking":"none"}) fireEvent,
		VuoInputData(VuoGenericType1) builtItem,
		VuoInputEvent({"data":"builtItem", "eventBlocking":"none"}) builtItemEvent,
		VuoOutputTrigger(builtList, VuoList_VuoGenericType1),
		VuoOutputTrigger(buildItem, VuoInteger),
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	// Only allow creating a new list if we've received back _all_ the fired events,
	// to prevent outputting partially-complete lists
	// when `fire` receives frequent events but the feedback loop is slow.
	if (fireEvent && !(*context)->list)
	{
		if (fire < 1)
		{
			builtList(VuoListCreate_VuoGenericType1());
			return;
		}

		(*context)->list = VuoListCreate_VuoGenericType1();
		VuoRetain((*context)->list);

		(*context)->count = 1;
		buildItem((*context)->count);
	}

	if (builtItemEvent && (*context)->list && (*context)->count <= fire)
	{
		VuoListAppendValue_VuoGenericType1((*context)->list, builtItem);

		++(*context)->count;
		if ((*context)->count <= fire)
			buildItem((*context)->count);
		else
		{
			builtList((*context)->list);
			VuoRelease((*context)->list);
			(*context)->count = 0;
			(*context)->list = NULL;
		}
	}
}

void nodeInstanceFini(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if ((*context)->list)
		VuoRelease((*context)->list);
}
