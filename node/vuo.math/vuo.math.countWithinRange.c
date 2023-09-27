/**
 * @file
 * vuo.math.countWithinRange node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Count within Range",
					  "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease",
					  "wrap", "%", "saturate", "limit", "loop", "modulus", "clock", "clamp" ],
					  "version" : "1.0.2",
					  "node": {
						  "exampleCompositions" : [ ]
					  },
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoInteger",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
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
		VuoInputData(VuoGenericType1, {"default":1}) increment,
		VuoInputEvent({"eventBlocking":"none","data":"increment"}) incrementEvent,
		VuoInputData(VuoGenericType1, {"default":1}) decrement,
		VuoInputEvent({"eventBlocking":"none","data":"decrement"}) decrementEvent,
		VuoInputData(VuoGenericType1, {"default":0}) setCount,
		VuoInputEvent({"eventBlocking":"none","data":"setCount"}) setCountEvent,
		VuoInputData(VuoGenericType1, {"default":0}) minimum,
		VuoInputData(VuoGenericType1, {"default":10}) maximum,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoGenericType1) count
)
{
	// Adjust initial count to be within range (in case minimum/maximum have changed).
	if(*countState > maximum)
		*countState = maximum;
	if(*countState < minimum)
		*countState = minimum;

	// Increment/decrement/set.
	if(incrementEvent)
		*countState += increment;
	if(decrementEvent)
		*countState -= decrement;
	if (setCountEvent)
		*countState = setCount;

	// Adjust incremented/decremented count to be within range.
	switch(wrapMode)
	{
		default:
		case VuoWrapMode_Wrap:
			*countState = VuoGenericType1_wrap(*countState, minimum, maximum);
			break;

		case VuoWrapMode_Saturate:
			if (*countState > maximum)
				*countState = maximum;
			if (*countState < minimum)
				*countState = minimum;
			break;
	}

	*count = *countState;
}
