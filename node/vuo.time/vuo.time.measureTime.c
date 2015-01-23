/**
 * @file
 * vuo.time.measureTime node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <dispatch/dispatch.h>

VuoModuleMetadata({
					  "title" : "Measure Time",
					  "keywords" : [ "animate", "frame", "stopwatch", "start", "pause", "elapsed" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  },
				  });

struct nodeInstanceData
{
	VuoFrameRequest lastStartOrResetTime;
	VuoFrameRequest lastPauseTime;
	VuoFrameRequest totalPauseTime;

	bool hasStarted;
	bool isRunning;
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
		VuoInputData(VuoFrameRequest,) time,
		VuoInputEvent(VuoPortEventBlocking_Door, time) timeEvent,
		VuoInputEvent(VuoPortEventBlocking_Wall,) start,
		VuoInputEvent(VuoPortEventBlocking_Wall,) pause,
		VuoInputEvent(VuoPortEventBlocking_Wall,) reset,
		VuoOutputData(VuoFrameRequest) elapsedTime,
		VuoOutputEvent(elapsedTime) elapsedTimeEvent
)
{
	if (reset || (start && ! (*context)->hasStarted))  // this event resets it
	{
		(*context)->lastStartOrResetTime = time;
		(*context)->lastPauseTime = time;
		(*context)->totalPauseTime = VuoFrameRequest_make(0, 0);
	}

	if (start)
	{
		if (! (*context)->isRunning && (*context)->hasStarted && ! reset)  // this event unpauses it
			(*context)->totalPauseTime = VuoFrameRequest_make((*context)->totalPauseTime.timestamp + time.timestamp - (*context)->lastPauseTime.timestamp,
															  (*context)->totalPauseTime.frameCount + time.frameCount - (*context)->lastPauseTime.frameCount);

		(*context)->isRunning = true;
		(*context)->hasStarted = true;
	}

	if (pause)
	{
		if ((*context)->isRunning)  // this event pauses it
			(*context)->lastPauseTime = time;

		(*context)->isRunning = false;
	}

	if ((*context)->isRunning)
	{
		*elapsedTime = VuoFrameRequest_make(time.timestamp - (*context)->lastStartOrResetTime.timestamp - (*context)->totalPauseTime.timestamp,
											time.frameCount - (*context)->lastStartOrResetTime.frameCount - (*context)->totalPauseTime.frameCount);
		*elapsedTimeEvent = true;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
