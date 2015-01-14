/**
 * @file
 * vuo.leap.filter.pointable.type node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Filter Pointables by Type",
					  "description" :
						  "<p>Finds all fingers or all tools in a list of pointables.</p> \
						  <ul> \
						  <li>`pointables` — The list of pointables to search in.</li> \
						  <li>`type` - The type of pointable to search for (finger or tool). Leap Motion considers a pointable to be a tool \
						  if it's thinner, straighter, and longer than Leap Motion's parameters for a finger.</li> \
						  <li>`filteredPointables` — All items from `pointables` that have type `type`.</li> \
						  </ul> \
						  ",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoLeapPointableType, {"default":"finger"}) type,
		VuoOutputData(VuoList_VuoLeapPointable) filteredPointables
)
{
	*filteredPointables = VuoListCreate_VuoLeapPointable();

	for(int i = 0; i < VuoListGetCount_VuoLeapPointable(pointables); i++)
	{
		VuoLeapPointable pointable = VuoListGetValueAtIndex_VuoLeapPointable(pointables, i);
		if (pointable.type == type)
			VuoListAppendValue_VuoLeapPointable(*filteredPointables, pointable);
	}
}
