/**
 * @file
 * VuoSpinBox interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSPINBOX_HH
#define VUOSPINBOX_HH

/**
 * A spin box where the buttons can have a different minimum and maximum than the line edit.
 */
class VuoSpinBox : public QSpinBox
{
public:
	VuoSpinBox(QWidget *parent = 0);
	void stepBy(int steps);
	void setButtonMinimum(int buttonMinimum);
	void setButtonMaximum(int buttonMaximum);

private:
	int buttonMinimum;
	int buttonMaximum;
};

#endif // VUOSPINBOX_HH
