/**
 * @file
 * VuoTitleEditor implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTitleEditor.hh"


/**
 * Convenience function for @c VuoInputEditorWithLineEdit::show(portLeftCenter, originalValue, details, portNamesAndValues) ,
 * useful for title editors since the @a portNamesAndValues does not apply.
 *
 * Returns a json_object with retain count +1; the caller is responsible for releasing it.
 */
json_object * VuoTitleEditor::show(QPoint portLeftCenter, json_object *originalValue, json_object *details)
{
	map<QString, json_object *> portNamesAndValues;
	json_object_object_add(details, "showArrow", json_object_new_boolean(false));
	return VuoInputEditorWithLineEdit::show(portLeftCenter, originalValue, details, portNamesAndValues);
}

/**
 * Removes quotation marks from the value to display in the line edit.
 */
QString VuoTitleEditor::convertToLineEditFormat(json_object *value)
{
	return json_object_get_string(value);
}

/**
 * Adds quotation marks around the value from the line edit.
 */
json_object * VuoTitleEditor::convertFromLineEditFormat(const QString &valueAsString)
{
	return json_object_new_string( lineEdit->text().toUtf8().constData() );
}
