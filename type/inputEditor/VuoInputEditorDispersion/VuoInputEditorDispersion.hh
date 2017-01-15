/**
 * @file
 * VuoInputEditorDispersion interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORDISPERSION_HH
#define VUOINPUTEDITORDISPERSION_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorDispersion factory.
 */
class VuoInputEditorDispersionFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorDispersion.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoWrapMode values.
 */
class VuoInputEditorDispersion : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORDISPERSION_HH
