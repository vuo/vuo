/**
 * @file
 * VuoEventLoop implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoEventLoop.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>

#include <dlfcn.h>

/**
 * Handles one or more blocks or application events (e.g., keypresses, mouse moves, window state changes).
 *
 * If an NSApp is available:
 *
 *    - If `mode` is VuoEventLoop_WaitIndefinitely, waits until an NSEvent arrives, processes the event, and returns.
 *    - If `mode` is VuoEventLoop_RunOnce, processes an NSEvent (if one is ready), and returns.
 *
 * If there's no NSApp, executes the CFRunLoop:
 *
 *    - If `mode` is VuoEventLoop_WaitIndefinitely, returns when the CFRunLoop exits.
 *    - If `mode` is VuoEventLoop_RunOnce, returns immediately after a single CFRunLoop iteration.
 *
 * In either case, when `mode` is VuoEventLoop_WaitIndefinitely, @ref VuoEventLoop_break will cause this function to return immediately.
 *
 * @threadMain
 */
void VuoEventLoop_processEvent(VuoEventLoopMode mode)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];

	// Can't just use `NSApp` directly, since the composition might not link to AppKit.
	id *nsAppGlobal = (id *)dlsym(RTLD_DEFAULT, "NSApp");

	// Support running either with or without an NSApplication.
	if (nsAppGlobal && *nsAppGlobal)
	{
		// http://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
		// http://stackoverflow.com/questions/6732400/cocoa-integrate-nsapplication-into-an-existing-c-mainloop

		// When the composition is ready to stop, it posts a killswitch NSEvent,
		// to ensure that this function returns immediately.
		NSEvent *event = [*nsAppGlobal nextEventMatchingMask:NSAnyEventMask
												   untilDate:(mode == VuoEventLoop_WaitIndefinitely ? [NSDate distantFuture] : [NSDate distantPast])
													  inMode:NSDefaultRunLoopMode
													 dequeue:YES];
		[*nsAppGlobal sendEvent:event];
		[*nsAppGlobal updateWindows];
	}
	else
	{
		// This blocks until CFRunLoopStop() is called.
		// When the composition is ready to stop (or switch into NSApplication mode), it calls CFRunLoopStop().
		if (mode == VuoEventLoop_WaitIndefinitely)
			CFRunLoopRun();
		else
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
	}

	[pool drain];
}

/**
 * Interrupts @ref VuoEventLoop_processEvent while it is waiting.
 */
void VuoEventLoop_break(void)
{
	// Can't just use `NSApp` directly, since the composition might not link to AppKit.
	id *nsAppGlobal = (id *)dlsym(RTLD_DEFAULT, "NSApp");

	if (nsAppGlobal && *nsAppGlobal)
	{
		// Send an event, to ensure VuoEventLoop_processEvent()'s call to `nextEventMatchingMask:…` returns immediately.
		NSEvent *killswitch = [NSEvent otherEventWithType:NSApplicationDefined
							   location:NSMakePoint(0,0)
						  modifierFlags:0
							  timestamp:0
						   windowNumber:0
								context:nil
								subtype:0
								  data1:0
								  data2:0];
		[*nsAppGlobal postEvent:killswitch atStart:NO];
	}
	else
		CFRunLoopStop(CFRunLoopGetMain());
}

/**
 * Interrupts @ref VuoEventLoop_processEvent if it's currently blocking, so that it can process NSEvents next time it's invoked.
 */
void VuoEventLoop_switchToAppMode(void)
{
	CFRunLoopStop(CFRunLoopGetMain());
}

/**
 * Returns false if the app is currently waiting on user input in a modal dialog or a window sheet.
 */
bool VuoEventLoop_mayBeTerminated(void)
{
	// Can't just use `NSApp` directly, since the composition might not link to AppKit.
	id *nsAppGlobal = (id *)dlsym(RTLD_DEFAULT, "NSApp");
	if (!nsAppGlobal || !*nsAppGlobal)
		return true;

	if ([*nsAppGlobal modalWindow])
		return false;

	for (NSWindow *window in [*nsAppGlobal windows])
		if ([window attachedSheet])
			return false;

	return true;
}
