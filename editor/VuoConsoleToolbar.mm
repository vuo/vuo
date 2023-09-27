/**
 * @file
 * VuoConsoleToolbar implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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
		copyImage11   = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"copy-1x1" ofType:@"pdf"]];
		copyImage1611 = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"copy-16x11" ofType:@"pdf"]];
		[copyImage11 setTemplate:YES];
		[copyImage1611 setTemplate:YES];
		copyImage = copyImage11;

		toolbarCopyItem = qtToolbar->addItem(QIcon(), tr("Copy"));
		NSToolbarItem *ti = toolbarCopyItem->nativeToolBarItem();
		[ti setImage:copyImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Copy the selected messages to the clipboard").toUtf8().data()];

		connect(toolbarCopyItem, &QMacToolBarItem::activated, console, &VuoConsole::copy);
	}

	// Save
	{
		saveImage11   = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"download-1x1" ofType:@"pdf"]];
		saveImage1611 = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"download-16x11" ofType:@"pdf"]];
		[saveImage11 setTemplate:YES];
		[saveImage1611 setTemplate:YES];
		saveImage = saveImage11;

		toolbarSaveItem = qtToolbar->addItem(QIcon(), tr("Save…"));
		NSToolbarItem *ti = toolbarSaveItem->nativeToolBarItem();
		[ti setImage:saveImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Save all messages to a file").toUtf8().data()];

		connect(toolbarSaveItem, &QMacToolBarItem::activated, console, &VuoConsole::save);
	}

	// Report a bug
	{
		reportImage11   = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"external-link-square-1x1" ofType:@"pdf"]];
		reportImage1611 = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"external-link-square-16x11" ofType:@"pdf"]];
		[reportImage11 setTemplate:YES];
		[reportImage1611 setTemplate:YES];
		reportImage = reportImage11;

		toolbarReportItem = qtToolbar->addItem(QIcon(), tr("Report a bug"));
		NSToolbarItem *ti = toolbarReportItem->nativeToolBarItem();
		[ti setImage:reportImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Go to the Bug Reports page on vuo.org").toUtf8().data()];

		connect(toolbarReportItem, &QMacToolBarItem::activated, this, [](){ QDesktopServices::openUrl(QUrl("https://vuo.org/bug")); });
	}

	// Clear
	{
		clearImage11   = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"times-circle-1x1" ofType:@"pdf"]];
		clearImage1611 = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"times-circle-16x11" ofType:@"pdf"]];
		[clearImage11 setTemplate:YES];
		[clearImage1611 setTemplate:YES];
		clearImage = clearImage11;

		toolbarClearItem = qtToolbar->addItem(QIcon(), tr("Clear"));
		NSToolbarItem *ti = toolbarClearItem->nativeToolBarItem();
		[ti setImage:clearImage];
		[ti setAutovalidates:NO];
		ti.toolTip = [NSString stringWithUTF8String:tr("Clear all messages").toUtf8().data()];

		connect(toolbarClearItem, &QMacToolBarItem::activated, console, &VuoConsole::clear);
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
