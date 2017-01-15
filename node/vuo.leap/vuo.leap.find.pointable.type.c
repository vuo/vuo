/**
 * @file
 * vuo.leap.find.pointable.type node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapPointableType.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Find Pointables by Type",
					  "keywords" : [ "filter", "finger", "tool" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoLeapPointableType, {"default":"finger"}) type,
		VuoOutputData(VuoList_VuoLeapPointable) foundPointables
)
{
	*foundPointables = VuoListCreate_VuoLeapPointable();

	unsigned long count = VuoListGetCount_VuoLeapPointable(pointables);
	for(unsigned long i = 1; i <= count; i++)
	{
		VuoLeapPointable pointable = VuoListGetValue_VuoLeapPointable(pointables, i);
		if (pointable.type == type)
			VuoListAppendValue_VuoLeapPointable(*foundPointables, pointable);
	}
}
