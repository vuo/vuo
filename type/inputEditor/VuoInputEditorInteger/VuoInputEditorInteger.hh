/**
 * @file
 * VuoInputEditorInteger interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORINTEGER_HH
#define VUOINPUTEDITORINTEGER_HH

#include "VuoInputEditorWithLineEdit.hh"

/**
 * A VuoInputEditorInteger factory.
 */
class VuoInputEditorIntegerFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorInteger.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoInteger value,
 * allowing the user either to select the value with a mouse (using a spin box or slider)
 * or to type the value into a text box (line edit).
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMin" and "suggestedMax" define the range of the slider (if both are defined)
 *		or spin box (if one is defined) but don't affect the line edit. By default, the dialog
 *      contains a spin box with unbounded range.
 *	 - "suggestedStep" defines the step size of the slider or spin box. By default, the step size is 1.
 *
 * @eg{
 *   {
 *     "suggestedMin" : 0,
 *     "suggestedMax" : 360,
 *     "suggestedStep" : 30
 *   }
 * }
 */
class VuoInputEditorInteger : public VuoInputEditorWithLineEdit
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * convertFromLineEditFormat(const QString &valueAsString);
	bool eventFilter(QObject *object, QEvent *event);

private:
	QSlider *slider;	///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
	QSpinBox *spinBox;	///< For use when a suggestedMin, suggestedMax, or both values, are left unspecified by the node class designer.
	QHBoxLayout *sliderLineEditLayout;

private slots:
	void updateSliderValue(QString newTextValue);
	void updateLineEditValue();
	void updateLineEditValue(int newSliderValue);

	void emitValueChanged();
};

#endif // VUOINPUTEDITORINTEGER_HH
