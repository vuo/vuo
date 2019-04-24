/**
 * @file
 * VuoInputEditorIntegerRange interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditor.hh"
#include "VuoInputEditorWithDialog.hh"
#include "VuoSpinBox.hh"

extern "C"
{
	#include "VuoIntegerRange.h"
}

/**
 * A VuoInputEditorIntegerRange factory.
 */
class VuoInputEditorIntegerRangeFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorIntegerRange.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that modifies VuoIntegerRange type.
 */
class VuoInputEditorIntegerRange : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	virtual void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	virtual json_object* getAcceptedValue();
	virtual bool supportsTabbingBetweenPorts(void) { return true; }			///< This editor does support tabbing between ports.
	bool eventFilter(QObject *object, QEvent *event);						///< Enable arrow keys to move around offset & gamma.

private:
	VuoIntegerRange currentValue;	/// Value currently shown in editor.
	VuoIntegerRange suggestedMin;	/// Suggested min.
	VuoIntegerRange suggestedMax;	/// Suggested max.
	VuoIntegerRange suggestedStep;	/// Suggested step.
	VuoIntegerRange lastValue;

	VuoSpinBox* 	spinbox_minimum;	///< VuoSpinBox in control of minimum.
	VuoSpinBox* 	spinbox_maximum;	///< VuoSpinBox in control of maximum.

	QCheckBox*		checkbox_minimum;	///< If min value is set (-INF otherwise)
	QCheckBox*		checkbox_maximum;	///< If max value is set (+INF otherwise)

private slots:
	void setMinBound(int state);
	void setMaxBound(int state);
	void setMinimum(QString newLineEditText);
	void setMaximum(QString newLineEditText);

	void setupSpinBox(VuoSpinBox* spin, int min, int max, int step, int value);
};

