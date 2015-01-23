/**
 * @file
 * vuo.math.countWithinRange.VuoReal node implementation.
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
						"wrap", "saturate", "loop", "modulus", "clock", "clamp" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


VuoReal * nodeInstanceInit
(
		VuoInputData(VuoReal) setCount
)
{
	VuoReal *countState = (VuoReal *) malloc(sizeof(VuoReal));
	VuoRegister(countState, free);
	*countState = setCount;
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
		VuoInputData(VuoReal, {"default":0.0}) minimum,
		VuoInputData(VuoReal, {"default":10.0}) maximum,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoReal) count
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

			if(**countState > maximum)
			{
				**countState = minimum + fmod(**countState-maximum, maximum-minimum);
			} else
			if(**countState < minimum)
			{
				**countState = maximum - fmod(minimum - **countState, maximum-minimum);
			}
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


void nodeInstanceFini(VuoInstanceData(VuoReal *) countState)
{
}
