/**
 * @file
 * VuoMainWindow interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A document editor window.
 */
class VuoMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	QString getWindowTitleWithoutPlaceholder();
};
