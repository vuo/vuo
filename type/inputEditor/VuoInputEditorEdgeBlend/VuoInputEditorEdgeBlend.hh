/**
 * @file
 * VuoInputEditorEdgeBlend interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditor.hh"
#include "VuoInputEditorWithDialog.hh"
#include "VuoDoubleSpinBox.hh"

extern "C"
{
	#include "VuoEdgeBlend.h"
}

/**
 * A VuoInputEditorEdgeBlend factory.
 */
class VuoInputEditorEdgeBlendFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorEdgeBlend.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that modifies VuoEdgeBlend type.
 */
class VuoInputEditorEdgeBlend : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object* getAcceptedValue();
	virtual bool supportsTabbingBetweenPorts(void) { return true; }			///< This editor does support tabbing between ports.

private:
	VuoEdgeBlend currentValue;	/// Value currently shown in editor.
	VuoEdgeBlend suggestedMin;	/// The minimum and maximum values as set either by the user or default {0,0} - {1,3}
	VuoEdgeBlend suggestedMax;	/// The minimum and maximum values as set either by the user or default {0,0} - {1,3}
	VuoEdgeBlend suggestedStep;	/// The suggested step.

	/// Range of the cutoff and gamma sliders used to translate back to double.
	int cutoff_slider_range,
		gamma_slider_range,
		crop_slider_range;

	VuoDoubleSpinBox* 	spinBox_cutoff;	///< VuoDoubleSpinBox in control of cutoff.
	VuoDoubleSpinBox* 	spinBox_gamma;	///< VuoDoubleSpinBox in control of gamma.
	VuoDoubleSpinBox* 	spinBox_crop;	///< VuoDoubleSpinBox in control of crop.
	QSlider* 			slider_cutoff;	///< QSlider in control of cutoff (may be null)
	QSlider* 			slider_gamma;	///< QSlider in control of gamma (may be null)
	QSlider* 			slider_crop;	///< QSlider in control of crop (may be null)

private slots:
	void setCutoff(QString newLineEditText);
	void setCutoffSlider(int value);
	void setGamma(QString newLineEditText);
	void setGammaSlider(int value);
	void setCrop(QString newLineEditText);
	void setCropSlider(int value);

	void setupSpinBox(VuoDoubleSpinBox* spin, double min, double max, double step, double value);	///< set up a spin box
	void setupSlider(QSlider* slider, double min, double max, double step, double value);	///< set up a slider
};

