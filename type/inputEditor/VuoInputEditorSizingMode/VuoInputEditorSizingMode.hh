/**
 * @file
 * VuoInputEditorSizingMode interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORSIZINGMODE_HH
#define VUOINPUTEDITORSIZINGMODE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorSizingMode factory.
 */
class VuoInputEditorSizingModeFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorSizingMode.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoWrapMode values.
 */
class VuoInputEditorSizingMode : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORSIZINGMODE_HH
