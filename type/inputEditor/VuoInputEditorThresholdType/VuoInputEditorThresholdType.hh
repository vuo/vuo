/**
 * @file
 * VuoInputEditorThresholdType interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORTHRESHOLDTYPE_HH
#define VUOINPUTEDITORTHRESHOLDTYPE_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorThresholdType factory.
 */
class VuoInputEditorThresholdTypeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorThresholdType.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoThresholdType values.
 */
class VuoInputEditorThresholdType: public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VUOINPUTEDITORTHRESHOLDTYPE_HH
