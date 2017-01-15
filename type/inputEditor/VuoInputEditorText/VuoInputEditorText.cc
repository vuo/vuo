/**
 * @file
 * VuoInputEditorText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
 * Creates an input editor whose show() function displays a text edit.
 */
VuoInputEditorText::VuoInputEditorText(void)
	: VuoInputEditorWithDialog()
{
	textEdit = NULL;
	isCodeEditor = false;
}

/**
 * Sets up a dialog containing a text edit.
 * Removes the quotation marks surrounding @a originalValue before displaying it in the line edit.
 */
void VuoInputEditorText::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	if (details)
	{
		json_object *o = NULL;
		if (json_object_object_get_ex(details, "isCodeEditor", &o))
			isCodeEditor = json_object_get_boolean(o);
	}

	textEdit = new QPlainTextEdit(&dialog);

	textEdit->setPlainText( convertToTextEditFormat(originalValue) );
	textEdit->setFocus();

	if (isCodeEditor)
	{
		textEdit->setStyleSheet(textEdit->styleSheet() + "QPlainTextEdit { font-family: Monaco; font-size: 10pt; } ");
	}
	else
	{
		textEdit->setTabChangesFocus(true);
		setFirstWidgetInTabOrder(textEdit);
		setLastWidgetInTabOrder(textEdit);
	}

	resizeToFitText();  // Although the height may be adjusted later, the width needs to be set now so the dialog's position can be calculated.
	connect(textEdit, SIGNAL(textChanged()), this, SLOT(resizeToFitText()));
	textEdit->verticalScrollBar()->installEventFilter(this);

	textEdit->setAcceptDrops(true);
	textEdit->installEventFilter(this);
	textEdit->viewport()->installEventFilter(this);
}

/**
 * Returns the current text in the text edit.
 */
json_object * VuoInputEditorText::getAcceptedValue(void)
{
	return convertFromTextEditFormat(textEdit->toPlainText());
}

/**
 * Removes quotation marks from the value to display in the text edit.
 */
QString VuoInputEditorText::convertToTextEditFormat(json_object *value)
{
	return VuoText_makeFromJson(value);
}

/**
 * Adds quotation marks around the value from the text edit.
 */
json_object * VuoInputEditorText::convertFromTextEditFormat(const QString &valueAsString)
{
	return VuoText_getJson( valueAsString.toUtf8().constData() );
}

/**
 * Resizes the text edit and dialog to the height of the text.
 */
void VuoInputEditorText::resizeToFitText(void)
{
	int textEditWidth = (isCodeEditor ? 600 : 270);

	QTextDocument *document = textEdit->document();
	qreal margin = document->documentMargin() + textEdit->frameWidth();
	int textWidth = textEditWidth - 2*margin;
	document->setTextWidth(textWidth);
	QFontMetrics fm(textEdit->font());
	int textHeight = document->lineCount() * fm.lineSpacing();
	int textEditHeight = textHeight + 2*margin;

	QRect screenRect = QApplication::desktop()->availableGeometry(textEdit);
	QRect textEditRect(textEdit->mapToGlobal(textEdit->pos()), QSize(textEditWidth, textEditHeight));
	QRect textEditInScreenRect = textEditRect.intersected(screenRect);
	int minTextEditHeight = fm.lineSpacing() + 2*margin;
	textEditHeight = qMax(textEditInScreenRect.height(), minTextEditHeight);

	int scrollBarWidth = (textEdit->verticalScrollBar()->isVisible() ? textEdit->verticalScrollBar()->width() : 0);
	textEdit->viewport()->setFixedSize(textEditWidth, textEditHeight);
	textEdit->setFixedSize(textEditWidth + scrollBarWidth, textEditHeight);
	getDialog()->setFixedSize(getDialog()->sizeHint());  // adjustSize() limits to 2/3 of the screen's size (http://doc.qt.io/qt-5/qwidget.html#adjustSize)

	textEdit->ensureCursorVisible();
}

/**
 * Handles some events before they reach the text edit or related widgets.
 */
