/**
 * @file
 * VuoInputEditorRange implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorRange.hh"

/**
 * Constructs a VuoInputEditorRange object.
 */
VuoInputEditor * VuoInputEditorRangeFactory::newInputEditor()
{
	return new VuoInputEditorRange();
}

/**
 * Creates a new label with sizePolicy(Max, Max)
 */
QLabel* makeLabel(const char* text)
{
	QLabel* label = new QLabel(text);
	label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	return label;
}

/**
 * Alloc and init a new VuoDoubleSpinBox.
 */
VuoDoubleSpinBox* VuoInputEditorRange::initSpinBox(QDialog& dialog, double initialValue, double min, double max, double step)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog);
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setButtonMinimum(min);
	spin->setButtonMaximum(max);
	spin->setSingleStep(step);
	spin->setValue(initialValue);
	spin->setFixedWidth(100);
	return spin;
}

/**
 * Alloc and init a new QSlider.
 */
QSlider* VuoInputEditorRange::initSlider(QSlider* slider, double initialValue, double min, double max, double step)
{
	double range = max - min;
	slider->setOrientation(Qt::Horizontal);
	slider->setFocusPolicy(Qt::NoFocus);
	slider->setMinimum(0);
	slider->setMaximum(slider->width());
	slider->setSingleStep( fmax(1, step * (slider->maximum() - slider->minimum()) / range ) );
	slider->setValue( VuoDoubleSpinBox::doubleToSlider(slider->minimum(), slider->maximum(), min, max, initialValue) );
	return slider;
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorRange::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	current = VuoRange_makeFromJson(originalValue);
	lastValue = current;

	const double max_dbl = std::numeric_limits<double>::max();

	suggestedMin = VuoRange_make(-max_dbl, -max_dbl);
	suggestedMax =  VuoRange_make(max_dbl, max_dbl);
	suggestedStep = VuoRange_make(0.1, 0.1);

	bool hasMinMax = true;

	requireMin = false;
	requireMax = false;

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
			suggestedMin = VuoRange_makeFromJson(suggestedMinValue);
		else
			hasMinMax = false;

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
			suggestedMax = VuoRange_makeFromJson(suggestedMaxValue);
		else
			hasMinMax = false;

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = VuoRange_makeFromJson(suggestedStepValue);

		// requireMin
		json_object *requireMinValue = NULL;
		if (json_object_object_get_ex(details, "requireMin", &requireMinValue))
			requireMin = VuoBoolean_makeFromJson(requireMinValue);

		// requireMax
		json_object *requireMaxValue = NULL;
		if (json_object_object_get_ex(details, "requireMax", &requireMaxValue))
			requireMax = VuoBoolean_makeFromJson(requireMaxValue);
	}

	spinbox_min = initSpinBox(dialog, current.minimum, suggestedMin.minimum, suggestedMax.minimum, suggestedStep.minimum);
	{
		spinbox_min->setMinimum(VuoRange_NoMinimum);
//		spinbox_min->setMaximum(current.maximum);
		spinbox_min->setMaximum(VuoRange_NoMaximum);
		spinbox_min->setSpecialValueText("-∞");
		connect(spinbox_min, SIGNAL(valueChanged(double)), this, SLOT(onSpinboxUpdate(double)));
	}

	spinbox_max = initSpinBox(dialog, current.maximum, suggestedMin.maximum, suggestedMax.maximum, suggestedStep.maximum);
	{
		// QDoubleSpinbox only can show the special string value if value == minimum
		// So if the max value is VuoRange_NoMaximum temporarily make the min/max both
		// VuoRange_NoMaximum (DBL_MAX)
//		if(current.maximum == VuoRange_NoMaximum)
//		{
//			spinbox_max->setMinimum(VuoRange_NoMaximum);
//			spinbox_max->setMaximum(VuoRange_NoMaximum);
//		}
//		else
//		{
//			spinbox_max->setMinimum(current.minimum);
//			spinbox_max->setMaximum(VuoRange_NoMaximum);
//		}

		connect(spinbox_max, SIGNAL(valueChanged(double)), this, SLOT(onSpinboxUpdate(double)));
	}

	if(hasMinMax)
	{
		slider_min = new QSlider(&dialog);
		slider_max = new QSlider(&dialog);

		slider_min = initSlider(slider_min, current.minimum, suggestedMin.minimum, suggestedMax.minimum, suggestedStep.minimum);
		slider_max = initSlider(slider_max, current.maximum, suggestedMin.maximum, suggestedMax.maximum, suggestedStep.maximum);

		connect(slider_min, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		connect(slider_max, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
	}

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	dialog.setLayout(layout);

	// left, top, right, bottom
	// when showing sliders, add a little extra margin on the bottom since QSlider takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 4, 12, 4);
	layout->setSpacing(4);
	layout->setHorizontalSpacing(8);

	int row = 0;

	/// MINIMUM VALUE
	/// {
	if(!requireMin)
	{
		toggle_hasMin = new QCheckBox("Minimum");
		toggle_hasMin->setChecked( !VuoReal_areEqual(current.minimum, VuoRange_NoMinimum) );
		toggle_hasMin->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		connect(toggle_hasMin, SIGNAL(stateChanged(int)), this, SLOT(setHasMinBound(int)));
		layout->addWidget(toggle_hasMin, row, 0);
		setHasMinBound(toggle_hasMin->isChecked() ? Qt::Checked : Qt::Unchecked);
	}
	else
	{
		layout->addWidget(makeLabel("Minimum"), row, 0);
	}

	spinbox_min->setEnabled(toggle_hasMin == NULL || toggle_hasMin->isChecked());
	layout->addWidget(spinbox_min, row++, 1);

	if(hasMinMax)
	{
		slider_min->setEnabled(toggle_hasMin == NULL || toggle_hasMin->isChecked());
		layout->addWidget(slider_min, row++, 1);
	}
	/// }

	/// MAXIMUM VALUE
	/// {
	if(!requireMax)
	{
		toggle_hasMax = new QCheckBox("Maximum");
		toggle_hasMax->setChecked( !VuoReal_areEqual(current.maximum, VuoRange_NoMaximum) );
		toggle_hasMax->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		connect(toggle_hasMax, SIGNAL(stateChanged(int)), this, SLOT(setHasMaxBound(int)));
		layout->addWidget(toggle_hasMax, row, 0);
		setHasMaxBound(toggle_hasMax->isChecked() ? Qt::Checked : Qt::Unchecked);
	}
	else
	{
		layout->addWidget(makeLabel("Maximum"), row, 0);
	}

	spinbox_max->setEnabled(toggle_hasMax == NULL || toggle_hasMax->isChecked());
	layout->addWidget(spinbox_max, row++, 1);

	if(hasMinMax)
	{
		slider_max->setEnabled(toggle_hasMax == NULL || toggle_hasMax->isChecked());
		layout->addWidget(slider_max, row, 1);
	}
	/// }

	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);
	dialog.adjustSize();

	setFirstWidgetInTabOrder(spinbox_min);
	setLastWidgetInTabOrder(spinbox_max);

	spinbox_min->setFocus();
}

