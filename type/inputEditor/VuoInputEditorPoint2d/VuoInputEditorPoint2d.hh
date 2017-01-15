/**
 * @file
 * VuoInputEditorPoint2d interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORPOINT2D_HH
#define VUOINPUTEDITORPOINT2D_HH

#include "VuoInputEditorWithLineEdit.hh"

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
class VuoInputEditorPoint2d : public VuoInputEditorWithLineEdit
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromLineEditsFormat(const QString &xValueAsString, const QString &yValueAsString);
	bool eventFilter(QObject *object, QEvent *event);

private:
	enum coord
	{
		x,
		y
	};

	QLineEdit *lineEditX;
	QLineEdit *lineEditY;

	double suggestedMinX;	///< The minimum x-value selectable via spinbox or slider.
	double suggestedMaxX;	///< The maximum x-value selectable via spinbox or slider.
	QSlider *sliderX;	///< For use when both the suggestedMinX and suggestedMaxX port annotation values are provided by the node class designer.
	QDoubleSpinBox *spinBoxX;	///< For use when either the suggestedMinX, suggestedMaxX, or both values, are left unspecified by the node class designer.

	double suggestedMinY;	///< The minimum y-value selectable via spinbox or slider.
	double suggestedMaxY;	///< The maximum y-value selectable via spinbox or slider.
	QSlider *sliderY;	///< For use when both the suggestedMinY and suggestedMaxY port annotation values are provided by the node class designer.
	QDoubleSpinBox *spinBoxY;	///< For use when either the suggestedMinY, suggestedMaxY, or both values, are left unspecified by the node class designer.

	int lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord);
	double sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord);

	void updateLineEditValue(int newSliderValue, coord whichCoord);

private slots:
	void updateSliderValue(QString newLineEditText);
	void updateLineEditValue();
	void updateLineEditValue(int newSliderValue);

	void emitValueChanged();
};

#endif // VUOINPUTEDITORPOINT2D_HH
