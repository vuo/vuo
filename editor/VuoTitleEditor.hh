/**
 * @file
 * VuoTitleEditor interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithLineEdit.hh"

/**
 * A line edit that pops up in its own dialog and lets the user edit some text.
 */
class VuoTitleEditor : public VuoInputEditorWithLineEdit
{
public:
	using VuoInputEditorWithLineEdit::show;
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details);

protected:
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromLineEditFormat(const QString &valueAsString);
};

