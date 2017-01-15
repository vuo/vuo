/**
 * @file
 * VuoInputEditorWithLineEdit interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORWITHLINEEDIT_HH
#define VUOINPUTEDITORWITHLINEEDIT_HH

#include "VuoInputEditorWithDialog.hh"

/**
 * A base class for input editors that display a line edit (text field).
 */
class VuoInputEditorWithLineEdit : public VuoInputEditorWithDialog
{
public:
	VuoInputEditorWithLineEdit(void);
	virtual bool supportsTabbingBetweenPorts(void);
	void setWidth(int width);

protected:
	virtual void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details=0);
	void setUpLineEdit(QLineEdit *existingLineEdit, json_object *originalValue);
	json_object * getAcceptedValue(void);
	virtual QString convertToLineEditFormat(json_object *value);
	virtual json_object * convertFromLineEditFormat(const QString &valueAsString);

	QLineEdit *lineEdit;  ///< The text field widget.

private:
	int width;
};

#endif // VUOINPUTEDITORWITHLINEEDIT_HH
