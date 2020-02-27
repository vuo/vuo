/**
 * @file
 * VuoInputEditorReal implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorReal.hh"

extern "C" {
}

/**
 * Constructs a VuoInputEditorReal object.
 */
VuoInputEditor * VuoInputEditorRealFactory::newInputEditor()
{
	return new VuoInputEditorReal();
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
VuoDoubleSpinBox* VuoInputEditorReal::initSpinBox(QDialog& dialog, double initialValue)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog, 11);
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setFixedWidth(100);
	spin->setButtonMinimum(suggestedMin);
	spin->setButtonMaximum(suggestedMax);
	spin->setSingleStep(suggestedStep);
	spin->setValue(initialValue);
	return spin;
}

/**
 * Alloc and init a new QSlider.
 */
QSlider* VuoInputEditorReal::initSlider(QDialog& dialog, double initialValue)
{
	double min = suggestedMin;
	double max = suggestedMax;
	double step = suggestedStep;
	double range = max - min;

	QSlider* slider = new QSlider(&dialog);
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
void VuoInputEditorReal::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	current = VuoReal_makeFromJson(originalValue);

	suggestedMin = -DBL_MAX;
	suggestedMax =  DBL_MAX;
	suggestedStep = 1.0;
	previous = current;
	defaultValue = 0;
	automatic = -INFINITY;

	bool hasMinMax = true, hasAutoValue = false;

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "default"
		json_object *suggestedDefaultValue = NULL;
		if (json_object_object_get_ex(details, "default", &suggestedDefaultValue))
			defaultValue = VuoReal_makeFromJson(suggestedDefaultValue);

		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
			suggestedMin = VuoReal_makeFromJson(suggestedMinValue);
		else
			hasMinMax = false;

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
			suggestedMax = VuoReal_makeFromJson(suggestedMaxValue);
		else
			hasMinMax = false;

		// "suggestedStep"
		if (hasMinMax)
			suggestedStep = (suggestedMax - suggestedMin)/10.;
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = VuoReal_makeFromJson(suggestedStepValue);

		json_object *autoValue = NULL;
		if(json_object_object_get_ex(details, "auto", &autoValue))
		{
			hasAutoValue = true;
			automatic = VuoReal_makeFromJson(autoValue);
		}

		json_object *autoSupersedesValue = NULL;
		if(json_object_object_get_ex(details, "autoSupersedesDefault", &autoSupersedesValue))
			autoSupersedesDefault = VuoBoolean_makeFromJson(autoSupersedesValue);
	}

	spinbox = initSpinBox(dialog, current);
	connect(spinbox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorReal::onSpinboxUpdate);

	if(hasMinMax)
	{
		slider = initSlider(dialog, current);
		connect(slider, &QSlider::valueChanged, this, &VuoInputEditorReal::onSliderUpdate);
	}

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	dialog.setLayout(layout);

	// left, top, right, bottom
	layout->setContentsMargins(4, 4, 12, 4);
	layout->setSpacing(4);
	if(hasAutoValue) layout->setHorizontalSpacing(8);

	int row = 0;

	if(hasAutoValue)
	{
		checkbox = new QCheckBox("Auto");
		connect(checkbox, &QCheckBox::stateChanged, this, &VuoInputEditorReal::setIsAuto);
		bool currentIsAuto = VuoReal_areEqual(current, automatic);
		checkbox->setChecked(currentIsAuto);
		if (currentIsAuto)
			previous = defaultValue;

		layout->addWidget(checkbox, row++, 0);
		checkbox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	}

	layout->addWidget(spinbox, row++, 0);

	if(hasMinMax)
	{
		// stick the slider in a horizontal layout so that we can apply a separate margin.
		// necessary because QSlider aligns itself with about 4px left padding and 0px right
		QHBoxLayout* sliderLayout = new QHBoxLayout();
		sliderLayout->setContentsMargins(-2, 0, 2, 0);
		sliderLayout->addWidget(slider);
		layout->addLayout(sliderLayout, row++, 0);
	}

	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);
	dialog.adjustSize();

	setFirstWidgetInTabOrder(spinbox);
	setLastWidgetInTabOrder(spinbox);

	spinbox->setFocus();
}

/**
 * Responds to changes in the state of the "Auto" checkbox.
 */
void VuoInputEditorReal::setIsAuto(int state)
{
	if(state == Qt::Unchecked)
	{
		spinbox->setSpecialValueText("");
		spinbox->setMinimum(suggestedMin);
		spinbox->setMaximum(suggestedMax);

		if(previous != automatic)
			current = previous;
		else
			current = (suggestedMax + suggestedMin) * .5;
	}
	else
	{
		previous = current;
		current = automatic;

		spinbox->setSpecialValueText("Auto");
		spinbox->setMinimum(automatic);
		spinbox->setMaximum(automatic);
	}

	spinbox->setValue(current);
	spinbox->setEnabled(!checkbox->isChecked());
	if(slider) slider->setEnabled(!checkbox->isChecked());
}

/**
 * Update the spinbox and stored VuoReal property.
 */
void VuoInputEditorReal::onSliderUpdate(int sliderValue)
{
	current = VuoDoubleSpinBox::sliderToDouble(	slider->minimum(),
												slider->maximum(),
												suggestedMin,
												suggestedMax,
												sliderValue);

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinbox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorReal::onSpinboxUpdate);
	spinbox->setValue( current );
	connect(spinbox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorReal::onSpinboxUpdate);

	spinbox->setFocus();
	spinbox->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored VuoReal.
 */
void VuoInputEditorReal::onSpinboxUpdate(double spinboxValue)
{
	current = spinboxValue;

	if(slider != NULL)
	{
		int sliderValue = VuoDoubleSpinBox::doubleToSlider(
			slider->minimum(),
			slider->maximum(),
			suggestedMin,
			suggestedMax,
			current);

		disconnect(slider, &QSlider::valueChanged, this, &VuoInputEditorReal::onSliderUpdate);
		slider->setValue( sliderValue );
		connect(slider, &QSlider::valueChanged, this, &VuoInputEditorReal::onSliderUpdate);
	}

	emit valueChanged( getAcceptedValue() );
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorReal::getAcceptedValue(void)
{
	return VuoReal_getJson(current);
}
