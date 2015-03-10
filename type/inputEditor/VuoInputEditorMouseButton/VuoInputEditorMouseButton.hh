/**
 * @file
 * VuoInputEditorMouseButton interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORMOUSEBUTTON_HH
#define VUOINPUTEDITORMOUSEBUTTON_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorMouseButton factory.
 */
class VuoInputEditorMouseButtonFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorMouseButton.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoMouseButton values.
 */
class VuoInputEditorMouseButton : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORMOUSEBUTTON_HH
