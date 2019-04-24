/**
 * @file
 * VuoAppDelegate implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoAppDelegate.h"

#include "module.h"

@implementation VuoAppDelegate

/**
 * When the user tries to quit the application, cleanly stops the composition.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	VuoStopCurrentComposition();
	return NSTerminateCancel;
}

@end
