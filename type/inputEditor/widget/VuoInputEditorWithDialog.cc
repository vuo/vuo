/**
 * @file
 * VuoInputEditorWithDialog implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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
	json_object *o = NULL;

	bool isDark = false;
	if (json_object_object_get_ex(details, "isDark", &o))
		isDark = json_object_get_boolean(o);

	bool showArrow = true;
	if (json_object_object_get_ex(details, "showArrow", &o))
		showArrow = json_object_get_boolean(o);


	VuoDialogForInputEditor dialog(isDark, showArrow);
	dialogPointer = &dialog;
	dialog.setFont(getDefaultFont());
	setUpDialog(dialog, originalValue, details);

	// Move children to account for margins.
	QMargins margin = dialog.getPopoverContentsMargins();
	QPoint topLeftMargin = QPoint(margin.left(),margin.top());
	foreach (QObject *widget, dialog.children())
		static_cast<QWidget *>(widget)->move(static_cast<QWidget *>(widget)->pos() + topLeftMargin);

	// Resize dialog to enclose child widgets and margins.
	dialog.resize(margin.left() + dialog.childrenRect().width() + margin.right(), margin.top() + dialog.childrenRect().height() + margin.bottom());

	// Position the right center of the dialog at the left center of the port.
	QPoint dialogTopLeft = portLeftCenter - QPoint(dialog.width() - (showArrow ? 0 : margin.right()), dialog.height()/2.);
	dialog.move(dialogTopLeft);

	dialog.show();  // Needed to position the dialog. (https://bugreports.qt-project.org/browse/QTBUG-31406)
	dialog.exec();

	dialogPointer = NULL;

	return (dialog.result() == QDialog::Accepted ? getAcceptedValue() : originalValue);
}

/**
 * Returns a pointer to the dialog displayed by show(). This pointer is only valid during a call to show().
 */
QDialog * VuoInputEditorWithDialog::getDialog(void)
{
	return dialogPointer;
}
