/**
 * @file
 * vuo.math.keep.average node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Keep Average",
					  "keywords" : [ "mix", "combine", "mean", "midpoint", "middle",
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
						  "exampleCompositions" : [ "AverageRandomPoints.vuo" ]
					  }
				  });


struct nodeInstanceData
{
	unsigned long valueCount;
	VuoReal average;
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);
	state->valueCount = 0;
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"none"}) reset,
		VuoOutputData(VuoGenericType1) average
)
{
	if (!(*state)->valueCount || reset)
	{
		*average = (*state)->average = value;
		(*state)->valueCount = 1;
	}
	else
	{
		++(*state)->valueCount;
		*average = (*state)->average = ((*state)->average * ((*state)->valueCount - 1.) + value) / (*state)->valueCount;
	}
}


void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) state)
{
}
