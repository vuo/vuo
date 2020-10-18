/**
 * @file
 * VuoDialogWithoutTitlebar interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Hides the titlebar (but keeps the close button),
 * to resemble Xcode's About and Welcome dialogs.
 */
class VuoDialogWithoutTitlebar : public QDialog
{
	Q_OBJECT
public:
	explicit VuoDialogWithoutTitlebar(QWidget *parent = 0);

private:
	bool event(QEvent *event);
};
