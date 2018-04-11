/**
 * @file
 * vuo.time.firePeriodically node implementation.
 *
 * See http://www.mikeash.com/pyblog/friday-qa-2010-07-02-background-timers.html and
 * http://www.fieryrobot.com/blog/2010/07/10/a-watchdog-timer-in-gcd/
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
					 "title" : "Fire Periodically",
					 "keywords" : [ "lfo", "wave generator", "signal", "tempo", "timer", "stop watch", "stopwatch", "clock",
						"metronome", "repeat", "seconds", "interval", "rate" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "CountSecondsWithFadeout.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoReal interval;  // seconds between timer firings
	VuoReal elapsed;  // seconds elapsed since the timer started
	dispatch_time_t lastFired;  // the most recent time that the timer fired

	dispatch_source_t timer;
	dispatch_queue_t timerQueue;
	dispatch_semaphore_t timerCanceled;

	void (*fired)(VuoReal);
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

static void setRepeatingTimer(struct nodeInstanceData *ctx, const VuoReal seconds, VuoOutputTrigger(fired, VuoReal))
{
	// If the timer starts paused, store now as the time last fired to track the time until the timer is unpaused.
	if (ctx->interval == -INFINITY && seconds <= 0)
		ctx->lastFired = dispatch_time(DISPATCH_TIME_NOW, 0);

	bool intervalChanged = (ctx->interval != seconds);
	if (intervalChanged)
	{
		cancelRepeatingTimer(ctx);
		ctx->interval = seconds;
	}

	dispatch_sync(ctx->timerQueue, ^{
		ctx->fired = fired;
	});

	if (intervalChanged && seconds > 0)
	{
		// For the interval between timer firings, use 'seconds'.
		uint64_t fullInterval = MAX(seconds, 0.001) * NSEC_PER_SEC;

		// For the interval between now and the first timer firing, deduct the time since the timer last fired.
		// If that is greater than 'seconds', fire immediately.
		uint64_t sinceLastFired = (ctx->lastFired > 0 ? dispatch_time(DISPATCH_TIME_NOW, 0) - ctx->lastFired : 0);
		uint64_t initialInterval;
		if (sinceLastFired <= fullInterval)
			initialInterval = fullInterval - sinceLastFired;
		else
		{
			initialInterval = 0;
			ctx->elapsed += ((double)sinceLastFired) / NSEC_PER_SEC - seconds;
		}

		ctx->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, VuoEventLoop_getDispatchStrictMask(), ctx->timerQueue);
		dispatch_source_set_timer(ctx->timer, dispatch_time(DISPATCH_TIME_NOW, initialInterval), fullInterval, 0);

		dispatch_source_set_event_handler(ctx->timer, ^{
			ctx->lastFired = dispatch_time(DISPATCH_TIME_NOW, 0);
			ctx->elapsed += seconds;
			if (ctx->fired)
				ctx->fired(ctx->elapsed);
		});

		dispatch_source_set_cancel_handler(ctx->timer, ^{
			dispatch_semaphore_signal(ctx->timerCanceled);
		});

		dispatch_resume(ctx->timer);
	}
}


struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);

	ctx->interval = -INFINITY;
	ctx->elapsed = 0;
	ctx->lastFired = 0;

	ctx->timer = NULL;
	ctx->timerQueue = dispatch_queue_create("vuo.time.firePeriodically", NULL);
	ctx->timerCanceled = dispatch_semaphore_create(0);

	ctx->fired = NULL;

	return ctx;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal) seconds,
		VuoOutputTrigger(fired, VuoReal)
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal) seconds,
		VuoOutputTrigger(fired, VuoReal)
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedStep":0.1}) seconds,
		VuoOutputTrigger(fired, VuoReal, {"eventThrottling":"drop"})
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	dispatch_sync((*ctx)->timerQueue, ^{
		(*ctx)->fired = NULL;
	});
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	cancelRepeatingTimer(*ctx);

	dispatch_release((*ctx)->timerQueue);
	dispatch_release((*ctx)->timerCanceled);
}
