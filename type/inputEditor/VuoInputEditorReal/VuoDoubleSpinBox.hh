/**
 * @file
 * VuoDoubleSpinBox interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * A spin box where the buttons can have a different minimum and maximum than the line edit.
 */
class VuoDoubleSpinBox : public QDoubleSpinBox
{
public:
	VuoDoubleSpinBox(QWidget *parent = 0);
	void stepBy(int steps);
	QString textFromValue(double value) const;
	void setButtonMinimum(double buttonMinimum);
	void setButtonMaximum(double buttonMaximum);

	static double sliderToDouble(int sliderMin, int sliderMax, double valueMin, double valueMax, int value);
	static int doubleToSlider(int sliderMin, int sliderMax, double valueMin, double valueMax, double value);

protected:
	QAbstractSpinBox::StepEnabled stepEnabled() const;
	virtual void hideEvent(QHideEvent * event);
	virtual void focusOutEvent(QFocusEvent * event);

private:
	double buttonMinimum;
	double buttonMaximum;
};

