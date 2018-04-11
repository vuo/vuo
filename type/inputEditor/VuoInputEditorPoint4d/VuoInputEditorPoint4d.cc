/**
 * @file
 * VuoInputEditorPoint4d implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorPoint4d.hh"

/**
 * Constructs a VuoInputEditorPoint4d object.
 */
VuoInputEditor * VuoInputEditorPoint4dFactory::newInputEditor()
{
	return new VuoInputEditorPoint4d();
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
VuoDoubleSpinBox* VuoInputEditorPoint4d::initSpinBox(coord whichCoord, QDialog& dialog, double initialValue)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog);
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setButtonMinimum(suggestedMinForCoord[whichCoord]);
	spin->setButtonMaximum(suggestedMaxForCoord[whichCoord]);
	spin->setSingleStep(suggestedStepForCoord[whichCoord]);
	spin->setValue(initialValue);
	return spin;
}

/**
 * Alloc and init a new QSlider.
 */
QSlider* VuoInputEditorPoint4d::initSlider(coord whichCoord, QDialog& dialog, double initialValue)
{
	double min = suggestedMinForCoord[whichCoord];
	double max = suggestedMaxForCoord[whichCoord];
	double step = suggestedStepForCoord[whichCoord];
	double range = max - min;

	QSlider* slider = new QSlider(&dialog);
	// slider->setAttribute(Qt::WA_MacSmallSize);
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
void VuoInputEditorPoint4d::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	current = VuoPoint4d_makeFromJson(originalValue);

	const double max_dbl = std::numeric_limits<double>::max();

	VuoPoint4d suggestedMin = VuoPoint4d_make(-max_dbl, -max_dbl, -max_dbl, -max_dbl);
	VuoPoint4d suggestedMax = VuoPoint4d_make( max_dbl,  max_dbl,  max_dbl,  max_dbl);
	VuoPoint4d suggestedStep = VuoPoint4d_make(1.0, 1.0, 1.0, 1.0);

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
			suggestedMin = VuoPoint4d_makeFromJson(suggestedMinValue);
		else
			hasMinMax = false;

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
			suggestedMax = VuoPoint4d_makeFromJson(suggestedMaxValue);
		else
			hasMinMax = false;

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = VuoPoint4d_makeFromJson(suggestedStepValue);
	}

	suggestedMinForCoord[coord_x] = suggestedMin.x;
	suggestedMinForCoord[coord_y] = suggestedMin.y;
	suggestedMinForCoord[coord_z] = suggestedMin.z;
	suggestedMinForCoord[coord_w] = suggestedMin.w;

	suggestedMaxForCoord[coord_x] = suggestedMax.x;
	suggestedMaxForCoord[coord_y] = suggestedMax.y;
	suggestedMaxForCoord[coord_z] = suggestedMax.z;
	suggestedMaxForCoord[coord_w] = suggestedMax.w;

	suggestedStepForCoord[coord_x] = suggestedStep.x;
	suggestedStepForCoord[coord_y] = suggestedStep.y;
	suggestedStepForCoord[coord_z] = suggestedStep.z;
	suggestedStepForCoord[coord_w] = suggestedStep.w;

	spinboxForCoord[coord_x] = initSpinBox(coord_x, dialog, current.x);
	spinboxForCoord[coord_y] = initSpinBox(coord_y, dialog, current.y);
	spinboxForCoord[coord_z] = initSpinBox(coord_z, dialog, current.z);
	spinboxForCoord[coord_w] = initSpinBox(coord_w, dialog, current.w);

	connect(spinboxForCoord[coord_x], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));
	connect(spinboxForCoord[coord_y], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));
	connect(spinboxForCoord[coord_z], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));
	connect(spinboxForCoord[coord_w], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));

	if(hasMinMax)
	{
		sliderForCoord[coord_x] = initSlider(coord_x, dialog, current.x);
		sliderForCoord[coord_y] = initSlider(coord_y, dialog, current.y);
		sliderForCoord[coord_z] = initSlider(coord_z, dialog, current.z);
		sliderForCoord[coord_w] = initSlider(coord_w, dialog, current.w);

		connect(sliderForCoord[coord_x], SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		connect(sliderForCoord[coord_y], SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		connect(sliderForCoord[coord_z], SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		connect(sliderForCoord[coord_w], SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
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

	layout->addWidget(makeLabel("Z"), row, 0, Qt::AlignHCenter);
	layout->addWidget(spinboxForCoord[coord_z], row++, 1);
	if(hasMinMax) layout->addWidget(sliderForCoord[coord_z], row++, 1);

	layout->addWidget(makeLabel("W"), row, 0, Qt::AlignHCenter);
	layout->addWidget(spinboxForCoord[coord_w], row++, 1);
	if(hasMinMax) layout->addWidget(sliderForCoord[coord_w], row++, 1);

	// forces dialog to min acceptable size
	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);

	dialog.adjustSize();

	// Layout details
	setFirstWidgetInTabOrder(spinboxForCoord[coord_x]);
	setLastWidgetInTabOrder(spinboxForCoord[coord_w]);

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward ? spinboxForCoord[coord_x] : spinboxForCoord[coord_w])->setFocus();
	(tabCycleForward ? spinboxForCoord[coord_x] : spinboxForCoord[coord_w])->selectAll();
}

bool VuoInputEditorPoint4d::getCoordFromQObject(QObject* sender, VuoInputEditorPoint4d::coord* whichCoord)
{
	if(sender == spinboxForCoord[coord_x] || sender == sliderForCoord[coord_x])
		*whichCoord = coord_x;
	else if(sender == spinboxForCoord[coord_y] || sender == sliderForCoord[coord_y])
		*whichCoord = coord_y;
	else if(sender == spinboxForCoord[coord_z] || sender == sliderForCoord[coord_z])
		*whichCoord = coord_z;
	else if(sender == spinboxForCoord[coord_w] || sender == sliderForCoord[coord_w])
		*whichCoord = coord_w;
	else
		return false;

	return true;
}

void VuoInputEditorPoint4d::setCoord(coord c, double value)
{
	if(c == coord_x)
		current.x = value;
	else if(c == coord_y)
		current.y = value;
	else if(c == coord_z)
		current.z = value;
	else if(c == coord_w)
		current.w = value;
}

/**
 * Update the spinbox and stored VuoPoint4d property.
 */
void VuoInputEditorPoint4d::onSliderUpdate(int sliderValue)
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
	disconnect(spinboxForCoord[whichCoord], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));
	spinboxForCoord[whichCoord]->setValue( value );
	connect(spinboxForCoord[whichCoord], SIGNAL(valueChanged(QString)), this, SLOT(onSpinboxUpdate(QString)));

	spinboxForCoord[whichCoord]->setFocus();
	spinboxForCoord[whichCoord]->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored VuoPoint4d.
 */
void VuoInputEditorPoint4d::onSpinboxUpdate(QString spinboxValue)
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
		disconnect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
		targetSlider->setValue( sliderValue );
		connect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderUpdate(int)));
	}

	emitValueChanged();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorPoint4d::getAcceptedValue(void)
{
	return VuoPoint4d_getJson(current);
}

/**
 * Send valueChanged.
 */
void VuoInputEditorPoint4d::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}
