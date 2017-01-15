/**
 * @file
 * vuo.leap.find.pointable.touchZone node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapTouchZone.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Find Pointables by Touch Zone",
					  "keywords" : [ "hover", "click", "tap", "menu", "plane", "filter" ],
					  "version" : "2.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoLeapTouchZone, {"default":"none"}) touchZone,
		VuoOutputData(VuoList_VuoLeapPointable) foundPointables
)
{
	*foundPointables = VuoListCreate_VuoLeapPointable();

	unsigned long count = VuoListGetCount_VuoLeapPointable(pointables);
	for(unsigned long i = 1; i <= count; i++)
	{
		VuoLeapPointable pointable = VuoListGetValue_VuoLeapPointable(pointables, i);
		if (pointable.touchZone == touchZone)
			VuoListAppendValue_VuoLeapPointable(*foundPointables, pointable);
	}
}
