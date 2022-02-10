/**
 * @file
 * VuoConsoleToolbar implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoConsoleToolbar.hh"
#include "VuoConsole.hh"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

/**
 * Creates a toolbar, with toolbar actions handled by @a console, and adds it to @a window.
 */
VuoConsoleToolbar * VuoConsoleToolbar::create(QMainWindow *window, VuoConsole *console)
{
	VuoConsoleToolbar *toolbar = new VuoConsoleToolbar(window, console);
	toolbar->setUp();
	return toolbar;
}

/**
 * Initializes an empty toolbar.
 */
VuoConsoleToolbar::VuoConsoleToolbar(QMainWindow *window, VuoConsole *console)
	: VuoToolbar(window)
{
	this->console = console;
}

/**
 * Populates the toolbar and adds it to the window.
 */
void VuoConsoleToolbar::addToolbarItems(void)
{
	// Copy
	{
		NSImage *image = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"copy" ofType:@"pdf"]];
		[image setTemplate:YES];

		QMacToolBarItem *item = qtToolbar->addItem(QIcon(), tr("Copy"));
		NSToolbarItem *ti = item->nativeToolBarItem();
		[ti setImage:image];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Copy the selected messages to the clipboard").toUtf8().data()];

		connect(item, &QMacToolBarItem::activated, console, &VuoConsole::copy);
	}

	// Save
	{
		NSImage *image = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"download" ofType:@"pdf"]];
		[image setTemplate:YES];

		QMacToolBarItem *item = qtToolbar->addItem(QIcon(), tr("Save…"));
		NSToolbarItem *ti = item->nativeToolBarItem();
		[ti setImage:image];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Save all messages to a file").toUtf8().data()];

		connect(item, &QMacToolBarItem::activated, console, &VuoConsole::save);
	}

	// Report a bug
	{
		NSImage *image = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"external-link-square" ofType:@"pdf"]];
		[image setTemplate:YES];

		QMacToolBarItem *item = qtToolbar->addItem(QIcon(), tr("Report a bug"));
		NSToolbarItem *ti = item->nativeToolBarItem();
		[ti setImage:image];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Go to the Bug Reports page on vuo.org").toUtf8().data()];

		connect(item, &QMacToolBarItem::activated, this, [](){ QDesktopServices::openUrl(QUrl("https://vuo.org/bug")); });
	}

	// Clear
	{
		NSImage *image = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"times-circle" ofType:@"pdf"]];
		[image setTemplate:YES];

		QMacToolBarItem *item = qtToolbar->addItem(QIcon(), tr("Clear"));
		NSToolbarItem *ti = item->nativeToolBarItem();
		[ti setImage:image];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Clear all messages").toUtf8().data()];

		connect(item, &QMacToolBarItem::activated, console, &VuoConsole::clear);
	}
}

/**
 * Since the console window is a singleton, it doesn't tab with other windows.
 */
bool VuoConsoleToolbar::allowsTabbingWithOtherWindows(void)
{
	return false;
}

/**
 * Unused.
 */
NSString * VuoConsoleToolbar::getTabbingIdentifier(void)
{
	return @"";
}
