/**
 * @file
 * vuo.test.firePeriodicallyWithCount node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Fire Periodically with Count",
					 "description" : "Fires an event every 'seconds' seconds. Along with the event is the count of events fired so far.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


struct nodeInstanceData
{
	dispatch_source_t timer;
	dispatch_semaphore_t timerCanceled;
	VuoInteger eventCount;
};

static void cancelRepeatingTimer(struct nodeInstanceData *ctx)
{
	dispatch_source_cancel(ctx->timer);
	dispatch_release(ctx->timer);
	ctx->timer = NULL;
	dispatch_semaphore_wait(ctx->timerCanceled, DISPATCH_TIME_FOREVER);
}

static void setRepeatingTimer(struct nodeInstanceData *ctx, const VuoReal seconds, VuoOutputTrigger(fired,VuoInteger))
{
	if (ctx->timer)
		cancelRepeatingTimer(ctx);

	dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	ctx->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, q);

	uint64_t nanoseconds = seconds * NSEC_PER_SEC;
	dispatch_source_set_timer(ctx->timer, dispatch_time(DISPATCH_TIME_NOW, nanoseconds), nanoseconds, 0);
	dispatch_source_set_event_handler(ctx->timer, ^{
		fired(++ctx->eventCount);
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
	ctx->eventCount = 0;
	return ctx;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":1}) seconds,
		VuoOutputTrigger(fired,VuoInteger)
)
{
	setRepeatingTimer(*ctx, seconds, fired);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":1}) seconds,
		VuoOutputTrigger(fired,VuoInteger)
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
