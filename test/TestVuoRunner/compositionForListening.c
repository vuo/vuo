/**
 * @file
 * compositionForListening implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "composition.h"
#include "VuoEventLoop.h"
#include "VuoCompositionState.h"

void *VuoApp_mainThread = NULL;  ///< A reference to the main thread

extern void vuoSendNodeExecutionStarted(struct VuoCompositionState *compositionState, const char *nodeIdentifier);
extern void vuoSendNodeExecutionFinished(struct VuoCompositionState *compositionState, const char *nodeIdentifier);
extern void *vuoRuntimeState;

dispatch_source_t timer = NULL;

int main(int argc, char **argv)
{
	VuoApp_mainThread = (void *)pthread_self();

	vuoInit(argc, argv);

	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT,queue);
		dispatch_source_set_timer(timer, dispatch_walltime(NULL, NSEC_PER_SEC/4), NSEC_PER_SEC/2, NSEC_PER_SEC/100);
		dispatch_source_set_event_handler(timer, ^{
			struct VuoCompositionState compositionState = { vuoRuntimeState, vuoTopLevelCompositionIdentifier };

			{
				const char *nodeIdentifier = "node.started";
				vuoSendNodeExecutionStarted(&compositionState, nodeIdentifier);
			}

			{
				const char *nodeIdentifier = "node.finished";
				vuoSendNodeExecutionFinished(&compositionState, nodeIdentifier);
			}
		});
		dispatch_resume(timer);
	}

	while (! vuoIsCurrentCompositionStopped())
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	return 0;
}

void vuoInstanceFini(void)
{
	dispatch_source_cancel(timer);
	dispatch_release(timer);
}
