/**
 * @file
 * vuo.list.cycle node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Cycle through List",
					 "keywords" : [ "pick", "select", "choose", "count", "item", "element", "member", "index" ],
					 "version" : "1.0.2",
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
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputEvent({"eventBlocking":"wall","data":"list"}) listEvent,
		VuoInputEvent({"eventBlocking":"none"}) goForward,
		VuoInputEvent({"eventBlocking":"none"}) goBackward,
		VuoInputEvent({"name":"Go to First", "eventBlocking":"wall"}) goToFirst,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoGenericType1) item
)
{
	int len = VuoListGetCount_VuoGenericType1(list);

	if(goForward)
	{
		(**index)++;

		if(**index > len)
		{
			switch(wrapMode)
			{
				case VuoWrapMode_Saturate:
					**index = len;
					break;

				default:
					**index = 1;
					break;
			}
		}
	}

	if(goBackward)
	{
		(**index)--;

		if(**index < 1)
		{
			switch(wrapMode)
			{
				case VuoWrapMode_Saturate:
					**index = 1;
					break;

				default:
					**index = len;
					break;
			}
		}
	}

	if (goToFirst)
		**index = 0;

	*item = VuoListGetValue_VuoGenericType1(list, **index);
}

void nodeInstanceFini(VuoInstanceData(VuoInteger *) index)
{
}
