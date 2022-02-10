/**
 * @file
 * vuo.time.schedule node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoDurationType.h"

VuoModuleMetadata({
					  "title" : "Schedule",
					  "keywords" : [ "animate", "frame", "stopwatch", "elapsed", "count", "cue", "alarm", "clock", "sequence", "order" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : ["AnimateOnSchedule.vuo", "FlashOnMousePress.vuo"]
					  },
				  });

const int maxScheduleCount = 8;

struct nodeInstanceData
{
	VuoReal previousLoopedTime;
	bool hasStarted[maxScheduleCount+1]; // 1-indexed
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	for (int i=1; i<=maxScheduleCount; ++i)
		context->hasStarted[i] = false;

	return context;
}

/**
 * Applies looping to `time`.
 */
static VuoReal vuo_time_schedule_loop(VuoLoopType loop, VuoReal time, VuoReal maxTime)
{
	VuoReal loopedTime = time;
	if (loop == VuoLoopType_Loop)
		loopedTime = VuoReal_wrap(time, 0, maxTime);
	else if (loop == VuoLoopType_Mirror)
	{
		loopedTime = VuoReal_wrap(time, 0, 2 * maxTime);
		if (loopedTime > maxTime)
			loopedTime = 2 * maxTime - loopedTime;
	}
	return loopedTime;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputData(VuoList_VuoReal, {"default":[1,2]}) schedule,
		VuoInputEvent({"eventBlocking":"wall","data":"schedule"}) scheduleEvent,
		VuoInputData(VuoDurationType, {"default":"until reset"}) durationType,
		VuoInputEvent({"eventBlocking":"wall","data":"durationType"}) durationTypeEvent,
		VuoInputData(VuoLoopType, {"default":"none", "includeValues":["none","loop"]}) loop,
		VuoInputEvent({"eventBlocking":"wall","data":"loop"}) loopEvent,
		VuoOutputData(VuoReal) elapsedTime1,
		VuoOutputEvent({"data":"elapsedTime1"}) elapsedTimeEvent1,
		VuoOutputData(VuoReal) elapsedTime2,
		VuoOutputEvent({"data":"elapsedTime2"}) elapsedTimeEvent2,
		VuoOutputData(VuoReal) elapsedTime3,
		VuoOutputEvent({"data":"elapsedTime3"}) elapsedTimeEvent3,
		VuoOutputData(VuoReal) elapsedTime4,
		VuoOutputEvent({"data":"elapsedTime4"}) elapsedTimeEvent4,
		VuoOutputData(VuoReal) elapsedTime5,
		VuoOutputEvent({"data":"elapsedTime5"}) elapsedTimeEvent5,
		VuoOutputData(VuoReal) elapsedTime6,
		VuoOutputEvent({"data":"elapsedTime6"}) elapsedTimeEvent6,
		VuoOutputData(VuoReal) elapsedTime7,
		VuoOutputEvent({"data":"elapsedTime7"}) elapsedTimeEvent7,
		VuoOutputData(VuoReal) elapsedTime8,
		VuoOutputEvent({"data":"elapsedTime8"}) elapsedTimeEvent8
		)
{
	if (timeEvent)
	{
		const unsigned long scheduleCount = VuoListGetCount_VuoReal(schedule);
		const VuoReal maxTime = VuoListGetValue_VuoReal(schedule, scheduleCount);

		VuoReal loopedTime = vuo_time_schedule_loop(loop, time, maxTime);
		bool justLooped = loopedTime < (*context)->previousLoopedTime;

		VuoReal *elapsedTimes[] = { NULL, elapsedTime1, elapsedTime2, elapsedTime3, elapsedTime4, elapsedTime5, elapsedTime6, elapsedTime7, elapsedTime8 };
		bool *elapsedTimeEvents[] = { NULL, elapsedTimeEvent1, elapsedTimeEvent2, elapsedTimeEvent3, elapsedTimeEvent4, elapsedTimeEvent5, elapsedTimeEvent6, elapsedTimeEvent7, elapsedTimeEvent8 };

		for (int i = 1; i <= maxScheduleCount; ++i)
		{
			if (durationType == VuoDurationType_Single && justLooped)
				(*context)->hasStarted[i] = false;

			bool previouslyPastStartTime = (*context)->hasStarted[i];
			bool currentlyPastStartTime = (scheduleCount >= i) && (loopedTime >= VuoListGetValue_VuoReal(schedule, i));
			bool justCrossedStartThreshold = (previouslyPastStartTime != currentlyPastStartTime);

			if (currentlyPastStartTime || justCrossedStartThreshold)
			{
				if (((durationType==VuoDurationType_Single) && !previouslyPastStartTime) ||
						((durationType==VuoDurationType_UntilNext) && (i==maxScheduleCount || !(*context)->hasStarted[i+1])) ||
						(durationType==VuoDurationType_UntilReset))
				{
					*elapsedTimes[i] = vuo_time_schedule_loop(loop, time - VuoListGetValue_VuoReal(schedule, i), maxTime);
					*elapsedTimeEvents[i] = true;
				}
			}

			(*context)->hasStarted[i] = currentlyPastStartTime;
		}

		(*context)->previousLoopedTime = loopedTime;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
