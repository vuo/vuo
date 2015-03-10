/**
 * @file
 * VuoInputEditorWithLineEdit implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	this->width = 0;
}

/**
 * Adds a line edit widget to the dialog.
 */
void VuoInputEditorWithLineEdit::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	setUpLineEdit(new QLineEdit(&dialog), originalValue);
}

/**
 * Configures the provided line edit to display the dialog's initial value.
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
 * Makes the given widget the first in this input editor's tab order.
 *
 * By default, the first widget is the line edit passed to setUpLineEdit().
 * Any call to setUpLineEdit() or setUpDialog() reverts the first widget to the default.
 */
void VuoInputEditorWithLineEdit::setFirstWidgetInTabOrder(QWidget *widget)
{
	widget->installEventFilter(this);
	this->firstWidgetInTabOrder = widget;
}

/**
 * Makes the given widget the last in this input editor's tab order.
 *
 * By default, the last widget is the line edit passed to setUpLineEdit().
 * Any call to setUpLineEdit() or setUpDialog() reverts the last widget to the default.
 */
void VuoInputEditorWithLineEdit::setLastWidgetInTabOrder(QWidget *widget)
{
	widget->installEventFilter(this);
	this->lastWidgetInTabOrder = widget;
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
 * Filters events on watched objects.
 */
bool VuoInputEditorWithLineEdit::eventFilter(QObject *object, QEvent *event)
{
	// If the input editor's final tab cycle element has received a 'Tab' keypress,
	// or its first tab cycle element has received a 'Shift'+'Tab' keypress,
	// emit the appropriate signal and then treat the 'Tab' as an 'Return' to accept and
	// close the input editor.
	if (event->type()==QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		bool tabPressed = (keyEvent->key() == Qt::Key_Tab);
		bool shiftTabPressed = (keyEvent->key() == Qt::Key_Backtab);

		bool aboutToCompleteReverseTabCycle = ((object==firstWidgetInTabOrder) && shiftTabPressed);
		bool aboutToCompleteTabCycle = ((object==lastWidgetInTabOrder) && tabPressed);

		if (aboutToCompleteReverseTabCycle || aboutToCompleteTabCycle)
		{
			QKeyEvent modifiedKeyEvent(event->type(), Qt::Key_Return, 0);
			QApplication::sendEvent(object, &modifiedKeyEvent);

			if (aboutToCompleteReverseTabCycle)
			{
				emit valueChanged(getAcceptedValue());
				emit tabbedBackwardPastFirstWidget();
			}

			else // if (aboutToCompleteTabCycle)
			{
				emit valueChanged(getAcceptedValue());
				emit tabbedPastLastWidget();
			}

			return true;
		}
	}

	return QObject::eventFilter(object, event);
}

/**
 * Returns a boolean indicating whether this input editor emits @c tabbedPastLastWidget() and
 * @c tabbedBackwardPastFirstWidget() signals when appropriate.
 */
bool VuoInputEditorWithLineEdit::supportsTabbingBetweenPorts(void)
{
	return true;
}
