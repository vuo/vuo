/**
 * @file
 * vuo.list.filter node implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Filter List",
	"keywords": [
		"iterate", "repeat", "multiple", "many", "foreach", "each", "loop", "while", "cycle",
		"process", "build", "array", "set", "group", "items", "elements",
		"include", "some", "sift", "take", "allow", "permit", "select", "choose", "pick",
		"exclude", "deny", "remove", "erase", "eliminate", "reject", "reduce", "trim", "pare", "prune", "cut out", "leave out", "omit", "pass", "skip", "drop", "ignore", "cleanup",
		"functional programming",
	],
	"version" : "1.0.0",
	"node":{
		"exampleCompositions": [ "RemoveEvenNumbers.vuo", ],
	}
});

struct nodeInstanceData
{
	VuoInteger count;
	VuoInteger total;
	VuoList_VuoGenericType1 inputList;
	VuoList_VuoGenericType1 filteredList;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart(VuoInstanceData(struct nodeInstanceData *) context)
{
	// If an event hits the `fire` port, but nothing is connected to the `filterItem` port,
	// this node will forever wait for the lost `filterItem` event to return to the `keep` port.
	// By resetting after a livecoding reload, the next event to `fire` will fire another event out the `filterItem` port,
	// thus giving the composition author another chance to return the event.
	// https://b33p.net/kosada/node/11755
	if ((*context)->filteredList)
	{
		VuoRelease((*context)->filteredList);
		(*context)->filteredList = NULL;
	}
}

void nodeInstanceEvent(
	VuoInputData(VuoList_VuoGenericType1, {"name":"Start Filtering"}) fire,
	VuoInputEvent({"data":"fire", "eventBlocking":"none"}) fireEvent,
	VuoInputData(VuoBoolean) keepItem,
	VuoInputEvent({"data":"keepItem", "eventBlocking":"none"}) keepItemEvent,
	VuoOutputTrigger(filteredList, VuoList_VuoGenericType1, {"name":"Finished Filtering"}),
	VuoOutputTrigger(filterItem, VuoGenericType1, {"name":"Item"}),
	VuoInstanceData(struct nodeInstanceData *) context)
{
	// Only allow starting a new list if we've received back _all_ the fired events,
	// to prevent outputting partially-complete lists
	// when `fire` receives frequent events but the feedback loop is slow.
	if (fireEvent && !(*context)->filteredList)
	{
		(*context)->total = VuoListGetCount_VuoGenericType1(fire);
		if ((*context)->total < 1)
		{
			filteredList(VuoListCreate_VuoGenericType1());
			return;
		}

		(*context)->inputList = fire;
		VuoRetain((*context)->inputList);

		(*context)->filteredList = VuoListCreate_VuoGenericType1();
		VuoRetain((*context)->filteredList);

		(*context)->count = 1;
		filterItem(VuoListGetValue_VuoGenericType1(fire, (*context)->count));
	}

	if (keepItemEvent && (*context)->filteredList && (*context)->count <= (*context)->total)
	{
		if (keepItem)
			VuoListAppendValue_VuoGenericType1((*context)->filteredList, VuoListGetValue_VuoGenericType1(fire, (*context)->count));

		++(*context)->count;
		if ((*context)->count <=  (*context)->total)
			filterItem(VuoListGetValue_VuoGenericType1((*context)->inputList, (*context)->count));
		else
		{
			filteredList((*context)->filteredList);
			VuoRelease((*context)->filteredList);
			(*context)->filteredList = NULL;

			VuoRelease((*context)->inputList);
			(*context)->inputList = NULL;

			(*context)->count = 0;
		}
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	if ((*context)->inputList)
		VuoRelease((*context)->inputList);
	if ((*context)->filteredList)
		VuoRelease((*context)->filteredList);
}
