/**
 * @file
 * VuoDoubleSpinBox implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

	setMinimum(buttonMinimum);
	setMaximum(buttonMaximum);
}

/**
 * Steps the spin box value up or down, clamping it to be within the buttons' minimum and maximum.
 */
void VuoDoubleSpinBox::stepBy(int steps)
{
	double _value = value();
	double _singleStep = singleStep();

	if (_singleStep > 0)
	{
		if (_value + steps * _singleStep < buttonMinimum)
			steps = ceil( (buttonMinimum - _value) / _singleStep );
		else if (_value + steps * _singleStep > buttonMaximum)
			steps = floor( (buttonMaximum - _value) / _singleStep );
	}

	QDoubleSpinBox::stepBy(steps);
}

/**
 * Implementation of the virtual function used by the spin box
 * whenever it needs to display the given @c value.
 */
QString VuoDoubleSpinBox::textFromValue(double value) const
{
	QString newLineEditText = QLocale::system().toString(value, 'g');

	if (qAbs(value) >= 1000.0)
		newLineEditText.remove(QLocale::system().groupSeparator());

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
