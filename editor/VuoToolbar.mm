/**
 * @file
 * VuoToolbar implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoToolbar.hh"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>
#include <objc/message.h>

#include "VuoEditor.hh"
#include "VuoToolbarTitleCell.hh"

/**
 * On macOS 10.12 through 10.15, positions the sheet directly beneath the titlebar/toolbar.
 * (On macOS 11+, window-modal dialogs are always centered inside the parent window.)
 */
static NSRect windowWillPositionSheetUsingRect(id self, SEL _cmd, NSWindow *window, NSWindow *sheet, NSRect rect)
{
	float titleAndToolbarHeight = window.frame.size.height - [window contentRectForFrameRect:window.frame].size.height;

#if VUO_PRO
	if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,14,0}])
	{
		// On macOS 10.14, we need to adjust the position by different amounts when toolbar labels are visible and hidden.
		if (static_cast<VuoEditor *>(qApp)->areToolbarLabelsVisible())
			rect.origin.y += titleAndToolbarHeight;
		else
			rect.origin.y += titleAndToolbarHeight * 2;
	}
	else if (!static_cast<VuoEditor *>(qApp)->areToolbarLabelsVisible())
		// On macOS 10.12 and 10.13, we only need to adjust the position when toolbar labels are hidden.
		rect.origin.y += titleAndToolbarHeight;
#else
	if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10,14,0}])
		rect.origin.y += titleAndToolbarHeight;
#endif

	return rect;
}

/**
 * Initializes an empty toolbar. VuoToolbar::setUp() needs to be called after this.
 */
VuoToolbar::VuoToolbar(QMainWindow *window)
	: QObject(window)
{
	this->window = window;
	qtToolbar = nullptr;
	nsWindow = nil;
	titleView = nil;
	titleViewController = nil;
}

/**
 * Populates the toolbar and adds it to the window.
 *
 * This function always needs to be called after the constructor, but can't be part of the constructor
 * because it needs to call a virtual function to allow derived classes to add items to the toolbar.
 */
void VuoToolbar::setUp(void)
{
	NSView *nsView = (NSView *)window->winId();
	nsWindow = [nsView window];

	titleView = [NSTextField new];
	titleView.cell = [VuoToolbarTitleCell new];
	titleView.font = [NSFont titleBarFontOfSize:0];
	titleView.selectable = NO;
	titleViewController = nil;

	qtToolbar = new QMacToolBar(window);
	{
		NSToolbar *tb = qtToolbar->nativeToolbar();

		// Workaround for apparent Qt bug, where QCocoaIntegration::clearToolbars() releases the toolbar after it's already been deallocated.
		[tb retain];

		[tb setAllowsUserCustomization:NO];
	}

	addToolbarItems();  // Toolbar items must be added before attachToWindow() or they won't appear.

	qtToolbar->attachToWindow(window->windowHandle());
	window->setUnifiedTitleAndToolBarOnMac(true);

	auto editor = static_cast<VuoEditor *>(qApp);
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoToolbar::updateColor);
	updateColor(editor->isInterfaceDark());

	class_addMethod(nsWindow.delegate.class,
					@selector(window:willPositionSheet:usingRect:),
					(IMP)windowWillPositionSheetUsingRect,
					"{CGRect={CGPoint=dd}{CGSize=dd}}@:@@{CGRect={CGPoint=dd}{CGSize=dd}}");

#if VUO_PRO
	setUp_Pro();
#endif
}

/**
 * Makes the titlebar/toolbar dark (Digital Color Meter set to "Display native values" reads 0x808080).
 */
void VuoToolbar::updateColor(bool isDark)
{
	{
		// Request a transparent titlebar (and toolbar) so the window's background color (set below) shows through.
		// "Previously NSWindow would make the titlebar transparent for certain windows with NSWindowStyleMaskTexturedBackground set,
		// even if titlebarAppearsTransparent was NO.  When linking against the 10.13 SDK, textured windows must
		// explicitly set titlebarAppearsTransparent to YES for this behavior."
		// - https://developer.apple.com/library/content/releasenotes/AppKit/RN-AppKit/
		nsWindow.titlebarAppearsTransparent = YES;

		if (allowsTabbingWithOtherWindows())
		{
			// "A window with titlebarAppearsTransparent is normally opted-out of automatic window tabbing."
			// - https://developer.apple.com/library/archive/releasenotes/AppKit/RN-AppKit/index.html
			// …but it works fine for us, so reenable it.
			// https://b33p.net/kosada/node/15412
			nsWindow.tabbingMode = NSWindowTabbingModeAutomatic;
			nsWindow.tabbingIdentifier = getTabbingIdentifier();
		}
	}

	if (isDark)
		[nsWindow setBackgroundColor:[NSColor colorWithCalibratedWhite:.47 alpha:1]];
	else
		[nsWindow setBackgroundColor:[NSColor colorWithCalibratedWhite:.92 alpha:1]];
}
