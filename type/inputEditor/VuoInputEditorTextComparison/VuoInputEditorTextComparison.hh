/**
 * @file
 * VuoInputEditorTextComparison interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithDialog.hh"

extern "C" {
#include "VuoTextComparison.h"
}

/**
 * A VuoInputEditorTextComparison factory.
 */
class VuoInputEditorTextComparisonFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorTextComparison.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoTextComparison value.
 */
class VuoInputEditorTextComparison : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	virtual bool supportsTabbingBetweenPorts(void) { return true; }	 ///< This editor does support tabbing between ports.

private:
	QComboBox *comboBoxComparisonType;
	QCheckBox *checkBoxCaseSensitive;
};

