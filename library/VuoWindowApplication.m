/**
 * @file
 * VuoWindowApplication implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowApplication.h"

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowApplication",
					 "dependencies" : [
						"AppKit.framework"
					 ]
				 });
#endif


@implementation VuoWindowApplication

/**
 * Creates the application and its menu.
 */
- (id)init
{
	if ((self = [super init]))
	{
		// http://stackoverflow.com/a/11010614/238387

		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

		[self setActivationPolicy:NSApplicationActivationPolicyRegular];
		NSMenu *menubar = [[NSMenu new] autorelease];
		NSMenuItem *appMenuItem = [[NSMenuItem new] autorelease];
		[menubar addItem:appMenuItem];
		[self setMainMenu:menubar];
		NSMenu *appMenu = [[NSMenu new] autorelease];
		NSString *appName = [[NSProcessInfo processInfo] processName];
		NSString *quitTitle = [@"Quit " stringByAppendingString:appName];
		NSMenuItem *quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
		[appMenu addItem:quitMenuItem];
		[appMenuItem setSubmenu:appMenu];

		windowMenuItems = nil;

		[pool release];
	}
	return self;
}

/**
 * Starts the event loop.
 */
- (void)run
{
	// http://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
	// http://stackoverflow.com/questions/6732400/cocoa-integrate-nsapplication-into-an-existing-c-mainloop

	[self finishLaunching];

	/// @todo event tracking run loop mode (https://b33p.net/kosada/node/5961)
	timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_get_main_queue());
	dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC/100, NSEC_PER_SEC/100);
	dispatch_source_set_event_handler(timer, ^{
		NSAutoreleasePool *pool = [NSAutoreleasePool new];
		NSEvent *event;
		do {
			event = [self nextEventMatchingMask:NSAnyEventMask
									  untilDate:[NSDate distantPast]
										 inMode:NSDefaultRunLoopMode
										dequeue:YES];
			[self sendEvent:event];
		} while (event != nil);
		[self updateWindows];
		[pool drain];
	});

	dispatch_resume(timer);
}

/**
 * Stops the event loop.
 */
- (void)stop:(id)sender
{
	dispatch_source_cancel(timer);
	dispatch_release(timer);
}

/**
 * Replaces the top-level menus in the menu bar, except for application-wide menus,
 * with the given menus.
 */
- (void)replaceWindowMenu:(NSArray *)newMenuItems
{
	NSMenu *menubar = [self mainMenu];

	for (NSMenuItem *oldMenu in windowMenuItems)
		[menubar removeItem:oldMenu];

	for (NSMenuItem *newMenu in newMenuItems)
		[menubar addItem:newMenu];

	[windowMenuItems release];
	windowMenuItems = [newMenuItems retain];
}

@end
