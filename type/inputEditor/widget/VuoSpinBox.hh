/**
 * @file
 * VuoSpinBox interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A spin box where the buttons can have a different minimum and maximum than the line edit.
 */
class VuoSpinBox : public QSpinBox
{
public:
	VuoSpinBox(QWidget *parent = 0);
	void stepBy(int steps);
	void unsetLineEditBounds();
	void setButtonMinimum(int buttonMinimum);
	void setButtonMaximum(int buttonMaximum);

protected:
	QAbstractSpinBox::StepEnabled stepEnabled() const;
	virtual void hideEvent(QHideEvent * event);
	virtual void focusOutEvent(QFocusEvent * event);

private:
	int buttonMinimum;
	int buttonMaximum;
};
