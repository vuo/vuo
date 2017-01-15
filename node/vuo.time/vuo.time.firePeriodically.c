/**
 * @file
 * vuo.time.firePeriodically node implementation.
 *
 * See http://www.mikeash.com/pyblog/friday-qa-2010-07-02-background-timers.html and
 * http://www.fieryrobot.com/blog/2010/07/10/a-watchdog-timer-in-gcd/
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Fire Periodically",
					 "keywords" : [ "lfo", "wave generator", "signal", "tempo", "timer", "stop watch", "stopwatch", "clock",
						"metronome", "repeat", "seconds", "interval", "rate" ],
					 "version" : "1.0.2",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });


struct nodeInstanceData
{
	dispatch_source_t timer;
	dispatch_semaphore_t timerCanceled;
};

static void cancelRepeatingTimer(struct nodeInstanceData *ctx)
{
	if (ctx->timer)
	{
		dispatch_source_cancel(ctx->timer);
		dispatch_release(ctx->timer);
		ctx->timer = NULL;
		dispatch_semaphore_wait(ctx->timerCanceled, DISPATCH_TIME_FOREVER);
	}
}

static void setRepeatingTimer(struct nodeInstanceData *ctx, const VuoReal seconds, VuoOutputTrigger(fired,void))
{
	cancelRepeatingTimer(ctx);

	if (seconds <= 0)
		return;

	dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	ctx->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, q);

	uint64_t nanoseconds = (seconds > 0.001 ? (seconds * NSEC_PER_SEC) : (NSEC_PER_SEC / 1000));
	dispatch_source_set_timer(ctx->timer, dispatch_time(DISPATCH_TIME_NOW, nanoseconds), nanoseconds, 0);
	dispatch_source_set_event_handler(ctx->timer, ^{
		fired();
	});
	dispatch_source_set_cancel_handler(ctx->timer, ^{
		dispatch_semaphore_signal(ctx->timerCanceled);
	});
	dispatch_resume(ctx->timer);
}


struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);
	ctx->timer = NULL;
	ctx->timerCanceled = dispatch_semaphore_create(0);
	return ctx;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal) seconds,
		VuoOutputTrigger(fired,void)
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal) seconds,
		VuoOutputTrigger(fired,void)
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedStep":0.1}) seconds,
		VuoOutputTrigger(fired,void,{"eventThrottling":"drop"})
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	cancelRepeatingTimer(*ctx);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	dispatch_release((*ctx)->timerCanceled);
}
