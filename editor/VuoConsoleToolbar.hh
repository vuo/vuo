/**
 * @file
 * VuoConsoleToolbar interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoToolbar.hh"

class VuoConsole;

/**
 * Toolbar for the console window.
 */
class VuoConsoleToolbar : VuoToolbar
{
	Q_OBJECT

public:
	static VuoConsoleToolbar * create(QMainWindow *window, VuoConsole *console);

protected:
	VuoConsoleToolbar(QMainWindow *window, VuoConsole *console);
	virtual void addToolbarItems(void) override;
	virtual bool allowsTabbingWithOtherWindows(void) override;
	virtual NSString * getTabbingIdentifier(void) override;

private:
	VuoConsole *console;

#ifdef VUO_PRO
#include "pro/VuoConsoleToolbar_Pro.hh"
#endif
};
