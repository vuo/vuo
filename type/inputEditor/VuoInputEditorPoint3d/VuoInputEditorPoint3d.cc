/**
 * @file
 * VuoInputEditorPoint3d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorPoint3d.hh"
#include "VuoDoubleSpinBox.hh"

extern "C"
{
	#include "VuoPoint3d.h"
	#include "VuoReal.h"
}

/**
 * Constructs a VuoInputEditorPoint3d object.
 */
VuoInputEditor * VuoInputEditorPoint3dFactory::newInputEditor()
{
	return new VuoInputEditorPoint3d();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorPoint3d::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;

	suggestedMinForCoord[x] = -std::numeric_limits<double>::max();
	suggestedMaxForCoord[x] = std::numeric_limits<double>::max();
	suggestedStepForCoord[x] = 1.0;

	suggestedMinForCoord[y] = -std::numeric_limits<double>::max();
	suggestedMaxForCoord[y] = std::numeric_limits<double>::max();
	suggestedStepForCoord[y] = 1.0;

	suggestedMinForCoord[z] = -std::numeric_limits<double>::max();
	suggestedMaxForCoord[z] = std::numeric_limits<double>::max();
	suggestedStepForCoord[z] = 1.0;

	bool detailsIncludeSuggestedMin = false;
	bool detailsIncludeSuggestedMax = false;

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
			suggestedMinForCoord[x] = VuoPoint3d_makeFromJson(suggestedMinValue).x;
			suggestedMinForCoord[y] = VuoPoint3d_makeFromJson(suggestedMinValue).y;
			suggestedMinForCoord[z] = VuoPoint3d_makeFromJson(suggestedMinValue).z;
			detailsIncludeSuggestedMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMaxForCoord[x] = VuoPoint3d_makeFromJson(suggestedMaxValue).x;
			suggestedMaxForCoord[y] = VuoPoint3d_makeFromJson(suggestedMaxValue).y;
			suggestedMaxForCoord[z] = VuoPoint3d_makeFromJson(suggestedMaxValue).z;
			detailsIncludeSuggestedMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			suggestedStepForCoord[x] = VuoPoint3d_makeFromJson(suggestedStepValue).x;
			suggestedStepForCoord[y] = VuoPoint3d_makeFromJson(suggestedStepValue).y;
			suggestedStepForCoord[z] = VuoPoint3d_makeFromJson(suggestedStepValue).z;
		}
	}

	// Display a QSlider or QSpinBox widget, depending whether a suggestedMin and suggestedMax were
	// both provided in the port's annotation details.
	sliderForCoord[x] = NULL;
	spinBoxForCoord[x] = NULL;

	sliderForCoord[y] = NULL;
	spinBoxForCoord[y] = NULL;

	sliderForCoord[z] = NULL;
	spinBoxForCoord[z] = NULL;

	// Layout details
	const int widgetVerticalSpacing = 4;
	const int widgetHorizontalSpacing = 5;

	labelForCoord[x] = new QLabel(&dialog);
	labelForCoord[x]->setText("x");
	labelForCoord[x]->resize(QSize(labelForCoord[x]->fontMetrics().boundingRect(labelForCoord[x]->text()).width()+widgetHorizontalSpacing, labelForCoord[x]->height()));

	labelForCoord[y] = new QLabel(&dialog);
	labelForCoord[y]->setText("y");
	labelForCoord[y]->resize(QSize(labelForCoord[y]->fontMetrics().boundingRect(labelForCoord[y]->text()).width()+widgetHorizontalSpacing, labelForCoord[y]->height()));

	labelForCoord[z] = new QLabel(&dialog);
	labelForCoord[z]->setText("z");
	labelForCoord[z]->resize(QSize(labelForCoord[z]->fontMetrics().boundingRect(labelForCoord[z]->text()).width()+widgetHorizontalSpacing, labelForCoord[z]->height()));

	// If suggestedMin and suggestedMax have both been provided, display a slider.
	if (detailsIncludeSuggestedMin && detailsIncludeSuggestedMax)
	{
		validator->setDecimals(decimalPrecision);

		// X value
		lineEditForCoord[x] = new QLineEdit(&dialog);
		setUpLineEdit(lineEditForCoord[x], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).x));
		lineEditForCoord[x]->setValidator(validator);
		lineEditForCoord[x]->installEventFilter(this);

		sliderForCoord[x] = new QSlider(&dialog);
		sliderForCoord[x]->setAttribute(Qt::WA_MacSmallSize);
		sliderForCoord[x]->setOrientation(Qt::Horizontal);
		sliderForCoord[x]->setFocusPolicy(Qt::NoFocus);
		sliderForCoord[x]->setMinimum(0);
		sliderForCoord[x]->setMaximum(sliderForCoord[x]->width());
		sliderForCoord[x]->setSingleStep(fmax(1, suggestedStepForCoord[x]*(sliderForCoord[x]->maximum()-sliderForCoord[x]->minimum())/(suggestedMaxForCoord[x]-suggestedMinForCoord[x])));

		double lineEditValueX = VuoPoint3d_makeFromJson(originalValue).x;
		int sliderValueX = lineEditValueToScaledSliderValue(lineEditValueX, x);
		sliderForCoord[x]->setValue(sliderValueX);

		connect(sliderForCoord[x], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditForCoord[x], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditForCoord[x], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Y value
		lineEditForCoord[y] = new QLineEdit(&dialog);
		setUpLineEdit(lineEditForCoord[y], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).y));
		lineEditForCoord[y]->setValidator(validator);
		lineEditForCoord[y]->installEventFilter(this);

		sliderForCoord[y] = new QSlider(&dialog);
		sliderForCoord[y]->setAttribute(Qt::WA_MacSmallSize);
		sliderForCoord[y]->setOrientation(Qt::Horizontal);
		sliderForCoord[y]->setFocusPolicy(Qt::NoFocus);
		sliderForCoord[y]->setMinimum(0);
		sliderForCoord[y]->setMaximum(sliderForCoord[y]->width());
		sliderForCoord[y]->setSingleStep(fmax(1, suggestedStepForCoord[y]*(sliderForCoord[y]->maximum()-sliderForCoord[y]->minimum())/(suggestedMaxForCoord[y]-suggestedMinForCoord[y])));

		double lineEditValueY = VuoPoint3d_makeFromJson(originalValue).y;
		int sliderValueY = lineEditValueToScaledSliderValue(lineEditValueY, y);
		sliderForCoord[y]->setValue(sliderValueY);

		connect(sliderForCoord[y], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditForCoord[y], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditForCoord[y], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Z value
		lineEditForCoord[z] = new QLineEdit(&dialog);
		setUpLineEdit(lineEditForCoord[z], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).z));
		lineEditForCoord[z]->setValidator(validator);
		lineEditForCoord[z]->installEventFilter(this);

		sliderForCoord[z] = new QSlider(&dialog);
		sliderForCoord[z]->setAttribute(Qt::WA_MacSmallSize);
		sliderForCoord[z]->setOrientation(Qt::Horizontal);
		sliderForCoord[z]->setFocusPolicy(Qt::NoFocus);
		sliderForCoord[z]->setMinimum(0);
		sliderForCoord[z]->setMaximum(sliderForCoord[z]->width());
		sliderForCoord[z]->setSingleStep(fmax(1, suggestedStepForCoord[z]*(sliderForCoord[z]->maximum()-sliderForCoord[z]->minimum())/(suggestedMaxForCoord[z]-suggestedMinForCoord[z])));

		double lineEditValueZ = VuoPoint3d_makeFromJson(originalValue).z;
		int sliderValueZ = lineEditValueToScaledSliderValue(lineEditValueZ, z);
		sliderForCoord[z]->setValue(sliderValueZ);

		connect(sliderForCoord[z], SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditForCoord[z], SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditForCoord[z], SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Layout details
		setFirstWidgetInTabOrder(lineEditForCoord[x]);
		setLastWidgetInTabOrder(lineEditForCoord[z]);

		labelForCoord[x]->move(labelForCoord[x]->pos().x(), labelForCoord[x]->pos().y());
		lineEditForCoord[x]->move(labelForCoord[x]->pos().x()+labelForCoord[x]->width()+widgetHorizontalSpacing, lineEditForCoord[x]->pos().y());
		sliderForCoord[x]->move(lineEditForCoord[x]->pos().x(), lineEditForCoord[x]->pos().y() + lineEditForCoord[x]->height() + widgetVerticalSpacing);
		sliderForCoord[x]->resize(sliderForCoord[x]->width(), sliderForCoord[x]->height() - 10);
		lineEditForCoord[x]->resize(sliderForCoord[x]->width(), lineEditForCoord[x]->height());

		labelForCoord[y]->move(labelForCoord[y]->pos().x(), sliderForCoord[x]->pos().y() + sliderForCoord[x]->height());
		lineEditForCoord[y]->move(labelForCoord[y]->pos().x()+labelForCoord[y]->width()+widgetHorizontalSpacing, sliderForCoord[x]->pos().y() + sliderForCoord[x]->height() + widgetVerticalSpacing);
		sliderForCoord[y]->move(lineEditForCoord[y]->pos().x(), lineEditForCoord[y]->pos().y() + lineEditForCoord[y]->height() + widgetVerticalSpacing);
		sliderForCoord[y]->resize(sliderForCoord[y]->width(), sliderForCoord[y]->height() - 10);
		lineEditForCoord[y]->resize(sliderForCoord[y]->width(), lineEditForCoord[y]->height());

		labelForCoord[z]->move(labelForCoord[z]->pos().x(), sliderForCoord[y]->pos().y() + sliderForCoord[y]->height());
		lineEditForCoord[z]->move(labelForCoord[z]->pos().x()+labelForCoord[z]->width()+widgetHorizontalSpacing, sliderForCoord[y]->pos().y() + sliderForCoord[y]->height() + widgetVerticalSpacing);
		sliderForCoord[z]->move(lineEditForCoord[z]->pos().x(), lineEditForCoord[z]->pos().y() + lineEditForCoord[z]->height() + widgetVerticalSpacing);
		sliderForCoord[z]->resize(sliderForCoord[z]->width(), sliderForCoord[z]->height() - 10);
		lineEditForCoord[z]->resize(sliderForCoord[z]->width(), lineEditForCoord[z]->height());

		dialog.resize(labelForCoord[x]->width()+
					  widgetHorizontalSpacing+
					  sliderForCoord[x]->width(),
					  sliderForCoord[z]->pos().y()+
					  sliderForCoord[z]->height());

		labelForCoord[x]->show();
		sliderForCoord[x]->show();

		labelForCoord[y]->show();
		sliderForCoord[y]->show();

		labelForCoord[z]->show();
		sliderForCoord[z]->show();
	}

	// If either suggestedMin or suggestedMax is left unspecified, display a spinbox.
	else
	{
		// X value
		spinBoxForCoord[x] = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[x])->setButtonMinimum(suggestedMinForCoord[x]);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[x])->setButtonMaximum(suggestedMaxForCoord[x]);
		spinBoxForCoord[x]->setSingleStep(suggestedStepForCoord[x]);
		spinBoxForCoord[x]->setDecimals(decimalPrecision);
		spinBoxForCoord[x]->setValue(VuoPoint3d_makeFromJson(originalValue).x);

		// For some reason the VuoPoint3d input editor is extremely wide
		// without the following resize() call:
		spinBoxForCoord[x]->resize(spinBoxForCoord[x]->size());

		lineEditForCoord[x] = spinBoxForCoord[x]->findChild<QLineEdit *>();
		VuoInputEditorWithLineEdit::setUpLineEdit(lineEditForCoord[x], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).x));
		spinBoxForCoord[x]->setKeyboardTracking(false);

		connect(spinBoxForCoord[x], SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		// Y value
		spinBoxForCoord[y] = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[y])->setButtonMinimum(suggestedMinForCoord[y]);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[y])->setButtonMaximum(suggestedMaxForCoord[y]);
		spinBoxForCoord[y]->setSingleStep(suggestedStepForCoord[y]);
		spinBoxForCoord[y]->setDecimals(decimalPrecision);
		spinBoxForCoord[y]->setValue(VuoPoint3d_makeFromJson(originalValue).y);

		// For some reason the VuoPoint3d input editor is extremely wide
		// without the following resize() call:
		spinBoxForCoord[y]->resize(spinBoxForCoord[y]->size());

		lineEditForCoord[y] = spinBoxForCoord[y]->findChild<QLineEdit *>();
		VuoInputEditorWithLineEdit::setUpLineEdit(lineEditForCoord[y], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).y));
		spinBoxForCoord[y]->setKeyboardTracking(false);

		connect(spinBoxForCoord[y], SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		// Z value
		spinBoxForCoord[z] = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[z])->setButtonMinimum(suggestedMinForCoord[z]);
		static_cast<VuoDoubleSpinBox *>(spinBoxForCoord[z])->setButtonMaximum(suggestedMaxForCoord[z]);
		spinBoxForCoord[z]->setSingleStep(suggestedStepForCoord[z]);
		spinBoxForCoord[z]->setDecimals(decimalPrecision);
		spinBoxForCoord[z]->setValue(VuoPoint3d_makeFromJson(originalValue).z);

		// For some reason the VuoPoint3d input editor is extremely wide
		// without the following resize() call:
		spinBoxForCoord[z]->resize(spinBoxForCoord[z]->size());

		lineEditForCoord[z] = spinBoxForCoord[z]->findChild<QLineEdit *>();
		VuoInputEditorWithLineEdit::setUpLineEdit(lineEditForCoord[z], VuoReal_getJson(VuoPoint3d_makeFromJson(originalValue).z));
		spinBoxForCoord[z]->setKeyboardTracking(false);

		connect(spinBoxForCoord[z], SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		// Layout details
		setFirstWidgetInTabOrder(spinBoxForCoord[x]);
		setLastWidgetInTabOrder(spinBoxForCoord[z]);

		labelForCoord[x]->move(labelForCoord[x]->pos().x(), labelForCoord[x]->pos().y());
		spinBoxForCoord[x]->move(labelForCoord[x]->pos().x()+labelForCoord[x]->width()+widgetHorizontalSpacing, spinBoxForCoord[x]->pos().y());

		labelForCoord[y]->move(labelForCoord[y]->pos().x(), spinBoxForCoord[x]->pos().y() + spinBoxForCoord[x]->height() + widgetVerticalSpacing);
		spinBoxForCoord[y]->move(spinBoxForCoord[x]->pos().x(), labelForCoord[y]->pos().y());

		labelForCoord[z]->move(labelForCoord[z]->pos().x(), spinBoxForCoord[y]->pos().y() + spinBoxForCoord[y]->height() + widgetVerticalSpacing);
		spinBoxForCoord[z]->move(spinBoxForCoord[x]->pos().x(), labelForCoord[z]->pos().y());

		dialog.resize(labelForCoord[x]->pos().x() + labelForCoord[x]->width()+
					  2*widgetHorizontalSpacing+
					  spinBoxForCoord[x]->width(),
					  spinBoxForCoord[z]->pos().y()+
					  spinBoxForCoord[z]->height()+
					  widgetVerticalSpacing);

		labelForCoord[x]->show();
		spinBoxForCoord[x]->show();

		labelForCoord[y]->show();
		spinBoxForCoord[y]->show();

		labelForCoord[z]->show();
		spinBoxForCoord[z]->show();
	}

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward? lineEditForCoord[x] : lineEditForCoord[z])->setFocus();
	(tabCycleForward? lineEditForCoord[x] : lineEditForCoord[z])->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorPoint3d::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditForCoord[x]->text(),
									  lineEditForCoord[y]->text(),
									  lineEditForCoord[z]->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorPoint3d::convertToLineEditFormat(json_object *value)
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
json_object * VuoInputEditorPoint3d::convertFromLineEditsFormat(const QString &xValueAsString,
																const QString &yValueAsString,
																const QString &zValueAsString)
{
	// X value
	double xValue = QLocale::system().toDouble(xValueAsString);
	QString xValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(xValue);

	if (qAbs(xValue) >= 1000.0)
		xValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! xValueAsStringInDefaultLocale.isEmpty() && xValueAsStringInDefaultLocale[0] == '.')
		xValueAsStringInDefaultLocale = "0" + xValueAsStringInDefaultLocale;

	// Y value
	double yValue = QLocale::system().toDouble(yValueAsString);
	QString yValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(yValue);

	if (qAbs(yValue) >= 1000.0)
		yValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! yValueAsStringInDefaultLocale.isEmpty() && yValueAsStringInDefaultLocale[0] == '.')
		yValueAsStringInDefaultLocale = "0" + yValueAsStringInDefaultLocale;

	// Z value
	double zValue = QLocale::system().toDouble(zValueAsString);
	QString zValueAsStringInDefaultLocale = QLocale(QLocale::C).toString(zValue);

	if (qAbs(zValue) >= 1000.0)
		zValueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! zValueAsStringInDefaultLocale.isEmpty() && zValueAsStringInDefaultLocale[0] == '.')
		zValueAsStringInDefaultLocale = "0" + zValueAsStringInDefaultLocale;

	// Point
	VuoPoint3d point;
	point.x = VuoReal_makeFromString(xValueAsStringInDefaultLocale.toUtf8().constData());
	point.y = VuoReal_makeFromString(yValueAsStringInDefaultLocale.toUtf8().constData());
	point.z = VuoReal_makeFromString(zValueAsStringInDefaultLocale.toUtf8().constData());
	return VuoPoint3d_getJson(point);
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorPoint3d::updateSliderValue(QString newLineEditText)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == lineEditForCoord[x]? x :
						(sender == lineEditForCoord[y]? y :
														z));

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

