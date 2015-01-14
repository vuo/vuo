/**
 * @file
 * VuoWindowApplication interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dispatch/dispatch.h>

#import <AppKit/AppKit.h>

/**
 * An application that runs its own event loop.
 */
@interface VuoWindowApplication : NSApplication
{
	dispatch_source_t timer;  ///< Process-wide libdispatch timer for processing events.
	NSArray *windowMenuItems;  ///< Top-level menus for the current main window.
}

- (void)replaceWindowMenu:(NSArray *)newMenuItems;

@end
