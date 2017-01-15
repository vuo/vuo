/**
 * @file
 * vuo.test.firePeriodically node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VuoModuleMetadata({
					 "title" : "TestFirePeriodically",
					 "description" : "Periodically fires events until a maximum number is reached. Together with each event, outputs either 'nodeTitle eventCount' (for all but the last event) or a sentinel string (for the last event).",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


/**
 * Sentinel output sent as the final event.
 */
#define SENTINEL "*"

struct nodeInstanceData
{
	dispatch_source_t timer;
	dispatch_semaphore_t timerCanceled;
	int eventCount;
};

static void cancelRepeatingTimer(struct nodeInstanceData *ctx)
{
	dispatch_source_cancel(ctx->timer);
	dispatch_release(ctx->timer);
	ctx->timer = NULL;
	dispatch_semaphore_wait(ctx->timerCanceled, DISPATCH_TIME_FOREVER);
}

static void setRepeatingTimer(struct nodeInstanceData *ctx, const int milliseconds, VuoOutputTrigger(fired,VuoText), int maxEventCount, const VuoText nodeTitle)
{
	if (ctx->timer)
		cancelRepeatingTimer(ctx);

	dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	ctx->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, q);

	uint64_t periodNsec = milliseconds * USEC_PER_SEC;
	dispatch_source_set_timer(ctx->timer, dispatch_time(DISPATCH_TIME_NOW, periodNsec), periodNsec, 0);
	dispatch_source_set_event_handler(ctx->timer, ^{
										  VuoText outString;
										  if (++(ctx->eventCount) > maxEventCount)
										  {
											  outString = VuoText_make(SENTINEL);
											  dispatch_source_cancel(ctx->timer);
										  }
										  else
										  {
											  char *triggerInfoOut = (char *)malloc(strlen(nodeTitle) + 12);
											  sprintf(triggerInfoOut, "%s %d", nodeTitle, ctx->eventCount);
											  outString = VuoText_make(triggerInfoOut);
											  free(triggerInfoOut);
										  }
										  fired(outString);
									  });
	dispatch_source_set_cancel_handler(ctx->timer, ^{
										   dispatch_semaphore_signal(ctx->timerCanceled);
									   });
	dispatch_resume(ctx->timer);
}


struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);
	ctx->timer = NULL;
	ctx->timerCanceled = dispatch_semaphore_create(0);
	ctx->eventCount = 0;
	return ctx;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) ctx,
	VuoInputData(VuoInteger, {"default":1}) milliseconds,
	VuoOutputTrigger(fired,VuoText),
	VuoInputData(VuoInteger, {"default":100}) maxEventCount,
	VuoInputData(VuoText, {"default":""}) triggerInfoIn,
	VuoInputData(VuoText, {"default":""}) in0,
	VuoInputData(VuoText, {"default":""}) in1,
	VuoInputData(VuoText, {"default":""}) nodeTitle,
	VuoOutputData(VuoText) triggerInfoOut,
	VuoOutputData(VuoText) nodeInfo
)
{
	setRepeatingTimer(*ctx, milliseconds, fired, maxEventCount, nodeTitle);

	*triggerInfoOut = triggerInfoIn;

	char *ni = (char *)malloc(strlen(*triggerInfoOut) + strlen(nodeTitle) + 2);
	sprintf(ni, "%s %s", *triggerInfoOut, nodeTitle);
	*nodeInfo = VuoText_make(ni);
	free(ni);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	if ((*ctx)->timer)
		cancelRepeatingTimer(*ctx);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	dispatch_release((*ctx)->timerCanceled);
}
