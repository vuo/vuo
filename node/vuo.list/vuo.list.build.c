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
	VuoInteger finalCount;
	VuoList_VuoGenericType1 list;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	// If an event hits the `fire` port, but nothing is connected to the `buildItem` port,
	// this node will forever wait for the lost `buildItem` event to return to the `builtItem` port.
	// By resetting after a livecoding reload, the next event to `fire` will fire another event out the `buildItem` port,
	// thus giving the composition author another chance to return the event.
	// https://b33p.net/kosada/node/11755
	if ((*context)->list)
	{
		VuoRelease((*context)->list);
		(*context)->list = NULL;
	}
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

		// Store the total number of iterations desired at the time iteration started,
		// instead of using whatever value happens to be in the `fire` port each time `builtItem` receives an event.
		// Fixes sporadic failure to fire `buildItem` events when the desired size changes faster than it takes to build the list.
		// https://b33p.net/kosada/node/11584
		(*context)->finalCount = fire;
	}

	if (builtItemEvent && (*context)->list && (*context)->count <= (*context)->finalCount)
	{
		VuoListAppendValue_VuoGenericType1((*context)->list, builtItem);

		++(*context)->count;
		if ((*context)->count <= (*context)->finalCount)
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
