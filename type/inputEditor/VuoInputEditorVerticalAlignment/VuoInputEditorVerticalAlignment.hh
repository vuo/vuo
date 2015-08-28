/**
 * @file
 * VuoInputEditorVerticalAlignment interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORVERTICALALIGNMENT_HH
#define VUOINPUTEDITORVERTICALALIGNMENT_HH

#include "VuoInputEditorWithEnumMenu.hh"

/**
 * A VuoInputEditorVerticalAlignment factory.
 */
class VuoInputEditorVerticalAlignmentFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorVerticalAlignment.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoLeapPointableType values.
 */
class VuoInputEditorVerticalAlignment : public VuoInputEditorWithEnumMenu
{
	Q_OBJECT
};

#endif // VUOINPUTEDITORVERTICALALIGNMENT_HH
