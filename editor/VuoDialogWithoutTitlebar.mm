/**
 * @file
 * VuoDialogWithoutTitlebar implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDialogWithoutTitlebar.hh"

#include "VuoMacOSSDKWorkaround.h"
#include <AppKit/AppKit.h>

/**
 * Creates a dialog.
 */
VuoDialogWithoutTitlebar::VuoDialogWithoutTitlebar(QWidget *parent) :
	QDialog(parent)
{
}

/**
 * Hides the titlebar (but keeps the close button),
 * to resemble Xcode's About and Welcome dialogs.
 */
bool VuoDialogWithoutTitlebar::event(QEvent *event)
{
	if (event->type() == QEvent::WinIdChange)
	{
		NSWindow *nsWindow = ((NSView *)winId()).window;
		nsWindow.titleVisibility = NSWindowTitleHidden;
		nsWindow.titlebarAppearsTransparent = YES;
		nsWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
		nsWindow.styleMask &= ~NSWindowStyleMaskResizable;
	}

	return QDialog::event(event);
}
