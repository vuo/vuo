/**
 * @file
 * VuoInputEditorEdgeBlend implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */
#include "VuoInputEditorEdgeBlend.hh"

/**
 * Constructs a VuoInputEditorEdgeBlend object.
 */
VuoInputEditor * VuoInputEditorEdgeBlendFactory::newInputEditor()
{
	return new VuoInputEditorEdgeBlend();
}

/**
 * Applies settings to a VuoSpinBox
 */
void VuoInputEditorEdgeBlend::setupSpinBox(VuoDoubleSpinBox* spin, double min, double max, double step, double value)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setButtonMinimum(min);
	spin->setButtonMaximum(max);
	spin->setSingleStep(step);
	spin->setValue(value);
}

void VuoInputEditorEdgeBlend::setupSlider(QSlider* slider, double min, double max, double step, double value)
{
	// slider->setAttribute(Qt::WA_MacSmallSize);
	slider->setOrientation(Qt::Horizontal);
	slider->setFocusPolicy(Qt::NoFocus);
	slider->setMinimum(0);
	slider->setMaximum(slider->width());
	slider->setSingleStep(fmax(1, step*(slider->maximum()-slider->minimum())/(max-min)));
	slider->setValue( VuoDoubleSpinBox::doubleToSlider(slider->minimum(), slider->maximum(), min, max, value) );
}

/**
 * Try to read in suggested details and return true if min/max are both found, false otherwise (though there still may be some usable data read).
 */
bool getDetails(json_object* details, VuoEdgeBlend* suggestedMin, VuoEdgeBlend* suggestedMax, VuoEdgeBlend* suggestedStep)
{
	bool hasMin = false, hasMax = false;

	if (details != NULL)
	{
		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			*suggestedMin = VuoEdgeBlend_makeFromJson(suggestedMinValue);
			hasMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			*suggestedMax = VuoEdgeBlend_makeFromJson(suggestedMaxValue);
			hasMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			*suggestedStep = VuoEdgeBlend_makeFromJson(suggestedStepValue);
	}

	return hasMin && hasMax;
}

/**
 * Prepare an edge blend dialog.
 */
void VuoInputEditorEdgeBlend::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	suggestedMin = 	VuoEdgeBlend_make(0,0,0);
	suggestedMax = 	VuoEdgeBlend_make(0.5,3,0.5);
	suggestedStep =	VuoEdgeBlend_make(.1,.1,.1);

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
	// when showing sliders, add a little extra margin on the bottom since VuoDoubleSpinBox takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 6, 12, hasMinMax ? 6 : 4);
	layout->setSpacing(8);

	currentValue = VuoEdgeBlend_makeFromJson(originalValue);

	unsigned int row = 0;

	// crop spinbox
	{
		spinBox_crop = new VuoDoubleSpinBox(&dialog, 7);
		setupSpinBox(spinBox_crop, suggestedMin.crop, suggestedMax.crop, suggestedStep.crop, currentValue.crop);
		connect(spinBox_crop, static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorEdgeBlend::setCrop);

		layout->addWidget( new QLabel("Crop"), row, 0 );
		layout->addWidget( spinBox_crop, row, 1 );

		row++;

		// show sliders if min/max are both provided
		if(hasMinMax)
		{
			slider_crop = new QSlider;
			setupSlider(slider_crop, suggestedMin.crop, suggestedMax.crop, suggestedStep.crop, currentValue.crop);
			crop_slider_range = slider_crop->maximum();
			connect(slider_crop, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCropSlider);

			layout->addWidget(slider_crop, row, 1);
			row++;
		}
		else
		{
			slider_crop = NULL;
		}
	}

	row++;

	// cutoff spinbox
	{
		spinBox_cutoff = new VuoDoubleSpinBox(&dialog, 7);
		setupSpinBox(spinBox_cutoff, suggestedMin.cutoff, suggestedMax.cutoff, suggestedStep.cutoff, currentValue.cutoff);

		connect(spinBox_cutoff, static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorEdgeBlend::setCutoff);
		layout->addWidget( new QLabel("Cutoff"), row, 0 );
		layout->addWidget( spinBox_cutoff, row, 1);

		row++;

		// show sliders if min/max are both provided
		if(hasMinMax)
		{
			slider_cutoff = new QSlider;
			setupSlider(slider_cutoff, suggestedMin.cutoff, suggestedMax.cutoff, suggestedStep.cutoff, currentValue.cutoff);
			cutoff_slider_range = slider_cutoff->maximum();
			connect(slider_cutoff, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCutoffSlider);

			layout->addWidget(slider_cutoff, row, 1);

			row++;
		}
		else
		{
			slider_cutoff = NULL;
		}
	}

	row++;

	// gamma spinbox
	{
		spinBox_gamma = new VuoDoubleSpinBox(&dialog, 7);
		setupSpinBox(spinBox_gamma, suggestedMin.gamma, suggestedMax.gamma, suggestedStep.gamma, currentValue.gamma);
		connect(spinBox_gamma, static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorEdgeBlend::setGamma);

		layout->addWidget( new QLabel("Gamma"), row, 0 );
		layout->addWidget( spinBox_gamma, row, 1 );

		row++;

		// show sliders if min/max are both provided
		if(hasMinMax)
		{
			slider_gamma = new QSlider;
			setupSlider(slider_gamma, suggestedMin.gamma, suggestedMax.gamma, suggestedStep.gamma, currentValue.gamma);
			gamma_slider_range = slider_gamma->maximum();
			connect(slider_gamma, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setGammaSlider);

			layout->addWidget(slider_gamma, row, 1);
			row++;
		}
		else
		{
			slider_gamma = NULL;
		}
	}

	dialog.setLayout(layout);

	dialog.setMaximumWidth(32);
	dialog.setMaximumHeight(32);

	setFirstWidgetInTabOrder(spinBox_crop);
	setLastWidgetInTabOrder(spinBox_gamma);

	(tabCycleForward ? spinBox_crop : spinBox_gamma)->setFocus();
	(tabCycleForward ? spinBox_crop : spinBox_gamma)->selectAll();

	return;
}

