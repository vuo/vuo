/**
 * @file
 * VuoEventLoop implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoEventLoop.h"
#include "VuoLog.h"
#include "VuoCompositionState.h"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
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

/**
 * Returns `DISPATCH_TIMER_STRICT` if it's supported on the current OS version;
 * otherwise returns 0.
 */
unsigned long VuoEventLoop_getDispatchStrictMask(void)
{
	static unsigned long mask = 0;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		SInt32 macMinorVersion;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
#pragma clang diagnostic pop
		if (macMinorVersion >= 9)
			mask = 0x1; // DISPATCH_TIMER_STRICT
	});
	return mask;
}

/**
 * Called when the process receives a SIGINT (control-C) or SIGTERM (kill).
 */
void VuoEventLoop_signalHandler(int signum)
{
	typedef void (*vuoStopCompositionType)(struct VuoCompositionState *);
	vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_SELF, "vuoStopComposition");
	if (!vuoStopComposition)
		vuoStopComposition = (vuoStopCompositionType) dlsym(RTLD_DEFAULT, "vuoStopComposition");
	vuoStopComposition(NULL);
}

/**
 * Installs SIGINT and SIGTERM handlers, to cleanly shut down the composition.
 */
void VuoEventLoop_installSignalHandlers(void)
{
	struct sigaction action = {{VuoEventLoop_signalHandler}, 0, 0};
	if (sigaction(SIGINT,  &action, NULL))
		VUserLog("Warning: Couldn't install SIGINT handler: %s", strerror(errno));
	if (sigaction(SIGTERM, &action, NULL))
		VUserLog("Warning: Couldn't install SIGTERM handler: %s", strerror(errno));
}

/**
 * Disable "App Nap" since even if our timers are set to strict (@ref VuoEventLoop_getDispatchStrictMask),
 * the OS still prevents the process from running smoothly while taking a nap.
 * https://b33p.net/kosada/node/12685
 */
void VuoEventLoop_disableAppNap(void)
{
	SInt32 macMinorVersion;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
#pragma clang diagnostic pop
	if (macMinorVersion < 9)
		return;

	id activityToken = [[NSProcessInfo processInfo] performSelector:@selector(beginActivityWithOptions:reason:)
		withObject: (id)((0x00FFFFFFULL | (1ULL << 20)) & ~(1ULL << 14)) // NSActivityUserInitiated & ~NSActivitySuddenTerminationDisabled
		withObject: @"Many Vuo compositions need to process input and send output even when the app's window is not visible."];

	[activityToken retain];
}
