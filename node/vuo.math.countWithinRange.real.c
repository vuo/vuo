/**
 * @file
 * vuo.math.countWithinRange.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Count within Range",
					 "description" :
						"<p>Keeps track of a count that can be incremented and decremented and always stays within a given range.</p> \
						<p>Before this node has received any events that change the count, the count is 0.</p> \
						<p><ul> \
						<li>`increment` — When this port receives an event, the count is incremented by this port's value.</li> \
						<li>`decrement` — When this port receives an event, the count is decremented by this port's value.</li> \
						<li>`setCount` — When this port receives an event, the count is changed to this port's value. (If the same event arrives at this port and other ports, the count is set, and the increment or decrement is ignored.  After the count is set, wrapping is applied.)</li> \
						<li>`minimum` — The minimum value that the count may have.</li> \
						<li>`maximum` — The maximum value that the count may have.</li> \
						<li>`wrapMode` — The way to adjust the count when it falls outside the range.<ul> \
						<li>\"Wrap\" makes the count wrap around using modular (clock) arithmetic.</li> \
						<li>\"Saturate\" makes the count stay at the maximum when it would go over, \
						and stay at the minimum when it would go under.</li> \
						</ul></li> \
						</ul></p>",
					 "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease",
						"wrap", "saturate", "loop", "modulus", "clock", "clamp" ],
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
		VuoInputData(VuoReal, {"default":1}) increment,
		VuoInputEvent(VuoPortEventBlocking_None,increment) incrementEvent,
		VuoInputData(VuoReal, {"default":1}) decrement,
		VuoInputEvent(VuoPortEventBlocking_None,decrement) decrementEvent,
		VuoInputData(VuoReal, {"default":0}) setCount,
		VuoInputEvent(VuoPortEventBlocking_None,setCount) setCountEvent,
		VuoInputData(VuoReal, {"default":0}) minimum,
		VuoInputData(VuoReal, {"default":10}) maximum,
		VuoInputData(VuoCountWrapMode, {"default":"wrap"}) wrapMode,
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
		case VuoCountWrapMode_Wrap:

			if(**countState > maximum)
			{
				**countState = minimum + fmod(**countState-maximum, maximum-minimum);
			} else
			if(**countState < minimum)
			{
				**countState = maximum - fmod(minimum - **countState, maximum-minimum);
			}
			break;

		case VuoCountWrapMode_Saturate:
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
