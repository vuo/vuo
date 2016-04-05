/**
 * @file
 * VuoRuntimeMain implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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
	{
		id pool = objc_msgSend((id)objc_getClass("NSAutoreleasePool"), sel_getUid("new"));
		CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
		objc_msgSend(pool, sel_getUid("drain"));
	}
	vuoFini();
	return 0;
}