void VuoInputEditorPoint3d::updateLineEditValue()
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[x]?	x :
						(sender == sliderForCoord[y]?	y :
														z));

	updateLineEditValue(((QSlider *)sender)->value(), whichCoord);
}

void VuoInputEditorPoint3d::updateLineEditValue(int newSliderValue)
{
	QObject *sender = QObject::sender();
	coord whichCoord =	(sender == sliderForCoord[x]?	x :
						(sender == sliderForCoord[y]?	y :
														z));

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
void VuoInputEditorPoint3d::updateLineEditValue(int newSliderValue, coord whichCoord)
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
int VuoInputEditorPoint3d::lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord)
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
double VuoInputEditorPoint3d::sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord)
{
	QSlider *targetSlider = sliderForCoord[whichCoord];
	double suggestedMin = suggestedMinForCoord[whichCoord];
	double suggestedMax = suggestedMaxForCoord[whichCoord];

	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-targetSlider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

void VuoInputEditorPoint3d::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorPoint3d::eventFilter(QObject *object, QEvent *event)
{
	QSlider *targetSlider = (object==lineEditForCoord[x]? sliderForCoord[x] :
							(object==lineEditForCoord[y]? sliderForCoord[y] :
							(object==lineEditForCoord[z]? sliderForCoord[z] :
														NULL)));

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
