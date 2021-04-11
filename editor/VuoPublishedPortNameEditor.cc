/**
 * @file
 * VuoPublishedPortNameEditor implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPublishedPortNameEditor.hh"
#include "VuoRendererPort.hh"

/**
 * Adds a line edit widget to the dialog and assigns it a validator to enforce
 * published port name requirements.
 */
void VuoPublishedPortNameEditor::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	this->originalValue = originalValue;

	setUpLineEdit(new QLineEdit(&dialog), originalValue);

	// The user-entered published port name must be a valid port identifier.
	QString portIdentifierRegExp = VuoRendererPort::getPortNameRegExp();

	// Exception: Allow the published port name temporarily to be empty during editing.
	// The empty string will be rejected upon submission (in VuoPublishedPortNameEditor::getAcceptedValue()).
	QString emptyStringRegExp("^$");
	QString portIdentifierOrEmptyStringRegExp = portIdentifierRegExp
													   .append("|")
													   .append(emptyStringRegExp);

	QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression(portIdentifierOrEmptyStringRegExp), this);
	lineEdit->setValidator(validator);
}

/**
 * Returns the current text in the line edit.
 * If the text is empty, returns the original value instead.
 */
json_object * VuoPublishedPortNameEditor::getAcceptedValue(void)
{
	QString currentText = lineEdit->text();
	if (!currentText.isEmpty())
		return convertFromLineEditFormat(lineEdit->text());

	return originalValue;
}