bool VuoInputEditorText::eventFilter(QObject *object, QEvent *event)
{
	// Handle the first part of drag-and-drop events.
	// If the intercepted event is a drag-and-drop-related event destined for
	// the text edit, re-route it to the input editor itself so that it may be
	// intercepted by the window filtering input editor events.
	// The window has access to more of the information (e.g., the
	// composition storage directory) necessary to usefully handle the event.
	if ((object == textEdit &&
			(event->type() == QEvent::DragEnter ||
			 event->type() == QEvent::DragMove ||
			 event->type() == QEvent::DragLeave)) ||
		(object == textEdit->viewport() &&
			(event->type() == QEvent::Drop)))   // drop events go to the viewport (https://bugreports.qt.io/browse/QTBUG-3748)
	{
		QApplication::sendEvent(this, event);
		return true;
	}

	// Handle Return and Option-Return key presses.
	// For regular text editors, this makes Return dismiss the input editor and Option-Return add a linebreak.
	// For code editors, this makes Return (or Option-Return) add a linebreak and Command-Return dismiss the input editor.
	if (object == textEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
		{
			bool optionKeyPressed = (keyEvent->modifiers() & Qt::AltModifier);
			bool commandKeyPressed = (keyEvent->modifiers() & Qt::ControlModifier);
			bool shouldInsertLinebreak = (isCodeEditor ?
											  ! commandKeyPressed :
											  optionKeyPressed);
			if (shouldInsertLinebreak)
				textEdit->textCursor().insertText("\n");
			else
				QApplication::sendEvent(textEdit->parent(), event);
			return true;
		}
	}

	// When the input editor is shown, perform some final setup.
	if (object == textEdit && event->type() == QEvent::Show)
	{
		// Set the tab width, which couldn't be done in setUpDialog() since textEdit->font() wasn't yet ready.
		QFontMetrics fm(textEdit->font());
		textEdit->setTabStopWidth(4 * fm.width("W"));

		// Resize the input editor if needed so that its height fits within the screen.
		resizeToFitText();

		if (textEdit->document()->lineCount() > 1)
		{
			// For multi-line text editors, set the cursor at the beginning and scroll to top.
			textEdit->moveCursor(QTextCursor::Start);
			textEdit->ensureCursorVisible();
		}
		else
		{
			// For one-line text editors, set the cursor at the end and select all.
			textEdit->selectAll();
		}

		return false;
	}

	// When the text edit gains or loses its vertical scroll bar, resize and reposition the input editor.
	if (object == textEdit->verticalScrollBar() && (event->type() == QEvent::Show || event->type() == QEvent::Hide))
	{
		int scrollBarWidth = textEdit->verticalScrollBar()->width();
		int offset = (event->type() == QEvent::Show ? -scrollBarWidth : scrollBarWidth);
		getDialog()->move( getDialog()->x() + offset, getDialog()->y() );
		resizeToFitText();
		return false;
	}

	return VuoInputEditorWithDialog::eventFilter(object, event);
}

/**
 * Handles some events received by the input editor.
 */
bool VuoInputEditorText::event(QEvent *event)
{
	if (event->type() == QEvent::Drop)
	{
		// Handle the second part of drag-and-drop events.

		// This input editor will receive drop events originally intended for
		// the text edit, re-routed to allow the dropped data to be modified by
		// the window filtering events on the input editor, and then passed back
		// to the input editor. Here, we forward the modified drop event
		// to the text edit for which it was originally intended.

		// But first: If the text edit's full text was selected at the time
		// of the drop, delete the old text before inserting the new text.
		QTextCursor cursor = textEdit->textCursor();
		if (cursor.selectionStart() == 0 && cursor.selectionEnd() == textEdit->toPlainText().length())
			textEdit->setPlainText("");

		textEdit->viewport()->removeEventFilter(this);
		QApplication::sendEvent(textEdit, event);
		textEdit->viewport()->installEventFilter(this);

		return true;
	}

	return VuoInputEditorWithDialog::event(event);
}

/**
 * Returns true.
 */
bool VuoInputEditorText::supportsTabbingBetweenPorts(void)
{
	return true;
}
