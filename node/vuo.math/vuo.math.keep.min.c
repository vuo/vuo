/**
 * @file
 * vuo.math.keep.min node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Keep Minimum",
					  "keywords" : [
						  "less", "least", "small", "few", "low", "<", "bottom", "lower", "limit", "bound", "range",
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
	bool minSet;
	VuoGenericType1 min;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);
	state->minSet = false;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"none"}) reset,
		VuoOutputData(VuoGenericType1) min
)
{
	if (!(*state)->minSet || reset)
	{
		(*state)->min = value;
		(*state)->minSet = true;
	}

	if (value < (*state)->min)
		(*state)->min = value;

	*min = (*state)->min;
}


void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) state)
{
}
