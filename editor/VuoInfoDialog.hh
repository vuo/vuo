/**
 * @file
 * VuoInfoDialog interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Displays a dialog to give the user a neutral (non-error) message, which the user can opt not to see again.
 */
class VuoInfoDialog : public QMessageBox
{
	Q_OBJECT

public:
	VuoInfoDialog(QWidget *parent, QString summary, QString details, QString checkboxLabel, QString settingsKey);
	void show();

signals:
	void notShown();  ///< Fired when show() is called, but the user previously unchecked the box to show it.

private:
	QString summary;
	QString details;
	QString checkboxLabel;
	QString settingsKey;
};
