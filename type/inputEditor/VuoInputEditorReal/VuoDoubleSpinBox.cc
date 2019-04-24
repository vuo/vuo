/**
 * @file
 * VuoDoubleSpinBox implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoDoubleSpinBox.hh"

/**
 * Creates a spin box where the buttons and line edit have no minimum and maximum.
 */
VuoDoubleSpinBox::VuoDoubleSpinBox(QWidget *parent)
	: QDoubleSpinBox(parent)
{
	buttonMinimum = -std::numeric_limits<double>::max();
	buttonMaximum = std::numeric_limits<double>::max();
	setKeyboardTracking(false);
	setMinimum(buttonMinimum);
	setMaximum(buttonMaximum);
}

/**
 * Determines whether the up and/or down arrows are enabled.
 */
QAbstractSpinBox::StepEnabled VuoDoubleSpinBox::stepEnabled() const
{
	double v = valueFromText(text());
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
void VuoDoubleSpinBox::stepBy(int steps)
{
	double _value = valueFromText(text());
	double _singleStep = singleStep();

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

	QDoubleSpinBox::stepBy(steps);
}

/**
 * Implementation of the virtual function used by the spin box
 * whenever it needs to display the given @c value.
 */
QString VuoDoubleSpinBox::textFromValue(double value) const
{
	QString newLineEditText = QLocale().toString(value, 'g');

	if (qAbs(value) >= 1000.0)
		newLineEditText.remove(QLocale().groupSeparator());

	return newLineEditText;
}

/**
 * Sets the minimum value that can be entered using the buttons.
 */
void VuoDoubleSpinBox::setButtonMinimum(double buttonMinimum)
{
	this->buttonMinimum = buttonMinimum;
}

/**
 * Sets the maximum value that can be entered using the buttons.
 */
void VuoDoubleSpinBox::setButtonMaximum(double buttonMaximum)
{
	this->buttonMaximum = buttonMaximum;
}

/**
 * Convert an integer range value to a double value.
 */
double VuoDoubleSpinBox::sliderToDouble(int sliderMin, int sliderMax, double valueMin, double valueMax, int value)
{
	double normalized = ((double)value - sliderMin) / (sliderMax - sliderMin);
	return valueMin + (normalized * (valueMax - valueMin));
}

/**
 * Convert a double value to an integer range value.
 */
int VuoDoubleSpinBox::doubleToSlider(int sliderMin, int sliderMax, double valueMin, double valueMax, double value)
{
	double normalized = (value - valueMin) / (valueMax - valueMin);
	int rounded = (int) (normalized * (sliderMax - sliderMin));
	return sliderMin + rounded;
}

/**
 * Don't emit value changed events when focusing out due to window close, otherwise Vuo will update and quickly revert
 * the value.
 */
void VuoDoubleSpinBox::focusOutEvent(QFocusEvent * event)
{
	Qt::FocusReason reason = event->reason();

	// if tabbing lost focus, keep the value
	if( reason == Qt::MouseFocusReason ||
		reason == Qt::TabFocusReason ||
		reason == Qt::BacktabFocusReason )
		QDoubleSpinBox::focusOutEvent(event);
}

/**
 * Don't emit value changed events when hiding event, otherwise Vuo will update and quickly revert
 * the value.
 */
void VuoDoubleSpinBox::hideEvent(QHideEvent * event)
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
		QDoubleSpinBox::hideEvent(event);
}


