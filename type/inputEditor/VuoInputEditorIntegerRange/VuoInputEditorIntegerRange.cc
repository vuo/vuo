/**
 * @file
 * VuoInputEditorIntegerRange implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoInputEditorIntegerRange.hh"
#include <stdio.h>

///< Tests if an integer value is within bounds of -INT_MAX and INT_MAX.  VuoInteger spinbox only accepts int32, not int64, so if a value is INT_MAX it's "infinity".
#define IS_FINITE(value) (abs(value) != INT_MAX)

/**
 * Constructs a VuoInputEditorIntegerRange object.
 */
VuoInputEditor * VuoInputEditorIntegerRangeFactory::newInputEditor()
{
	return new VuoInputEditorIntegerRange();
}

/**
 * Applies settings to a VuoSpinBox
 */
void VuoInputEditorIntegerRange::setupSpinBox(VuoSpinBox* spin, int min, int max, int step, int value)
{
//	spin->setButtonMinimum(min);
//	spin->setButtonMaximum(max);
	spin->setMinimum( -INT_MAX );
	spin->setMaximum(  INT_MAX );
	spin->setSingleStep(step);
	spin->setValue(value);
}

/**
 * Try to read in suggested details and return true if min/max are both found, false otherwise (though there still may be some usable data read).
 */
bool getDetails(json_object* details, VuoIntegerRange* suggestedMin, VuoIntegerRange* suggestedMax, VuoIntegerRange* suggestedStep)
{
	bool hasMin = false, hasMax = false;

	if (details != NULL)
	{
		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			*suggestedMin = VuoIntegerRange_makeFromJson(suggestedMinValue);
			hasMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			*suggestedMax = VuoIntegerRange_makeFromJson(suggestedMaxValue);
			hasMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			*suggestedStep = VuoIntegerRange_makeFromJson(suggestedStepValue);
	}

	return hasMin && hasMax;
}

/**
 * Prepare a VuoIntegerRange dialog.
 */
void VuoInputEditorIntegerRange::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	suggestedMin = 	VuoIntegerRange_make( -INT_MAX, -INT_MAX );
	suggestedMax = 	VuoIntegerRange_make( INT_MAX, INT_MAX );
	suggestedStep =	VuoIntegerRange_make(1, 1);

	// get suggestedMin/Max/Step if provided, otherwise use the defaults
	bool hasMinMax = getDetails(details, &suggestedMin, &suggestedMax, &suggestedStep);

	bool tabCycleForward = true;

	if(details)
	{
		json_object *forwardTabTraversal = NULL;
		if (json_object_object_get_ex(details, "forwardTabTraversal", &forwardTabTraversal))
			tabCycleForward = json_object_get_boolean(forwardTabTraversal);
	}

	QGridLayout* layout = new QGridLayout;

	// left, top, right, bottom
	// when showing sliders, add a little extra margin on the bottom since QSlider takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 6, 12, hasMinMax ? 6 : 4);
	layout->setSpacing(8);

	currentValue = VuoIntegerRange_makeFromJson(originalValue);
	// Covnert from int64 to int32.
	if (currentValue.minimum == VuoIntegerRange_NoMinimum)
		currentValue.minimum = -INT_MAX;
	if (currentValue.maximum == VuoIntegerRange_NoMaximum)
		currentValue.maximum = INT_MAX;

	lastValue.minimum = IS_FINITE(currentValue.minimum) ? currentValue.minimum : ( IS_FINITE(suggestedMin.minimum) ? suggestedMin.minimum :  0 );
	lastValue.maximum = IS_FINITE(currentValue.maximum) ? currentValue.maximum : ( IS_FINITE(suggestedMin.maximum) ? suggestedMin.maximum : 10 );

	// minimum checkbox & spinbox
	{
		checkbox_minimum = new QCheckBox("Minimum");
		checkbox_minimum->setChecked( IS_FINITE(currentValue.minimum) );
		checkbox_minimum->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		connect(checkbox_minimum, SIGNAL(stateChanged(int)), this, SLOT(setMinBound(int)));

		layout->addWidget(checkbox_minimum, 0, 0 );

		spinbox_minimum = new VuoSpinBox(&dialog);
		setupSpinBox(spinbox_minimum, suggestedMin.minimum, MIN(suggestedMax.minimum, currentValue.maximum), suggestedStep.minimum, currentValue.minimum);
		spinbox_minimum->installEventFilter(this);
		connect(spinbox_minimum, SIGNAL(valueChanged(QString)), this, SLOT(setMinimum(QString)));

		spinbox_minimum->setEnabled( checkbox_minimum->isChecked() );

		layout->addWidget( spinbox_minimum, 0, 1 );
	}

	// maximum checkbox & spinbox
	{
		checkbox_maximum = new QCheckBox("Maximum");
		checkbox_maximum->setChecked( IS_FINITE(currentValue.maximum) );
		checkbox_maximum->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		connect(checkbox_maximum, SIGNAL(stateChanged(int)), this, SLOT(setMaxBound(int)));

		layout->addWidget(checkbox_maximum, 1, 0 );

		spinbox_maximum = new VuoSpinBox(&dialog);
		setupSpinBox(spinbox_maximum, MAX(suggestedMin.maximum, currentValue.minimum), suggestedMax.maximum, suggestedStep.maximum, currentValue.maximum);
		spinbox_maximum->installEventFilter(this);
		connect(spinbox_maximum, SIGNAL(valueChanged(QString)), this, SLOT(setMaximum(QString)));

		spinbox_maximum->setEnabled( checkbox_maximum->isChecked() );

		layout->addWidget( spinbox_maximum, 1, 1 );
	}

	setMinBound(checkbox_minimum->isChecked() ? Qt::Checked : Qt::Unchecked);
	setMaxBound(checkbox_maximum->isChecked() ? Qt::Checked : Qt::Unchecked);

	dialog.setLayout(layout);

	// line edit inputs should be 90px wide
	dialog.setMaximumWidth(178);
	dialog.setMaximumHeight(32);

	dialog.adjustSize();

	setFirstWidgetInTabOrder(spinbox_minimum);
	setLastWidgetInTabOrder(spinbox_maximum);

	(tabCycleForward ? spinbox_minimum : spinbox_maximum)->setFocus();
	(tabCycleForward ? spinbox_minimum : spinbox_maximum)->selectAll();

	return;
}

