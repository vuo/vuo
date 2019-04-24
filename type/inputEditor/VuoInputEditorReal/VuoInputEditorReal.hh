/**
 * @file
 * VuoInputEditorReal interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithDialog.hh"
#include "VuoDoubleSpinbox.hh"

extern "C" {
#include "VuoReal.h"
#include "VuoBoolean.h"
}

/**
 * A VuoInputEditorReal factory.
 */
class VuoInputEditorRealFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorReal.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoReal value,
 * allowing the user either to select the value with a mouse (using a spin box or slider)
 * or to type the value into a text box (line edit).
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMin" and "suggestedMax" define the range of the slider (if both are defined)
 *		or spin box (if one is defined) but don't affect the line edit. By default, the dialog
 *      contains a spin box with unbounded range.
 *	 - "suggestedStep" defines the step size of the slider or spin box. By default, the step size is 1/10 of the range.
 *
 * @eg{
 *   {
 *     "suggestedMin" : -1.0,
 *     "suggestedMax" : 1.0,
 *     "suggestedStep" : 0.25
 *   }
 * }
 */
class VuoInputEditorReal : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	virtual bool supportsTabbingBetweenPorts(void) { return true; }		///< This editor does support tabbing between ports.
	VuoDoubleSpinBox* spinbox; 	///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
friend class TestInputEditors;

private:

	VuoReal current;				///< The current value.
	VuoReal defaultValue;			///< The default value.
	VuoReal previous;				///< The last known user-input value.
	VuoReal suggestedMin; 			///< The minimum values selectable via spinbox or slider.
	VuoReal suggestedMax;			///< The maximum values selectable via spinbox or slider.
	VuoReal suggestedStep;			///< The step value via spinbox or slider.
	VuoReal automatic;				///< If current == automatic, show "Auto" in field and don't let user change field.
	bool autoSupersedesDefault; 	///< If true the default value will be "Auto", but still use "default" value when "Auto" is toggled off.

	QCheckBox* checkbox;		///< Used to toggle between a user set value and "auto".
	QSlider* slider; 			///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.

	VuoDoubleSpinBox* initSpinBox(QDialog& dialog, double initialValue);
	QSlider* initSlider(QDialog& dialog, double initialValue);

private slots:

	void setIsAuto(int state);
	void onSliderUpdate(int sliderValue);
	void onSpinboxUpdate(double spinboxValue);
};

