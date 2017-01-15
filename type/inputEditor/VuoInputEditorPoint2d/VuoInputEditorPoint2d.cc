/**
 * @file
 * VuoInputEditorPoint2d implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorPoint2d.hh"
#include "VuoDoubleSpinBox.hh"

extern "C"
{
	#include "VuoPoint2d.h"
	#include "VuoReal.h"
}

/**
 * Constructs a VuoInputEditorPoint2d object.
 */
VuoInputEditor * VuoInputEditorPoint2dFactory::newInputEditor()
{
	return new VuoInputEditorPoint2d();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorPoint2d::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;;

	suggestedMinX = -std::numeric_limits<double>::max();
	suggestedMaxX = std::numeric_limits<double>::max();

	suggestedMinY = -std::numeric_limits<double>::max();
	suggestedMaxY = std::numeric_limits<double>::max();

	double suggestedStepX = 1.0;
	double suggestedStepY = 1.0;

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
			suggestedMinX = VuoPoint2d_makeFromJson(suggestedMinValue).x;
			suggestedMinY = VuoPoint2d_makeFromJson(suggestedMinValue).y;
			detailsIncludeSuggestedMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMaxX = VuoPoint2d_makeFromJson(suggestedMaxValue).x;
			suggestedMaxY = VuoPoint2d_makeFromJson(suggestedMaxValue).y;
			detailsIncludeSuggestedMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			suggestedStepX = VuoPoint2d_makeFromJson(suggestedStepValue).x;
			suggestedStepY = VuoPoint2d_makeFromJson(suggestedStepValue).y;
		}
	}

	// Display a QSlider or QSpinBox widget, depending whether a suggestedMin and suggestedMax were
	// both provided in the port's annotation details.
	sliderX = NULL;
	spinBoxX = NULL;

	sliderY = NULL;
	spinBoxY = NULL;

	// Layout details
	const int widgetVerticalSpacing = 4;
	const int widgetHorizontalSpacing = 5;

	QLabel *labelX = new QLabel(&dialog);
	labelX->setText("x");
	labelX->resize(QSize(labelX->fontMetrics().boundingRect(labelX->text()).width()+widgetHorizontalSpacing, labelX->height()));

	QLabel *labelY = new QLabel(&dialog);
	labelY->setText("y");
	labelY->resize(QSize(labelY->fontMetrics().boundingRect(labelY->text()).width()+widgetHorizontalSpacing, labelY->height()));

	// If suggestedMin and suggestedMax have both been provided, display a slider.
	if (detailsIncludeSuggestedMin && detailsIncludeSuggestedMax)
	{
		validator->setDecimals(decimalPrecision);

		// X value
		lineEditX = new QLineEdit(&dialog);
		setUpLineEdit(lineEditX, VuoReal_getJson(VuoPoint2d_makeFromJson(originalValue).x));
		lineEditX->setValidator(validator);
		lineEditX->installEventFilter(this);

		sliderX = new QSlider(&dialog);
		sliderX->setAttribute(Qt::WA_MacSmallSize);
		sliderX->setOrientation(Qt::Horizontal);
		sliderX->setFocusPolicy(Qt::NoFocus);
		sliderX->setMinimum(0);
		sliderX->setMaximum(sliderX->width());
		sliderX->setSingleStep(fmax(1, suggestedStepX*(sliderX->maximum()-sliderX->minimum())/(suggestedMaxX-suggestedMinX)));

		double lineEditValueX = VuoPoint2d_makeFromJson(originalValue).x;
		int sliderValueX = lineEditValueToScaledSliderValue(lineEditValueX, x);
		sliderX->setValue(sliderValueX);

		connect(sliderX, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditX, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditX, SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Y value
		lineEditY = new QLineEdit(&dialog);
		setUpLineEdit(lineEditY, VuoReal_getJson(VuoPoint2d_makeFromJson(originalValue).y));
		lineEditY->setValidator(validator);
		lineEditY->installEventFilter(this);

		sliderY = new QSlider(&dialog);
		sliderY->setAttribute(Qt::WA_MacSmallSize);
		sliderY->setOrientation(Qt::Horizontal);
		sliderY->setFocusPolicy(Qt::NoFocus);
		sliderY->setMinimum(0);
		sliderY->setMaximum(sliderY->width());
		sliderY->setSingleStep(fmax(1, suggestedStepY*(sliderY->maximum()-sliderY->minimum())/(suggestedMaxY-suggestedMinY)));

		double lineEditValueY = VuoPoint2d_makeFromJson(originalValue).y;
		int sliderValueY = lineEditValueToScaledSliderValue(lineEditValueY, y);
		sliderY->setValue(sliderValueY);

		connect(sliderY, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditY, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditY, SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Layout details
		setFirstWidgetInTabOrder(lineEditX);
		setLastWidgetInTabOrder(lineEditY);

		labelX->move(labelX->pos().x(), labelX->pos().y());
		lineEditX->move(labelX->pos().x()+labelX->width()+widgetHorizontalSpacing, lineEditX->pos().y());
		sliderX->move(lineEditX->pos().x(), lineEditX->pos().y() + lineEditX->height() + widgetVerticalSpacing);
		sliderX->resize(sliderX->width(), sliderX->height() - 10);
		lineEditX->resize(sliderX->width(), lineEditX->height());

		labelY->move(labelY->pos().x(), sliderX->pos().y() + sliderX->height() + widgetVerticalSpacing);
		lineEditY->move(labelY->pos().x()+labelY->width()+widgetHorizontalSpacing, sliderX->pos().y() + sliderX->height() + widgetVerticalSpacing);
		sliderY->move(lineEditY->pos().x(), lineEditY->pos().y() + lineEditY->height() + widgetVerticalSpacing);
		sliderY->resize(sliderY->width(), sliderY->height() - 10);
		lineEditY->resize(sliderY->width(), lineEditY->height());

		labelX->show();
		sliderX->show();

		labelY->show();
		sliderY->show();
	}

	// If either suggestedMin or suggestedMax is left unspecified, display a spinbox.
	else
	{
		// X value
		spinBoxX = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBoxX)->setButtonMinimum(suggestedMinX);
		static_cast<VuoDoubleSpinBox *>(spinBoxX)->setButtonMaximum(suggestedMaxX);
		spinBoxX->setSingleStep(suggestedStepX);
		spinBoxX->setDecimals(decimalPrecision);
		spinBoxX->setValue(VuoPoint2d_makeFromJson(originalValue).x);

		// For some reason the VuoPoint2d input editor is extremely wide
		// without the following resize() call:
		spinBoxX->resize(spinBoxX->size());

		lineEditX = spinBoxX->findChild<QLineEdit *>();
		VuoInputEditorWithLineEdit::setUpLineEdit(lineEditX, VuoReal_getJson(VuoPoint2d_makeFromJson(originalValue).x));
		spinBoxX->setKeyboardTracking(false);

		connect(spinBoxX, SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		// Y value
		spinBoxY = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBoxY)->setButtonMinimum(suggestedMinY);
		static_cast<VuoDoubleSpinBox *>(spinBoxY)->setButtonMaximum(suggestedMaxY);
		spinBoxY->setSingleStep(suggestedStepY);
		spinBoxY->setDecimals(decimalPrecision);
		spinBoxY->setValue(VuoPoint2d_makeFromJson(originalValue).y);

		// For some reason the VuoPoint2d input editor is extremely wide
		// without the following resize() call:
		spinBoxY->resize(spinBoxY->size());

		lineEditY = spinBoxY->findChild<QLineEdit *>();
		VuoInputEditorWithLineEdit::setUpLineEdit(lineEditY, VuoReal_getJson(VuoPoint2d_makeFromJson(originalValue).y));
		spinBoxY->setKeyboardTracking(false);

		connect(spinBoxY, SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		// Layout details
		setFirstWidgetInTabOrder(spinBoxX);
		setLastWidgetInTabOrder(spinBoxY);

		labelX->move(labelX->pos().x(), labelX->pos().y());
		spinBoxX->move(labelX->pos().x()+labelX->width()+widgetHorizontalSpacing, spinBoxX->pos().y());

		labelY->move(labelY->pos().x(), spinBoxX->pos().y() + spinBoxX->height() + widgetVerticalSpacing);
		spinBoxY->move(spinBoxX->pos().x(), labelY->pos().y());

		labelX->show();
		spinBoxX->show();

		labelY->show();
		spinBoxY->show();
	}

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	(tabCycleForward? lineEditX : lineEditY)->setFocus();
	(tabCycleForward? lineEditX : lineEditY)->selectAll();
}

/**
 * Returns the current text in the line edits.
 */
json_object * VuoInputEditorPoint2d::getAcceptedValue(void)
{
	return convertFromLineEditsFormat(lineEditX->text(), lineEditY->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorPoint2d::convertToLineEditFormat(json_object *value)
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
json_object * VuoInputEditorPoint2d::convertFromLineEditsFormat(const QString &xValueAsString, const QString &yValueAsString)
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

	// Point
	VuoPoint2d point;
	point.x = VuoReal_makeFromString(xValueAsStringInDefaultLocale.toUtf8().constData());
	point.y = VuoReal_makeFromString(yValueAsStringInDefaultLocale.toUtf8().constData());
	return VuoPoint2d_getJson(point);
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorPoint2d::updateSliderValue(QString newLineEditText)
{
	QObject *sender = QObject::sender();
	coord whichCoord = (sender == lineEditX? x : y);

	double newLineEditValue = QLocale::system().toDouble(newLineEditText);
	int newSliderValue = lineEditValueToScaledSliderValue(newLineEditValue, whichCoord);

	QSlider *targetSlider = (whichCoord == x? sliderX : sliderY);

	disconnect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	targetSlider->setValue(newSliderValue);
	connect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
}

/**
 * Converts the slider's current value() to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 */

void VuoInputEditorPoint2d::updateLineEditValue()
{
	QObject *sender = QObject::sender();
	coord whichCoord = (sender == sliderX? x : y);
	updateLineEditValue(((QSlider *)sender)->value(), whichCoord);
}

void VuoInputEditorPoint2d::updateLineEditValue(int newSliderValue)
{
	QObject *sender = QObject::sender();
	coord whichCoord = (sender == sliderX? x : y);

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
void VuoInputEditorPoint2d::updateLineEditValue(int newSliderValue, coord whichCoord)
{
	double newLineEditValue = sliderValueToScaledLineEditValue(newSliderValue, whichCoord);
	QLineEdit *targetLineEdit = (whichCoord == x? lineEditX : lineEditY);
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
int VuoInputEditorPoint2d::lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord)
{
	QSlider *targetSlider = (whichCoord == x? sliderX : sliderY);
	double suggestedMin = (whichCoord == x? suggestedMinX : suggestedMinY);
	double suggestedMax = (whichCoord == x? suggestedMaxX : suggestedMaxY);
	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	int scaledSliderValue = targetSlider->minimum() + ((lineEditValue-suggestedMin)/(1.0*(lineEditRange)))*sliderRange;

	return scaledSliderValue;
}

/**
 * Scales the input @c sliderValue to match the range of the
 * port's suggestedMin and suggestedMax.
 */
double VuoInputEditorPoint2d::sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord)
{
	QSlider *targetSlider = (whichCoord == x? sliderX : sliderY);
	double suggestedMin = (whichCoord == x? suggestedMinX : suggestedMinY);
	double suggestedMax = (whichCoord == x? suggestedMaxX : suggestedMaxY);
	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-targetSlider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

void VuoInputEditorPoint2d::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorPoint2d::eventFilter(QObject *object, QEvent *event)
{
	QSlider *targetSlider = (object==lineEditX? sliderX :
							(object==lineEditY? sliderY :
												NULL));

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