void VuoInputEditorEdgeBlend::setCutoffSlider(int newSliderValue)
{
	currentValue.cutoff = VuoDoubleSpinBox::sliderToDouble(slider_cutoff->minimum(), slider_cutoff->maximum(), suggestedMin.cutoff, suggestedMax.cutoff, newSliderValue);
	spinBox_cutoff->setValue(currentValue.cutoff);
	spinBox_cutoff->setFocus();
	spinBox_cutoff->selectAll();
	emit valueChanged( getAcceptedValue() );
}

void VuoInputEditorEdgeBlend::setGammaSlider(int newSliderValue)
{
	currentValue.gamma = VuoDoubleSpinBox::sliderToDouble(slider_gamma->minimum(), slider_gamma->maximum(), suggestedMin.gamma, suggestedMax.gamma, newSliderValue);
	spinBox_gamma->setValue(currentValue.gamma);
	spinBox_gamma->setFocus();
	spinBox_gamma->selectAll();
	emit valueChanged( getAcceptedValue() );
}

void VuoInputEditorEdgeBlend::setCropSlider(int newSliderValue)
{
	currentValue.crop = VuoDoubleSpinBox::sliderToDouble(slider_crop->minimum(), slider_crop->maximum(), suggestedMin.crop, suggestedMax.crop, newSliderValue);
	spinBox_crop->setValue(currentValue.crop);
	spinBox_crop->setFocus();
	spinBox_crop->selectAll();
	emit valueChanged( getAcceptedValue() );
}

void VuoInputEditorEdgeBlend::setCutoff(QString newLineEditText)
{
	double newLineEditValue = QLocale().toDouble(newLineEditText);

	if(slider_cutoff != NULL)
	{
		int newSliderValue = VuoDoubleSpinBox::doubleToSlider(slider_cutoff->minimum(), slider_cutoff->maximum(), suggestedMin.cutoff, suggestedMax.cutoff, newLineEditValue);
		disconnect(slider_cutoff, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCutoffSlider);
		slider_cutoff->setValue(newSliderValue);
		connect(slider_cutoff, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCutoffSlider);
	}

	currentValue.cutoff = newLineEditValue;
	emit valueChanged( getAcceptedValue() );
}

void VuoInputEditorEdgeBlend::setGamma(QString newLineEditText)
{
	double newLineEditValue = QLocale().toDouble(newLineEditText);

	if(slider_gamma != NULL)
	{
		int newSliderValue = VuoDoubleSpinBox::doubleToSlider(slider_gamma->minimum(), slider_gamma->maximum(), suggestedMin.gamma, suggestedMax.gamma, newLineEditValue);

		disconnect(slider_gamma, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setGammaSlider);
		slider_gamma->setValue(newSliderValue);
		connect(slider_gamma, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setGammaSlider);
	}

	currentValue.gamma = newLineEditValue;
	emit valueChanged( getAcceptedValue() );
}

void VuoInputEditorEdgeBlend::setCrop(QString newLineEditText)
{
	double newLineEditValue = QLocale().toDouble(newLineEditText);

	if(slider_crop != NULL)
	{
		int newSliderValue = VuoDoubleSpinBox::doubleToSlider(slider_crop->minimum(), slider_crop->maximum(), suggestedMin.crop, suggestedMax.crop, newLineEditValue);

		disconnect(slider_crop, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCropSlider);
		slider_crop->setValue(newSliderValue);
		connect(slider_crop, &QSlider::valueChanged, this, &VuoInputEditorEdgeBlend::setCropSlider);
	}

	currentValue.crop = newLineEditValue;
	emit valueChanged( getAcceptedValue() );
}

/**
 * Returns the value currently set in the dialog's widgets.
 */
json_object* VuoInputEditorEdgeBlend::getAcceptedValue()
{
	return VuoEdgeBlend_getJson( currentValue );
}
