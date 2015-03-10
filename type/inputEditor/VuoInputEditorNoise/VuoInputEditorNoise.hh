/**
 * @file
 * VuoInputEditorNoise interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORNOISE_HH
#define VUOINPUTEDITORNOISE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorNoise factory.
 */
class VuoInputEditorNoiseFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorNoise.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoNoise values.
 */
class VuoInputEditorNoise : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORNOISE_HH
