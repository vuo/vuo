/**
 * @file
 * vuo.time.measureTime node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <dispatch/dispatch.h>

VuoModuleMetadata({
					  "title" : "Measure Time",
					  "keywords" : [ "animate", "frame", "stopwatch", "start", "pause", "elapsed", "count", "cue" ],
					  "version" : "2.0.1",
					  "node": {
						  "exampleCompositions" : [ "ShowStopwatch.vuo", "FlashOnMousePress.vuo", "RotateOnCue.vuo" ]
					  },
				  });

struct nodeInstanceData
{
	VuoReal lastStartOrResetTime;
	VuoReal lastPauseTime;
	VuoReal totalPauseTime;

	bool hasStarted;
	bool isRunning;
	bool needsTimeEventAfterReset;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}
void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) start,
		VuoInputEvent({"eventBlocking":"wall"}) pause,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoReal) elapsedTime,
		VuoOutputEvent({"data":"elapsedTime"}) elapsedTimeEvent
)
{
	if (reset || (start && ! (*context)->hasStarted))  // this event resets it
	{
		(*context)->lastStartOrResetTime = time;
		(*context)->lastPauseTime = time;
		(*context)->totalPauseTime = 0;
	}

	if (reset)
	{
		(*context)->isRunning = false;
		(*context)->needsTimeEventAfterReset = true;
	}

	if (start)
	{
		if (! (*context)->isRunning && (*context)->hasStarted && ! reset)  // this event unpauses it
			(*context)->totalPauseTime = (*context)->totalPauseTime + time - (*context)->lastPauseTime;

		(*context)->isRunning = true;
		(*context)->hasStarted = true;
	}

	if (pause)
	{
		if ((*context)->isRunning)  // this event pauses it
			(*context)->lastPauseTime = time;

		(*context)->isRunning = false;
	}

	if (timeEvent)
	{
		if ((*context)->needsTimeEventAfterReset)
		{
			*elapsedTime = 0;
			*elapsedTimeEvent = true;
			(*context)->needsTimeEventAfterReset = false;
		}

		if ((*context)->isRunning)
		{
			*elapsedTime = time - (*context)->lastStartOrResetTime - (*context)->totalPauseTime;
			*elapsedTimeEvent = true;
		}
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
