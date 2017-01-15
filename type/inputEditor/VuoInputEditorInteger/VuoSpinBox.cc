/**
 * @file
 * VuoSpinBox implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

	setMinimum(buttonMinimum);
	setMaximum(buttonMaximum);
}

/**
 * Steps the spin box value up or down, clamping it to be within the buttons' minimum and maximum.
 */
void VuoSpinBox::stepBy(int steps)
{
	int _value = value();
	int _singleStep = singleStep();

	if (_singleStep > 0)
	{
		if (_value + steps * _singleStep < buttonMinimum)
			steps = (buttonMinimum - _value) / _singleStep;
		else if (_value + steps * _singleStep > buttonMaximum)
			steps = (buttonMaximum - _value) / _singleStep;
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
