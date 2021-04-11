/**
 * @file
 * vuo.math.keep.average node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
    "title": "Keep Average",
    "keywords": [
        "mix", "combine", "mean", "midpoint", "middle",
        "Kalman filter",
        "store", "retain", "hold", "sample", "preserve",
    ],
    "version": "1.1.0",
    "genericTypes": {
        "VuoGenericType1": {
            "defaultType": "VuoReal",
            "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
        },
    },
    "node": {
        "exampleCompositions": [ "AverageRandomPoints.vuo" ],
    },
});

struct nodeInstanceData
{
	unsigned long valueCount;
	VuoGenericType1 average;
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
		VuoInputEvent({"data":"value", "hasPortAction":true}) valueEvent,
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
		VuoGenericType1 countMinusOne = (*state)->valueCount - 1;
		VuoGenericType1 count = (*state)->valueCount;
		*average = (*state)->average = ((*state)->average * countMinusOne + value) / count;
	}
}
