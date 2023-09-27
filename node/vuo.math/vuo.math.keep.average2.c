/**
 * @file
 * vuo.math.keep.average2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
    "title": "Keep Average",
    "keywords": [
        "mix", "combine", "mean", "midpoint", "middle",
        "Kalman filter",
		"store", "retain", "hold", "sample", "preserve", "history", "recent",
		"rolling", "moving", "SMA",
		"FIR", "finite impulse response",  // https://en.wikipedia.org/wiki/Finite_impulse_response#Moving_average_example
		"smooth", "calm", "steady", "continuous"
    ],
    "version": "2.0.0",
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
	VuoInteger previousLookback;  ///< The most recent Lookback port data.

	VuoGenericType1 previousAverage;  ///< For averaging all values: The most recent Average port data (output).
	VuoInteger valueHistoryCount;  ///< For averaging all values: The number of data that previousAverage averages over.

	VuoList_VuoGenericType1 valueHistory;  ///< For a moving average: The N most recent Value port data (0 <= N <= Lookback), in order from oldest to newest.
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);

	state->previousLookback = 0;

	state->previousAverage = 0;
	state->valueHistoryCount = 0;

	state->valueHistory = VuoListCreate_VuoGenericType1();
	VuoRetain(state->valueHistory);

	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"data":"value", "hasPortAction":true}) valueEvent,
		VuoInputData(VuoInteger, {"default":10, "auto":-1, "autoSupersedesDefault":true, "suggestedMin":0, "suggestedMax":100}) lookback,
		VuoInputEvent({"data":"lookback", "eventBlocking":"wall"}) lookbackEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoGenericType1) average
)
{
	if (reset)
	{
		(*state)->previousLookback = 0;
		(*state)->valueHistoryCount = 0;
		VuoListRemoveAll_VuoGenericType1((*state)->valueHistory);
	}

	if ((*state)->previousLookback != lookback)
	{
		if ((*state)->previousLookback < 0 && lookback >= 0)  // average all values -> moving average
		{
			VuoRelease((*state)->valueHistory);
			(*state)->valueHistory = VuoListCreateWithCount_VuoGenericType1(MIN(lookback, (*state)->valueHistoryCount), (*state)->previousAverage);
			VuoRetain((*state)->valueHistory);

			(*state)->valueHistoryCount = 0;
		}
		else if ((*state)->previousLookback >= 0 && lookback < 0)  // moving average -> average all values
		{
			(*state)->previousAverage = VuoGenericType1_average((*state)->valueHistory);
			(*state)->valueHistoryCount = VuoListGetCount_VuoGenericType1((*state)->valueHistory);

			VuoListRemoveAll_VuoGenericType1((*state)->valueHistory);
		}
		else if ((*state)->previousLookback > lookback && lookback >= 0)  // moving average -> smaller moving average
		{
			unsigned long valueHistoryCount = VuoListGetCount_VuoGenericType1((*state)->valueHistory);
			for ( ; valueHistoryCount > lookback; --valueHistoryCount)
				VuoListRemoveFirstValue_VuoGenericType1((*state)->valueHistory);
		}

		(*state)->previousLookback = lookback;
	}

	if (valueEvent)
	{
		if (lookback < 0)  // average all values
		{
			if ((*state)->valueHistoryCount == 0)
			{
				*average = value;
				(*state)->valueHistoryCount = 1;
			}
			else
			{
				VuoInteger previousValueHistoryCount = (*state)->valueHistoryCount++;
				*average = ((*state)->previousAverage * previousValueHistoryCount + value) / (*state)->valueHistoryCount;
			}

			(*state)->previousAverage = *average;
		}
		else  // moving average
		{
			VuoListAppendValue_VuoGenericType1((*state)->valueHistory, value);
			*average = VuoGenericType1_average((*state)->valueHistory);

			if (VuoListGetCount_VuoGenericType1((*state)->valueHistory) > lookback)
				VuoListRemoveFirstValue_VuoGenericType1((*state)->valueHistory);
		}
	}
}

void nodeInstanceFini (VuoInstanceData(struct nodeInstanceData *) state)
{
	VuoRelease((*state)->valueHistory);
}
