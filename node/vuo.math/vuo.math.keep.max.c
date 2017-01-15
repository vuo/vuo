/**
 * @file
 * vuo.math.keep.max node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Keep Maximum",
					  "keywords" : [
						  "greatest", "large", "big", "high", "more", "most", ">", "top", "upper", "peak", "limit", "bound", "range",
						  "store", "retain", "hold", "sample", "preserve"
					  ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "SpreadBars.vuo" ],
					  }
				  });


struct nodeInstanceData
{
	bool maxSet;
	VuoGenericType1 max;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);
	state->maxSet = false;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"none"}) reset,
		VuoOutputData(VuoGenericType1) max
)
{
	if (!(*state)->maxSet || reset)
	{
		(*state)->max = value;
		(*state)->maxSet = true;
	}

	if (value > (*state)->max)
		(*state)->max = value;

	*max = (*state)->max;
}


void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) state)
{
}
