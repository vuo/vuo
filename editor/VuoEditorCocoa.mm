/**
 * @file
 * VuoEditorCocoa implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorCocoa.hh"

#include "VuoEditor.hh"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

/**
 * Intercept and forward certain overhead submenu keyboard shortcuts that otherwise
 * aren't received by the Qt application when there are no open windows.
 */
void VuoEditorCocoa_detectSubmenuKeyEvents()
{
	VuoEditor *editor = (VuoEditor *)qApp;

	[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^(NSEvent *event) {
		// Cmd+Shift+O (Open Most Recent File)
		if ((event.modifierFlags & NSEventModifierFlagCommand) && (event.modifierFlags & NSEventModifierFlagShift)
				&& (event.keyCode == kVK_ANSI_O))
		{
			editor->openMostRecentFile();
			return (NSEvent *)nil; // event was handled by this monitor (avoids the system beep)
		}
		// Cmd+Option+O (Open Random Example)
		else if ((event.modifierFlags & NSEventModifierFlagCommand) && (event.modifierFlags & NSEventModifierFlagOption)
				 && (event.keyCode == kVK_ANSI_O))
		{
			editor->openRandomExample();
			return (NSEvent *)nil; // event was handled by this monitor (avoids the system beep)
		}

		// event was not handled by this monitor; pass it on.
		return event;
	}];
}

/**
 * Returns the number of seconds (including fractional seconds) since the computer booted.
 *
 * Used by `NSEvent`, which `QTouchEvent` is based on.
 */
double VuoEditorCocoa_systemUptime()
{
	return [[NSProcessInfo processInfo] systemUptime];
}

/**
 * Registers for Cocoa app notifications, and forwards them to Qt.
 */
@interface VuoEditorCocoaNotifications : NSObject
@end
@implementation VuoEditorCocoaNotifications
+ (void)load
{
	VuoEditorCocoaNotifications *n = [VuoEditorCocoaNotifications new];
	[[NSNotificationCenter defaultCenter] addObserver:n selector:@selector(applicationWillHide)  name:NSApplicationWillHideNotification  object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:n selector:@selector(applicationDidUnhide) name:NSApplicationDidUnhideNotification object:nil];
}
- (void)applicationWillHide
{
	VuoEditor *editor = (VuoEditor *)qApp;
	emit editor->applicationWillHide();
}
- (void)applicationDidUnhide
{
	VuoEditor *editor = (VuoEditor *)qApp;
	emit editor->applicationDidUnhide();
}
@end
