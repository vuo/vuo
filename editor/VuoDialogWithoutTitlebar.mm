/**
 * @file
 * VuoDialogWithoutTitlebar implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDialogWithoutTitlebar.hh"

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#include <Cocoa/Cocoa.h>

/**
 * Creates a dialog.
 */
VuoDialogWithoutTitlebar::VuoDialogWithoutTitlebar(QWidget *parent) :
	QDialog(parent)
{
}

/**
 * Makes the dialog visible.
 */
void VuoDialogWithoutTitlebar::showEvent(QShowEvent *event)
{
	NSWindow *nsWindow = ((NSView *)winId()).window;
	nsWindow.titleVisibility = NSWindowTitleHidden;
	nsWindow.titlebarAppearsTransparent = YES;
	nsWindow.styleMask |= NSFullSizeContentViewWindowMask;
	nsWindow.styleMask &= ~NSResizableWindowMask;
}
