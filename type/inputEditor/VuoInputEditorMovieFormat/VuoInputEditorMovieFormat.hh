/**
 * @file
 * VuoInputEditorMovieFormat interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORMOVIEFORMAT_HH
#define VUOINPUTEDITORMOVIEFORMAT_HH

#include "VuoInputEditorWithLineEdit.hh"
#include <dlfcn.h>

/**
 * A VuoInputEditorMovieFormat factory.
 */
class VuoInputEditorMovieFormatFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorMovieFormat.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoMovieFormat value,
 * allowing the user to select the image and audio encodings from a drop-down menu, and
 * the image and audio quality values either with a mouse (using a slider) or
 * by typing the values into a text box (line edit).
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMin" and "suggestedMax" define the range of the sliders
 *		but don't affect the line edit. By default, the dialog
 *      contains a slider with a [0,1] range.
 *	 - "suggestedStep" defines the step size of the sliders. By default, the step size is 0.1.
 *
 * @eg{
 *   {
 *     "suggestedMin" : 0.5,
 *     "suggestedMax" : 1.0,
 *     "suggestedStep" : 0.25
 *   }
 * }
 */
class VuoInputEditorMovieFormat : public VuoInputEditorWithLineEdit
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromSubwidgetFormats(const QVariant &imageEncoding,
											  const QString &imageQualityAsString,
											  const QVariant &audioEncoding,
											  const QString &audioQualityAsString);
	bool eventFilter(QObject *object, QEvent *event);

private:
	enum qualityAttribute
	{
		imageQuality,
		audioQuality
	};

	bool isDark;
	bool tabCycleForward;

	QComboBox *comboBoxImageEncoding;
	QComboBox *comboBoxAudioEncoding;

	QLabel *labelImageQuality;
	QLabel *labelAudioQuality;

	QLineEdit *lineEditImageQuality;
	QLineEdit *lineEditAudioQuality;

	double suggestedMinImageQuality;	///< The minimum image quality value selectable by slider.
	double suggestedMaxImageQuality;	///< The maximum image quality value selectable by slider.
	QSlider *sliderImageQuality;

	double suggestedMinAudioQuality;	///< The minimum audio quality value selectable by slider.
	double suggestedMaxAudioQuality;	///< The maximum audio quality value selectable by slider.
	QSlider *sliderAudioQuality;

	int lineEditValueToScaledSliderValue(double lineEditValue, qualityAttribute whichQualityAttribute);
	double sliderValueToScaledLineEditValue(int sliderValue, qualityAttribute whichQualityAttribute);

	void updateLineEditValue(int newSliderValue, qualityAttribute whichQualityAttribute);
	void updateQualitySliderEnabledStatus();

	QComboBox * setUpComboBoxForType(QComboBox *comboBox, QString type, string originalValueAsString);

	static const QColor darkModeEnabledLabelTextColor;
	static const QColor darkModeDisabledLabelTextColor;

private slots:
	void updateSliderValue(QString newLineEditText);
	void updateLineEditValue();
	void updateLineEditValue(int newSliderValue);

	void updateQualitySliderEnabledStatusAndEmitValueChanged();
	void emitValueChanged();
};

#endif // VUOINPUTEDITORMOVIEFORMAT_HH
