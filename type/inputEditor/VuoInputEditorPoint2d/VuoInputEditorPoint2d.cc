/**
 * @file
 * VuoInputEditorPoint2d implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorPoint2d.hh"

/**
 * Constructs a VuoInputEditorPoint2d object.
 */
VuoInputEditor * VuoInputEditorPoint2dFactory::newInputEditor()
{
	return new VuoInputEditorPoint2d();
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
VuoDoubleSpinBox* VuoInputEditorPoint2d::initSpinBox(coord whichCoord, QDialog& dialog, double initialValue)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog, 7);
	const int decimalPrecision = FLT_MAX_10_EXP + FLT_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setFixedWidth(100);
	spin->setButtonMinimum(suggestedMinForCoord[whichCoord]);
	spin->setButtonMaximum(suggestedMaxForCoord[whichCoord]);
	spin->setSingleStep(suggestedStepForCoord[whichCoord]);
	spin->setValue(initialValue);
	return spin;
}

/**
 * Alloc and init a new QSlider.
 */
QSlider* VuoInputEditorPoint2d::initSlider(coord whichCoord, QDialog& dialog, double initialValue)
{
	double min = suggestedMinForCoord[whichCoord];
	double max = suggestedMaxForCoord[whichCoord];
	double step = suggestedStepForCoord[whichCoord];
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
void VuoInputEditorPoint2d::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	current = VuoPoint2d_makeFromJson(originalValue);

	const double max_dbl = std::numeric_limits<double>::max();

	VuoPoint2d suggestedMin = VuoPoint2d_make(-max_dbl, -max_dbl);
	VuoPoint2d suggestedMax = VuoPoint2d_make( max_dbl,  max_dbl);
	VuoPoint2d suggestedStep = VuoPoint2d_make(1.0, 1.0);

	bool hasMinMax = true;
	bool tabCycleForward = true;

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		json_object *forwardTabTraversal = NULL;
		if (json_object_object_get_ex(details, "forwardTabTraversal", &forwardTabTraversal))
			tabCycleForward = json_object_get_boolean(forwardTabTraversal);

		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
			suggestedMin = VuoPoint2d_makeFromJson(suggestedMinValue);
		else
			hasMinMax = false;

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
			suggestedMax = VuoPoint2d_makeFromJson(suggestedMaxValue);
		else
			hasMinMax = false;

		// "suggestedStep"
		if (hasMinMax)
			suggestedStep = VuoPoint2d_make((suggestedMax.x - suggestedMin.x)/10.,
											(suggestedMax.y - suggestedMin.y)/10.);
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = VuoPoint2d_makeFromJson(suggestedStepValue);
	}

	suggestedMinForCoord[coord_x] = suggestedMin.x;
	suggestedMinForCoord[coord_y] = suggestedMin.y;
	suggestedMaxForCoord[coord_x] = suggestedMax.x;
	suggestedMaxForCoord[coord_y] = suggestedMax.y;
	suggestedStepForCoord[coord_x] = suggestedStep.x;
	suggestedStepForCoord[coord_y] = suggestedStep.y;

	spinboxForCoord[coord_x] = initSpinBox(coord_x, dialog, current.x);
	spinboxForCoord[coord_y] = initSpinBox(coord_y, dialog, current.y);
	connect(spinboxForCoord[coord_x], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorPoint2d::onSpinboxUpdate);
	connect(spinboxForCoord[coord_y], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorPoint2d::onSpinboxUpdate);

	if(hasMinMax)
	{
		sliderForCoord[coord_x] = initSlider(coord_x, dialog, current.x);
		sliderForCoord[coord_y] = initSlider(coord_y, dialog, current.y);
		connect(sliderForCoord[coord_x], &QSlider::valueChanged, this, &VuoInputEditorPoint2d::onSliderUpdate);
		connect(sliderForCoord[coord_y], &QSlider::valueChanged, this, &VuoInputEditorPoint2d::onSliderUpdate);
	}

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	dialog.setLayout(layout);

	// left, top, right, bottom
	// when showing sliders, add a little extra margin on the bottom since QSlider takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 4, 16, 4);
	layout->setSpacing(4);

	int row = 0;

	layout->addWidget(makeLabel("X"), row, 0, Qt::AlignHCenter);
	layout->addWidget(spinboxForCoord[coord_x], row++, 1);
	if(hasMinMax) layout->addWidget(sliderForCoord[coord_x], row++, 1);

	layout->addWidget(makeLabel("Y"), row, 0, Qt::AlignHCenter);
	layout->addWidget(spinboxForCoord[coord_y], row++, 1);
	if(hasMinMax) layout->addWidget(sliderForCoord[coord_y], row++, 1);

	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);

	dialog.adjustSize();

	// Layout details
	setFirstWidgetInTabOrder(spinboxForCoord[coord_x]);
	setLastWidgetInTabOrder(spinboxForCoord[coord_y]);

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward ? spinboxForCoord[coord_x] : spinboxForCoord[coord_y])->setFocus();
	(tabCycleForward ? spinboxForCoord[coord_x] : spinboxForCoord[coord_y])->selectAll();
}

bool VuoInputEditorPoint2d::getCoordFromQObject(QObject* sender, VuoInputEditorPoint2d::coord* whichCoord)
{
	if(sender == spinboxForCoord[coord_x] || sender == sliderForCoord[coord_x])
		*whichCoord = coord_x;
	else if(sender == spinboxForCoord[coord_y] || sender == sliderForCoord[coord_y])
		*whichCoord = coord_y;
	else
		return false;

	return true;
}

void VuoInputEditorPoint2d::setCoord(coord c, double value)
{
	if(c == coord_x)
		current.x = value;
	else if(c == coord_y)
		current.y = value;
}

/**
 * Update the spinbox and stored VuoPoint2d property.
 */
void VuoInputEditorPoint2d::onSliderUpdate(int sliderValue)
{
	coord whichCoord;

	if(!getCoordFromQObject(QObject::sender(), &whichCoord))
		return;

	QSlider *targetSlider =	sliderForCoord[whichCoord];

	double value = VuoDoubleSpinBox::sliderToDouble(targetSlider->minimum(),
													targetSlider->maximum(),
													suggestedMinForCoord[whichCoord],
													suggestedMaxForCoord[whichCoord],
													sliderValue);
	setCoord(whichCoord, value);

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorPoint2d::onSpinboxUpdate);
	spinboxForCoord[whichCoord]->setValue( value );
	connect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorPoint2d::onSpinboxUpdate);

	spinboxForCoord[whichCoord]->setFocus();
	spinboxForCoord[whichCoord]->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored VuoPoint2d.
 */
void VuoInputEditorPoint2d::onSpinboxUpdate(QString spinboxValue)
{
	coord whichCoord;

	if(!getCoordFromQObject(QObject::sender(), &whichCoord))
		return;

	double value = QLocale().toDouble(spinboxValue);
	setCoord(whichCoord, value);

	QSlider *targetSlider =	sliderForCoord[whichCoord];

	if(targetSlider != NULL)
	{
		int sliderValue = VuoDoubleSpinBox::doubleToSlider(targetSlider->minimum(), targetSlider->maximum(), suggestedMinForCoord[whichCoord], suggestedMaxForCoord[whichCoord], value);
		disconnect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorPoint2d::onSliderUpdate);
		targetSlider->setValue( sliderValue );
		connect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorPoint2d::onSliderUpdate);
	}

	emitValueChanged();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorPoint2d::getAcceptedValue(void)
{
	return VuoPoint2d_getJson(current);
}

/**
 * Send valueChanged.
 */
void VuoInputEditorPoint2d::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}
