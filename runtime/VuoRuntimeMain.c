/**
 * @file
 * VuoRuntimeMain implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <unistd.h>
#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <pthread.h>

#include "VuoRuntime.h"
#include "VuoEventLoop.h"

void *VuoApp_mainThread = NULL;	///< A reference to the main thread

/**
 * Get a reference to the main thread, so we can perform runtime thread assertions.
 */
static void __attribute__((constructor)) VuoRuntimeMain_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// Calls _TSGetMainThread().
	// https://b33p.net/kosada/node/12944
	YieldToAnyThread();
#pragma clang diagnostic pop
}

/**
 * Starts a composition and runs it until the composition is stopped.
 */
int main(int argc, char **argv)
{
	vuoInit(argc, argv);
	VuoEventLoop_installSignalHandlers();
	VuoEventLoop_disableAppNap();

	while (! vuoIsCurrentCompositionStopped())
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);

	vuoFini();
	return 0;
}
