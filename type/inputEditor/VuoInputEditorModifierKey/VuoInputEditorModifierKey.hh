/**
 * @file
 * VuoInputEditorModifierKey interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORMODIFIERKEY_HH
#define VUOINPUTEDITORMODIFIERKEY_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorModifierKey factory.
 */
class VuoInputEditorModifierKeyFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorModifierKey.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoModifierKey values.
 */
class VuoInputEditorModifierKey : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif
