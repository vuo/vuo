/**
 * @file
 * VuoInputEditorKey implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorKey.hh"

/**
 * Constructs a VuoInputEditorKey object.
 */
VuoInputEditor * VuoInputEditorKeyFactory::newInputEditor()
{
	return new VuoInputEditorKey();
}

/**
 * Adds a combo box to the dialog for displaying and editing the selected key.
 */
void VuoInputEditorKey::setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details)
{
	comboBox = new VuoKeyComboBox(&dialog);
	comboBox->adjustSize();

	comboBox->setCurrentKey( VuoKey_makeFromJson(originalValue) );
}

/**
 * Returns the most recently selected key.
 */
json_object * VuoInputEditorKey::getAcceptedValue(void)
{
	return VuoKey_getJson( comboBox->getCurrentKey() );
}
