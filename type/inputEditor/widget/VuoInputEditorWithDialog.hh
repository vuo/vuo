/**
 * @file
 * VuoInputEditorWithDialog interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
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
	QDialog * getDialog(void);

	/**
	 * Adds widgets to the dialog and configures them to display the dialog's initial value.
	 *
	 * @param dialog The dialog, which is initially empty.
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
};

#endif // VUOINPUTEDITORWITHDIALOG_HH
