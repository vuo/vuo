/**
 * @file
 * VuoSubcompositionSaveAsDialog interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

namespace Ui {
class VuoSubcompositionSaveAsDialog;
}

/**
 * A widget for the user to install a subcomposition under a provided name.
 */
class VuoSubcompositionSaveAsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VuoSubcompositionSaveAsDialog(QWidget *parent, Qt::WindowFlags flags,  QString operationTitle,
										   QString defaultNodeTitle, QString defaultNodeCategory,
										   QString currentNodeTitle, QString currentNodeCategory);
	~VuoSubcompositionSaveAsDialog();
	QString nodeTitle();
	QString nodeCategory();

private:
	Ui::VuoSubcompositionSaveAsDialog *ui;

private slots:
	void updateNodeClassName();
};
