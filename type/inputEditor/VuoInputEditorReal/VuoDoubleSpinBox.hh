/**
 * @file
 * VuoDoubleSpinBox interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODOUBLESPINBOX_HH
#define VUODOUBLESPINBOX_HH

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

private:
	double buttonMinimum;
	double buttonMaximum;
};

#endif // VUODOUBLESPINBOX_HH
