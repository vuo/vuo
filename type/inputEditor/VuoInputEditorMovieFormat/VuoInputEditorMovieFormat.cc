/**
 * @file
 * VuoInputEditorMovieFormat implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorMovieFormat.hh"

const QColor VuoInputEditorMovieFormat::darkModeEnabledLabelTextColor = QColor(207, 207, 207);
const QColor VuoInputEditorMovieFormat::darkModeDisabledLabelTextColor = QColor(127, 127, 127);

extern "C"
{
	#include "VuoMovieFormat.h"
	#include "VuoReal.h"
}

/**
 * Constructs a VuoInputEditorMovieFormat object.
 */
VuoInputEditor * VuoInputEditorMovieFormatFactory::newInputEditor()
{
	return new VuoInputEditorMovieFormat();
}

/**
 * Sets up a dialog containing slider/line-edit combinations for video quality and audio quality,
 * and drop-down menus for video encoding and audio encoding.
 */
void VuoInputEditorMovieFormat::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	const int decimalPrecision = DBL_MAX_10_EXP + DBL_DIG;

	suggestedMinImageQuality = 0;
	suggestedMaxImageQuality = 1;

	suggestedMinAudioQuality = 0;
	suggestedMaxAudioQuality = 1;

	double suggestedStepImageQuality = 0.1;
	double suggestedStepAudioQuality = 0.1;

	QDoubleValidator *validator = new QDoubleValidator(this);
	validator->setDecimals(decimalPrecision);

	isDark = false;
	tabCycleForward = true;

	// Parse supported port annotations from the port's "details" JSON object:
	if (details)
	{
		json_object *darkMode = NULL;
		if (json_object_object_get_ex(details, "isDark", &darkMode))
			isDark = json_object_get_boolean(darkMode);

		json_object *forwardTabTraversal = NULL;
		if (json_object_object_get_ex(details, "forwardTabTraversal", &forwardTabTraversal))
			tabCycleForward = json_object_get_boolean(forwardTabTraversal);

		// "suggestedMin"
		json_object *suggestedMinValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMin", &suggestedMinValue))
		{
			suggestedMinImageQuality = VuoMovieFormat_makeFromJson(suggestedMinValue).imageQuality;
			suggestedMinAudioQuality = VuoMovieFormat_makeFromJson(suggestedMinValue).audioQuality;
		}

		// "suggestedMax"
		json_object *suggestedMaxValue = NULL;
		if (json_object_object_get_ex(details, "suggestedMax", &suggestedMaxValue))
		{
			suggestedMaxImageQuality = VuoMovieFormat_makeFromJson(suggestedMaxValue).imageQuality;
			suggestedMaxAudioQuality = VuoMovieFormat_makeFromJson(suggestedMaxValue).audioQuality;
		}

		// "suggestedStep"
		json_object *suggestedStepValue = NULL;
		if (json_object_object_get_ex(details, "suggestedStep", &suggestedStepValue))
		{
			suggestedStepImageQuality = VuoMovieFormat_makeFromJson(suggestedStepValue).imageQuality;
			suggestedStepAudioQuality = VuoMovieFormat_makeFromJson(suggestedStepValue).audioQuality;
		}
	}

	dialog.setStyleSheet("");
	QFont font = QFont("Signika", 10, QFont::Light, false);

	// Display menus for "Image Encoding" and "Audio Encoding".
	{
		string originalMovieImageEncodingAsString = json_object_to_json_string_ext(VuoMovieImageEncoding_getJson(VuoMovieFormat_makeFromJson(originalValue).imageEncoding), JSON_C_TO_STRING_PLAIN);
		comboBoxImageEncoding = new QComboBox(&dialog);
		comboBoxImageEncoding->setFont(font);
		setUpComboBoxForType(comboBoxImageEncoding, "VuoMovieImageEncoding", originalMovieImageEncodingAsString);
		connect(comboBoxImageEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(updateQualitySliderEnabledStatusAndEmitValueChanged()));

		string originalAudioEncodingAsString = json_object_to_json_string_ext(VuoAudioEncoding_getJson(VuoMovieFormat_makeFromJson(originalValue).audioEncoding), JSON_C_TO_STRING_PLAIN);
		comboBoxAudioEncoding = new QComboBox(&dialog);
		comboBoxAudioEncoding->setFont(font);
		setUpComboBoxForType(comboBoxAudioEncoding, "VuoAudioEncoding", originalAudioEncodingAsString);
		connect(comboBoxAudioEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(updateQualitySliderEnabledStatusAndEmitValueChanged()));
	}

	// Display sliders for "Image Quality" and "Audio Quality".
	{
		// Image Quality
		lineEditImageQuality = new QLineEdit(&dialog);
		lineEditImageQuality->setFont(font);
		lineEditImageQuality->setText( convertToLineEditFormat( VuoReal_getJson(VuoMovieFormat_makeFromJson(originalValue).imageQuality) ) );
		lineEditImageQuality->adjustSize();

		lineEditImageQuality->setValidator(validator);
		lineEditImageQuality->installEventFilter(this);

		sliderImageQuality = new QSlider(&dialog);
		sliderImageQuality->setAttribute(Qt::WA_MacSmallSize);
		sliderImageQuality->setOrientation(Qt::Horizontal);
		sliderImageQuality->setFocusPolicy(Qt::NoFocus);
		sliderImageQuality->setMinimum(0);
		sliderImageQuality->setMaximum(sliderImageQuality->width());
		sliderImageQuality->setSingleStep(fmax(1, suggestedStepImageQuality*(sliderImageQuality->maximum()-sliderImageQuality->minimum())/(suggestedMaxImageQuality-suggestedMinImageQuality)));

		double lineEditValueImageQuality = VuoMovieFormat_makeFromJson(originalValue).imageQuality;
		int sliderValueImageQuality = lineEditValueToScaledSliderValue(lineEditValueImageQuality, imageQuality);
		sliderImageQuality->setValue(sliderValueImageQuality);

		connect(sliderImageQuality, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditImageQuality, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditImageQuality, SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));

		// Audio Quality
		lineEditAudioQuality = new QLineEdit(&dialog);
		lineEditAudioQuality->setFont(font);
		lineEditAudioQuality->setText( convertToLineEditFormat( VuoReal_getJson(VuoMovieFormat_makeFromJson(originalValue).audioQuality) ) );
		lineEditAudioQuality->adjustSize();

		lineEditAudioQuality->setValidator(validator);
		lineEditAudioQuality->installEventFilter(this);

		sliderAudioQuality = new QSlider(&dialog);
		sliderAudioQuality->setAttribute(Qt::WA_MacSmallSize);
		sliderAudioQuality->setOrientation(Qt::Horizontal);
		sliderAudioQuality->setFocusPolicy(Qt::NoFocus);
		sliderAudioQuality->setMinimum(0);
		sliderAudioQuality->setMaximum(sliderAudioQuality->width());
		sliderAudioQuality->setSingleStep(fmax(1, suggestedStepAudioQuality*(sliderAudioQuality->maximum()-sliderAudioQuality->minimum())/(suggestedMaxAudioQuality-suggestedMinAudioQuality)));

		double lineEditValueAudioQuality = VuoMovieFormat_makeFromJson(originalValue).audioQuality;
		int sliderValueAudioQuality = lineEditValueToScaledSliderValue(lineEditValueAudioQuality, audioQuality);
		sliderAudioQuality->setValue(sliderValueAudioQuality);

		connect(sliderAudioQuality, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
		connect(lineEditAudioQuality, SIGNAL(textEdited(QString)), this, SLOT(updateSliderValue(QString)));
		connect(lineEditAudioQuality, SIGNAL(editingFinished()), this, SLOT(emitValueChanged()));
	}

	// Layout details
	{
		const int widgetVerticalSpacing = 4;
		const int widgetHorizontalSpacing = 5;

		// Tweak to better align the text of labels with the text in their associated widgets.
		const int comboLabelVerticalCorrection = 3;
		const int lineEditLabelVerticalCorrection = 1;

		QLabel *labelImageEncoding = new QLabel("Image Encoding", &dialog);
		QLabel *labelAudioEncoding = new QLabel("Audio Encoding", &dialog);
		labelImageQuality = new QLabel("Image Quality", &dialog);
		labelAudioQuality = new QLabel("Audio Quality", &dialog);

		setFirstWidgetInTabOrder(lineEditImageQuality);
		setLastWidgetInTabOrder(lineEditAudioQuality);

		// Formatting specific to Dark Interface mode
		// @todo https://b33p.net/kosada/node/10139 : Adapt global dark-mode stylesheet to handle disabled widgets.
		if (isDark)
		{
			QPalette labelPalette = labelImageEncoding->palette();
			labelPalette.setColor(QPalette::Foreground, darkModeEnabledLabelTextColor);
			labelImageEncoding->setPalette(labelPalette);
			labelAudioEncoding->setPalette(labelPalette);
			labelImageQuality->setPalette(labelPalette);
			labelAudioQuality->setPalette(labelPalette);
		}

		updateQualitySliderEnabledStatus();

		labelImageEncoding->move(	labelImageEncoding->pos().x(),
									labelImageEncoding->pos().y() + comboLabelVerticalCorrection);
		comboBoxImageEncoding->move(labelImageEncoding->pos().x()+labelImageEncoding->width() + widgetHorizontalSpacing,
									labelImageEncoding->pos().y() - comboLabelVerticalCorrection);

		labelImageQuality->move(	labelImageEncoding->pos().x(),
									comboBoxImageEncoding->pos().y() + comboBoxImageEncoding->height() + widgetVerticalSpacing + lineEditLabelVerticalCorrection);
		lineEditImageQuality->move(	comboBoxImageEncoding->pos().x(),
									labelImageQuality->pos().y() - lineEditLabelVerticalCorrection);
		sliderImageQuality->move(	lineEditImageQuality->pos().x(),
									lineEditImageQuality->pos().y() + lineEditImageQuality->height() + widgetVerticalSpacing - lineEditLabelVerticalCorrection);

		sliderImageQuality->resize(sliderImageQuality->width(), sliderImageQuality->height() - 10);
		lineEditImageQuality->resize(sliderImageQuality->width(), lineEditImageQuality->height());

		labelAudioEncoding->move(	labelImageQuality->pos().x(),
									sliderImageQuality->pos().y() + sliderImageQuality->height() + widgetVerticalSpacing + comboLabelVerticalCorrection);
		comboBoxAudioEncoding->move(labelAudioEncoding->pos().x() + labelAudioEncoding->width() + widgetHorizontalSpacing,
									labelAudioEncoding->pos().y() - comboLabelVerticalCorrection);


		labelAudioQuality->move(	labelAudioEncoding->pos().x(),
									comboBoxAudioEncoding->pos().y() + comboBoxAudioEncoding->height() + widgetVerticalSpacing + lineEditLabelVerticalCorrection);
		lineEditAudioQuality->move(	comboBoxAudioEncoding->pos().x(),
									labelAudioQuality->pos().y() - lineEditLabelVerticalCorrection);
		sliderAudioQuality->move(	lineEditAudioQuality->pos().x(),
									lineEditAudioQuality->pos().y() + lineEditAudioQuality->height() + widgetVerticalSpacing - lineEditLabelVerticalCorrection);

		sliderAudioQuality->resize(sliderAudioQuality->width(), sliderAudioQuality->height() - 10);
		lineEditAudioQuality->resize(sliderAudioQuality->width(), lineEditAudioQuality->height());

		labelImageEncoding->show();
		comboBoxImageEncoding->show();

		labelImageQuality->show();
		sliderImageQuality->show();

		labelAudioEncoding->show();
		comboBoxAudioEncoding->show();

		labelAudioQuality->show();
		sliderAudioQuality->show();

		dialog.adjustSize();
	}
}

