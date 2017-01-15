/**
 * @file
 * VuoInputEditorBlendMode interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORBLENDMODE_HH
#define VUOINPUTEDITORBLENDMODE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorBlendMode factory.
 */
class VuoInputEditorBlendModeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorBlendMode.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoBlendMode values.
 */
class VuoInputEditorBlendMode : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem *setUpMenuTree(json_object *details);
};

#endif // VUOINPUTEDITORBLENDMODE_HH
