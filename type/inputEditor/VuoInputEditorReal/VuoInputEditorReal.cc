/**
 * @file
 * VuoInputEditorReal implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorReal.hh"
#include "VuoDoubleSpinBox.hh"

extern "C"
{
	#include "VuoReal.h"
}

/**
 * Constructs a VuoInputEditorReal object.
 */
VuoInputEditor * VuoInputEditorRealFactory::newInputEditor()
{
	return new VuoInputEditorReal();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorReal::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;

	suggestedMin = -std::numeric_limits<double>::max();
	suggestedMax = std::numeric_limits<double>::max();
	double suggestedStep = 1.0;

	bool detailsIncludeSuggestedMin = false;
	bool detailsIncludeSuggestedMax = false;

	QDoubleValidator *validator = new QDoubleValidator(this);

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			suggestedMin = json_object_get_double(suggestedMinValue);
			detailsIncludeSuggestedMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMax = json_object_get_double(suggestedMaxValue);
			detailsIncludeSuggestedMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = json_object_get_double(suggestedStepValue);
	}

	// Display a QSlider or QSpinBox widget, depending whether a suggestedMin and suggestedMax were
	// both provided in the port's annotation details.
	slider = NULL;
	spinBox = NULL;

	// If suggestedMin and suggestedMax have both been provided, display a slider.
	if (detailsIncludeSuggestedMin && detailsIncludeSuggestedMax)
	{
		VuoInputEditorWithLineEdit::setUpDialog(dialog, originalValue, details);
		validator->setDecimals(decimalPrecision);
		lineEdit->setValidator(validator);
		lineEdit->installEventFilter(this);

		slider = new QSlider(&dialog);
		slider->setAttribute(Qt::WA_MacSmallSize);
		slider->setOrientation(Qt::Horizontal);
		slider->setFocusPolicy(Qt::NoFocus);
		slider->setMinimum(0);
		slider->setMaximum(slider->width());
		slider->setSingleStep(fmax(1, suggestedStep*(slider->maximum()-slider->minimum())/(suggestedMax-suggestedMin)));

		double lineEditValue = VuoReal_makeFromJson(originalValue);
		int sliderValue = lineEditValueToScaledSliderValue(lineEditValue);
		slider->setValue(sliderValue);

		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));

		setFirstWidgetInTabOrder(lineEdit);
		setLastWidgetInTabOrder(lineEdit);

		const int widgetVerticalSpacing = 5;
		slider->move(slider->pos().x(), slider->pos().y() + lineEdit->height() + widgetVerticalSpacing);

		slider->resize(slider->width(), slider->height() - 10);
		lineEdit->resize(slider->width(), lineEdit->height());

		slider->show();
	}

	// If either suggestedMin or suggestedMax is left unspecified, display a spinbox.
	else
	{
		spinBox = new VuoDoubleSpinBox(&dialog);
		static_cast<VuoDoubleSpinBox *>(spinBox)->setButtonMinimum(suggestedMin);
		static_cast<VuoDoubleSpinBox *>(spinBox)->setButtonMaximum(suggestedMax);
		spinBox->setSingleStep(suggestedStep);
		spinBox->setDecimals(decimalPrecision);
		spinBox->setValue(VuoReal_makeFromJson(originalValue));

		// For some reason the VuoReal input editor is extremely wide
		// without the following resize() call:
		spinBox->resize(spinBox->size());

		VuoInputEditorWithLineEdit::setUpLineEdit(spinBox->findChild<QLineEdit *>(), originalValue);
		spinBox->setKeyboardTracking(false);
		connect(spinBox, SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		setFirstWidgetInTabOrder(spinBox);
		setLastWidgetInTabOrder(spinBox);

		spinBox->show();
	}
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorReal::convertToLineEditFormat(json_object *value)
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
json_object * VuoInputEditorReal::convertFromLineEditFormat(const QString &valueAsString)
{
	QString valueAsStringCopy = valueAsString;
	double value = QLocale::system().toDouble(valueAsString);
	QString valueAsStringInDefaultLocale = QLocale(QLocale::C).toString(value);

	if (qAbs(value) >= 1000.0)
		valueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! valueAsStringInDefaultLocale.isEmpty() && valueAsStringInDefaultLocale[0] == '.')
		valueAsStringInDefaultLocale = "0" + valueAsStringInDefaultLocale;

	return VuoReal_getJson( VuoReal_makeFromString(valueAsStringInDefaultLocale.toUtf8().constData()) );
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorReal::updateSliderValue(QString newLineEditText)
{
	double newLineEditValue = QLocale::system().toDouble(newLineEditText);
	int newSliderValue = lineEditValueToScaledSliderValue(newLineEditValue);

	disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	slider->setValue(newSliderValue);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
}

/**
 * Converts the slider's current value() to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 */
void VuoInputEditorReal::updateLineEditValue()
{
	updateLineEditValue(slider->value());
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
void VuoInputEditorReal::updateLineEditValue(int newSliderValue)
{
	double newLineEditValue = sliderValueToScaledLineEditValue(newSliderValue);
	const QString originalLineEditText = lineEdit->text();
	QString newLineEditText = QLocale::system().toString(newLineEditValue, 'g');

	if (qAbs(newLineEditValue) >= 1000.0)
		newLineEditText.remove(QLocale::system().groupSeparator());

	if (originalLineEditText != newLineEditText)
	{
		lineEdit->setText(newLineEditText);
		lineEdit->setFocus();
		lineEdit->selectAll();
		emitValueChanged();
	}
}

void VuoInputEditorReal::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Scales the input @c lineEditValue to match the range of the slider.
 */
int VuoInputEditorReal::lineEditValueToScaledSliderValue(double lineEditValue)
{
	const double lineEditRange = suggestedMax - suggestedMin;
	const int sliderRange = slider->maximum() - slider->minimum();
	int scaledSliderValue = slider->minimum() + ((lineEditValue-suggestedMin)/(1.0*(lineEditRange)))*sliderRange;

	return scaledSliderValue;
}

/**
 * Scales the input @c sliderValue to match the range of the
 * port's suggestedMin and suggestedMax.
 */
double VuoInputEditorReal::sliderValueToScaledLineEditValue(int sliderValue)
{
	const double lineEditRange = suggestedMax - suggestedMin;
	const int sliderRange = slider->maximum() - slider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-slider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorReal::eventFilter(QObject *object, QEvent *event)
{
	if (event->type()==QEvent::Wheel && slider)
	{
		// Let the slider handle mouse wheel events.
		QApplication::sendEvent(slider, event);
		return true;
	}

	else if (event->type()==QEvent::KeyPress && slider)
	{
		// Let the slider handle keypresses of the up and down arrows.
		QKeyEvent *keyEvent = (QKeyEvent *)(event);
		if ((keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down))
		{
			QApplication::sendEvent(slider, event);
			return true;
		}
	}

	return VuoInputEditorWithLineEdit::eventFilter(object, event);
}