/**
 * Returns the current values held in the child widgets.
 */
json_object * VuoInputEditorMovieFormat::getAcceptedValue(void)
{
	return convertFromSubwidgetFormats(comboBoxImageEncoding->currentData(),
									  lineEditImageQuality->text(),
									  comboBoxAudioEncoding->currentData(),
									  lineEditAudioQuality->text());
}

/**
 * Returns the text that should appear in the line edit to represent @c value.
 */
QString VuoInputEditorMovieFormat::convertToLineEditFormat(json_object *value)
{
	QString valueAsStringInDefaultLocale = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
	double realValue = QLocale(QLocale::C).toDouble(valueAsStringInDefaultLocale);
	QString valueAsStringInUserLocale = QLocale::system().toString(realValue);

	if (qAbs(realValue) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale;
}

/**
 * Formats the values from the constituent widgets to conform to the JSON specification for VuoMovieFormats.
 */
json_object * VuoInputEditorMovieFormat::convertFromSubwidgetFormats(const QVariant &imageEncoding,
																	const QString &imageQualityAsString,
																	const QVariant &audioEncoding,
																	const QString &audioQualityAsString)
{
	// Image Encoding
	json_object *imageEncodingValue = (json_object *)imageEncoding.value<void *>();

	// Audio Encoding
	json_object *audioEncodingValue = (json_object *)audioEncoding.value<void *>();

	// Image Quality
	double imageQualityValue = QLocale::system().toDouble(imageQualityAsString);
	QString imageQualityAsStringInDefaultLocale = QLocale(QLocale::C).toString(imageQualityValue);

	if (qAbs(imageQualityValue) >= 1000.0)
		imageQualityAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! imageQualityAsStringInDefaultLocale.isEmpty() && imageQualityAsStringInDefaultLocale[0] == '.')
		imageQualityAsStringInDefaultLocale = "0" + imageQualityAsStringInDefaultLocale;

	// Audio Quality
	double audioQualityValue = QLocale::system().toDouble(audioQualityAsString);
	QString audioQualityAsStringInDefaultLocale = QLocale(QLocale::C).toString(audioQualityValue);

	if (qAbs(audioQualityValue) >= 1000.0)
		audioQualityAsStringInDefaultLocale.remove(QLocale(QLocale::C).groupSeparator());

	if (! audioQualityAsStringInDefaultLocale.isEmpty() && audioQualityAsStringInDefaultLocale[0] == '.')
		audioQualityAsStringInDefaultLocale = "0" + audioQualityAsStringInDefaultLocale;

	// VuoMovieFormat
	VuoMovieFormat format;
	format.imageEncoding = VuoMovieImageEncoding_makeFromJson(imageEncodingValue);
	format.audioEncoding = VuoAudioEncoding_makeFromJson(audioEncodingValue);
	format.imageQuality = VuoReal_makeFromString(imageQualityAsStringInDefaultLocale.toUtf8().constData());
	format.audioQuality = VuoReal_makeFromString(audioQualityAsStringInDefaultLocale.toUtf8().constData());
	return VuoMovieFormat_getJson(format);
}

