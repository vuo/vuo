/**
 * @file
 * vuo.time.schedule node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDurationType.h"

VuoModuleMetadata({
					  "title" : "Schedule",
					  "keywords" : [ "animate", "frame", "stopwatch", "elapsed", "count", "cue", "alarm", "clock" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : ["AnimateOnSchedule.vuo", "FlashOnMousePress.vuo"]
					  },
				  });

const int maxScheduleCount = 8;

struct nodeInstanceData
{
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
void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputData(VuoList_VuoReal, {"default":[1,2]}) schedule,
		VuoInputEvent({"eventBlocking":"wall","data":"schedule"}) scheduleEvent,
		VuoInputData(VuoDurationType, {"default":"until reset"}) durationType,
		VuoInputEvent({"eventBlocking":"wall","data":"durationType"}) durationTypeEvent,
		VuoInputData(VuoLoopType, {"default":"none"}) loop,
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

		VuoReal loopedTime = time;
		if (loop==VuoLoopType_Loop)
			loopedTime = VuoReal_wrap(time, 0, maxTime);
		else if (loop==VuoLoopType_Mirror)
		{
			loopedTime = VuoReal_wrap(time, 0, 2*maxTime);
			if (loopedTime > maxTime)
				loopedTime = 2*maxTime-loopedTime;
		}

		for (int i = 1; i <= maxScheduleCount; ++i)
		{
			bool previouslyPastStartTime = (*context)->hasStarted[i];
			bool currentlyPastStartTime = (scheduleCount >= i) && (loopedTime >= VuoListGetValue_VuoReal(schedule, i));
			bool justCrossedStartThreshold = (previouslyPastStartTime != currentlyPastStartTime);

			if (currentlyPastStartTime || justCrossedStartThreshold)
			{
				if (((durationType==VuoDurationType_Single) && !previouslyPastStartTime) ||
						((durationType==VuoDurationType_UntilNext) && (i==maxScheduleCount || !(*context)->hasStarted[i+1])) ||
						(durationType==VuoDurationType_UntilReset) ||
						justCrossedStartThreshold)
				{
					if (i==1)
					{
						*elapsedTime1 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent1 = true;
					}
					if (i==2)
					{
						*elapsedTime2 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent2 = true;
					}
					if (i==3)
					{
						*elapsedTime3 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent3 = true;
					}
					if (i==4)
					{
						*elapsedTime4 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent4 = true;
					}
					if (i==5)
					{
						*elapsedTime5 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent5 = true;
					}
					if (i==6)
					{
						*elapsedTime6 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent6 = true;
					}
					if (i==7)
					{
						*elapsedTime7 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent7 = true;
					}
					if (i==8)
					{
						*elapsedTime8 = loopedTime - VuoListGetValue_VuoReal(schedule, i);
						*elapsedTimeEvent8 = true;
					}
				}
			}

			(*context)->hasStarted[i] = currentlyPastStartTime;
		}
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
