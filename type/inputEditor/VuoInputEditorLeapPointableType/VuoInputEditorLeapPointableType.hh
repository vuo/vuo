/**
 * @file
 * VuoInputEditorLeapPointableType interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORLEAPPOINTABLETYPE_HH
#define VUOINPUTEDITORLEAPPOINTABLETYPE_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorLeapPointableType factory.
 */
class VuoInputEditorLeapPointableTypeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorLeapPointableType.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoLeapPointableType values.
 */
class VuoInputEditorLeapPointableType : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VUOINPUTEDITORLEAPPOINTABLETYPE_HH
