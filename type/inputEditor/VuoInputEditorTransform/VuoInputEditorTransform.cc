/**
 * @file
 * VuoInputEditorTransform implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorTransform.hh"

#define DEG2RAD 0.0174532925f ///< Convert degrees to radians.
#define RAD2DEG 57.295779513f ///< Convert radians to degrees.

/**
 * Constructs a VuoInputEditorTransform object.
 */
VuoInputEditor * VuoInputEditorTransformFactory::newInputEditor()
{
	return new VuoInputEditorTransform();
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
 * Set all values on a map<coord, double> type with transform.
 */
void VuoInputEditorTransform::setCoordMap(map<coord, double>* coordMap, VuoTransform transform)
{
	(*coordMap)[xTranslation] = transform.translation.x;
	(*coordMap)[yTranslation] = transform.translation.y;
	(*coordMap)[zTranslation] = transform.translation.z;

	VuoPoint3d rotation = VuoTransform_getEuler(transform);
	(*coordMap)[xRotation] = RAD2DEG * rotation.x;
	(*coordMap)[yRotation] = RAD2DEG * rotation.y;
	(*coordMap)[zRotation] = RAD2DEG * rotation.z;

	(*coordMap)[xScale] = transform.scale.x;
	(*coordMap)[yScale] = transform.scale.y;
	(*coordMap)[zScale] = transform.scale.z;
}

/**
 * Alloc and init a new VuoDoubleSpinBox.
 */
VuoDoubleSpinBox* VuoInputEditorTransform::initSpinBox(coord whichCoord, QDialog& dialog, double initialValue)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog, 7);
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
QSlider* VuoInputEditorTransform::initSlider(coord whichCoord, QDialog& dialog, double initialValue)
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
void VuoInputEditorTransform::setUpDialog(QDialog& dialog, json_object *originalValue, json_object *details)
{
	currentTransform = VuoTransform_makeFromJson(originalValue);

	bool tabCycleForward = true;

	VuoTransform default_min = VuoTransform_makeEuler( VuoPoint3d_make(-1, -1, -1), VuoPoint3d_make(-180. * DEG2RAD, -180. * DEG2RAD, -180. * DEG2RAD), VuoPoint3d_make(0, 0, 0));
	VuoTransform default_max = VuoTransform_makeEuler( VuoPoint3d_make(1, 1, 1), VuoPoint3d_make(180. * DEG2RAD, 180. * DEG2RAD, 180. * DEG2RAD), VuoPoint3d_make(2, 2, 2));
	VuoTransform default_step = VuoTransform_makeEuler( VuoPoint3d_make(.01, .01, .01), VuoPoint3d_make(5. * DEG2RAD, 5. * DEG2RAD, 5. * DEG2RAD), VuoPoint3d_make(.01, .01, .01));

	setCoordMap(&suggestedMinForCoord, default_min);
	setCoordMap(&suggestedMaxForCoord, default_max);
	setCoordMap(&suggestedStepForCoord, default_step);

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		json_object *forwardTabTraversal = NULL;

		if (json_object_object_get_ex(details, "forwardTabTraversal", &forwardTabTraversal))
			tabCycleForward = json_object_get_boolean(forwardTabTraversal);

		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			VuoTransform minTransform = VuoTransform_makeFromJson(suggestedMinValue);
			setCoordMap(&suggestedMinForCoord, minTransform);
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			VuoTransform maxTransform = VuoTransform_makeFromJson(suggestedMaxValue);
			setCoordMap(&suggestedMaxForCoord, maxTransform);
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			VuoTransform stepTransform = VuoTransform_makeFromJson(suggestedStepValue);
			setCoordMap(&suggestedStepForCoord, stepTransform);
		}
	}

	// x-translation
	spinboxForCoord[xTranslation] = initSpinBox(xTranslation, dialog, currentTransform.translation.x);
	sliderForCoord[xTranslation] = initSlider(xTranslation, dialog, currentTransform.translation.x);
	connect(sliderForCoord[xTranslation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[xTranslation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// y-translation
	spinboxForCoord[yTranslation] = initSpinBox(yTranslation, dialog, currentTransform.translation.y);
	sliderForCoord[yTranslation] = initSlider(yTranslation, dialog, currentTransform.translation.y);
	connect(sliderForCoord[yTranslation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[yTranslation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// z-translation
	spinboxForCoord[zTranslation] = initSpinBox(zTranslation, dialog, currentTransform.translation.z);
	sliderForCoord[zTranslation] = initSlider(zTranslation, dialog, currentTransform.translation.z);
	connect(sliderForCoord[zTranslation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[zTranslation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// x-rotation
	VuoPoint3d rotation = VuoTransform_getEuler(currentTransform);
	spinboxForCoord[xRotation] = initSpinBox(xRotation, dialog, rotation.x * RAD2DEG);
	sliderForCoord[xRotation] = initSlider(xRotation, dialog, rotation.x * RAD2DEG);
	connect(sliderForCoord[xRotation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[xRotation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// y-rotation
	spinboxForCoord[yRotation] = initSpinBox(yRotation, dialog, rotation.y * RAD2DEG);
	sliderForCoord[yRotation] = initSlider(yRotation, dialog, rotation.y * RAD2DEG);
	connect(sliderForCoord[yRotation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[yRotation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// z-rotation
	spinboxForCoord[zRotation] = initSpinBox(zRotation, dialog, rotation.z * RAD2DEG);
	sliderForCoord[zRotation] = initSlider(zRotation, dialog, rotation.z * RAD2DEG);
	connect(sliderForCoord[zRotation], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[zRotation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// x-scale
	spinboxForCoord[xScale] = initSpinBox(xScale, dialog, currentTransform.scale.x);
	sliderForCoord[xScale] = initSlider(xScale, dialog, currentTransform.scale.x);
	connect(sliderForCoord[xScale], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[xScale], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// y-scale
	spinboxForCoord[yScale] = initSpinBox(yScale, dialog, currentTransform.scale.y);
	sliderForCoord[yScale] = initSlider(yScale, dialog, currentTransform.scale.y);
	connect(sliderForCoord[yScale], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[yScale], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// z-scale
	spinboxForCoord[zScale] = initSpinBox(zScale, dialog, currentTransform.scale.z);
	sliderForCoord[zScale] = initSlider(zScale, dialog, currentTransform.scale.z);
	connect(sliderForCoord[zScale], &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	connect(spinboxForCoord[zScale], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	// layout dialog
	QGridLayout* layout = new QGridLayout;
	dialog.setLayout(layout);

	// left, top, right, bottom
	// when showing sliders, add a little extra margin on the bottom since QSlider takes up the
	// entire vertical spacing
	layout->setContentsMargins(4, 4, 16, 4);
	layout->setSpacing(8);

	unsigned int row = 0;

	layout->addWidget(makeLabel("X"), row, 1, Qt::AlignHCenter);
	layout->addWidget(makeLabel("Y"), row, 2, Qt::AlignHCenter);
	layout->addWidget(makeLabel("Z"), row, 3, Qt::AlignHCenter);

	row++;

	layout->addWidget(makeLabel("Translation"), row, 0);
	layout->addWidget( spinboxForCoord[xTranslation], row, 1);
	layout->addWidget( spinboxForCoord[yTranslation], row, 2);
	layout->addWidget( spinboxForCoord[zTranslation], row, 3);

	row++;

	layout->addWidget( sliderForCoord[xTranslation], row, 1);
	layout->addWidget( sliderForCoord[yTranslation], row, 2);
	layout->addWidget( sliderForCoord[zTranslation], row, 3);

	row++;

	layout->addWidget(makeLabel("Rotation"), row, 0);
	layout->addWidget( spinboxForCoord[xRotation], row, 1);
	layout->addWidget( spinboxForCoord[yRotation], row, 2);
	layout->addWidget( spinboxForCoord[zRotation], row, 3);

	row++;

	layout->addWidget( sliderForCoord[xRotation], row, 1);
	layout->addWidget( sliderForCoord[yRotation], row, 2);
	layout->addWidget( sliderForCoord[zRotation], row, 3);

	row++;

	row++;

	layout->addWidget(makeLabel("Scale"), row, 0);
	layout->addWidget( spinboxForCoord[xScale], row, 1);
	layout->addWidget( spinboxForCoord[yScale], row, 2);
	layout->addWidget( spinboxForCoord[zScale], row, 3);

	row++;

	layout->addWidget( sliderForCoord[xScale], row, 1);
	layout->addWidget( sliderForCoord[yScale], row, 2);
	layout->addWidget( sliderForCoord[zScale], row, 3);

	row++;

	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);

	dialog.adjustSize();

	// Layout details
	setFirstWidgetInTabOrder(spinboxForCoord[xTranslation]);
	setLastWidgetInTabOrder(spinboxForCoord[zScale]);

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward ? spinboxForCoord[xTranslation] : spinboxForCoord[zScale])->setFocus();
	(tabCycleForward ? spinboxForCoord[xTranslation] : spinboxForCoord[zScale])->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorTransform::getAcceptedValue(void)
{
	return VuoTransform_getJson(currentTransform);
}

void VuoInputEditorTransform::setTransformProperty(coord whichCoord, double value)
{
	if(whichCoord == xTranslation)
		currentTransform.translation.x = value;
	else if(whichCoord == yTranslation)
		currentTransform.translation.y = value;
	else if(whichCoord == zTranslation)
		currentTransform.translation.z = value;
	else if(whichCoord == xRotation || whichCoord == yRotation || whichCoord == zRotation)
	{
		VuoPoint3d euler = VuoTransform_getEuler(currentTransform);

		if(whichCoord == xRotation)
			euler.x = value * DEG2RAD;
		else if(whichCoord == yRotation)
			euler.y = value * DEG2RAD;
		else if(whichCoord == zRotation)
			euler.z = value * DEG2RAD;

		currentTransform = VuoTransform_makeEuler(currentTransform.translation, euler, currentTransform.scale);
	}
	else if(whichCoord == xScale)
		currentTransform.scale.x = value;
	else if(whichCoord == yScale)
		currentTransform.scale.y = value;
	else if(whichCoord == zScale)
		currentTransform.scale.z = value;
}

VuoInputEditorTransform::coord VuoInputEditorTransform::getCoordFromQObject(QObject* sender)
{
	if( sender == sliderForCoord[xTranslation] || sender == spinboxForCoord[xTranslation] )
		return xTranslation;
	else if( sender == sliderForCoord[yTranslation] || sender == spinboxForCoord[yTranslation] )
		return yTranslation;
	else if( sender == sliderForCoord[zTranslation] || sender == spinboxForCoord[zTranslation] )
		return zTranslation;
	else if( sender == sliderForCoord[xRotation] || sender == spinboxForCoord[xRotation] )
		return xRotation;
	else if( sender == sliderForCoord[yRotation] || sender == spinboxForCoord[yRotation] )
		return yRotation;
	else if( sender == sliderForCoord[zRotation] || sender == spinboxForCoord[zRotation] )
		return zRotation;
	else if( sender == sliderForCoord[xScale] || sender == spinboxForCoord[xScale] )
		return xScale;
	else if( sender == sliderForCoord[yScale] || sender == spinboxForCoord[yScale] )
		return yScale;
	else if( sender == sliderForCoord[zScale] || sender == spinboxForCoord[zScale] )
		return zScale;

	return xTranslation;
}

/**
 * Update the spinbox and stored transform property.
 */
void VuoInputEditorTransform::onSliderUpdate(int sliderValue)
{
	coord whichCoord = getCoordFromQObject(QObject::sender());
	QSlider *targetSlider =	sliderForCoord[whichCoord];
	double value = VuoDoubleSpinBox::sliderToDouble(targetSlider->minimum(), targetSlider->maximum(), suggestedMinForCoord[whichCoord], suggestedMaxForCoord[whichCoord], sliderValue);
	setTransformProperty(whichCoord, value);

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);
	spinboxForCoord[whichCoord]->setValue( value );
	connect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform::onSpinboxUpdate);

	spinboxForCoord[whichCoord]->setFocus();
	spinboxForCoord[whichCoord]->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored transform property.
 */
void VuoInputEditorTransform::onSpinboxUpdate(QString spinboxValue)
{
	coord whichCoord = getCoordFromQObject(QObject::sender());

	double value = QLocale().toDouble(spinboxValue);
	setTransformProperty(whichCoord, value);

	QSlider *targetSlider =	sliderForCoord[whichCoord];
	int sliderValue = VuoDoubleSpinBox::doubleToSlider(targetSlider->minimum(), targetSlider->maximum(), suggestedMinForCoord[whichCoord], suggestedMaxForCoord[whichCoord], value);

	disconnect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);
	targetSlider->setValue( sliderValue );
	connect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorTransform::onSliderUpdate);

	emit valueChanged( getAcceptedValue() );
}
