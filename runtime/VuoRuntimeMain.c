/**
 * @file
 * VuoRuntimeMain implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <unistd.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include "VuoRuntime.h"

/**
 * Private API function in libdispatch.
 */
extern void _dispatch_main_queue_callback_4CF(mach_msg_header_t *msg);

extern bool isStopped;

/**
 * Starts a composition and runs it until the composition is stopped.
 */
int main(int argc, char **argv)
{
	vuoInit(argc, argv);
	while (! isStopped)
	{
		_dispatch_main_queue_callback_4CF(0);  // Feature request submitted to make this a public API call - https://b33p.net/kosada/node/3255, http://stackoverflow.com/q/12570394/64860, http://libdispatch.macosforge.org/trac/ticket/38
		usleep(10000);
	}
	vuoFini();
	return 0;
}
