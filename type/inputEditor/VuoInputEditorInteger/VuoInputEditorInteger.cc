/**
 * @file
 * VuoInputEditorInteger implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorInteger.hh"

/**
 * Constructs a VuoInputEditorInteger object.
 */
VuoInputEditor * VuoInputEditorIntegerFactory::newInputEditor()
{
	return new VuoInputEditorInteger();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorInteger::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	current = VuoInteger_makeFromJson(originalValue);

	suggestedMin = INT_MIN;
	suggestedMax = INT_MAX;
	VuoInteger suggestedStep = 1;
	previous = current;
	defaultValue = 0;

	bool hasMinMax = details != NULL, hasAutoValue = details != NULL;

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "auto"
		json_object *autoJsonValue = NULL;
		if (json_object_object_get_ex(details, "auto", &autoJsonValue))
			autoValue = VuoInteger_makeFromJson(autoJsonValue);
		else
			hasAutoValue = false;

		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
			suggestedMin = VuoInteger_makeFromJson(suggestedMinValue);
		else
			hasMinMax = false;

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
			suggestedMax = VuoInteger_makeFromJson(suggestedMaxValue);
		else
			hasMinMax = false;

		// "suggestedStep"
		if (hasMinMax)
			suggestedStep = fmax((suggestedMax - suggestedMin)/10, 1);
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = VuoInteger_makeFromJson(suggestedStepValue);

		// "default"
		json_object *defaultJsonValue = NULL;
		if (json_object_object_get_ex(details, "default", &defaultJsonValue))
			defaultValue = VuoInteger_makeFromJson(defaultJsonValue);
	}

	spinbox = new VuoSpinBox(&dialog);
	spinbox->setButtonMinimum(suggestedMin);
	spinbox->setButtonMaximum(suggestedMax);
	spinbox->setSingleStep(suggestedStep);
	spinbox->setValue(current);
	spinbox->setEnabled(autoToggle == NULL || !autoToggle->isChecked());

	connect(spinbox, static_cast<void (QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged), this, &VuoInputEditorInteger::onSpinboxUpdate);

	if(hasMinMax)
	{
		slider = new QSlider(&dialog);
		slider->setOrientation(Qt::Horizontal);
		slider->setFocusPolicy(Qt::NoFocus);
		slider->setMinimum(suggestedMin);
		slider->setMaximum(suggestedMax);
		slider->setSingleStep(suggestedStep);
		slider->setValue(current);
		slider->setEnabled(autoToggle == NULL || !autoToggle->isChecked());

		connect(slider, &QSlider::valueChanged, this, &VuoInputEditorInteger::onSliderUpdate);
	}

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	dialog.setLayout(layout);

	// left, top, right, bottom
	// when showing sliders, add a little extra margin on the bottom since QSlider takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 4, 12, 4);
	layout->setSpacing(4);
	if(hasAutoValue) layout->setHorizontalSpacing(8);

	int row = 0;

	if(hasAutoValue)
	{
		autoToggle = new QCheckBox("Auto");
		connect(autoToggle, &QCheckBox::stateChanged, this, &VuoInputEditorInteger::setAutoToggled);
		bool currentIsAuto = (current == autoValue);
		autoToggle->setChecked(currentIsAuto);
		if (currentIsAuto)
			previous = defaultValue;

		layout->addWidget(autoToggle, row++, 0);
		autoToggle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
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
void VuoInputEditorInteger::setAutoToggled(int state)
{
	if(state == Qt::Unchecked)
	{
		spinbox->setSpecialValueText("");
		spinbox->setMinimum(suggestedMin);
		spinbox->setMaximum(suggestedMax);
		current = previous;
	}
	else
	{
		previous = current;
		current = autoValue;
		spinbox->setSpecialValueText(" Auto            ");
		spinbox->setMinimum(autoValue);
		spinbox->setMaximum(autoValue);
	}

	spinbox->setValue(current);
	spinbox->setEnabled(!autoToggle->isChecked());
	if(slider) slider->setEnabled(!autoToggle->isChecked());
}

/**
 * Update the spinbox and stored VuoInteger property.
 */
void VuoInputEditorInteger::onSliderUpdate(int sliderValue)
{
	current = sliderValue;

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinbox, static_cast<void (QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged), this, &VuoInputEditorInteger::onSpinboxUpdate);
	spinbox->setValue( current );
	connect(spinbox, static_cast<void (QSpinBox::*)(const QString &)>(&QSpinBox::valueChanged), this, &VuoInputEditorInteger::onSpinboxUpdate);

	spinbox->setFocus();
	spinbox->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored VuoInteger.
 */
void VuoInputEditorInteger::onSpinboxUpdate(QString spinboxValue)
{
	current = QLocale().toInt(spinboxValue);

	if(slider != NULL)
	{
		disconnect(slider, &QSlider::valueChanged, this, &VuoInputEditorInteger::onSliderUpdate);
		slider->setValue( current );
		connect(slider, &QSlider::valueChanged, this, &VuoInputEditorInteger::onSliderUpdate);
	}

	emit valueChanged(getAcceptedValue());
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorInteger::getAcceptedValue(void)
{
	return VuoInteger_getJson(current);
}
