/**
 * @file
 * vuo.time.allowPeriodic node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <stdio.h>

#include "VuoEventLoop.h"

VuoModuleMetadata({
					 "title" : "Allow Periodic Events",
					 "keywords" : [ "lfo", "wave generator", "signal", "tempo", "timer", "stop watch", "stopwatch", "clock",
						"metronome", "repeat", "seconds", "interval", "rate" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "ChangeRipplePeriodically.vuo" ]
					 }
				 });

VuoReal *nodeInstanceInit(void)
{
	VuoReal *lastTime = (VuoReal *) malloc(sizeof(VuoReal));
	VuoRegister(lastTime, free);
	*lastTime = 0;
	return lastTime;
}

void nodeInstanceEvent
(
	VuoInstanceData(VuoReal *) lastTime,
	VuoInputData(VuoReal) time,
	VuoInputEvent({"data":"time","eventBlocking":"door"}) timeEvent,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedStep":0.1}) period,
	VuoInputEvent({"data":"period","eventBlocking":"wall"}) periodEvent,
	VuoOutputData(VuoReal) periodicTime,
	VuoOutputEvent({"data":"periodicTime"}) periodicTimeEvent
)
{
	if (timeEvent)
	{
		if (period <= 0)
		{
			**lastTime = time;
			return;
		}

		if (time < **lastTime)
			// Unexpected time, so reset.
			**lastTime = time;

		if (**lastTime + period <= time)
		{
			**lastTime = **lastTime + floor((time - **lastTime) / period) * period;
			*periodicTime = time;
			*periodicTimeEvent = true;
		}
	}
}

void nodeInstanceFini
(
	VuoInstanceData(VuoReal *) lastTime
)
{
}
