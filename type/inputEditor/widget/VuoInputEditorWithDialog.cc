/**
 * @file
 * VuoInputEditorWithDialog implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorWithDialog.hh"
#include "VuoDialogForInputEditor.hh"

/**
 * Creates an input editor whose show() function displays a frameless dialog.
 */
VuoInputEditorWithDialog::VuoInputEditorWithDialog(void)
	: VuoInputEditor()
{
	dialogPointer = NULL;
	firstWidgetInTabOrder = NULL;
	lastWidgetInTabOrder = NULL;
}

/**
 * Displays a frameless dialog.
 *
 * Returns a json_object with retain count +1; the caller is responsible for releasing it.
 */
json_object * VuoInputEditorWithDialog::show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues)
{
	json_object *o = NULL;

	isDark = false;
	if (json_object_object_get_ex(details, "isDark", &o))
		isDark = json_object_get_boolean(o);

	bool showArrow = true;
	if (json_object_object_get_ex(details, "showArrow", &o))
		showArrow = json_object_get_boolean(o);

	VuoDialogForInputEditor dialog(isDark, showArrow);
	dialogPointer = &dialog;
	setUpDialog(dialog, originalValue, details);

	// Workaround for the window background being opaque black on macOS 10.12 and prior.
	// https://b33p.net/kosada/vuo/vuo/-/merge_requests/5#note_2117211
	dialog.setFixedSize(dialog.layout() ? dialog.layout()->sizeHint() : dialog.sizeHint());

	// Move children to account for margins if no autolayout is used.
	QMargins margin = dialog.getPopoverContentsMargins();

	if( dialog.layout() == 0 )
	{
		QPoint topLeftMargin = QPoint(margin.left(),margin.top());
		foreach (QObject *widget, dialog.children())
			static_cast<QWidget *>(widget)->move(static_cast<QWidget *>(widget)->pos() + topLeftMargin);
	}

	// Resize dialog to enclose child widgets and margins.
	dialog.adjustSize();

	// Position the right center of the dialog at the left center of the port, or wherever required
	// so that the entire dialog fits within the available screenspace.
	QPoint dialogTopLeft = portLeftCenter - QPoint(dialog.width() - (showArrow ? 0 : margin.right()), dialog.height()/2.);
	QScreen *dialogScreen = QGuiApplication::screenAt(portLeftCenter);
	if (dialogScreen)
	{
		int availableSpaceLeftBoundary = dialogScreen->availableGeometry().left();
		bool screenSpaceLimitsPos = (availableSpaceLeftBoundary > dialogTopLeft.x());
		if (screenSpaceLimitsPos)
		{
			dialogTopLeft = QPoint(availableSpaceLeftBoundary, dialogTopLeft.y());
			showArrow = false;
		}
	}
	dialog.setShowArrow(showArrow);
	dialog.move(dialogTopLeft);


	dialog.show();  // Needed to position the dialog. (https://bugreports.qt-project.org/browse/QTBUG-31406)
	dialog.exec();

	dialogPointer = NULL;

	return (dialog.result() == QDialog::Accepted ? getAcceptedValue() : json_object_get(originalValue));
}

/**
 * Returns a pointer to the dialog displayed by show(). This pointer is only valid during a call to show().
 */
QDialog * VuoInputEditorWithDialog::getDialog(void)
{
	return dialogPointer;
}

/**
 * Makes the given widget the first in this input editor's tab order.
 *
 * This function sets up the input editor to emit tabbedBackwardPastFirstWidget() signals.
 */
void VuoInputEditorWithDialog::setFirstWidgetInTabOrder(QWidget *widget)
{
	if (firstWidgetInTabOrder)
		firstWidgetInTabOrder->removeEventFilter(this);

	if (widget)
		widget->installEventFilter(this);

	firstWidgetInTabOrder = widget;
}

/**
 * Makes the given widget the last in this input editor's tab order.
 *
 * This function sets up the input editor to emit tabbedPastLastWidget() signals.
 */
void VuoInputEditorWithDialog::setLastWidgetInTabOrder(QWidget *widget)
{
	if (lastWidgetInTabOrder)
		lastWidgetInTabOrder->removeEventFilter(this);

	if (widget)
		widget->installEventFilter(this);

	lastWidgetInTabOrder = widget;
}

/**
 * Handles tabbing past the last widget or reverse-tabbing past the first widget in the tab order.
 */
bool VuoInputEditorWithDialog::eventFilter(QObject *object, QEvent *event)
{
	// If the input editor's final tab cycle element has received a 'Tab' keypress,
	// or its first tab cycle element has received a 'Shift'+'Tab' keypress,
	// emit the appropriate signal and then treat the 'Tab' as an 'Return' to accept and
	// close the input editor.
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		bool tabPressed = (keyEvent->key() == Qt::Key_Tab);
		bool shiftTabPressed = (keyEvent->key() == Qt::Key_Backtab);

		bool aboutToCompleteReverseTabCycle = ((object == firstWidgetInTabOrder) && shiftTabPressed);
		bool aboutToCompleteTabCycle = ((object == lastWidgetInTabOrder) && tabPressed);

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

	return VuoInputEditor::eventFilter(object, event);
}
