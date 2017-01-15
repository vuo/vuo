/**
 * @file
 * VuoInputEditorWithLineEdit implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWithLineEdit.hh"

/**
 * Creates an input editor whose @c show function displays a line edit.
 */
VuoInputEditorWithLineEdit::VuoInputEditorWithLineEdit(void)
	: VuoInputEditorWithDialog()
{
	lineEdit = NULL;
	width = 0;
}

/**
 * Adds a line edit widget to the dialog.
 *
 * Sets this line edit as the first and last widget in this input editor's tab order.
 */
void VuoInputEditorWithLineEdit::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	setUpLineEdit(new QLineEdit(&dialog), originalValue);
}

/**
 * Configures the provided line edit to display the dialog's initial value.
 *
 * Sets the line edit as the first and last widget in this input editor's tab order.
 *
 * @param existingLineEdit The already-initialized QLineEdit widget to configure for the dialog.
 * @param originalValue The value to display initially in the dialog.
 */
void VuoInputEditorWithLineEdit::setUpLineEdit(QLineEdit *existingLineEdit, json_object *originalValue)
{
	lineEdit = existingLineEdit;
	lineEdit->setText( convertToLineEditFormat(originalValue) );
	lineEdit->setFocus();
	lineEdit->selectAll();

	lineEdit->adjustSize();
	if (width > 0)
		lineEdit->resize(width, lineEdit->height());

	setFirstWidgetInTabOrder(lineEdit);
	setLastWidgetInTabOrder(lineEdit);
}

/**
 * Returns the current text in the line edit.
 */
json_object * VuoInputEditorWithLineEdit::getAcceptedValue(void)
{
	return convertFromLineEditFormat(lineEdit->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorWithLineEdit::convertToLineEditFormat(json_object *value)
{
	return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
}

/**
 * Returns the value represented when the given text appears in the line edit.
 */
json_object * VuoInputEditorWithLineEdit::convertFromLineEditFormat(const QString &valueAsString)
{
	return json_tokener_parse(valueAsString.toUtf8().constData());
}

/**
 * Sets the width of the line edit.
 */
void VuoInputEditorWithLineEdit::setWidth(int width)
{
	this->width = width;
}

/**
 * Returns true.
 */
bool VuoInputEditorWithLineEdit::supportsTabbingBetweenPorts(void)
{
	return true;
}
