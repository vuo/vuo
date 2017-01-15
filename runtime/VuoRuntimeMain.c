/**
 * @file
 * VuoRuntimeMain implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <unistd.h>
#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <pthread.h>

#include "VuoRuntime.h"
#include "VuoEventLoop.h"

extern bool isStopped;

void *VuoApp_mainThread = NULL;	///< A reference to the main thread

/**
 * Get a reference to the main thread, so we can perform runtime thread-sanity assertions.
 */
static void __attribute__((constructor)) VuoRuntimeMain_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();
}

/**
 * Starts a composition and runs it until the composition is stopped.
 */
int main(int argc, char **argv)
{
	vuoInit(argc, argv);

	while (! isStopped)
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);

	vuoFini();
	return 0;
}
