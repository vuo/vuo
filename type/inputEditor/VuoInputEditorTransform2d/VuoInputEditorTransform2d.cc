/**
 * @file
 * VuoInputEditorTransform2d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorTransform2d::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;

	suggestedMinForCoord[xTranslation] = -1.;
	suggestedMaxForCoord[xTranslation] = 1.;
	suggestedStepForCoord[xTranslation] = .01;

	suggestedMinForCoord[yTranslation] = -0.75;
	suggestedMaxForCoord[yTranslation] = 0.75;
	suggestedStepForCoord[yTranslation] = 0.01;

	suggestedMinForCoord[rotation] = 0.;
	suggestedMaxForCoord[rotation] = 360.;
	suggestedStepForCoord[rotation] = 5.0;

	suggestedMinForCoord[xScale] = 0.;
	suggestedMaxForCoord[xScale] = 2.;
	suggestedStepForCoord[xScale] = 1.0;

	suggestedMinForCoord[yScale] = 0.;
	suggestedMaxForCoord[yScale] = 2.;
	suggestedStepForCoord[yScale] = 1.0;

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
			suggestedMinForCoord[xTranslation] = VuoTransform2d_makeFromJson(suggestedMinValue).translation.x;
			suggestedMinForCoord[yTranslation] = VuoTransform2d_makeFromJson(suggestedMinValue).translation.y;
			suggestedMinForCoord[rotation] = RAD2DEG * VuoTransform2d_makeFromJson(suggestedMinValue).rotation;
			suggestedMinForCoord[xScale] = VuoTransform2d_makeFromJson(suggestedMinValue).scale.x;
			suggestedMinForCoord[yScale] = VuoTransform2d_makeFromJson(suggestedMinValue).scale.y;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMaxForCoord[xTranslation] = VuoTransform2d_makeFromJson(suggestedMaxValue).translation.x;
			suggestedMaxForCoord[yTranslation] = VuoTransform2d_makeFromJson(suggestedMaxValue).translation.y;
			suggestedMaxForCoord[rotation] = RAD2DEG * VuoTransform2d_makeFromJson(suggestedMaxValue).rotation;
			suggestedMaxForCoord[xScale] = VuoTransform2d_makeFromJson(suggestedMaxValue).scale.x;
			suggestedMaxForCoord[yScale] = VuoTransform2d_makeFromJson(suggestedMaxValue).scale.y;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			suggestedStepForCoord[xTranslation] = VuoTransform2d_makeFromJson(suggestedStepValue).translation.x;
			suggestedStepForCoord[yTranslation] = VuoTransform2d_makeFromJson(suggestedStepValue).translation.y;
			suggestedStepForCoord[rotation] = RAD2DEG * VuoTransform2d_makeFromJson(suggestedStepValue).rotation;
			suggestedStepForCoord[xScale] = VuoTransform2d_makeFromJson(suggestedStepValue).scale.x;
			suggestedStepForCoord[yScale] = VuoTransform2d_makeFromJson(suggestedStepValue).scale.y;
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

	labelForCoord[rotation] = new QLabel(&dialog);
	labelForCoord[rotation]->setText("rotation");
	labelForCoord[rotation]->resize(QSize(labelForCoord[rotation]->fontMetrics().boundingRect(labelForCoord[rotation]->text()).width()+widgetHorizontalSpacing, labelForCoord[rotation]->height()));

	labelForCoord[xScale] = new QLabel(&dialog);
	labelForCoord[xScale]->setText("x-scale");
	labelForCoord[xScale]->resize(QSize(labelForCoord[xScale]->fontMetrics().boundingRect(labelForCoord[xScale]->text()).width()+widgetHorizontalSpacing, labelForCoord[xScale]->height()));

	labelForCoord[yScale] = new QLabel(&dialog);
	labelForCoord[yScale]->setText("y-scale");
	labelForCoord[yScale]->resize(QSize(labelForCoord[yScale]->fontMetrics().boundingRect(labelForCoord[yScale]->text()).width()+widgetHorizontalSpacing, labelForCoord[yScale]->height()));


	validator->setDecimals(decimalPrecision);

	// x-translation
	lineEditForCoord[xTranslation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[xTranslation], VuoReal_getJson(VuoTransform2d_makeFromJson(originalValue).translation.x));
	lineEditForCoord[xTranslation]->setValidator(validator);
	lineEditForCoord[xTranslation]->installEventFilter(this);

	sliderForCoord[xTranslation] = new QSlider(&dialog);
	sliderForCoord[xTranslation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[xTranslation]->setOrientation(Qt::Horizontal);
	sliderForCoord[xTranslation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[xTranslation]->setMinimum(0);
	sliderForCoord[xTranslation]->setMaximum(sliderForCoord[xTranslation]->width());
	sliderForCoord[xTranslation]->setSingleStep(fmax(1, suggestedStepForCoord[xTranslation]*(sliderForCoord[xTranslation]->maximum()-sliderForCoord[xTranslation]->minimum())/(suggestedMaxForCoord[xTranslation]-suggestedMinForCoord[xTranslation])));

	double lineEditValueXTranslation = VuoTransform2d_makeFromJson(originalValue).translation.x;
	int sliderValueXTranslation = lineEditValueToScaledSliderValue(lineEditValueXTranslation, xTranslation);
	sliderForCoord[xTranslation]->setValue(sliderValueXTranslation);

	connect(sliderForCoord[xTranslation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[xTranslation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[xTranslation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// y-translation
	lineEditForCoord[yTranslation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[yTranslation], VuoReal_getJson(VuoTransform2d_makeFromJson(originalValue).translation.y));
	lineEditForCoord[yTranslation]->setValidator(validator);
	lineEditForCoord[yTranslation]->installEventFilter(this);

	sliderForCoord[yTranslation] = new QSlider(&dialog);
	sliderForCoord[yTranslation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[yTranslation]->setOrientation(Qt::Horizontal);
	sliderForCoord[yTranslation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[yTranslation]->setMinimum(0);
	sliderForCoord[yTranslation]->setMaximum(sliderForCoord[yTranslation]->width());
	sliderForCoord[yTranslation]->setSingleStep(fmax(1, suggestedStepForCoord[yTranslation]*(sliderForCoord[yTranslation]->maximum()-sliderForCoord[yTranslation]->minimum())/(suggestedMaxForCoord[yTranslation]-suggestedMinForCoord[yTranslation])));

	double lineEditValueYTranslation = VuoTransform2d_makeFromJson(originalValue).translation.y;
	int sliderValueYTranslation = lineEditValueToScaledSliderValue(lineEditValueYTranslation, yTranslation);
	sliderForCoord[yTranslation]->setValue(sliderValueYTranslation);

	connect(sliderForCoord[yTranslation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[yTranslation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[yTranslation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// Rotation
	lineEditForCoord[rotation] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[rotation], VuoReal_getJson(RAD2DEG * VuoTransform2d_makeFromJson(originalValue).rotation));
	lineEditForCoord[rotation]->setValidator(validator);
	lineEditForCoord[rotation]->installEventFilter(this);

	sliderForCoord[rotation] = new QSlider(&dialog);
	sliderForCoord[rotation]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[rotation]->setOrientation(Qt::Horizontal);
	sliderForCoord[rotation]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[rotation]->setMinimum(0);
	sliderForCoord[rotation]->setMaximum(sliderForCoord[rotation]->width());
	sliderForCoord[rotation]->setSingleStep(fmax(1, suggestedStepForCoord[rotation]*(sliderForCoord[rotation]->maximum()-sliderForCoord[rotation]->minimum())/(suggestedMaxForCoord[rotation]-suggestedMinForCoord[rotation])));

	double lineEditValueRotation = RAD2DEG * VuoTransform2d_makeFromJson(originalValue).rotation;
	int sliderValueRotation = lineEditValueToScaledSliderValue(lineEditValueRotation, rotation);
	sliderForCoord[rotation]->setValue(sliderValueRotation);

	connect(sliderForCoord[rotation], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[rotation], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[rotation], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// x-scale
	lineEditForCoord[xScale] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[xScale], VuoReal_getJson(VuoTransform2d_makeFromJson(originalValue).scale.x));
	lineEditForCoord[xScale]->setValidator(validator);
	lineEditForCoord[xScale]->installEventFilter(this);

	sliderForCoord[xScale] = new QSlider(&dialog);
	sliderForCoord[xScale]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[xScale]->setOrientation(Qt::Horizontal);
	sliderForCoord[xScale]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[xScale]->setMinimum(0);
	sliderForCoord[xScale]->setMaximum(sliderForCoord[xScale]->width());
	sliderForCoord[xScale]->setSingleStep(fmax(1, suggestedStepForCoord[xScale]*(sliderForCoord[xScale]->maximum()-sliderForCoord[xScale]->minimum())/(suggestedMaxForCoord[xScale]-suggestedMinForCoord[xScale])));

	double lineEditValueXScale = VuoTransform2d_makeFromJson(originalValue).scale.x;
	int sliderValueXScale = lineEditValueToScaledSliderValue(lineEditValueXScale, xScale);
	sliderForCoord[xScale]->setValue(sliderValueXScale);

	connect(sliderForCoord[xScale], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[xScale], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[xScale], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// y-scale
	lineEditForCoord[yScale] = new QLineEdit(&dialog);
	setUpLineEdit(lineEditForCoord[yScale], VuoReal_getJson(VuoTransform2d_makeFromJson(originalValue).scale.y));
	lineEditForCoord[yScale]->setValidator(validator);
	lineEditForCoord[yScale]->installEventFilter(this);

	sliderForCoord[yScale] = new QSlider(&dialog);
	sliderForCoord[yScale]->setAttribute(Qt::WA_MacSmallSize);
	sliderForCoord[yScale]->setOrientation(Qt::Horizontal);
	sliderForCoord[yScale]->setFocusPolicy(Qt::NoFocus);
	sliderForCoord[yScale]->setMinimum(0);
	sliderForCoord[yScale]->setMaximum(sliderForCoord[yScale]->width());
	sliderForCoord[yScale]->setSingleStep(fmax(1, suggestedStepForCoord[yScale]*(sliderForCoord[yScale]->maximum()-sliderForCoord[yScale]->minimum())/(suggestedMaxForCoord[yScale]-suggestedMinForCoord[yScale])));

	double lineEditValueYScale = VuoTransform2d_makeFromJson(originalValue).scale.y;
	int sliderValueYScale = lineEditValueToScaledSliderValue(lineEditValueYScale, yScale);
	sliderForCoord[yScale]->setValue(sliderValueYScale);

	connect(sliderForCoord[yScale], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	connect(lineEditForCoord[yScale], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
	connect(lineEditForCoord[yScale], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

	// Layout details
	setFirstWidgetInTabOrder(lineEditForCoord[xTranslation]);
	setLastWidgetInTabOrder(lineEditForCoord[yScale]);

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

	labelForCoord[rotation]->move(labelForCoord[yTranslation]->pos().x(), sliderForCoord[yTranslation]->pos().y() + sliderForCoord[yTranslation]->height() + widgetVerticalSpacing);
	lineEditForCoord[rotation]->move(lineEditForCoord[yTranslation]->pos().x(), sliderForCoord[yTranslation]->pos().y() + sliderForCoord[yTranslation]->height() + widgetVerticalSpacing);
	sliderForCoord[rotation]->move(lineEditForCoord[rotation]->pos().x(), lineEditForCoord[rotation]->pos().y() + lineEditForCoord[rotation]->height() + widgetVerticalSpacing);
	sliderForCoord[rotation]->resize(sliderForCoord[rotation]->width(), sliderForCoord[rotation]->height() - 10);
	lineEditForCoord[rotation]->resize(sliderForCoord[rotation]->width(), lineEditForCoord[rotation]->height());

	labelForCoord[xScale]->move(labelForCoord[rotation]->pos().x(), sliderForCoord[rotation]->pos().y() + sliderForCoord[rotation]->height() + widgetVerticalSpacing);
	lineEditForCoord[xScale]->move(lineEditForCoord[rotation]->pos().x(), sliderForCoord[rotation]->pos().y() + sliderForCoord[rotation]->height() + widgetVerticalSpacing);
	sliderForCoord[xScale]->move(lineEditForCoord[xScale]->pos().x(), lineEditForCoord[xScale]->pos().y() + lineEditForCoord[xScale]->height() + widgetVerticalSpacing);
	sliderForCoord[xScale]->resize(sliderForCoord[xScale]->width(), sliderForCoord[xScale]->height() - 10);
	lineEditForCoord[xScale]->resize(sliderForCoord[xScale]->width(), lineEditForCoord[xScale]->height());

	labelForCoord[yScale]->move(labelForCoord[xScale]->pos().x(), sliderForCoord[xScale]->pos().y() + sliderForCoord[xScale]->height() + widgetVerticalSpacing);
	lineEditForCoord[yScale]->move(lineEditForCoord[xScale]->pos().x(), sliderForCoord[xScale]->pos().y() + sliderForCoord[xScale]->height() + widgetVerticalSpacing);
	sliderForCoord[yScale]->move(lineEditForCoord[yScale]->pos().x(), lineEditForCoord[yScale]->pos().y() + lineEditForCoord[yScale]->height() + widgetVerticalSpacing);
	sliderForCoord[yScale]->resize(sliderForCoord[yScale]->width(), sliderForCoord[yScale]->height() - 10);
	lineEditForCoord[yScale]->resize(sliderForCoord[yScale]->width(), lineEditForCoord[yScale]->height());

	dialog.resize(labelForCoord[xTranslation]->width()+
				  widgetHorizontalSpacing+
				  sliderForCoord[xTranslation]->width(),
				  sliderForCoord[yScale]->pos().y()+
				  sliderForCoord[yScale]->height());

	labelForCoord[xTranslation]->show();
	sliderForCoord[xTranslation]->show();

	labelForCoord[yTranslation]->show();
	sliderForCoord[yTranslation]->show();

	labelForCoord[rotation]->show();
	sliderForCoord[rotation]->show();

	labelForCoord[xScale]->show();
	sliderForCoord[xScale]->show();

	labelForCoord[yScale]->show();
	sliderForCoord[yScale]->show();

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward? lineEditForCoord[xTranslation] : lineEditForCoord[yScale])->setFocus();
	(tabCycleForward? lineEditForCoord[xTranslation] : lineEditForCoord[yScale])->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorTransform2d::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditForCoord[xTranslation]->text(),
									  lineEditForCoord[yTranslation]->text(),
									  lineEditForCoord[rotation]->text(),
									  lineEditForCoord[xScale]->text(),
									  lineEditForCoord[yScale]->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorTransform2d::convertToLineEditFormat(json_object *value)
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
json_object * VuoInputEditorTransform2d::convertFromLineEditsFormat(const QString &xTranslationAsString,
																const QString &yTranslationAsString,
																const QString &rotationAsString,
																const QString &xScaleAsString,
																const QString &yScaleAsString)
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

	// Rotation
	double rotation = QLocale::system().toDouble(rotationAsString);
	QString rotationAsStringInDefaultLocale = QLocale(QLocale::C).toString(rotation);

	if (qAbs(rotation) >= 1000.0)
		rotationAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! rotationAsStringInDefaultLocale.isEmpty() && rotationAsStringInDefaultLocale[0] == '.')
		rotationAsStringInDefaultLocale = "0" + rotationAsStringInDefaultLocale;

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

	// 2d Transform
	VuoTransform2d transform;
	transform.translation.x = VuoReal_makeFromString(xTranslationAsStringInDefaultLocale.toUtf8().constData());
	transform.translation.y = VuoReal_makeFromString(yTranslationAsStringInDefaultLocale.toUtf8().constData());
	transform.rotation = DEG2RAD * VuoReal_makeFromString(rotationAsStringInDefaultLocale.toUtf8().constData());
	transform.scale.x = VuoReal_makeFromString(xScaleAsStringInDefaultLocale.toUtf8().constData());
	transform.scale.y = VuoReal_makeFromString(yScaleAsStringInDefaultLocale.toUtf8().constData());
	return VuoTransform2d_getJson(transform);
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorTransform2d::updateSliderValue(QString newLineEditText)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == lineEditForCoord[xTranslation]?	xTranslation :
						(sender == lineEditForCoord[yTranslation]?	yTranslation :
						(sender == lineEditForCoord[rotation]?		rotation :
						(sender == lineEditForCoord[xScale]?		xScale :
																	yScale))));

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

void VuoInputEditorTransform2d::updateLineEditValue()
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[xTranslation]?	xTranslation :
						(sender == sliderForCoord[yTranslation]?	yTranslation :
						(sender == sliderForCoord[rotation]?		rotation :
						(sender == sliderForCoord[xScale]?			xScale :
																	yScale))));

	updateLineEditValue(((QSlider *)sender)->value(), whichCoord);
}

void VuoInputEditorTransform2d::updateLineEditValue(int newSliderValue)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[xTranslation]?	xTranslation :
						(sender == sliderForCoord[yTranslation]?	yTranslation :
						(sender == sliderForCoord[rotation]?		rotation :
						(sender == sliderForCoord[xScale]?			xScale :
																	yScale))));

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
void VuoInputEditorTransform2d::updateLineEditValue(int newSliderValue, coord whichCoord)
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
int VuoInputEditorTransform2d::lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord)
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
double VuoInputEditorTransform2d::sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord)
{
	QSlider *targetSlider = sliderForCoord[whichCoord];
	double suggestedMin = suggestedMinForCoord[whichCoord];
	double suggestedMax = suggestedMaxForCoord[whichCoord];

	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-targetSlider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

void VuoInputEditorTransform2d::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorTransform2d::eventFilter(QObject *object, QEvent *event)
{
	QSlider *targetSlider = (object==lineEditForCoord[xTranslation]? sliderForCoord[xTranslation] :
							(object==lineEditForCoord[yTranslation]? sliderForCoord[yTranslation] :
							(object==lineEditForCoord[rotation]? sliderForCoord[rotation] :
							(object==lineEditForCoord[xScale]? sliderForCoord[xScale] :
							(object==lineEditForCoord[yScale]? sliderForCoord[yScale] :
														NULL)))));

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
