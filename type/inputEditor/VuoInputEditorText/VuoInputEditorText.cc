/**
 * @file
 * VuoInputEditorText implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorText.hh"
#include "VuoInputEditorWithLineEdit.hh"

extern "C"
{
	#include "VuoText.h"
}

/**
 * Constructs a VuoInputEditorText object.
 */
VuoInputEditor * VuoInputEditorTextFactory::newInputEditor()
{
	return new VuoInputEditorText();
}

/**
 * Sets up a dialog containing a line edit and a (possibly empty) warning message.
 * Removes the quotation marks surrounding @c originalValue before displaying it in the line edit.
 */
void VuoInputEditorText::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	VuoInputEditorWithLineEdit::setUpDialog(dialog, originalValue, details);

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "suggestedMinLength" (currently unused)
		//json_object *suggestedMinLengthValue = NULL;
		//if (json_object_object_get_ex(details, "suggestedMinLength", &suggestedMinLengthValue))
		//	int suggestedMinLength = json_object_get_int(suggestedMinLengthValue);

		// "suggestedMaxLength"
		json_object *suggestedMaxLengthValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMaxLength", &suggestedMaxLengthValue))
		{
			int suggestedMaxLength= json_object_get_int(suggestedMaxLengthValue);
			lineEdit->setMaxLength(suggestedMaxLength);
		}
	}
}

/**
 * Removes quotation marks from the value to display in the line edit.
 */
QString VuoInputEditorText::convertToLineEditFormat(json_object *value)
{
	return VuoText_valueFromJson(value);
}

/**
 * Adds quotation marks around the value from the line edit.
 */
json_object * VuoInputEditorText::convertFromLineEditFormat(const QString &valueAsString)
{
	return VuoText_jsonFromValue( lineEdit->text().toUtf8().constData() );
}
