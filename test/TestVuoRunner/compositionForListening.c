/**
 * @file
 * compositionForListening implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "composition.h"
#include "VuoEventLoop.h"

dispatch_source_t timer = NULL;

int main(int argc, char **argv)
{
	vuoInit(argc, argv);

	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,queue);
		dispatch_source_set_timer(timer, dispatch_walltime(NULL, NSEC_PER_SEC/4), NSEC_PER_SEC/2, NSEC_PER_SEC/100);
		dispatch_source_set_event_handler(timer, ^{
			{
				zmq_msg_t messages[1];
				const char *nodeIdentifier = "node.started";
				zmq_msg_init_size(&messages[0], strlen(nodeIdentifier) + 1);
				memcpy(zmq_msg_data(&messages[0]), nodeIdentifier, strlen(nodeIdentifier) + 1);
				vuoTelemetrySend(VuoTelemetryNodeExecutionStarted, messages, 1);
			}

			{
				zmq_msg_t messages[1];
				const char *nodeIdentifier = "node.finished";
				zmq_msg_init_size(&messages[0], strlen(nodeIdentifier) + 1);
				memcpy(zmq_msg_data(&messages[0]), nodeIdentifier, strlen(nodeIdentifier) + 1);
				vuoTelemetrySend(VuoTelemetryNodeExecutionFinished, messages, 1);
			}
		});
		dispatch_resume(timer);
	}

	while (! isStopped)
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	return 0;
}

void vuoInstanceFini(void)
{
	dispatch_source_cancel(timer);
	dispatch_release(timer);
}
