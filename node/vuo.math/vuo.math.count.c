﻿/**
 * @file
 * vuo.math.count node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Count",
					  "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease", "number" ],
					  "version" : "1.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


VuoGenericType1 nodeInstanceInit
(
		VuoInputData(VuoGenericType1) setCount
)
{
	return setCount;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoGenericType1) countState,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoReal":1.0}}) increment,
		VuoInputEvent({"eventBlocking":"none","data":"increment"}) incrementEvent,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":1, "VuoReal":1.0}}) decrement,
		VuoInputEvent({"eventBlocking":"none","data":"decrement"}) decrementEvent,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) setCount,
		VuoInputEvent({"eventBlocking":"none","data":"setCount"}) setCountEvent,
		VuoOutputData(VuoGenericType1) count
)
{
	if (incrementEvent)
		*countState += increment;
	if (decrementEvent)
		*countState -= decrement;
	if (setCountEvent)
		*countState = setCount;
	*count = *countState;
}
