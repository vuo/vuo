/**
 * @file
 * VuoCommentEditor implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommentEditor.hh"

extern "C"
{
	#include "VuoText.h"
}

const int VuoCommentEditor::commentMargin = 15; ///<  Leave some space for the comment border to show behind the text editor.

/**
 * Creates an input editor whose show() function displays a text edit.
 */
VuoCommentEditor::VuoCommentEditor(void)
	: VuoTextEditor()
{
	width = 0;
	height = 0;
}

/**
 * Convenience function for @c VuoInputEditorWithDialog::show(portLeftCenter, originalValue, details, portNamesAndValues) ,
 * useful for comment editors since the @a portNamesAndValues does not apply.
 *
 * Returns a json_object with retain count +1; the caller is responsible for releasing it.
 */
json_object * VuoCommentEditor::show(QPoint portLeftCenter, json_object *originalValue, json_object *details)
{
	map<QString, json_object *> portNamesAndValues;
	json_object_object_add(details, "showArrow", json_object_new_boolean(false));
	return VuoInputEditorWithDialog::show(portLeftCenter-QPoint(commentMargin,0), originalValue, details, portNamesAndValues);
}

/**
 * Resizes the text edit and dialog to the height of the text.
 */
void VuoCommentEditor::resizeToFitText(void)
{
	VuoTextEditor::resizeToFitTextWithBaseline(width-2*commentMargin, height-2*commentMargin);
}

/**
 * Sets the width of the comment editor.
 */
void VuoCommentEditor::setWidth(int width)
{
	this->width = width;
}

/**
 * Sets the height of the comment editor.
 */
void VuoCommentEditor::setHeight(int height)
{
	this->height = height;
}

/**
 * Returns a boolean indicating whether this comment editor is in code-editing mode,
 * given an optional details object.
 */
bool VuoCommentEditor::getCodeEditor(json_object *details)
{
	return false;
}

/**
 * Returns false.
 */
bool VuoCommentEditor::supportsTabbingBetweenPorts(void)
{
	return false;
}
