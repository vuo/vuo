/**
 * @file
 * VuoRuntimeMain implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */


#include <CoreServices/CoreServices.h>
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

	while (! vuoIsCurrentCompositionStopped())
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);

	vuoFini();
	return 0;
}
