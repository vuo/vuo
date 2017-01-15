/**
 * @file
 * VuoInputEditorScreen interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORSCREEN_HH
#define VUOINPUTEDITORSCREEN_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorScreen factory.
 */
class VuoInputEditorScreenFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorScreen.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoScreen values.
 */
class VuoInputEditorScreen : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORSCREEN_HH
