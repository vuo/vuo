/**
 * @file
 * VuoSpinBox implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSpinBox.hh"

/**
 * Creates a spin box where the buttons and line edit have no minimum and maximum.
 */
VuoSpinBox::VuoSpinBox(QWidget *parent)
	: QSpinBox(parent)
{
	buttonMinimum = INT_MIN;
	buttonMaximum = INT_MAX;

	setKeyboardTracking(false);
	setMinimum(buttonMinimum);
	setMaximum(buttonMaximum);
}

/**
 * Determines whether the up and/or down arrows are enabled.
 */
QAbstractSpinBox::StepEnabled VuoSpinBox::stepEnabled() const
{
	int v = valueFromText(text());
	QAbstractSpinBox::StepEnabled e = 0;
	if (v > buttonMinimum)
		e |= StepDownEnabled;
	if (v < buttonMaximum)
		e |= StepUpEnabled;
	return e;
}

/**
 * Steps the spin box value up or down, clamping it to be within the buttons' minimum and maximum.
 */
void VuoSpinBox::stepBy(int steps)
{
	int _value = valueFromText(text());
	int _singleStep = singleStep();

	if (_singleStep > 0)
	{
		if (_value + steps * _singleStep < buttonMinimum)
		{
			setValue(buttonMinimum);
			return;
		}
		else if (_value + steps * _singleStep > buttonMaximum)
		{
			setValue(buttonMaximum);
			return;
		}
	}

	QSpinBox::stepBy(steps);
}

/**
 * Sets the minimum value that can be entered using the buttons.
 */
void VuoSpinBox::setButtonMinimum(int buttonMinimum)
{
	this->buttonMinimum = buttonMinimum;
}

/**
 * Sets the maximum value that can be entered using the buttons.
 */
void VuoSpinBox::setButtonMaximum(int buttonMaximum)
{
	this->buttonMaximum = buttonMaximum;
}

/**
 * Don't emit value changed events when focusing out due to window close, otherwise Vuo will update and quickly revert
 * the value.
 */
void VuoSpinBox::focusOutEvent(QFocusEvent * event)
{
	Qt::FocusReason reason = event->reason();

	// if tabbing lost focus, keep the value
	if( reason == Qt::MouseFocusReason ||
		reason == Qt::TabFocusReason ||
		reason == Qt::BacktabFocusReason )
		QSpinBox::focusOutEvent(event);
}

/**
 * Don't emit value changed events when hiding event, otherwise Vuo will update and quickly revert
 * the value.
 */
void VuoSpinBox::hideEvent(QHideEvent * event)
{
	// If hideEvent is called while the window still has focus
	// that means the mouse clicked outside the window and we
	// should commit the changes.
	//
	// This is necessary because in focusOutEvent the FocusReason
	// does not distinguish the difference between clicking
	// outside a window and hitting Escape to exit - both
	// return "ActiveWindowFocusReason"

	if(hasFocus())
		QSpinBox::hideEvent(event);
}
