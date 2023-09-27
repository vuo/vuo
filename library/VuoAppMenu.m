/**
 * @file
 * VuoAppMenu implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoAppAboutBox.h"

/**
 * Overrides keyDown event handling.
 */
@interface VuoAppMenu : NSMenu
@end

@implementation VuoAppMenu

/**
 * Overrides keyDown event handling.
 */
- (BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
	[super performKeyEquivalent:theEvent];

	// https://b33p.net/kosada/node/11966
	// Always return YES, even if the event wasn't handled, to prevent the `NSBeep()`
	// (which happens by default if no window or menu handles the event).
	return YES;
}

@end

/**
 * Replaces the top-level menus in the menu bar, except for application-wide menus,
 * with the given menus.
 *
 * `items` should be an `NSArray` of `NSMenuItem`s.
 *
 * Returns the old menu.
 */
void *VuoApp_setMenuItems(void *items)
{
	NSMenu *oldMenu = [NSApp mainMenu];

	NSMenu *menubar = [[VuoAppMenu new] autorelease];

	// Application menu
	{
		NSMenu *appMenu = [[NSMenu new] autorelease];
		NSString *appName = [[NSProcessInfo processInfo] processName];

		NSString *aboutTitle = [@"About " stringByAppendingString:appName];
		NSMenuItem *aboutMenuItem = [[[NSMenuItem alloc] initWithTitle:aboutTitle action:@selector(displayAboutPanel:) keyEquivalent:@""] autorelease];

		static VuoAppAboutBox *aboutDialog;
		static dispatch_once_t aboutDialogInitialized = 0;
		dispatch_once(&aboutDialogInitialized, ^{
			aboutDialog = [[VuoAppAboutBox alloc] init];
		});
		[aboutMenuItem setTarget:aboutDialog];

		[appMenu addItem:aboutMenuItem];

		[appMenu addItem:[NSMenuItem separatorItem]];

		NSString *quitTitle = [@"Quit " stringByAppendingString:appName];
		NSMenuItem *quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
		[appMenu addItem:quitMenuItem];

		NSMenuItem *appMenuItem = [[NSMenuItem new] autorelease];
		[appMenuItem setSubmenu:appMenu];

		[menubar addItem:appMenuItem];
	}

	// Custom menus
	if (items)
	{
		NSArray *itemsArray = (NSArray *)items;
		for (NSMenuItem *item in itemsArray)
			[menubar addItem:item];
	}

	[NSApp setMainMenu:menubar];

	return oldMenu;
}

/**
 * Replaces the top-level menus in the menu bar with the given menu.
 *
 * `menu` should be an `NSMenu`.
 */
void VuoApp_setMenu(void *menu)
{
	[NSApp setMainMenu:menu];
}
