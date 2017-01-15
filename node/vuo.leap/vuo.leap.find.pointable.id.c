/**
 * @file
 * vuo.leap.find.pointable.id node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Find Pointables by ID",
					  "keywords" : [ "controller", "motion", "finger", "tool", "filter" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoInteger, {"default":0, "name":"ID"}) id,
		VuoOutputData(VuoList_VuoLeapPointable) foundPointables
)
{
	*foundPointables = VuoListCreate_VuoLeapPointable();
	int count = VuoListGetCount_VuoLeapPointable(pointables);

	for(int i = 1; i < count+1; i++)
	{
		VuoLeapPointable pointable = VuoListGetValue_VuoLeapPointable(pointables, i);

		if( pointable.id == id )
			VuoListAppendValue_VuoLeapPointable(*foundPointables, pointable);
	}

}
