/**
 * @file
 * VuoDialogForInputEditor implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoDialogForInputEditor.hh"

/**
 * If the enter key is pressed, closes the dialog.
 */
void VuoDialogForInputEditor::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
		accept();
	}

	QDialog::keyPressEvent(e);
}

/**
 * If focus is lost (the user has clicked elsewhere), closes the dialog.
 */
bool VuoDialogForInputEditor::event(QEvent *e)
{
	if (e->type() == QEvent::WindowDeactivate)
	{
		accept();
		return true;
	}

	return QDialog::event(e);
}
