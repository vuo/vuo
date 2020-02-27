/**
 * @file
 * VuoInputEditorRange interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithDialog.hh"
#include "VuoDoubleSpinBox.hh"

extern "C" {
#include "VuoRange.h"
#include "VuoBoolean.h"
}

/**
 * A VuoInputEditorRange factory.
 */
class VuoInputEditorRangeFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorRange.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoRange value,
 * allowing the user either to select the value with a mouse (using a spin box or slider)
 * or to type the value into a text box (line edit).
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMin" and "suggestedMax" define the range of the slider (if both are defined)
 *		or spin box (if one is defined) but don't affect the line edit. By default, the dialog
 *      contains a spin box with unbounded range.
 *	 - "suggestedStep" defines the step size of the slider or spin box. By default, the step size is 1.
 *   - "requireMin" and "requireMax" to enforce a numeric value (instead of unbounded).
 *
 * @eg{
 *   {
 *     "suggestedMin" : { "minimum":-1.0, "maximum":-1.0 },
 *     "suggestedMax" : { "minimum": 1.0, "maximum": 1.0 },
 *     "suggestedStep" : { "minimum": 0.1, "maximum": 0.1 }
 *	   "requireMin":true
 *   }
 * }
 */
class VuoInputEditorRange : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	virtual bool supportsTabbingBetweenPorts(void) { return true; }		///< This editor does support tabbing between ports.

private:

	VuoRange current;			///< The current value.
	VuoRange lastValue;			///< Used to restore values when checking and unchecking "has{Min,Max}Bound"
	VuoRange suggestedMin; 		///< The minimum values selectable via spinbox or slider.
	VuoRange suggestedMax;		///< The maximum values selectable via spinbox or slider.
	VuoRange suggestedStep;		///< The step value via spinbox or slider.

	VuoBoolean requireMin;		///< Whether or not to show the "Has Min" checkbox.
	VuoBoolean requireMax;		///< Whether or not to show the "Has Max" checkbox.

	QSlider	*slider_min; 		///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
	QSlider	*slider_max; 		///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.

	VuoDoubleSpinBox* spinbox_min; 	///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
	VuoDoubleSpinBox* spinbox_max; 	///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.

	QCheckBox* toggle_hasMin;
	QCheckBox* toggle_hasMax;

	VuoDoubleSpinBox* initSpinBox(QDialog& dialog, double initialValue, double min, double max, double step);
	QSlider* initSlider(QSlider* slider, double initialValue, double min, double max, double step);

private slots:

	void setMinimumValue(double value);
	void setMaximumValue(double value);

	void setHasMinBound(int state);
	void setHasMaxBound(int state);

	void onSliderUpdate(int sliderValue);
	void onSpinboxUpdate(double spinboxValue);
};

