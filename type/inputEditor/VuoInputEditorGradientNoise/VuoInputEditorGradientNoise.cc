/**
 * @file
 * VuoInputEditorGradientNoise implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorGradientNoise.hh"

extern "C"
{
	#include "VuoGradientNoise.h"
}

/**
 * Constructs a VuoInputEditorGradientNoise object.
 */
VuoInputEditor * VuoInputEditorGradientNoiseFactory::newInputEditor()
{
	return new VuoInputEditorGradientNoise();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorGradientNoise::setUpMenuTree()
{
	VuoInputEditorMenuItem *gradientTree = new VuoInputEditorMenuItem("root");

	gradientTree->addItem(new VuoInputEditorMenuItem(VuoGradientNoise_summaryFromValue(VuoGradientNoise_Perlin), VuoGradientNoise_jsonFromValue(VuoGradientNoise_Perlin)));
	gradientTree->addItem(new VuoInputEditorMenuItem(VuoGradientNoise_summaryFromValue(VuoGradientNoise_Simplex), VuoGradientNoise_jsonFromValue(VuoGradientNoise_Simplex)));

	return gradientTree;
}