/**
 * Converts the input @c newLineEditText to an integer and updates this
 * input editor's @c slider widget to reflect that integer value.
 */
void VuoInputEditorMovieFormat::updateSliderValue(QString newLineEditText)
{
	QObject *sender = QObject::sender();
	qualityAttribute whichQualityAttribute = (sender == lineEditImageQuality? imageQuality : audioQuality);

	double newLineEditValue = QLocale::system().toDouble(newLineEditText);
	int newSliderValue = lineEditValueToScaledSliderValue(newLineEditValue, whichQualityAttribute);

	QSlider *targetSlider = (whichQualityAttribute == imageQuality? sliderImageQuality : sliderAudioQuality);

	disconnect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
	targetSlider->setValue(newSliderValue);
	connect(targetSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditValue(int)));
}

/**
 * Converts the slider's current value() to a string and updates this
 * input editor's @c lineEdit widget to reflect that string value;
 * gives keyboard focus to the @c lineEdit.
 */

void VuoInputEditorMovieFormat::updateLineEditValue()
{
	QObject *sender = QObject::sender();
	qualityAttribute whichQualityAttribute = (sender == sliderImageQuality? imageQuality: audioQuality);
	updateLineEditValue(((QSlider *)sender)->value(), whichQualityAttribute);
}

