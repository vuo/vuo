/**
 * @file
 * vuo.leap.filter.pointable.type node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeapPointable.h"
#include "VuoLeapPointableType.h"
#include "VuoList_VuoLeapPointable.h"

VuoModuleMetadata({
					  "title" : "Filter Pointables by Type",
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
