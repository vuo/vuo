/**
 * @file
 * VuoInputEditorInteger implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorInteger.hh"
#include "VuoSpinBox.hh"

extern "C"
{
	#include "VuoInteger.h"
}

/**
 * Constructs a VuoInputEditorInteger object.
 */
VuoInputEditor * VuoInputEditorIntegerFactory::newInputEditor()
{
	return new VuoInputEditorInteger();
}

/**
 * Sets up a dialog containing either a slider and line edit (if @c details contains both "suggestedMin"
 * and "suggestedMax") or a spin box (which includes a line edit).
 */
void VuoInputEditorInteger::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	int suggestedMin = INT_MIN;
	int suggestedMax = INT_MAX;
	int suggestedStep = 1;

	bool detailsIncludeSuggestedMin = false;
	bool detailsIncludeSuggestedMax = false;

	QIntValidator *validator = new QIntValidator(this);

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			suggestedMin = json_object_get_int(suggestedMinValue);
			detailsIncludeSuggestedMin = true;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMax = json_object_get_int(suggestedMaxValue);
			detailsIncludeSuggestedMax = true;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
			suggestedStep = json_object_get_int(suggestedStepValue);
	}

	// Display a QSlider or QSpinBox widget, depending whether a suggestedMin and suggestedMax were
	// both provided in the port's annotation details.
	slider = NULL;
	spinBox = NULL;

	// If suggestedMin and suggestedMax have both been provided, display a slider.
	if (detailsIncludeSuggestedMin && detailsIncludeSuggestedMax)
	{
		VuoInputEditorWithLineEdit::setUpDialog(dialog, originalValue, details);
		lineEdit->setValidator(validator);
		lineEdit->installEventFilter(this);

		slider = new QSlider(&dialog);
		slider->setOrientation(Qt::Horizontal);
		slider->setFocusPolicy(Qt::NoFocus);
		slider->setMinimum(suggestedMin);
		slider->setMaximum(suggestedMax);
		slider->setSingleStep(suggestedStep);

		slider->setValue(VuoInteger_makeFromJson(originalValue));

		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEdit, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));

		setFirstWidgetInTabOrder(lineEdit);
		setLastWidgetInTabOrder(lineEdit);

		const int widgetVerticalSpacing = 1;
		slider->move(slider->pos().x(), slider->pos().y() + lineEdit->height() + widgetVerticalSpacing);

		slider->resize(slider->width(), slider->height() - 10);
		lineEdit->resize(slider->width(), lineEdit->height());

		slider->show();
	}

	// If either suggestedMin or suggestedMax is left unspecified, display a spinbox.
	else
	{
		spinBox = new VuoSpinBox(&dialog);
		static_cast<VuoSpinBox *>(spinBox)->setButtonMinimum(suggestedMin);
		static_cast<VuoSpinBox *>(spinBox)->setButtonMaximum(suggestedMax);
		spinBox->setSingleStep(suggestedStep);
		spinBox->setValue(VuoInteger_makeFromJson(originalValue));

		// For some reason the input editor is extremely wide
		// without the following resize() call:
		spinBox->resize(spinBox->size());

		VuoInputEditorWithLineEdit::setUpLineEdit(spinBox->findChild<QLineEdit *>(), originalValue);
		lineEdit->setValidator(validator);

		spinBox->setKeyboardTracking(false);
		connect(spinBox, SIGNAL(valueChanged(QString)), this, SLOT(emitValueChanged()));

		setFirstWidgetInTabOrder(spinBox);
		setLastWidgetInTabOrder(spinBox);

		spinBox->show();
	}
}

/**
 * Formats the value from the line edit to conform to the JSON specification for numbers.
 */
json_object * VuoInputEditorInteger::convertFromLineEditFormat(const QString &valueAsString)
{
	QString valueAsStringCopy = valueAsString;
	int value = QLocale::system().toInt(valueAsString);
	QString valueAsStringInDefaultLocale = QLocale(QLocale::C).toString(value);

	if (qAbs(value) >= 1000)
		valueAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	return VuoInteger_getJson( VuoInteger_makeFromString(valueAsStringInDefaultLocale.toUtf8().constData()) );
}

/**
 * Converts the input @c newTextValue to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorInteger::updateSliderValue(QString newTextValue)
{
	int newValue = QLocale::system().toInt(newTextValue);

	disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	slider->setValue(newValue);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
}

/**
 * Converts the slider's current value() to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 */
void VuoInputEditorInteger::updateLineEditValue()
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
void VuoInputEditorInteger::updateLineEditValue(int newSliderValue)
{
	const QString originalLineEditText = lineEdit->text();
	QString newLineEditText = QLocale::system().toString(newSliderValue);

	if (qAbs(newSliderValue) >= 1000)
		newLineEditText.remove(QLocale::system().groupSeparator());

	if (originalLineEditText != newLineEditText)
	{
		lineEdit->setText(newLineEditText);
		lineEdit->setFocus();
		lineEdit->selectAll();
		emitValueChanged();
	}
}

void VuoInputEditorInteger::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorInteger::eventFilter(QObject *object, QEvent *event)
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
