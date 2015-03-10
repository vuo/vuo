/**
 * @file
 * VuoInputEditorWithLineEdit interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	void setWidth(int width);
	bool supportsTabbingBetweenPorts(void);

protected:
	virtual void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details=0);
	void setUpLineEdit(QLineEdit *existingLineEdit, json_object *originalValue);
	void setFirstWidgetInTabOrder(QWidget *widget);
	void setLastWidgetInTabOrder(QWidget *widget);
	json_object * getAcceptedValue(void);
	virtual QString convertToLineEditFormat(json_object *value);
	virtual json_object * convertFromLineEditFormat(const QString &valueAsString);

	bool eventFilter(QObject *object, QEvent *event);

	QLineEdit *lineEdit;  ///< The text field widget.

private:
	int width;
	QWidget *firstWidgetInTabOrder;  ///< The first widget in this input editor's tab order.
	QWidget *lastWidgetInTabOrder;  ///< The last widget in this input editor's tab order.
};

#endif // VUOINPUTEDITORWITHLINEEDIT_HH
