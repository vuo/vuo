/**
 * @file
 * VuoSubcompositionSaveAsDialog implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSubcompositionSaveAsDialog.hh"
#include "ui_VuoSubcompositionSaveAsDialog.h"
#include "VuoEditorWindow.hh"

/**
 * Creates a new widget for the user to install a subcomposition under a provided name.
 */
VuoSubcompositionSaveAsDialog::VuoSubcompositionSaveAsDialog(QWidget *parent, Qt::WindowFlags flags, QString operationTitle,
															 QString defaultNodeTitle, QString defaultNodeCategory,
															 QString currentNodeTitle, QString currentNodeCategory) :
	QDialog(parent, flags),
	ui(new Ui::VuoSubcompositionSaveAsDialog)
{
	ui->setupUi(this);
	ui->titleLabel->setText(operationTitle);

	QWidget::setTabOrder(ui->nodeTitleLineEdit, ui->nodeCategoryLineEdit);
	QWidget::setTabOrder(ui->nodeCategoryLineEdit, ui->buttonBox);

	connect(ui->nodeTitleLineEdit, &QLineEdit::textChanged, this, &VuoSubcompositionSaveAsDialog::updateNodeClassName);
	connect(ui->nodeCategoryLineEdit, &QLineEdit::textChanged, this, &VuoSubcompositionSaveAsDialog::updateNodeClassName);

	ui->nodeTitleLineEdit->setPlaceholderText(defaultNodeTitle);
	ui->nodeCategoryLineEdit->setPlaceholderText(defaultNodeCategory);

	ui->nodeTitleLineEdit->setText(currentNodeTitle);
	ui->nodeCategoryLineEdit->setText(currentNodeCategory);

	ui->nodeTitleLineEdit->selectAll();
}

/**
 * Returns the subcomposition title.
 */
QString VuoSubcompositionSaveAsDialog::nodeTitle()
{
    return ui->nodeTitleLineEdit->text();
}

/**
 * Returns the subcomposition category.
 */
QString VuoSubcompositionSaveAsDialog::nodeCategory()
{
    return ui->nodeCategoryLineEdit->text();
}

void VuoSubcompositionSaveAsDialog::updateNodeClassName()
{
	//: Appears in the dialog shown when creating a subcomposition.
	ui->nodeClassNameLabel->setText(tr("Node Class") + ": "
									+ VuoEditorWindow::getNodeClassNameForDisplayNameAndCategory(nodeTitle(),
																									   nodeCategory(),
																									   ui->nodeTitleLineEdit->placeholderText(),
																									   ui->nodeCategoryLineEdit->placeholderText()));
}

VuoSubcompositionSaveAsDialog::~VuoSubcompositionSaveAsDialog()
{
	delete ui;
}
