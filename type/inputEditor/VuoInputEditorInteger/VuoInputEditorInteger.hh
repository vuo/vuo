/**
 * @file
 * VuoInputEditorInteger interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithDialog.hh"
#include "VuoSpinBox.hh"

extern "C" {
#include "VuoInteger.h"
}

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
 *	 - "suggestedStep" defines the step size of the slider or spin box. By default, the step size is 1/10 of the range.
 *
 * @eg{
 *   {
 *     "suggestedMin" : -10,
 *     "suggestedMax" : 10,
 *     "suggestedStep" : 1
 *   }
 * }
 */
class VuoInputEditorInteger : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	virtual bool supportsTabbingBetweenPorts(void) { return true; }		///< This editor does support tabbing between ports.

private:

	VuoInteger previous;			///< If 'auto' was toggled then un-toggled, this lets us restore the previous value.
	VuoInteger current;				///< The current value.
	VuoInteger defaultValue;		///< The default value.
	VuoInteger autoValue;			///< Value to match if auto is enabled.
	VuoInteger suggestedMin;		///< Minimum value this field should allow.
	VuoInteger suggestedMax;		///< Maximum value this field should allow.
	bool hasAutoValue;				///< Does metadata provide an autoValue?

	QSlider* slider; 				///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
	VuoSpinBox* spinbox; 			///< For use when both the suggestedMin and suggestedMax port annotation values are provided by the node class designer.
	QCheckBox* autoToggle;			///< Enable/disable auto value.

	VuoSpinBox* initSpinBox(QDialog& dialog, VuoInteger initialValue);
	QSlider* initSlider(QDialog& dialog, VuoInteger initialValue);

private slots:
	void setAutoToggled(int state);
	void onSliderUpdate(int sliderValue);
	void onSpinboxUpdate(QString spinboxValue);
};

