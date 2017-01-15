/**
 * @file
 * VuoInputEditorTransform implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorTransform.hh"

extern "C"
{
	#include "VuoTransform.h"
	#include "VuoReal.h"
}

/**
 * Constructs a VuoInputEditorTransform object.
 */
VuoInputEditor * VuoInputEditorTransformFactory::newInputEditor()
{
	return new VuoInputEditorTransform();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorTransform::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;

	suggestedMinForCoord[xTranslation] = -1.;
	suggestedMaxForCoord[xTranslation] = 1.;
	suggestedStepForCoord[xTranslation] = .01;

	suggestedMinForCoord[yTranslation] = -1.;
	suggestedMaxForCoord[yTranslation] = 1.;
	suggestedStepForCoord[yTranslation] = 0.01;

	suggestedMinForCoord[zTranslation] = -1.;
	suggestedMaxForCoord[zTranslation] = 1.;
	suggestedStepForCoord[zTranslation] = 0.01;

	suggestedMinForCoord[xRotation] = 0.;
	suggestedMaxForCoord[xRotation] = 360.;
	suggestedStepForCoord[xRotation] = 5.0;

	suggestedMinForCoord[yRotation] = 0.;
	suggestedMaxForCoord[yRotation] = 360.;
	suggestedStepForCoord[yRotation] = 5.0;

	suggestedMinForCoord[zRotation] = 0.;
	suggestedMaxForCoord[zRotation] = 360.;
	suggestedStepForCoord[zRotation] = 5.0;

	suggestedMinForCoord[xScale] = 0.;
	suggestedMaxForCoord[xScale] = 2.;
	suggestedStepForCoord[xScale] = 1.0;

	suggestedMinForCoord[yScale] = 0.;
	suggestedMaxForCoord[yScale] = 2.;
	suggestedStepForCoord[yScale] = 1.0;

	suggestedMinForCoord[zScale] = 0.;
	suggestedMaxForCoord[zScale] = 2.;
	suggestedStepForCoord[zScale] = 1.0;

	QDoubleValidator *validator = new QDoubleValidator(this);

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
		{
			suggestedMinForCoord[xTranslation] = VuoTransform_makeFromJson(suggestedMinValue).translation.x;
			suggestedMinForCoord[yTranslation] = VuoTransform_makeFromJson(suggestedMinValue).translation.y;
			suggestedMinForCoord[zTranslation] = VuoTransform_makeFromJson(suggestedMinValue).translation.z;
			suggestedMinForCoord[xRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMinValue)), 180./M_PI).x;
			suggestedMinForCoord[yRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMinValue)), 180./M_PI).y;
			suggestedMinForCoord[zRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMinValue)), 180./M_PI).z;
			suggestedMinForCoord[xScale] = VuoTransform_makeFromJson(suggestedMinValue).scale.x;
			suggestedMinForCoord[yScale] = VuoTransform_makeFromJson(suggestedMinValue).scale.y;
			suggestedMinForCoord[zScale] = VuoTransform_makeFromJson(suggestedMinValue).scale.z;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMaxForCoord[xTranslation] = VuoTransform_makeFromJson(suggestedMaxValue).translation.x;
			suggestedMaxForCoord[yTranslation] = VuoTransform_makeFromJson(suggestedMaxValue).translation.y;
			suggestedMaxForCoord[zTranslation] = VuoTransform_makeFromJson(suggestedMaxValue).translation.z;
			suggestedMaxForCoord[xRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMaxValue)), 180./M_PI).x;
			suggestedMaxForCoord[yRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMaxValue)), 180./M_PI).y;
			suggestedMaxForCoord[zRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedMaxValue)), 180./M_PI).z;
			suggestedMaxForCoord[xScale] = VuoTransform_makeFromJson(suggestedMaxValue).scale.x;
			suggestedMaxForCoord[yScale] = VuoTransform_makeFromJson(suggestedMaxValue).scale.y;
			suggestedMaxForCoord[zScale] = VuoTransform_makeFromJson(suggestedMaxValue).scale.z;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			suggestedStepForCoord[xTranslation] = VuoTransform_makeFromJson(suggestedStepValue).translation.x;
			suggestedStepForCoord[yTranslation] = VuoTransform_makeFromJson(suggestedStepValue).translation.y;
			suggestedStepForCoord[zTranslation] = VuoTransform_makeFromJson(suggestedStepValue).translation.z;
			suggestedStepForCoord[xRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedStepValue)), 180./M_PI).x;
			suggestedStepForCoord[yRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedStepValue)), 180./M_PI).y;
			suggestedStepForCoord[zRotation] = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(suggestedStepValue)), 180./M_PI).z;
			suggestedStepForCoord[xScale] = VuoTransform_makeFromJson(suggestedStepValue).scale.x;
			suggestedStepForCoord[yScale] = VuoTransform_makeFromJson(suggestedStepValue).scale.y;
			suggestedStepForCoord[zScale] = VuoTransform_makeFromJson(suggestedStepValue).scale.z;
		}
	}

	// Layout details
	const int widgetVerticalSpacing = 4;
	const int widgetHorizontalSpacing = -3; // Something odd going on here, but a negative value results in the desired spacing.

	labelForCoord[xTranslation] = new QLabel(&dialog);
	labelForCoord[xTranslation]->setText("x-translation");
	labelForCoord[xTranslation]->resize(QSize(labelForCoord[xTranslation]->fontMetrics().boundingRect(labelForCoord[xTranslation]->text()).width()+widgetHorizontalSpacing, labelForCoord[xTranslation]->height()));

	labelForCoord[yTranslation] = new QLabel(&dialog);
	labelForCoord[yTranslation]->setText("y-translation");
	labelForCoord[yTranslation]->resize(QSize(labelForCoord[yTranslation]->fontMetrics().boundingRect(labelForCoord[yTranslation]->text()).width()+widgetHorizontalSpacing, labelForCoord[yTranslation]->height()));

	labelForCoord[zTranslation] = new QLabel(&dialog);
	labelForCoord[zTranslation]->setText("z-translation");
	labelForCoord[zTranslation]->resize(QSize(labelForCoord[zTranslation]->fontMetrics().boundingRect(labelForCoord[zTranslation]->text()).width()+widgetHorizontalSpacing, labelForCoord[zTranslation]->height()));


	labelForCoord[xRotation] = new QLabel(&dialog);
	labelForCoord[xRotation]->setText("x-rotation");
	labelForCoord[xRotation]->resize(QSize(labelForCoord[xRotation]->fontMetrics().boundingRect(labelForCoord[xRotation]->text()).width()+widgetHorizontalSpacing, labelForCoord[xRotation]->height()));

	labelForCoord[yRotation] = new QLabel(&dialog);
	labelForCoord[yRotation]->setText("y-rotation");
	labelForCoord[yRotation]->resize(QSize(labelForCoord[yRotation]->fontMetrics().boundingRect(labelForCoord[yRotation]->text()).width()+widgetHorizontalSpacing, labelForCoord[yRotation]->height()));

	labelForCoord[zRotation] = new QLabel(&dialog);
	labelForCoord[zRotation]->setText("z-rotation");
	labelForCoord[zRotation]->resize(QSize(labelForCoord[zRotation]->fontMetrics().boundingRect(labelForCoord[zRotation]->text()).width()+widgetHorizontalSpacing, labelForCoord[zRotation]->height()));

	labelForCoord[xScale] = new QLabel(&dialog);
	labelForCoord[xScale]->setText("x-scale");
	labelForCoord[xScale]->resize(QSize(labelForCoord[xScale]->fontMetrics().boundingRect(labelForCoord[xScale]->text()).width()+widgetHorizontalSpacing, labelForCoord[xScale]->height()));

	labelForCoord[yScale] = new QLabel(&dialog);
	labelForCoord[yScale]->setText("y-scale");
	labelForCoord[yScale]->resize(QSize(labelForCoord[yScale]->fontMetrics().boundingRect(labelForCoord[yScale]->text()).width()+widgetHorizontalSpacing, labelForCoord[yScale]->height()));

	labelForCoord[zScale] = new QLabel(&dialog);
	labelForCoord[zScale]->setText("z-scale");
	labelForCoord[zScale]->resize(QSize(labelForCoord[zScale]->fontMetrics().boundingRect(labelForCoord[zScale]->text()).width()+widgetHorizontalSpacing, labelForCoord[zScale]->height()));


	validator->setDecimals(decimalPrecision);

	// x-translation
	lineEditForCoord[xTranslation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[xTranslation], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).translation.x));
	lineEditForCoord[xTranslation]->setValidator(validator);
	lineEditForCoord[xTranslation]->installEventFilter(this);

	sliderForCoord[xTranslation] = new QSlider(&dialog);
	sliderForCoord[xTranslation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[xTranslation]->setOrientation(Qt::Horizontal);
	sliderForCoord[xTranslation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[xTranslation]->setMinimum(0);
	sliderForCoord[xTranslation]->setMaximum(sliderForCoord[xTranslation]->width());
	sliderForCoord[xTranslation]->setSingleStep(fmax(1, suggestedStepForCoord[xTranslation]*(sliderForCoord[xTranslation]->maximum()-sliderForCoord[xTranslation]->minimum())/(suggestedMaxForCoord[xTranslation]-suggestedMinForCoord[xTranslation])));

	double lineEditValueXTranslation = VuoTransform_makeFromJson(originalValue).translation.x;
	int sliderValueXTranslation = lineEditValueToScaledSliderValue(lineEditValueXTranslation, xTranslation);
	sliderForCoord[xTranslation]->setValue(sliderValueXTranslation);

	connect(sliderForCoord[xTranslation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[xTranslation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[xTranslation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// y-translation
	lineEditForCoord[yTranslation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[yTranslation], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).translation.y));
	lineEditForCoord[yTranslation]->setValidator(validator);
	lineEditForCoord[yTranslation]->installEventFilter(this);

	sliderForCoord[yTranslation] = new QSlider(&dialog);
	sliderForCoord[yTranslation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[yTranslation]->setOrientation(Qt::Horizontal);
	sliderForCoord[yTranslation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[yTranslation]->setMinimum(0);
	sliderForCoord[yTranslation]->setMaximum(sliderForCoord[yTranslation]->width());
	sliderForCoord[yTranslation]->setSingleStep(fmax(1, suggestedStepForCoord[yTranslation]*(sliderForCoord[yTranslation]->maximum()-sliderForCoord[yTranslation]->minimum())/(suggestedMaxForCoord[yTranslation]-suggestedMinForCoord[yTranslation])));

	double lineEditValueYTranslation = VuoTransform_makeFromJson(originalValue).translation.y;
	int sliderValueYTranslation = lineEditValueToScaledSliderValue(lineEditValueYTranslation, yTranslation);
	sliderForCoord[yTranslation]->setValue(sliderValueYTranslation);

	connect(sliderForCoord[yTranslation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[yTranslation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[yTranslation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// z-translation
	lineEditForCoord[zTranslation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[zTranslation], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).translation.z));
	lineEditForCoord[zTranslation]->setValidator(validator);
	lineEditForCoord[zTranslation]->installEventFilter(this);

	sliderForCoord[zTranslation] = new QSlider(&dialog);
	sliderForCoord[zTranslation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[zTranslation]->setOrientation(Qt::Horizontal);
	sliderForCoord[zTranslation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[zTranslation]->setMinimum(0);
	sliderForCoord[zTranslation]->setMaximum(sliderForCoord[zTranslation]->width());
	sliderForCoord[zTranslation]->setSingleStep(fmax(1, suggestedStepForCoord[zTranslation]*(sliderForCoord[zTranslation]->maximum()-sliderForCoord[zTranslation]->minimum())/(suggestedMaxForCoord[zTranslation]-suggestedMinForCoord[zTranslation])));

	double lineEditValueZTranslation = VuoTransform_makeFromJson(originalValue).translation.z;
	int sliderValueZTranslation = lineEditValueToScaledSliderValue(lineEditValueZTranslation, zTranslation);
	sliderForCoord[zTranslation]->setValue(sliderValueZTranslation);

	connect(sliderForCoord[zTranslation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[zTranslation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[zTranslation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// x-rotation
	lineEditForCoord[xRotation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[xRotation], VuoReal_getJson(VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).x));
	lineEditForCoord[xRotation]->setValidator(validator);
	lineEditForCoord[xRotation]->installEventFilter(this);

	sliderForCoord[xRotation] = new QSlider(&dialog);
	sliderForCoord[xRotation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[xRotation]->setOrientation(Qt::Horizontal);
	sliderForCoord[xRotation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[xRotation]->setMinimum(0);
	sliderForCoord[xRotation]->setMaximum(sliderForCoord[xRotation]->width());
	sliderForCoord[xRotation]->setSingleStep(fmax(1, suggestedStepForCoord[xRotation]*(sliderForCoord[xRotation]->maximum()-sliderForCoord[xRotation]->minimum())/(suggestedMaxForCoord[xRotation]-suggestedMinForCoord[xRotation])));

	double lineEditValueXRotation = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).x;
	int sliderValueXRotation = lineEditValueToScaledSliderValue(lineEditValueXRotation, xRotation);
	sliderForCoord[xRotation]->setValue(sliderValueXRotation);

	connect(sliderForCoord[xRotation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[xRotation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[xRotation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// y-rotation
	lineEditForCoord[yRotation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[yRotation], VuoReal_getJson(VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).y));
	lineEditForCoord[yRotation]->setValidator(validator);
	lineEditForCoord[yRotation]->installEventFilter(this);

	sliderForCoord[yRotation] = new QSlider(&dialog);
	sliderForCoord[yRotation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[yRotation]->setOrientation(Qt::Horizontal);
	sliderForCoord[yRotation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[yRotation]->setMinimum(0);
	sliderForCoord[yRotation]->setMaximum(sliderForCoord[yRotation]->width());
	sliderForCoord[yRotation]->setSingleStep(fmax(1, suggestedStepForCoord[yRotation]*(sliderForCoord[yRotation]->maximum()-sliderForCoord[yRotation]->minimum())/(suggestedMaxForCoord[yRotation]-suggestedMinForCoord[yRotation])));

	double lineEditValueYRotation = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).y;
	int sliderValueYRotation = lineEditValueToScaledSliderValue(lineEditValueYRotation, yRotation);
	sliderForCoord[yRotation]->setValue(sliderValueYRotation);

	connect(sliderForCoord[yRotation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[yRotation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[yRotation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// z-rotation
	lineEditForCoord[zRotation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[zRotation], VuoReal_getJson(VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).z));
	lineEditForCoord[zRotation]->setValidator(validator);
	lineEditForCoord[zRotation]->installEventFilter(this);

	sliderForCoord[zRotation] = new QSlider(&dialog);
	sliderForCoord[zRotation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[zRotation]->setOrientation(Qt::Horizontal);
	sliderForCoord[zRotation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[zRotation]->setMinimum(0);
	sliderForCoord[zRotation]->setMaximum(sliderForCoord[zRotation]->width());
	sliderForCoord[zRotation]->setSingleStep(fmax(1, suggestedStepForCoord[zRotation]*(sliderForCoord[zRotation]->maximum()-sliderForCoord[zRotation]->minimum())/(suggestedMaxForCoord[zRotation]-suggestedMinForCoord[zRotation])));

	double lineEditValueZRotation = VuoPoint3d_multiply(VuoTransform_getEuler(VuoTransform_makeFromJson(originalValue)), 180./M_PI).z;
	int sliderValueZRotation = lineEditValueToScaledSliderValue(lineEditValueZRotation, zRotation);
	sliderForCoord[zRotation]->setValue(sliderValueZRotation);

	connect(sliderForCoord[zRotation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[zRotation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[zRotation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// x-scale
	lineEditForCoord[xScale] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[xScale], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).scale.x));
	lineEditForCoord[xScale]->setValidator(validator);
	lineEditForCoord[xScale]->installEventFilter(this);

	sliderForCoord[xScale] = new QSlider(&dialog);
	sliderForCoord[xScale]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[xScale]->setOrientation(Qt::Horizontal);
	sliderForCoord[xScale]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[xScale]->setMinimum(0);
	sliderForCoord[xScale]->setMaximum(sliderForCoord[xScale]->width());
	sliderForCoord[xScale]->setSingleStep(fmax(1, suggestedStepForCoord[xScale]*(sliderForCoord[xScale]->maximum()-sliderForCoord[xScale]->minimum())/(suggestedMaxForCoord[xScale]-suggestedMinForCoord[xScale])));

	double lineEditValueXScale = VuoTransform_makeFromJson(originalValue).scale.x;
	int sliderValueXScale = lineEditValueToScaledSliderValue(lineEditValueXScale, xScale);
	sliderForCoord[xScale]->setValue(sliderValueXScale);

	connect(sliderForCoord[xScale], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[xScale], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[xScale], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// y-scale
	lineEditForCoord[yScale] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[yScale], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).scale.y));
	lineEditForCoord[yScale]->setValidator(validator);
	lineEditForCoord[yScale]->installEventFilter(this);

	sliderForCoord[yScale] = new QSlider(&dialog);
	sliderForCoord[yScale]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[yScale]->setOrientation(Qt::Horizontal);
	sliderForCoord[yScale]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[yScale]->setMinimum(0);
	sliderForCoord[yScale]->setMaximum(sliderForCoord[yScale]->width());
	sliderForCoord[yScale]->setSingleStep(fmax(1, suggestedStepForCoord[yScale]*(sliderForCoord[yScale]->maximum()-sliderForCoord[yScale]->minimum())/(suggestedMaxForCoord[yScale]-suggestedMinForCoord[yScale])));

	double lineEditValueYScale = VuoTransform_makeFromJson(originalValue).scale.y;
	int sliderValueYScale = lineEditValueToScaledSliderValue(lineEditValueYScale, yScale);
	sliderForCoord[yScale]->setValue(sliderValueYScale);

	connect(sliderForCoord[yScale], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[yScale], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[yScale], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// z-scale
	lineEditForCoord[zScale] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[zScale], VuoReal_getJson(VuoTransform_makeFromJson(originalValue).scale.z));
	lineEditForCoord[zScale]->setValidator(validator);
	lineEditForCoord[zScale]->installEventFilter(this);

	sliderForCoord[zScale] = new QSlider(&dialog);
	sliderForCoord[zScale]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[zScale]->setOrientation(Qt::Horizontal);
	sliderForCoord[zScale]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[zScale]->setMinimum(0);
	sliderForCoord[zScale]->setMaximum(sliderForCoord[zScale]->width());
	sliderForCoord[zScale]->setSingleStep(fmax(1, suggestedStepForCoord[zScale]*(sliderForCoord[zScale]->maximum()-sliderForCoord[zScale]->minimum())/(suggestedMaxForCoord[zScale]-suggestedMinForCoord[zScale])));

	double lineEditValueZScale = VuoTransform_makeFromJson(originalValue).scale.z;
	int sliderValueZScale = lineEditValueToScaledSliderValue(lineEditValueZScale, zScale);
	sliderForCoord[zScale]->setValue(sliderValueZScale);

	connect(sliderForCoord[zScale], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[zScale], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[zScale], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// Layout details
	setFirstWidgetInTabOrder(lineEditForCoord[xTranslation]);
	setLastWidgetInTabOrder(lineEditForCoord[zScale]);

	labelForCoord[xTranslation]->move(labelForCoord[xTranslation]->pos().x(), labelForCoord[xTranslation]->pos().y());
	lineEditForCoord[xTranslation]->move(labelForCoord[xTranslation]->pos().x()+labelForCoord[xTranslation]->width()+widgetHorizontalSpacing, lineEditForCoord[xTranslation]->pos().y());
	sliderForCoord[xTranslation]->move(lineEditForCoord[xTranslation]->pos().x(), lineEditForCoord[xTranslation]->pos().y() + lineEditForCoord[xTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[xTranslation]->resize(sliderForCoord[xTranslation]->width(), sliderForCoord[xTranslation]->height() - 10);
	lineEditForCoord[xTranslation]->resize(sliderForCoord[xTranslation]->width(), lineEditForCoord[xTranslation]->height());

	labelForCoord[yTranslation]->move(labelForCoord[xTranslation]->pos().x(), sliderForCoord[xTranslation]->pos().y() + sliderForCoord[xTranslation]->height() + widgetVerticalSpacing);
	lineEditForCoord[yTranslation]->move(lineEditForCoord[xTranslation]->pos().x(), sliderForCoord[xTranslation]->pos().y() + sliderForCoord[xTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[yTranslation]->move(lineEditForCoord[yTranslation]->pos().x(), lineEditForCoord[yTranslation]->pos().y() + lineEditForCoord[yTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[yTranslation]->resize(sliderForCoord[yTranslation]->width(), sliderForCoord[yTranslation]->height() - 10);
	lineEditForCoord[yTranslation]->resize(sliderForCoord[yTranslation]->width(), lineEditForCoord[yTranslation]->height());

	labelForCoord[zTranslation]->move(labelForCoord[yTranslation]->pos().x(), sliderForCoord[yTranslation]->pos().y() + sliderForCoord[yTranslation]->height() + widgetVerticalSpacing);
	lineEditForCoord[zTranslation]->move(lineEditForCoord[yTranslation]->pos().x(), sliderForCoord[yTranslation]->pos().y() + sliderForCoord[yTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[zTranslation]->move(lineEditForCoord[zTranslation]->pos().x(), lineEditForCoord[zTranslation]->pos().y() + lineEditForCoord[zTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[zTranslation]->resize(sliderForCoord[zTranslation]->width(), sliderForCoord[zTranslation]->height() - 10);
	lineEditForCoord[zTranslation]->resize(sliderForCoord[zTranslation]->width(), lineEditForCoord[zTranslation]->height());

	labelForCoord[xRotation]->move(labelForCoord[zTranslation]->pos().x(), sliderForCoord[zTranslation]->pos().y() + sliderForCoord[zTranslation]->height() + widgetVerticalSpacing);
	lineEditForCoord[xRotation]->move(lineEditForCoord[zTranslation]->pos().x(), sliderForCoord[zTranslation]->pos().y() + sliderForCoord[zTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[xRotation]->move(lineEditForCoord[xRotation]->pos().x(), lineEditForCoord[xRotation]->pos().y() + lineEditForCoord[xRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[xRotation]->resize(sliderForCoord[xRotation]->width(), sliderForCoord[xRotation]->height() - 10);
	lineEditForCoord[xRotation]->resize(sliderForCoord[xRotation]->width(), lineEditForCoord[xRotation]->height());

	labelForCoord[yRotation]->move(labelForCoord[xRotation]->pos().x(), sliderForCoord[xRotation]->pos().y() + sliderForCoord[xRotation]->height() + widgetVerticalSpacing);
	lineEditForCoord[yRotation]->move(lineEditForCoord[xRotation]->pos().x(), sliderForCoord[xRotation]->pos().y() + sliderForCoord[xRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[yRotation]->move(lineEditForCoord[yRotation]->pos().x(), lineEditForCoord[yRotation]->pos().y() + lineEditForCoord[yRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[yRotation]->resize(sliderForCoord[yRotation]->width(), sliderForCoord[yRotation]->height() - 10);
	lineEditForCoord[yRotation]->resize(sliderForCoord[yRotation]->width(), lineEditForCoord[yRotation]->height());

	labelForCoord[zRotation]->move(labelForCoord[yRotation]->pos().x(), sliderForCoord[yRotation]->pos().y() + sliderForCoord[yRotation]->height() + widgetVerticalSpacing);
	lineEditForCoord[zRotation]->move(lineEditForCoord[yRotation]->pos().x(), sliderForCoord[yRotation]->pos().y() + sliderForCoord[yRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[zRotation]->move(lineEditForCoord[zRotation]->pos().x(), lineEditForCoord[zRotation]->pos().y() + lineEditForCoord[zRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[zRotation]->resize(sliderForCoord[zRotation]->width(), sliderForCoord[zRotation]->height() - 10);
	lineEditForCoord[zRotation]->resize(sliderForCoord[zRotation]->width(), lineEditForCoord[zRotation]->height());

	labelForCoord[xScale]->move(labelForCoord[zRotation]->pos().x(), sliderForCoord[zRotation]->pos().y() + sliderForCoord[zRotation]->height() + widgetVerticalSpacing);
	lineEditForCoord[xScale]->move(lineEditForCoord[zRotation]->pos().x(), sliderForCoord[zRotation]->pos().y() + sliderForCoord[zRotation]->height() + widgetVerticalSpacing);
	sliderForCoord[xScale]->move(lineEditForCoord[xScale]->pos().x(), lineEditForCoord[xScale]->pos().y() + lineEditForCoord[xScale]->height() + widgetVerticalSpacing);
	sliderForCoord[xScale]->resize(sliderForCoord[xScale]->width(), sliderForCoord[xScale]->height() - 10);
	lineEditForCoord[xScale]->resize(sliderForCoord[xScale]->width(), lineEditForCoord[xScale]->height());

	labelForCoord[yScale]->move(labelForCoord[xScale]->pos().x(), sliderForCoord[xScale]->pos().y() + sliderForCoord[xScale]->height() + widgetVerticalSpacing);
	lineEditForCoord[yScale]->move(lineEditForCoord[xScale]->pos().x(), sliderForCoord[xScale]->pos().y() + sliderForCoord[xScale]->height() + widgetVerticalSpacing);
	sliderForCoord[yScale]->move(lineEditForCoord[yScale]->pos().x(), lineEditForCoord[yScale]->pos().y() + lineEditForCoord[yScale]->height() + widgetVerticalSpacing);
	sliderForCoord[yScale]->resize(sliderForCoord[yScale]->width(), sliderForCoord[yScale]->height() - 10);
	lineEditForCoord[yScale]->resize(sliderForCoord[yScale]->width(), lineEditForCoord[yScale]->height());

	labelForCoord[zScale]->move(labelForCoord[yScale]->pos().x(), sliderForCoord[yScale]->pos().y() + sliderForCoord[yScale]->height() + widgetVerticalSpacing);
	lineEditForCoord[zScale]->move(lineEditForCoord[yScale]->pos().x(), sliderForCoord[yScale]->pos().y() + sliderForCoord[yScale]->height() + widgetVerticalSpacing);
	sliderForCoord[zScale]->move(lineEditForCoord[zScale]->pos().x(), lineEditForCoord[zScale]->pos().y() + lineEditForCoord[zScale]->height() + widgetVerticalSpacing);
	sliderForCoord[zScale]->resize(sliderForCoord[zScale]->width(), sliderForCoord[zScale]->height() - 10);
	lineEditForCoord[zScale]->resize(sliderForCoord[zScale]->width(), lineEditForCoord[zScale]->height());

	dialog.resize(labelForCoord[xTranslation]->width()+
				  widgetHorizontalSpacing+
				  sliderForCoord[xTranslation]->width(),
				  sliderForCoord[zScale]->pos().y()+
				  sliderForCoord[zScale]->height());

	labelForCoord[xTranslation]->show();
	sliderForCoord[xTranslation]->show();

	labelForCoord[yTranslation]->show();
	sliderForCoord[yTranslation]->show();

	labelForCoord[zTranslation]->show();
	sliderForCoord[zTranslation]->show();

	labelForCoord[xRotation]->show();
	sliderForCoord[xRotation]->show();

	labelForCoord[yRotation]->show();
	sliderForCoord[yRotation]->show();

	labelForCoord[zRotation]->show();
	sliderForCoord[zRotation]->show();

	labelForCoord[xScale]->show();
	sliderForCoord[xScale]->show();

	labelForCoord[yScale]->show();
	sliderForCoord[yScale]->show();

	labelForCoord[zScale]->show();
	sliderForCoord[zScale]->show();

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward? lineEditForCoord[xTranslation] : lineEditForCoord[zScale])->setFocus();
	(tabCycleForward? lineEditForCoord[xTranslation] : lineEditForCoord[zScale])->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorTransform::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditForCoord[xTranslation]->text(),
									  lineEditForCoord[yTranslation]->text(),
									  lineEditForCoord[zTranslation]->text(),
									  lineEditForCoord[xRotation]->text(),
									  lineEditForCoord[yRotation]->text(),
									  lineEditForCoord[zRotation]->text(),
									  lineEditForCoord[xScale]->text(),
									  lineEditForCoord[yScale]->text(),
									  lineEditForCoord[zScale]->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorTransform::convertToLineEditFormat(json_object *value)
{
	QString valueAsStringInDefaultLocale = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
	double realValue = QLocale(QLocale::C).toDouble(valueAsStringInDefaultLocale);
	QString valueAsStringInUserLocale = QLocale::system().toString(realValue);

	if (qAbs(realValue) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale;
}

/**
 * Formats the value from the line edit to conform to the JSON specification for numbers.
 */
json_object * VuoInputEditorTransform::convertFromLineEditsFormat(const QString &xTranslationAsString,
																const QString &yTranslationAsString,
																const QString &zTranslationAsString,
																const QString &xRotationAsString,
																const QString &yRotationAsString,
																const QString &zRotationAsString,
																const QString &xScaleAsString,
																const QString &yScaleAsString,
																const QString &zScaleAsString)
{
	// x-translation
	double xTranslation = QLocale::system().toDouble(xTranslationAsString);
	QString xTranslationAsStringInDefaultLocale = QLocale(QLocale::C).toString(xTranslation);

	if (qAbs(xTranslation) >= 1000.0)
		xTranslationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! xTranslationAsStringInDefaultLocale.isEmpty() && xTranslationAsStringInDefaultLocale[0] == '.')
		xTranslationAsStringInDefaultLocale = "0" + xTranslationAsStringInDefaultLocale;

	// y-translation
	double yTranslation = QLocale::system().toDouble(yTranslationAsString);
	QString yTranslationAsStringInDefaultLocale = QLocale(QLocale::C).toString(yTranslation);

	if (qAbs(yTranslation) >= 1000.0)
		yTranslationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! yTranslationAsStringInDefaultLocale.isEmpty() && yTranslationAsStringInDefaultLocale[0] == '.')
		yTranslationAsStringInDefaultLocale = "0" + yTranslationAsStringInDefaultLocale;

	// z-translation
	double zTranslation = QLocale::system().toDouble(zTranslationAsString);
	QString zTranslationAsStringInDefaultLocale = QLocale(QLocale::C).toString(zTranslation);

	if (qAbs(zTranslation) >= 1000.0)
		zTranslationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! zTranslationAsStringInDefaultLocale.isEmpty() && zTranslationAsStringInDefaultLocale[0] == '.')
		zTranslationAsStringInDefaultLocale = "0" + zTranslationAsStringInDefaultLocale;

	// x-rotation
	double xRotation = QLocale::system().toDouble(xRotationAsString);
	QString xRotationAsStringInDefaultLocale = QLocale(QLocale::C).toString(xRotation);

	if (qAbs(xRotation) >= 1000.0)
		xRotationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! xRotationAsStringInDefaultLocale.isEmpty() && xRotationAsStringInDefaultLocale[0] == '.')
		xRotationAsStringInDefaultLocale = "0" + xRotationAsStringInDefaultLocale;

	// y-rotation
	double yRotation = QLocale::system().toDouble(yRotationAsString);
	QString yRotationAsStringInDefaultLocale = QLocale(QLocale::C).toString(yRotation);

	if (qAbs(yRotation) >= 1000.0)
		yRotationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! yRotationAsStringInDefaultLocale.isEmpty() && yRotationAsStringInDefaultLocale[0] == '.')
		yRotationAsStringInDefaultLocale = "0" + yRotationAsStringInDefaultLocale;

	// z-rotation
	double zRotation = QLocale::system().toDouble(zRotationAsString);
	QString zRotationAsStringInDefaultLocale = QLocale(QLocale::C).toString(zRotation);

	if (qAbs(zRotation) >= 1000.0)
		zRotationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! zRotationAsStringInDefaultLocale.isEmpty() && zRotationAsStringInDefaultLocale[0] == '.')
		zRotationAsStringInDefaultLocale = "0" + zRotationAsStringInDefaultLocale;

	// x-scale
	double xScale = QLocale::system().toDouble(xScaleAsString);
	QString xScaleAsStringInDefaultLocale = QLocale(QLocale::C).toString(xScale);

	if (qAbs(xScale) >= 1000.0)
		xScaleAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! xScaleAsStringInDefaultLocale.isEmpty() && xScaleAsStringInDefaultLocale[0] == '.')
		xScaleAsStringInDefaultLocale = "0" + xScaleAsStringInDefaultLocale;

	// y-scale
	double yScale = QLocale::system().toDouble(yScaleAsString);
	QString yScaleAsStringInDefaultLocale = QLocale(QLocale::C).toString(yScale);

	if (qAbs(yScale) >= 1000.0)
		yScaleAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! yScaleAsStringInDefaultLocale.isEmpty() && yScaleAsStringInDefaultLocale[0] == '.')
		yScaleAsStringInDefaultLocale = "0" + yScaleAsStringInDefaultLocale;

	// z-scale
	double zScale = QLocale::system().toDouble(zScaleAsString);
	QString zScaleAsStringInDefaultLocale = QLocale(QLocale::C).toString(zScale);

	if (qAbs(zScale) >= 1000.0)
		zScaleAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! zScaleAsStringInDefaultLocale.isEmpty() && zScaleAsStringInDefaultLocale[0] == '.')
		zScaleAsStringInDefaultLocale = "0" + zScaleAsStringInDefaultLocale;


	VuoPoint3d translation;
	translation.x = VuoReal_makeFromString(xTranslationAsStringInDefaultLocale.toUtf8().constData());
	translation.y = VuoReal_makeFromString(yTranslationAsStringInDefaultLocale.toUtf8().constData());
	translation.z = VuoReal_makeFromString(zTranslationAsStringInDefaultLocale.toUtf8().constData());

	VuoPoint3d rotation;
	rotation.x = VuoReal_makeFromString(xRotationAsStringInDefaultLocale.toUtf8().constData());
	rotation.y = VuoReal_makeFromString(yRotationAsStringInDefaultLocale.toUtf8().constData());
	rotation.z = VuoReal_makeFromString(zRotationAsStringInDefaultLocale.toUtf8().constData());

	VuoPoint3d scale;
	scale.x = VuoReal_makeFromString(xScaleAsStringInDefaultLocale.toUtf8().constData());
	scale.y = VuoReal_makeFromString(yScaleAsStringInDefaultLocale.toUtf8().constData());
	scale.z = VuoReal_makeFromString(zScaleAsStringInDefaultLocale.toUtf8().constData());

	// Transform
	return VuoTransform_getJson(VuoTransform_makeEuler(translation, VuoPoint3d_multiply(rotation, M_PI/180.), scale));
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorTransform::updateSliderValue(QString newLineEditText)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == lineEditForCoord[xTranslation]?	xTranslation :
						(sender == lineEditForCoord[yTranslation]?	yTranslation :
						(sender == lineEditForCoord[zTranslation]?	zTranslation :
						(sender == lineEditForCoord[xRotation]?		xRotation :
						(sender == lineEditForCoord[yRotation]?		yRotation :
						(sender == lineEditForCoord[zRotation]?		zRotation :
						(sender == lineEditForCoord[xScale]?		xScale :
						(sender == lineEditForCoord[yScale]?		yScale :
																	zScale))))))));

	double newLineEditValue = QLocale::system().toDouble(newLineEditText);
	int newSliderValue = lineEditValueToScaledSliderValue(newLineEditValue, whichCoord);

	QSlider *targetSlider =	sliderForCoord[whichCoord];

	disconnect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	targetSlider->setValue(newSliderValue);
	connect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
}

/**
 * Converts the slider's current value() to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 */

void VuoInputEditorTransform::updateLineEditValue()
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[xTranslation]?	xTranslation :
						(sender == sliderForCoord[yTranslation]?	yTranslation :
						(sender == sliderForCoord[zTranslation]?	zTranslation :
						(sender == sliderForCoord[xRotation]?		xRotation :
						(sender == sliderForCoord[yRotation]?		yRotation :
						(sender == sliderForCoord[zRotation]?		zRotation :
						(sender == sliderForCoord[xScale]?			xScale :
						(sender == sliderForCoord[yScale]?			yScale :
																	zScale))))))));

	updateLineEditValue(((QSlider *)sender)->value(), whichCoord);
}

void VuoInputEditorTransform::updateLineEditValue(int newSliderValue)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[xTranslation]?	xTranslation :
						(sender == sliderForCoord[yTranslation]?	yTranslation :
						(sender == sliderForCoord[zTranslation]?	zTranslation :
						(sender == sliderForCoord[xRotation]?		xRotation :
						(sender == sliderForCoord[yRotation]?		yRotation :
						(sender == sliderForCoord[zRotation]?		zRotation :
						(sender == sliderForCoord[xScale]?			xScale :
						(sender == sliderForCoord[yScale]?			yScale :
																	zScale))))))));

	updateLineEditValue(newSliderValue, whichCoord);
}

/**
 * Converts the input @c newSliderValue to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 *
 * Note: The slider's sliderMoved(int) signal must be connected to this
 * slot rather than to the version of this slot that takes no arguments
 * in order to respect the most recently updated slider value.
 */
void VuoInputEditorTransform::updateLineEditValue(int newSliderValue, coord whichCoord)
{
	double newLineEditValue = sliderValueToScaledLineEditValue(newSliderValue, whichCoord);
	QLineEdit *targetLineEdit = lineEditForCoord[whichCoord];

	const QString originalLineEditText = targetLineEdit->text();
	QString newLineEditText = QLocale::system().toString(newLineEditValue, 'g');

	if (qAbs(newLineEditValue) >= 1000.0)
		newLineEditText.remove(QLocale::system().groupSeparator());

	if (originalLineEditText != newLineEditText)
	{
		targetLineEdit->setText(newLineEditText);
		targetLineEdit->setFocus();
		targetLineEdit->selectAll();

		emitValueChanged();
	}
}

/**
 * Scales the input @c lineEditValue to match the range of the slider.
 */
int VuoInputEditorTransform::lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord)
{
	QSlider *targetSlider = sliderForCoord[whichCoord];
	double suggestedMin = suggestedMinForCoord[whichCoord];
	double suggestedMax = suggestedMaxForCoord[whichCoord];

	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	int scaledSliderValue = targetSlider->minimum() + ((lineEditValue-suggestedMin)/(1.0*(lineEditRange)))*sliderRange;

	return scaledSliderValue;
}

/**
 * Scales the input @c sliderValue to match the range of the
 * port's suggestedMin and suggestedMax.
 */
double VuoInputEditorTransform::sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord)
{
	QSlider *targetSlider = sliderForCoord[whichCoord];
	double suggestedMin = suggestedMinForCoord[whichCoord];
	double suggestedMax = suggestedMaxForCoord[whichCoord];

	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-targetSlider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

void VuoInputEditorTransform::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorTransform::eventFilter(QObject *object, QEvent *event)
{
	QSlider *targetSlider = (object==lineEditForCoord[xTranslation]? sliderForCoord[xTranslation] :
							(object==lineEditForCoord[yTranslation]? sliderForCoord[yTranslation] :
							(object==lineEditForCoord[zTranslation]? sliderForCoord[zTranslation] :
							(object==lineEditForCoord[xRotation]? sliderForCoord[xRotation] :
							(object==lineEditForCoord[yRotation]? sliderForCoord[yRotation] :
							(object==lineEditForCoord[zRotation]? sliderForCoord[zRotation] :
							(object==lineEditForCoord[xScale]? sliderForCoord[xScale] :
							(object==lineEditForCoord[yScale]? sliderForCoord[yScale] :
							(object==lineEditForCoord[zScale]? sliderForCoord[zScale] :
														NULL)))))))));

	if (event->type()==QEvent::Wheel && targetSlider)
	{
		// Let the slider handle mouse wheel events.
		QApplication::sendEvent(targetSlider, event);
		return true;
	}

	else if (event->type()==QEvent::KeyPress && targetSlider)
	{
		// Let the slider handle keypresses of the up and down arrows.
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		if ((keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down))
		{
			QApplication::sendEvent(targetSlider, event);
			return true;
		}
	}

	return VuoInputEditorWithLineEdit::eventFilter(object, event);
}
