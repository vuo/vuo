/**
 * @file
 * VuoMainWindow implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMainWindow.hh"

/**
 * Returns the window title, minus the `[*]` window-modified placeholder.
 */
QString VuoMainWindow::getWindowTitleWithoutPlaceholder()
{
	return windowTitle().remove("[*]");
}
