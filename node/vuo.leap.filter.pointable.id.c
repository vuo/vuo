/**
 * @file
 * vuo.leap.filter.pointable.id node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Filter Pointables by ID",
					  "description" :
						  "<p>Finds all pointables (fingers and tools) in the list that have a certain ID.</p> \
						  <ul> \
						  <li>`pointables` — The list of pointables to search in.</li> \
						  <li>`id` — The ID to search for. Each pointable has a unique ID that can be used to track the pointable \
						  across consecutive frames in which the pointable is detected. If the pointable is lost and re-detected, \
						  its ID changes.</li> \
						  <li>`filteredPointables` — All items from `pointables` that have ID `id`.</li> \
						  </ul> \
						  ",
					  "keywords" : [ "controller", "motion", "finger", "tool" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoInteger, {"default":0}) id,
		VuoOutputData(VuoList_VuoLeapPointable) filteredPointables
)
{
	*filteredPointables = VuoListCreate_VuoLeapPointable();
	int count = VuoListGetCount_VuoLeapPointable(pointables);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapPointable pointable = VuoListGetValueAtIndex_VuoLeapPointable(pointables, i);

		if( pointable.id == id )
			VuoListAppendValue_VuoLeapPointable(*filteredPointables, pointable);
	}

}
