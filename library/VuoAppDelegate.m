/**
 * @file
 * VuoAppDelegate implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoAppDelegate.h"

#include "VuoAppSplashWindow.h"

@implementation VuoAppDelegate

/**
 * When the user tries to quit the application, cleanly stops the composition.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	// Ignore quit requests while the splash window is visible
	// (since runtime initialization hasn't completed yet, it crashes).
	if (!VuoApp_splashWindow)
		VuoStopCurrentComposition();

	return NSTerminateCancel;
}

@end
