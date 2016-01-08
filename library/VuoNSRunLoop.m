/**
 * @file
 * VuoNSRunLoop implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "module.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>
#include <dispatch/dispatch.h>

#include "VuoNSRunLoop.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoNSRunLoop",
					  "dependencies" : [
						"AppKit.framework"
					  ]
				  });
#endif

unsigned int VuoNSRunLoop_useCount = 0;	///< Process-wide count of callers (typically node instances) using the NSRunLoop.
dispatch_source_t VuoNSRunLoop_timer;	///< Process-wide libdispatch timer for processing events.

/**
 * Indicates that the caller needs an NSRunLoop on the main thread.
 *
 * @threadAny
 */
void VuoNSRunLoop_use(void)
{
	++VuoNSRunLoop_useCount;

	if (VuoNSRunLoop_useCount == 1)
	{
		// http://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
		// http://stackoverflow.com/questions/6732400/cocoa-integrate-nsapplication-into-an-existing-c-mainloop

		/// @todo event tracking run loop mode (https://b33p.net/kosada/node/5961)
		VuoNSRunLoop_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_get_main_queue());
		dispatch_source_set_timer(VuoNSRunLoop_timer, dispatch_walltime(NULL,0), NSEC_PER_SEC/100, NSEC_PER_SEC/100);
		NSRunLoop *mainRunLoop = [NSRunLoop mainRunLoop];
		dispatch_source_set_event_handler(VuoNSRunLoop_timer, ^{
			NSAutoreleasePool *pool = [NSAutoreleasePool new];
			[mainRunLoop runUntilDate:[NSDate distantPast]];
			[pool drain];
		});
		dispatch_resume(VuoNSRunLoop_timer);
	}
}

/**
 * Indicates that the caller no longer needs an NSRunLoop.
 *
 * (The NSRunLoop may continue running, if other callers still need it.)
 *
 * @threadAny
 */
void VuoNSRunLoop_disuse(void)
{
	--VuoNSRunLoop_useCount;

	if (VuoNSRunLoop_useCount == 0)
	{
		dispatch_source_cancel(VuoNSRunLoop_timer);
		dispatch_release(VuoNSRunLoop_timer);
	}
}
