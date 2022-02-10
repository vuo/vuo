/**
 * @file
 * vuo.list.cycle node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Cycle through List",
					 "keywords" : [ "step", "pick", "select", "choose", "count", "item", "element", "member", "index" ],
					 "version" : "2.0.1",
					 "node": {
						 "exampleCompositions": [ "CycleSeasons.vuo" ]
					 }
				 });

VuoInteger * nodeInstanceInit(void)
{
	VuoInteger *index = (VuoInteger *)malloc(sizeof(VuoInteger));
	VuoRegister(index, free);
	*index = 0;
	return index;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoInteger *) index,
		VuoInputEvent({"eventBlocking":"none"}) goForward,
		VuoInputEvent({"eventBlocking":"none"}) goBackward,
		VuoInputEvent({"name":"Go to First", "eventBlocking":"wall"}) goToFirst,
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputEvent({"eventBlocking":"wall","data":"list"}) listEvent,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoInputEvent({"eventBlocking":"wall","data":"wrapMode"}) wrapModeEvent,
		VuoOutputData(VuoGenericType1) item,
		VuoOutputData(VuoInteger) position
)
{
	// Upon return from this function, the index is either a number between 1 and the list length
	// (indicating the current position within the list) or 0 (indicating a "go to first" event,
	// which puts us just before the first item in the list).

	if (goForward || goBackward)
	{
		unsigned long len = VuoListGetCount_VuoGenericType1(list);

		// Adjust the index in case the list size has decreased since the previous call.
		if (**index > len)
			**index = len;

		if (goForward)
			(**index)++;

		if (goBackward && **index != 0)
			(**index)--;

		// Wrap after moving the index to ensure that it's still within the list.
		switch (wrapMode)
		{
			case VuoWrapMode_Wrap:
				**index = VuoInteger_wrap(**index, 1, len);
				break;
			case VuoWrapMode_Saturate:
				**index = VuoInteger_clamp(**index, 1, len);
				break;
		}
	}

	if (goToFirst)
		**index = 0;

	*item = VuoListGetValue_VuoGenericType1(list, **index);
	*position = **index;
}
