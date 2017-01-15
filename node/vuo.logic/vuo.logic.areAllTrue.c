/**
 * @file
 * vuo.logic.areAllTrue node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Are All True",
					 "keywords" : [ "boolean", "condition", "test", "check", "gate", "and", "&&", "0", "1", "false" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "IsMouseWithinIntersectingRectangles.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoBoolean) values,
		VuoOutputData(VuoBoolean) allTrue
)
{
	*allTrue = true;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		*allTrue = *allTrue && VuoListGetValue_VuoBoolean(values, i);
}
