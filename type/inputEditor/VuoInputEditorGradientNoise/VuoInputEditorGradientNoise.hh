/**
 * @file
 * VuoInputEditorGradientNoise interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORGRADIENTNOISE_HH
#define VUOINPUTEDITORGRADIENTNOISE_HH

#include "VuoInputEditorWithMenu.hh"

/**
 * A VuoInputEditorGradientNoise factory.
 */
class VuoInputEditorGradientNoiseFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorGradientNoise.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a menu of VuoInputEditorGradientNoise values.
 */
class VuoInputEditorGradientNoise : public VuoInputEditorWithMenu
{
	Q_OBJECT

public:
	VuoInputEditorMenuItem * setUpMenuTree();
};

#endif // VUOINPUTEDITORGRADIENTNOISE_HH
