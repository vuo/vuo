/**
 * @file
 * VuoInputEditorPoint2d interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithDialog.hh"
#include "VuoDoubleSpinbox.hh"

extern "C" {
#include "VuoPoint2d.h"
}

/**
 * A VuoInputEditorPoint2d factory.
 */
class VuoInputEditorPoint2dFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorPoint2d.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoPoint2d value,
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
 *     "suggestedMin" : -1.0,
 *     "suggestedMax" : 1.0,
 *     "suggestedStep" : 0.25
 *   }
 * }
 */
class VuoInputEditorPoint2d : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);

	/**
	 * Indicates that this input editor supports tabbing.
	 */
	virtual bool supportsTabbingBetweenPorts(void) { return true; }

private:

	enum coord {
		coord_x, coord_y
	};

	VuoPoint2d current;			///< The current value.
	map<coord, double> suggestedMinForCoord; 	///< The minimum values selectable via spinbox or slider.
	map<coord, double> suggestedMaxForCoord;	///< The maximum values selectable via spinbox or slider.
	map<coord, double> suggestedStepForCoord;	///< The step value via spinbox or slider.

	map<coord, QSlider*> sliderForCoord; ///< For use when both the suggestedMinX and suggestedMaxX port annotation values are provided by the node class designer.
	map<coord, VuoDoubleSpinBox*> spinboxForCoord; ///< For use when both the suggestedMinX and suggestedMaxX port annotation values are provided by the node class designer.

	bool getCoordFromQObject(QObject* sender, coord* whichCoord);	///< Determine what coord a GUI field belongs to.
	void setCoord(coord c, double value); ///< Set the coordinate value on current.

	VuoDoubleSpinBox* initSpinBox(coord whichCoord, QDialog& dialog, double initialValue);
	QSlider* initSlider(coord whichCoord, QDialog& dialog, double initialValue);

private slots:

	void onSliderUpdate(int sliderValue);
	void onSpinboxUpdate(QString spinboxValue);
	void emitValueChanged();
};

