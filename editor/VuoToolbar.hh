/**
 * @file
 * VuoToolbar interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/// @{ Stub.
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
Q_FORWARD_DECLARE_OBJC_CLASS(NSTextField);
Q_FORWARD_DECLARE_OBJC_CLASS(NSTitlebarAccessoryViewController);
Q_FORWARD_DECLARE_OBJC_CLASS(NSWindow);
/// @}

/**
 * Base class for window toolbars that respect the Show/Hide Toolbar Labels setting.
 */
class VuoToolbar : public QObject
{
	Q_OBJECT

protected:
	explicit VuoToolbar(QMainWindow *window);
	void setUp(void);

	/**
	 * Adds buttons and separators to the toolbar.
	 */
	virtual void addToolbarItems(void) = 0;

	/**
	 * True if the window containing this toolbar can be tabbed together with other windows.
	 */
	virtual bool allowsTabbingWithOtherWindows(void) = 0;

	/**
	 * Only used if VuoToolbar::allowsTabbingWithOtherWindows() returns true: an identifier for grouping related windows.
	 */
	virtual NSString * getTabbingIdentifier(void) = 0;

	virtual void updateColor(bool isDark);

	QMainWindow *window;  ///< The window that contains the toolbar.
	QMacToolBar *qtToolbar;  ///< The actual toolbar widget.

	NSWindow *nsWindow;  ///< The native window (derived from `window`) that contains the toolbar.

private:
	NSTextField *titleView;
	NSTitlebarAccessoryViewController *titleViewController;

#ifdef VUO_PRO
#include "pro/VuoToolbar_Pro.hh"
#endif

};
