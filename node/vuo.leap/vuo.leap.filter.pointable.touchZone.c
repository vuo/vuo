/**
 * @file
 * vuo.leap.filter.pointable.type node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapTouchZone.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Filter Pointables by Touch Zone",
					  "keywords" : [ "hover", "click", "tap", "menu", "plane" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoLeapTouchZone, {"default":"none"}) touchZone,
		VuoOutputData(VuoList_VuoLeapPointable) filteredPointables
)
{
	*filteredPointables = VuoListCreate_VuoLeapPointable();

	unsigned long count = VuoListGetCount_VuoLeapPointable(pointables);
	for(unsigned long i = 1; i <= count; i++)
	{
		VuoLeapPointable pointable = VuoListGetValueAtIndex_VuoLeapPointable(pointables, i);
		if (pointable.touchZone == touchZone)
			VuoListAppendValue_VuoLeapPointable(*filteredPointables, pointable);
	}
}
