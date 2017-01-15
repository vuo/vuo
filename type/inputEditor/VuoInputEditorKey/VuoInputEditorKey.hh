/**
 * @file
 * VuoInputEditorKey interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORKEY_HH
#define VUOINPUTEDITORKEY_HH

#include "VuoInputEditorWithDialog.hh"
#include "VuoKeyComboBox.hh"

extern "C"
{
	#include "VuoKey.h"
}

/**
 * A VuoInputEditorKey factory.
 */
class VuoInputEditorKeyFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorKey.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a dialog for picking a keyboard key.
 */
class VuoInputEditorKey : public VuoInputEditorWithDialog
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);

private:
	VuoKeyComboBox *comboBox;
};

#endif // VUOINPUTEDITORKEY_HH
