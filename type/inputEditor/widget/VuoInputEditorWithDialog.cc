/**
 * @file
 * VuoInputEditorWithDialog implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWithDialog.hh"
#include "VuoDialogForInputEditor.hh"

/**
 * Displays a frameless dialog.
 */
json_object * VuoInputEditorWithDialog::show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues)
{
	VuoDialogForInputEditor dialog;
	dialog.setWindowFlags(Qt::FramelessWindowHint);
	dialog.setFont(getDefaultFont());
	setUpDialog(dialog, originalValue, details);

	// Position the right center of the dialog at the left center of the port.
	QPoint dialogTopLeft = portLeftCenter - QPoint(dialog.childrenRect().width(), dialog.childrenRect().height()/2.);
	dialog.move(dialogTopLeft);

	dialog.show();  // Needed to position the dialog. (https://bugreports.qt-project.org/browse/QTBUG-31406)
	dialog.exec();

	return (dialog.result() == QDialog::Accepted ? getAcceptedValue() : originalValue);
}
