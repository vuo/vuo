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
	setWidth(270);

	VuoInputEditorWithLineEdit::setUpDialog(dialog, originalValue, details);
	lineEdit->installEventFilter(this);
	lineEdit->setAcceptDrops(true);

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

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorText::eventFilter(QObject *object, QEvent *event)
{
	// If the intercepted event is a drag-and-drop-related event destined for
	// the lineEdit, re-route it to the input editor itself so that it may be
	// intercepted by the window filtering input editor events.
	// The window has access to more of the information (e.g., the
	// composition storage directory) necessary to usefully handle the event.
	if ((object == lineEdit) &&
			((event->type() == QEvent::DragEnter) ||
			 (event->type() == QEvent::DragMove) ||
			 (event->type() == QEvent::DragLeave) ||
			 (event->type() == QEvent::Drop)))
	{
		QApplication::sendEvent(this, event);
		return true;
	}

	return VuoInputEditorWithLineEdit::eventFilter(object, event);
}

/**
 * Handles events for the input editor.
 */
bool VuoInputEditorText::event(QEvent *event)
{
	// Handle dropped data.
	if (event->type() == QEvent::Drop)
	{
		// This input editor will receive drop events originally intended for
		// the lineEdit, re-routed to allow the dropped data to be modified by
		// the window filtering events on the input editor, and then passed back
		// to the input editor. Here, we forward the modified drop event
		// to the lineEdit for which it was originally intended.

		// But first: If the lineEdit's full text was selected at the time
		// of the drop, delete the old text before inserting the new text.
		if (lineEdit->selectedText() == lineEdit->text())
			lineEdit->del();

		lineEdit->removeEventFilter(this);
		QApplication::sendEvent(lineEdit, event);
		lineEdit->installEventFilter(this);

		return true;
	}

	return VuoInputEditorWithLineEdit::event(event);
}
