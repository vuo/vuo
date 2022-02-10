/**
 * @file
 * VuoInputEditorTextComparison implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorTextComparison.hh"
#include "VuoComboBox.hh"

/**
 * Constructs a VuoInputEditorTextComparison object.
 */
VuoInputEditor * VuoInputEditorTextComparisonFactory::newInputEditor()
{
	return new VuoInputEditorTextComparison();
}

/**
 * Sets up a dialog containing a combo box for the comparison type and a checkbox for case-sensitivity.
 */
void VuoInputEditorTextComparison::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	VuoTextComparison originalTextComparison = VuoTextComparison_makeFromJson(originalValue);

	comboBoxComparisonType = new VuoComboBox(&dialog);
	comboBoxComparisonType->addItem("Equals");
	comboBoxComparisonType->addItem("Contains");
	comboBoxComparisonType->addItem("Begins with");
	comboBoxComparisonType->addItem("Ends with");
	comboBoxComparisonType->addItem("Matches wildcard");
	comboBoxComparisonType->addItem("Matches regular expression");
	comboBoxComparisonType->setCurrentIndex(originalTextComparison.type);

	checkBoxCaseSensitive = new QCheckBox("Case-sensitive");
	checkBoxCaseSensitive->setChecked(originalTextComparison.isCaseSensitive);

	QGridLayout *layout = new QGridLayout;
	dialog.setLayout(layout);

	layout->setContentsMargins(4, 3, 12, 5);
	layout->setSpacing(4);

	layout->addWidget(comboBoxComparisonType, 0, 0);
	layout->addWidget(checkBoxCaseSensitive, 1, 0);

	dialog.setMaximumWidth(1);
	dialog.setMaximumHeight(1);

	dialog.adjustSize();
}

/**
 * Returns the value currently set in the dialog's widgets.
 */
json_object * VuoInputEditorTextComparison::getAcceptedValue(void)
{
	VuoTextComparison textComparison;
	textComparison.type = (VuoTextComparisonType)comboBoxComparisonType->currentIndex();
	textComparison.isCaseSensitive = checkBoxCaseSensitive->isChecked();
	return VuoTextComparison_getJson(textComparison);
}
