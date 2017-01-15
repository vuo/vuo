/**
 * @file
 * VuoInputEditorWithDialog interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORWITHDIALOG_HH
#define VUOINPUTEDITORWITHDIALOG_HH

#include "VuoInputEditor.hh"

/**
 * A base class for input editors that display a frameless dialog.
 */
class VuoInputEditorWithDialog : public VuoInputEditor
{
public:
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues);

protected:
	VuoInputEditorWithDialog(void);
	QDialog * getDialog(void);
	void setFirstWidgetInTabOrder(QWidget *widget);
	void setLastWidgetInTabOrder(QWidget *widget);
	bool eventFilter(QObject *object, QEvent *event);

	/**
	 * Adds widgets to the dialog and configures them to display the dialog's initial value.
	 *
	 * @param dialog The dialog, which is initially empty. It has a style sheet that makes the dialog and any
	 *		widgets added to it default to a standard style, which depends on the Vuo Editor's "Dark Interface"
	 *		setting. If changing fonts of widgest, you may need to use `QWidget::setStyleSheet()` instead of
	 *		`QWidget::setFont()`, since the latter may have no effect (http://doc.qt.io/qt-5/qwidget.html#font-prop).
	 * @param originalValue The value to display initially in the dialog.
	 * @param details Additional details (e.g., suggested min. and max. values) pertaining to the port data.
	 */
	virtual void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details) = 0;

	/**
	 * Returns the value currently set in the dialog's widgets.
	 */
	virtual json_object * getAcceptedValue(void) = 0;

private:
	QDialog *dialogPointer;
	QWidget *firstWidgetInTabOrder;
	QWidget *lastWidgetInTabOrder;
};

#endif // VUOINPUTEDITORWITHDIALOG_HH
