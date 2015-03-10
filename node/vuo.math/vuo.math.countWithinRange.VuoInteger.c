/**
 * @file
 * vuo.math.countWithinRange.VuoInteger node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Count within Range",
					 "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease",
						"wrap", "saturate", "limit", "loop", "modulus", "clock", "clamp" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


VuoInteger * nodeInstanceInit
(
		VuoInputData(VuoInteger) setCount
)
{
	VuoInteger *countState = (VuoInteger *) malloc(sizeof(VuoInteger));
	VuoRegister(countState, free);
	*countState = setCount;
	return countState;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoInteger *) countState,
		VuoInputData(VuoInteger, {"default":1}) increment,
		VuoInputEvent(VuoPortEventBlocking_None,increment) incrementEvent,
		VuoInputData(VuoInteger, {"default":1}) decrement,
		VuoInputEvent(VuoPortEventBlocking_None,decrement) decrementEvent,
		VuoInputData(VuoInteger, {"default":0}) setCount,
		VuoInputEvent(VuoPortEventBlocking_None,setCount) setCountEvent,
		VuoInputData(VuoInteger, {"default":0}) minimum,
		VuoInputData(VuoInteger, {"default":10}) maximum,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoInteger) count
)
{
	// Adjust initial count to be within range (in case minimum/maximum have changed).
	if(**countState > maximum)
		**countState = maximum;
	if(**countState < minimum)
		**countState = minimum;

	// Increment/decrement/set.
	if(incrementEvent)
		**countState += increment;
	if(decrementEvent)
		**countState -= decrement;
	if (setCountEvent)
		**countState = setCount;

	// Adjust incremented/decremented count to be within range.
	switch(wrapMode)
	{
		default:
		case VuoWrapMode_Wrap:
			if (**countState > maximum)
				**countState = minimum + ((**countState-maximum-1) % (maximum-minimum+1));
			else if (**countState < minimum)
				**countState = maximum + ((**countState-minimum+1) % (maximum-minimum+1));
			break;

		case VuoWrapMode_Saturate:
			if(**countState > maximum)
				**countState = maximum;
			if(**countState < minimum)
				**countState = minimum;
			break;
	}

	*count = **countState;
}

void nodeInstanceFini(VuoInstanceData(VuoInteger *) countState)
{
}