void VuoInputEditorMovieFormat::updateLineEditValue(int newSliderValue)
{
	QObject *sender = QObject::sender();
	qualityAttribute whichQualityAttribute = (sender == sliderImageQuality? imageQuality: audioQuality);

	updateLineEditValue(newSliderValue, whichQualityAttribute);
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
void VuoInputEditorMovieFormat::updateLineEditValue(int newSliderValue, qualityAttribute whichQualityAttribute)
{
	double newLineEditValue = sliderValueToScaledLineEditValue(newSliderValue, whichQualityAttribute);
	QLineEdit *targetLineEdit = (whichQualityAttribute == imageQuality? lineEditImageQuality : lineEditAudioQuality);
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
int VuoInputEditorMovieFormat::lineEditValueToScaledSliderValue(double lineEditValue, qualityAttribute whichQualityAttribute)
{
	QSlider *targetSlider = (whichQualityAttribute == imageQuality? sliderImageQuality : sliderAudioQuality);
	double suggestedMin = (whichQualityAttribute == imageQuality? suggestedMinImageQuality : suggestedMinAudioQuality);
	double suggestedMax = (whichQualityAttribute == imageQuality? suggestedMaxImageQuality : suggestedMaxAudioQuality);
	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	int scaledSliderValue = targetSlider->minimum() + ((lineEditValue-suggestedMin)/(1.0*(lineEditRange)))*sliderRange;

	return scaledSliderValue;
}

/**
 * Scales the input @c sliderValue to match the range of the
 * port's suggestedMin and suggestedMax.
 */
double VuoInputEditorMovieFormat::sliderValueToScaledLineEditValue(int sliderValue, qualityAttribute whichQualityAttribute)
{
	QSlider *targetSlider = (whichQualityAttribute == imageQuality? sliderImageQuality : sliderAudioQuality);
	double suggestedMin = (whichQualityAttribute == imageQuality? suggestedMinImageQuality : suggestedMinAudioQuality);
	double suggestedMax = (whichQualityAttribute == imageQuality? suggestedMaxImageQuality : suggestedMaxAudioQuality);
	const double lineEditRange = suggestedMax - suggestedMin;

	const int sliderRange = targetSlider->maximum() - targetSlider->minimum();
	double scaledLineEditValue = suggestedMin + ((sliderValue-targetSlider->minimum())/(1.0*sliderRange))*lineEditRange;

	return scaledLineEditValue;
}

void VuoInputEditorMovieFormat::updateQualitySliderEnabledStatusAndEmitValueChanged()
{
	updateQualitySliderEnabledStatus();
	emitValueChanged();
}

/**
 * Disables the quality-related widgets for image and audio formats that don't support it.
 */
void VuoInputEditorMovieFormat::updateQualitySliderEnabledStatus()
{
	// Image Quality
	json_object *acceptedFormatJson = getAcceptedValue();
	VuoMovieFormat acceptedFormat = VuoMovieFormat_makeFromJson(acceptedFormatJson);
	bool enableImageQualityWidgets = ((acceptedFormat.imageEncoding == VuoMovieImageEncoding_JPEG) ||
									 (acceptedFormat.imageEncoding == VuoMovieImageEncoding_H264));

	labelImageQuality->setEnabled(enableImageQualityWidgets);
	lineEditImageQuality->setEnabled(enableImageQualityWidgets);
	sliderImageQuality->setEnabled(enableImageQualityWidgets);

	if (!enableImageQualityWidgets)
	{
		int sliderValueImageQuality = lineEditValueToScaledSliderValue(1., imageQuality);
		sliderImageQuality->setValue(sliderValueImageQuality);
	}

	// Audio Quality
	bool enableAudioQualityWidgets = (acceptedFormat.audioEncoding != VuoAudioEncoding_LinearPCM);

	labelAudioQuality->setEnabled(enableAudioQualityWidgets);
	lineEditAudioQuality->setEnabled(enableAudioQualityWidgets);
	sliderAudioQuality->setEnabled(enableAudioQualityWidgets);

	if (!enableAudioQualityWidgets)
	{
		int sliderValueAudioQuality = lineEditValueToScaledSliderValue(1., audioQuality);
		sliderAudioQuality->setValue(sliderValueAudioQuality);
	}

	// Formatting specific to Dark Interface mode
	// @todo https://b33p.net/kosada/node/10139 : Adapt global dark-mode stylesheet to handle disabled widgets.
	if (isDark)
	{
		// Image Quality
		QPalette imageLabelPalette = labelImageQuality->palette();
		imageLabelPalette.setColor(QPalette::Foreground,
								   (enableImageQualityWidgets? darkModeEnabledLabelTextColor : darkModeDisabledLabelTextColor));
		labelImageQuality->setPalette(imageLabelPalette);

		// Audio Quality
		QPalette audioLabelPalette = labelAudioQuality->palette();
		audioLabelPalette.setColor(QPalette::Foreground,
								   (enableAudioQualityWidgets? darkModeEnabledLabelTextColor : darkModeDisabledLabelTextColor));
		labelAudioQuality->setPalette(audioLabelPalette);
	}

	QLineEdit *firstEnabledLineEdit = (enableImageQualityWidgets? lineEditImageQuality :
																  (enableAudioQualityWidgets? lineEditAudioQuality :
																							  NULL));

	QLineEdit *lastEnabledLineEdit = (enableAudioQualityWidgets? lineEditAudioQuality :
																 (enableImageQualityWidgets? lineEditImageQuality :
																							 NULL));

	// Return focus to the topmost line edit by default, or to the bottommost
	// line edit if tab-cycling backwards.
	// To be handled properly for https://b33p.net/kosada/node/6365 .
	if (tabCycleForward && firstEnabledLineEdit)
	{
		firstEnabledLineEdit->setFocus();
		firstEnabledLineEdit->selectAll();
	}
	else if (!tabCycleForward && lastEnabledLineEdit)
	{
		lastEnabledLineEdit->setFocus();
		lastEnabledLineEdit->selectAll();
	}

	setFirstWidgetInTabOrder(firstEnabledLineEdit);
	setLastWidgetInTabOrder(lastEnabledLineEdit);
}

void VuoInputEditorMovieFormat::emitValueChanged()
{
	emit valueChanged(getAcceptedValue());
}

/**
 * Filters events on watched objects.
 */
bool VuoInputEditorMovieFormat::eventFilter(QObject *object, QEvent *event)
{
	QSlider *targetSlider = (object==lineEditImageQuality? sliderImageQuality :
							(object==lineEditAudioQuality? sliderAudioQuality :
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

/**
 * Creates the tree that models the menu.
 * Populates the provided `comboBox` with the allowed values associated with the provided `type`;
 * checks the value matching `originalValueAsString`.
 * @todo https://b33p.net/kosada/node/9848 Some content copied from VuoInputEditorWithEnumMenu; re-factor?
 */
QComboBox * VuoInputEditorMovieFormat::setUpComboBoxForType(QComboBox *comboBox, QString type, string originalValueAsString)
{
	QString allowedValuesFunctionName = type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.toUtf8().constData());

	QString summaryFunctionName = type + "_getSummary";
	typedef char *(*summaryFunctionType)(int);
	summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_SELF, summaryFunctionName.toUtf8().constData());

	QString jsonFunctionName = type + "_getJson";
	typedef json_object *(*jsonFunctionType)(int);
	jsonFunctionType jsonFunction = (jsonFunctionType)dlsym(RTLD_SELF, jsonFunctionName.toUtf8().constData());

	QString listCountFunctionName = "VuoListGetCount_" + type;
	typedef unsigned long (*listCountFunctionType)(void *);
	listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_SELF, listCountFunctionName.toUtf8().constData());

	QString listValueFunctionName = "VuoListGetValue_" + type;
	typedef int (*listValueFunctionType)(void *, unsigned long);
	listValueFunctionType listValueFunction = (listValueFunctionType)dlsym(RTLD_SELF, listValueFunctionName.toUtf8().constData());

	if (allowedValuesFunction && summaryFunction && jsonFunction && listCountFunction && listValueFunction)
	{
		void *allowedValues = allowedValuesFunction();
		unsigned long count = listCountFunction(allowedValues);
		for (unsigned long i=1; i<=count; ++i)
		{
			int value = listValueFunction(allowedValues, i);
			char *summary = summaryFunction(value);
			QVariant variantValue = QVariant::fromValue((void *)jsonFunction(value));
			comboBox->addItem(summary, variantValue);

			if (json_object_to_json_string_ext(jsonFunction(value), JSON_C_TO_STRING_PLAIN) == originalValueAsString)
				comboBox->setCurrentText(summary);

			free(summary);
		}
	}

	return comboBox;
}
