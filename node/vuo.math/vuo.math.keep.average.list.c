/**
 * @file
 * vuo.math.keep.average.list node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Keep Average (List)",
	"keywords": [
		"mix", "combine", "mean", "midpoint", "middle",
		"Kalman filter",
		"store", "retain", "hold", "sample", "preserve", "history", "recent",
		"rolling", "moving", "SMA",
		"smooth", "calm", "steady", "continuous"
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ],
		},
	},
	"node": {
		"exampleCompositions": [ "SmoothAudioFrequencies.vuo" ],
	},
});

struct nodeInstanceData
{
	VuoInteger previousLookback;  ///< The most recent Lookback port data.

	VuoGenericType1 *previousAverages;  ///< For averaging all values: The most recent Averages port data (output). previousAverages[i] is the (i+1)th item in the Averages list.
	VuoInteger *valueHistoryCounts;  ///< For averaging all values: The number of data that each previousAverages[i] averages over.

	VuoList_VuoGenericType1 *valueHistories;  ///< The recent Values port data. valueHistories[i] is for the (i+1)th item in the Values list, and contains the N most recent data (0 <= N <= Lookback) in order from oldest to newest.

	size_t previousValuesCount;  ///< The most recent number of items in the Values list.
};

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *state = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(state, free);

	state->previousLookback = 0;

	state->previousAverages = NULL;
	state->valueHistoryCounts = NULL;

	state->valueHistories = NULL;

	state->previousValuesCount = 0;

	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) state,
		VuoInputData(VuoList_VuoGenericType1) values,
		VuoInputEvent({"data":"values", "hasPortAction":true}) valuesEvent,
		VuoInputData(VuoInteger, {"default":10, "auto":-1, "autoSupersedesDefault":true, "suggestedMin":0, "suggestedMax":100}) lookback,
		VuoInputEvent({"data":"lookback", "eventBlocking":"wall"}) lookbackEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoList_VuoGenericType1) averages
)
{
	if (reset)
	{
		(*state)->previousLookback = 0;
		for (size_t i = 0; i < (*state)->previousValuesCount; ++i)
		{
			(*state)->valueHistoryCounts[i] = 0;
			VuoListRemoveAll_VuoGenericType1((*state)->valueHistories[i]);
		}
	}

	if ((*state)->previousLookback != lookback)
	{
		if ((*state)->previousLookback < 0 && lookback >= 0)  // average all values -> moving average
		{
			for (size_t i = 0; i < (*state)->previousValuesCount; ++i)
			{
				VuoRelease((*state)->valueHistories[i]);
				(*state)->valueHistories[i] = VuoListCreateWithCount_VuoGenericType1(MIN(lookback, (*state)->valueHistoryCounts[i]), (*state)->previousAverages[i]);
				VuoRetain((*state)->valueHistories[i]);

				(*state)->valueHistoryCounts[i] = 0;
			}
		}
		else if ((*state)->previousLookback >= 0 && lookback < 0)  // moving average -> average all values
		{
			for (size_t i = 0; i < (*state)->previousValuesCount; ++i)
			{
				(*state)->previousAverages[i] = VuoGenericType1_average((*state)->valueHistories[i]);
				(*state)->valueHistoryCounts[i] = VuoListGetCount_VuoGenericType1((*state)->valueHistories[i]);

				VuoListRemoveAll_VuoGenericType1((*state)->valueHistories[i]);
			}
		}
		else if ((*state)->previousLookback > lookback && lookback >= 0)  // moving average -> smaller moving average
		{
			for (size_t i = 0; i < (*state)->previousValuesCount; ++i)
			{
				unsigned long valueHistoryCount = VuoListGetCount_VuoGenericType1((*state)->valueHistories[i]);
				for ( ; valueHistoryCount > lookback; --valueHistoryCount)
					VuoListRemoveFirstValue_VuoGenericType1((*state)->valueHistories[i]);
			}
		}

		(*state)->previousLookback = lookback;
	}

	if (valuesEvent)
	{
		unsigned long valuesCount = VuoListGetCount_VuoGenericType1(values);
		if ((*state)->previousValuesCount != valuesCount)
		{
			VuoList_VuoGenericType1 *newValueHistories = (VuoList_VuoGenericType1 *) malloc(valuesCount * sizeof(VuoList_VuoGenericType1));
			memcpy(newValueHistories, (*state)->valueHistories, MIN((*state)->previousValuesCount, valuesCount) * sizeof(VuoList_VuoGenericType1));

			VuoInteger *newValueHistoryCounts = (VuoInteger *) calloc(valuesCount, sizeof(VuoInteger));
			memcpy(newValueHistoryCounts, (*state)->valueHistoryCounts, MIN((*state)->previousValuesCount, valuesCount) * sizeof(VuoInteger));

			VuoGenericType1 *newPreviousAverages = (VuoGenericType1 *) calloc(valuesCount, sizeof(VuoGenericType1));
			memcpy(newPreviousAverages, (*state)->previousAverages, MIN((*state)->previousValuesCount, valuesCount) * sizeof(VuoGenericType1));

			for (size_t i = (*state)->previousValuesCount; i < valuesCount; ++i)
			{
				newValueHistories[i] = VuoListCreate_VuoGenericType1();
				VuoRetain(newValueHistories[i]);
			}

			for (size_t i = valuesCount; i < (*state)->previousValuesCount; ++i)
			{
				VuoRelease((*state)->valueHistories[i]);
			}

			free((*state)->valueHistories);
			(*state)->valueHistories = newValueHistories;

			free((*state)->valueHistoryCounts);
			(*state)->valueHistoryCounts = newValueHistoryCounts;

			free((*state)->previousAverages);
			(*state)->previousAverages = newPreviousAverages;

			(*state)->previousValuesCount = valuesCount;
		}

		if (lookback < 0)  // average all values
		{
			for (size_t i = 0; i < valuesCount; ++i)
			{
				VuoGenericType1 value = VuoListGetValue_VuoGenericType1(values, i+1);

				if ((*state)->valueHistoryCounts[i] == 0)
				{
					(*state)->previousAverages[i] = value;
					(*state)->valueHistoryCounts[i] = 1;
				}
				else
				{
					VuoInteger previousValueHistoryCount = (*state)->valueHistoryCounts[i]++;
					(*state)->previousAverages[i] = ((*state)->previousAverages[i] * previousValueHistoryCount + value) / (*state)->valueHistoryCounts[i];
				}
			}
		}
		else  // moving average
		{
			for (size_t i = 0; i < valuesCount; ++i)
			{
				VuoGenericType1 value = VuoListGetValue_VuoGenericType1(values, i+1);

				VuoListAppendValue_VuoGenericType1((*state)->valueHistories[i], value);
				(*state)->previousAverages[i] = VuoGenericType1_average((*state)->valueHistories[i]);

				if (VuoListGetCount_VuoGenericType1((*state)->valueHistories[i]) > lookback)
					VuoListRemoveFirstValue_VuoGenericType1((*state)->valueHistories[i]);
			}
		}

		*averages = VuoListCreateWithCount_VuoGenericType1(valuesCount, VuoGenericType1_makeFromJson(NULL));
		VuoGenericType1 *averagesData = VuoListGetData_VuoGenericType1(*averages);
		memcpy(averagesData, (*state)->previousAverages, valuesCount * sizeof(VuoGenericType1));
	}
}

void nodeInstanceFini (VuoInstanceData(struct nodeInstanceData *) state)
{
	for (size_t i = 0; i < (*state)->previousValuesCount; ++i)
		VuoRelease((*state)->valueHistories[i]);

	free((*state)->valueHistories);
	free((*state)->valueHistoryCounts);
	free((*state)->previousAverages);
}
