/**
 * @file
 * VuoInputEditorLoopType interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORLOOPTYPE_HH
#define VUOINPUTEDITORLOOPTYPE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorLoopType factory.
 */
class VuoInputEditorLoopTypeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorLoopType.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoLoopType values.
 */
class VuoInputEditorLoopType : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORLOOPTYPE_HH
