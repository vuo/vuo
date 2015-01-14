/**
 * @file
 * vuo.math.count.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Count",
					 "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease" ],
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


VuoReal * nodeInstanceInit()
{
	VuoReal *countState = (VuoReal *) malloc(sizeof(VuoReal));
	VuoRegister(countState, free);
	*countState = 0;
	return countState;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoReal *) countState,
		VuoInputData(VuoReal, {"default":1.0}) increment,
		VuoInputEvent(VuoPortEventBlocking_None,increment) incrementEvent,
		VuoInputData(VuoReal, {"default":1.0}) decrement,
		VuoInputEvent(VuoPortEventBlocking_None,decrement) decrementEvent,
		VuoInputData(VuoReal, {"default":0.0}) setCount,
		VuoInputEvent(VuoPortEventBlocking_None,setCount) setCountEvent,
		VuoOutputData(VuoReal) count
)
{
	if (incrementEvent)
		**countState += increment;
	if (decrementEvent)
		**countState -= decrement;
	if (setCountEvent)
		**countState = setCount;
	*count = **countState;
}


void nodeInstanceFini
(
		VuoInstanceData(VuoReal *) countState
)
{
}