void VuoInputEditorRange::setHasMinBound(int state)
{
	if(state == Qt::Unchecked)
	{
		// had min bound, now we don't
		lastValue.minimum = current.minimum;
		current.minimum = VuoRange_NoMinimum;
	}
	else
	{
		// was unbound, now has bound
		if( VuoReal_areEqual(VuoRange_NoMinimum, lastValue.minimum)  )
			current.minimum = fmin(fmax(suggestedMin.minimum, 0.), current.maximum);
		else
			current.minimum = lastValue.minimum;
	}

	spinbox_min->setValue(current.minimum);

	spinbox_min->setEnabled(state != Qt::Unchecked);
	if(slider_min) slider_min->setEnabled(state != Qt::Unchecked);
}

void VuoInputEditorRange::setHasMaxBound(int state)
{
	if(state == Qt::Unchecked)
	{
		lastValue.maximum = current.maximum;
		current.maximum = VuoRange_NoMaximum;
		spinbox_max->setSpecialValueText("∞");
		spinbox_max->setMinimum(VuoRange_NoMaximum);
		spinbox_max->setValue(current.maximum);
		spinbox_max->setEnabled(false);
		if(slider_max) slider_max->setEnabled(false);
	}
	else
	{
		if(lastValue.maximum == VuoRange_NoMaximum)
			current.maximum = fmax(fmin(suggestedMax.maximum, 1), current.minimum);
		else
			current.maximum = lastValue.maximum;

		spinbox_max->setSpecialValueText("");
		spinbox_max->setMinimum(current.minimum);
		spinbox_max->setValue(current.maximum);
		spinbox_max->setEnabled(true);
		if(slider_max) slider_max->setEnabled(true);
	}
}

void VuoInputEditorRange::setMinimumValue(double value)
{
	current.minimum = value;
//	spinbox_max->setMinimum(current.minimum);
}

void VuoInputEditorRange::setMaximumValue(double value)
{
	current.maximum = value;
//	spinbox_min->setMaximum(current.maximum);
}

/**
 * Update the spinbox_min and stored VuoRange property.
 */
void VuoInputEditorRange::onSliderUpdate(int sliderValue)
{
	if(QObject::sender() == NULL)
		return;

	bool isMin = (QObject::sender() == spinbox_min || QObject::sender() == slider_min);

	QSlider* slider = isMin ? slider_min : slider_max;

	double val = VuoDoubleSpinBox::sliderToDouble(	slider->minimum(),
													slider->maximum(),
													isMin ? suggestedMin.minimum : suggestedMin.maximum,
													isMin ? suggestedMax.minimum : suggestedMax.maximum,
													sliderValue);
	if(isMin)
		setMinimumValue(val);
	else
		setMaximumValue(val);

	VuoDoubleSpinBox* spinbox = isMin ? spinbox_min : spinbox_max;

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(onSpinboxUpdate(double)));
	spinbox->setValue( isMin ? current.minimum : current.maximum );
	connect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(onSpinboxUpdate(double)));

	spinbox->setFocus();
	spinbox->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored VuoRange.
 */
void VuoInputEditorRange::onSpinboxUpdate(double value)
{
	if(QObject::sender() == NULL)
		return;

	bool isMin = (QObject::sender() == spinbox_min || QObject::sender() == slider_min);

	if(isMin)
		setMinimumValue(value);
	else
		setMaximumValue(value);

	QSlider* slider = isMin ? slider_min : slider_max;

	if(slider != NULL)
	{
		int sliderValue = VuoDoubleSpinBox::doubleToSlider(
			slider->minimum(),
			slider->maximum(),
			isMin ? suggestedMin.minimum : suggestedMin.maximum,
			isMin ? suggestedMax.minimum : suggestedMax.maximum,
			value);

		disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		slider->setValue( sliderValue );
		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
	}

	emit valueChanged( getAcceptedValue() );
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorRange::getAcceptedValue(void)
{
	return VuoRange_getJson(current);
}
