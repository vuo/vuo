/**
 * @file
 * VuoInputEditorTransform2d implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorTransform2d.hh"

extern "C"
{
	#include "VuoTransform2d.h"
	#include "VuoReal.h"
}

#define DEG2RAD 0.0174532925f                                                           ///< Convert degrees to radians.
#define RAD2DEG 57.295779513f                                                           ///< Convert radians to degrees.

/**
 * Constructs a VuoInputEditorTransform2d object.
 */
VuoInputEditor * VuoInputEditorTransform2dFactory::newInputEditor()
{
	return new VuoInputEditorTransform2d();
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
void VuoInputEditorTransform2d::setCoordMap(map<coord, double>* coordMap, VuoTransform2d transform)
{
	(*coordMap)[xTranslation] = transform.translation.x;
	(*coordMap)[yTranslation] = transform.translation.y;
	(*coordMap)[rotation] = RAD2DEG * transform.rotation;
	(*coordMap)[xScale] = transform.scale.x;
	(*coordMap)[yScale] = transform.scale.y;
}

/**
 * Alloc and init a new VuoDoubleSpinBox.
 */
VuoDoubleSpinBox* VuoInputEditorTransform2d::initSpinBox(coord whichCoord, QDialog& dialog, double initialValue)
{
	VuoDoubleSpinBox* spin = new VuoDoubleSpinBox(&dialog, 7);
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;
	spin->setDecimals(decimalPrecision);
	spin->setButtonMinimum(suggestedMinForCoord[whichCoord]);
	spin->setButtonMaximum(suggestedMaxForCoord[whichCoord]);
	spin->setSingleStep(suggestedStepForCoord[whichCoord]);
	spin->setValue(initialValue);
	spin->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

	return spin;
}

/**
 * Alloc and init a new QSlider.
 */
QSlider* VuoInputEditorTransform2d::initSlider(coord whichCoord, QDialog& dialog, double initialValue)
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
void VuoInputEditorTransform2d::setUpDialog(QDialog& dialog, json_object *originalValue, json_object *details)
{
	currentTransform = VuoTransform2d_makeFromJson(originalValue);

	bool tabCycleForward = true;

	VuoTransform2d default_min = VuoTransform2d_make( VuoPoint2d_make(-1, -.75), -180. * DEG2RAD, VuoPoint2d_make(0,0));
	VuoTransform2d default_max = VuoTransform2d_make( VuoPoint2d_make(1, .75), 180. * DEG2RAD, VuoPoint2d_make(2,2));
	VuoTransform2d default_step = VuoTransform2d_make( VuoPoint2d_make(.01, .01), 5. * DEG2RAD, VuoPoint2d_make(.01,.01));

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
			VuoTransform2d minTransform = VuoTransform2d_makeFromJson(suggestedMinValue);
			setCoordMap(&suggestedMinForCoord, minTransform);
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			VuoTransform2d maxTransform = VuoTransform2d_makeFromJson(suggestedMaxValue);
			setCoordMap(&suggestedMaxForCoord, maxTransform);
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			VuoTransform2d stepTransform = VuoTransform2d_makeFromJson(suggestedStepValue);
			setCoordMap(&suggestedStepForCoord, stepTransform);
		}
	}

	// x-translation
	spinboxForCoord[xTranslation] = initSpinBox(xTranslation, dialog, currentTransform.translation.x);
	sliderForCoord[xTranslation] = initSlider(xTranslation, dialog, currentTransform.translation.x);
	connect(sliderForCoord[xTranslation], &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	connect(spinboxForCoord[xTranslation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

	// y-translation
	spinboxForCoord[yTranslation] = initSpinBox(yTranslation, dialog, currentTransform.translation.y);
	sliderForCoord[yTranslation] = initSlider(yTranslation, dialog, currentTransform.translation.y);
	connect(sliderForCoord[yTranslation], &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	connect(spinboxForCoord[yTranslation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

	// rotation
	spinboxForCoord[rotation] = initSpinBox(rotation, dialog, currentTransform.rotation * RAD2DEG);
	sliderForCoord[rotation] = initSlider(rotation, dialog, currentTransform.rotation * RAD2DEG);
	connect(sliderForCoord[rotation], &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	connect(spinboxForCoord[rotation], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

	// x-scale
	spinboxForCoord[xScale] = initSpinBox(xScale, dialog, currentTransform.scale.x);
	sliderForCoord[xScale] = initSlider(xScale, dialog, currentTransform.scale.x);
	connect(sliderForCoord[xScale], &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	connect(spinboxForCoord[xScale], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

	// y-scale
	spinboxForCoord[yScale] = initSpinBox(yScale, dialog, currentTransform.scale.y);
	sliderForCoord[yScale] = initSlider(yScale, dialog, currentTransform.scale.y);
	connect(sliderForCoord[yScale], &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	connect(spinboxForCoord[yScale], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

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

	row++;

	layout->addWidget(makeLabel("Translation"), row, 0);
	layout->addWidget( spinboxForCoord[xTranslation], row, 1);
	layout->addWidget( spinboxForCoord[yTranslation], row, 2);

	row++;

	layout->addWidget( sliderForCoord[xTranslation], row, 1);
	layout->addWidget( sliderForCoord[yTranslation], row, 2);

	row++;

	layout->addWidget(makeLabel("Rotation"), row, 0);

	QHBoxLayout* rotation_layout = new QHBoxLayout;
	rotation_layout->addStretch(0);
	QVBoxLayout* rotation_vertical = new QVBoxLayout();
	rotation_vertical->addWidget( spinboxForCoord[rotation] );
	rotation_vertical->addWidget( sliderForCoord[rotation] );
	rotation_layout->addLayout(rotation_vertical);
	rotation_layout->addStretch(0);

	layout->addLayout(rotation_layout, row, 1, 2, 2);

	row++;

	layout->addWidget(makeLabel(""), row, 0);

	row++;

	layout->addWidget(makeLabel("Scale"), row, 0);
	layout->addWidget( spinboxForCoord[xScale], row, 1);
	layout->addWidget( spinboxForCoord[yScale], row, 2);

	row++;

	layout->addWidget( sliderForCoord[xScale], row, 1);
	layout->addWidget( sliderForCoord[yScale], row, 2);

	row++;

	// min acceptable size
	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);

	dialog.adjustSize();

	// Layout details
	setFirstWidgetInTabOrder(spinboxForCoord[xTranslation]);
	setLastWidgetInTabOrder(spinboxForCoord[yScale]);

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward ? spinboxForCoord[xTranslation] : spinboxForCoord[yScale])->setFocus();
	(tabCycleForward ? spinboxForCoord[xTranslation] : spinboxForCoord[yScale])->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorTransform2d::getAcceptedValue(void)
{
	return VuoTransform2d_getJson(currentTransform);
}

void VuoInputEditorTransform2d::setTransformProperty(coord whichCoord, double value)
{
	if(whichCoord == xTranslation)
		currentTransform.translation.x = value;
	else if(whichCoord == yTranslation)
		currentTransform.translation.y = value;
	else if(whichCoord == rotation)
		currentTransform.rotation = value * DEG2RAD;
	else if(whichCoord == xScale)
		currentTransform.scale.x = value;
	else if(whichCoord == yScale)
		currentTransform.scale.y = value;
}

VuoInputEditorTransform2d::coord VuoInputEditorTransform2d::getCoordFromQObject(QObject* sender)
{
	if( sender == sliderForCoord[xTranslation] || sender == spinboxForCoord[xTranslation] )
		return xTranslation;
	else if( sender == sliderForCoord[yTranslation] || sender == spinboxForCoord[yTranslation] )
		return yTranslation;
	else if( sender == sliderForCoord[rotation] || sender == spinboxForCoord[rotation] )
		return rotation;
	else if( sender == sliderForCoord[xScale] || sender == spinboxForCoord[xScale] )
		return xScale;
	else if( sender == sliderForCoord[yScale] || sender == spinboxForCoord[yScale] )
		return yScale;

	return xTranslation;
}

/**
 * Update the spinbox and stored transform property.
 */
void VuoInputEditorTransform2d::onSliderUpdate(int sliderValue)
{
	coord whichCoord = getCoordFromQObject(QObject::sender());
	QSlider *targetSlider =	sliderForCoord[whichCoord];
	double value = VuoDoubleSpinBox::sliderToDouble(targetSlider->minimum(), targetSlider->maximum(), suggestedMinForCoord[whichCoord], suggestedMaxForCoord[whichCoord], sliderValue);
	setTransformProperty(whichCoord, value);

	// disconnect before setting spinbox value otherwise onSpinboxUpdate is called and the whole thing just cycles
	disconnect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);
	spinboxForCoord[whichCoord]->setValue( value );
	connect(spinboxForCoord[whichCoord], static_cast<void (QDoubleSpinBox::*)(const QString &)>(&QDoubleSpinBox::valueChanged), this, &VuoInputEditorTransform2d::onSpinboxUpdate);

	spinboxForCoord[whichCoord]->setFocus();
	spinboxForCoord[whichCoord]->selectAll();

	emit valueChanged( getAcceptedValue() );
}

/**
 * Update the slider and stored transform property.
 */
void VuoInputEditorTransform2d::onSpinboxUpdate(QString spinboxValue)
{
	coord whichCoord = getCoordFromQObject(QObject::sender());

	double value = QLocale().toDouble(spinboxValue);
	setTransformProperty(whichCoord, value);

	QSlider *targetSlider =	sliderForCoord[whichCoord];
	int sliderValue = VuoDoubleSpinBox::doubleToSlider(targetSlider->minimum(), targetSlider->maximum(), suggestedMinForCoord[whichCoord], suggestedMaxForCoord[whichCoord], value);

	disconnect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);
	targetSlider->setValue( sliderValue );
	connect(targetSlider, &QSlider::valueChanged, this, &VuoInputEditorTransform2d::onSliderUpdate);

	emit valueChanged( getAcceptedValue() );
}