/**
 * Sets the minimum to either -infinity or the last finite minimum value.
 */
void VuoInputEditorIntegerRange::setMinBound(int state)
{
	if(state == Qt::Unchecked)
	{
		lastValue.minimum = currentValue.minimum;
		spinbox_minimum->setValue( -INT_MAX );
		spinbox_minimum->setSpecialValueText("-∞");
		spinbox_minimum->setEnabled(false);
	}
	else
	{
		if( !IS_FINITE( currentValue.minimum ) )
		{
			if (!IS_FINITE(lastValue.minimum))
				lastValue.minimum = 0;
			lastValue.minimum = MIN(lastValue.minimum, currentValue.maximum);
			spinbox_minimum->setValue( lastValue.minimum );
		}
		spinbox_minimum->setSpecialValueText("");
		spinbox_minimum->setEnabled(true);
	}

//	spinbox_maximum->setButtonMinimum( MAX(suggestedMin.maximum, currentValue.minimum) );
}

/**
 * Sets the maximum to either infinity or the last finite maximum value.
 */
void VuoInputEditorIntegerRange::setMaxBound(int state)
{
	if(state == Qt::Unchecked)
	{
		lastValue.maximum = currentValue.maximum;
		spinbox_maximum->setMinimum(INT_MAX);
		spinbox_maximum->setValue( INT_MAX );
		spinbox_maximum->setSpecialValueText("∞");
		spinbox_maximum->setEnabled(false);
	}
	else
	{
		if( !IS_FINITE( currentValue.maximum ) )
		{
			if (!IS_FINITE(lastValue.maximum))
				lastValue.maximum = 10;
			lastValue.maximum = MAX(lastValue.maximum, currentValue.minimum);
			spinbox_maximum->setMinimum(suggestedMin.maximum);
			spinbox_maximum->setValue( lastValue.maximum );
		}
		spinbox_maximum->setSpecialValueText("");
		spinbox_maximum->setEnabled(true);
	}

//	spinbox_minimum->setButtonMaximum( MIN(suggestedMax.minimum, currentValue.maximum) );
}

/**
 * Set the new minimum from the min spinbox.
 */
void VuoInputEditorIntegerRange::setMinimum(QString newLineEditText)
{
	bool ok;
	double newLineEditValue = QLocale().toDouble(newLineEditText, &ok);
	if (ok)
	{
		currentValue.minimum = newLineEditValue;
//		currentValue.minimum = MIN(newLineEditValue, currentValue.maximum);
//		spinbox_maximum->setButtonMinimum( MAX(suggestedMin.maximum, currentValue.minimum) );
	}

	emit valueChanged(getAcceptedValue());
}

/**
 * Set the new minimum from the max spinbox.
 */
void VuoInputEditorIntegerRange::setMaximum(QString newLineEditText)
{
	bool ok;
	double newLineEditValue = QLocale().toDouble(newLineEditText, &ok);
	if (ok)
	{
		currentValue.maximum = newLineEditValue;
//		currentValue.maximum = MAX(newLineEditValue, currentValue.minimum);
//		spinbox_minimum->setButtonMaximum( MIN(suggestedMax.minimum, currentValue.maximum) );
	}

	emit valueChanged(getAcceptedValue());
}

/**
 * Returns the value currently set in the dialog's widgets.
 */
json_object* VuoInputEditorIntegerRange::getAcceptedValue()
{
	// Covnert from int32 to int64.
	VuoIntegerRange r = currentValue;
	if (!IS_FINITE(r.minimum))
		r.minimum = VuoIntegerRange_NoMinimum;
	if (!IS_FINITE(r.maximum))
		r.maximum = VuoIntegerRange_NoMaximum;

	return VuoIntegerRange_getJson(r);
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorIntegerRange::eventFilter(QObject *object, QEvent *event)
{
	VuoSpinBox* spin = dynamic_cast<VuoSpinBox*>(object);

	if (event->type() == QEvent::KeyPress && spin)
	{
		// Let the slider handle keypresses of the up and down arrows.
		QKeyEvent *keyEvent = (QKeyEvent *)(event);

		if (keyEvent->key() == Qt::Key_Up)
		{
			if(spin == spinbox_minimum)
				spin->setValue( MIN(spin->value() + spin->singleStep(), MIN(suggestedMax.minimum, currentValue.maximum)) );
			else if(spin == spinbox_maximum)
				spin->setValue( MIN(spin->value() + spin->singleStep(), suggestedMax.minimum) );

			return true;
		}
		else if (keyEvent->key() == Qt::Key_Down)
		{
			if(spin == spinbox_minimum)
				spin->setValue( MAX(spin->value() - spin->singleStep(), suggestedMin.minimum) );
			else if(spin == spinbox_maximum)
				spin->setValue( MAX(spin->value() - spin->singleStep(), MAX(suggestedMin.maximum, currentValue.minimum)) );

			return true;
		}
	}

	return VuoInputEditorWithDialog::eventFilter(object, event);
}

