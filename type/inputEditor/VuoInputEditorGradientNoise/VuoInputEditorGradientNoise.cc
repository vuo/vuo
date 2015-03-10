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

	json_object *perlinAsJson = VuoGradientNoise_jsonFromValue(VuoGradientNoise_Perlin);
	const char *perlinSummary = VuoGradientNoise_summaryFromValue(VuoGradientNoise_Perlin);
	json_object *simplexAsJson = VuoGradientNoise_jsonFromValue(VuoGradientNoise_Simplex);
	const char *simplexSummary = VuoGradientNoise_summaryFromValue(VuoGradientNoise_Simplex);

	VuoInputEditorMenuItem *perlin = new VuoInputEditorMenuItem(perlinSummary, perlinAsJson);
	VuoInputEditorMenuItem *simplex = new VuoInputEditorMenuItem(simplexSummary, simplexAsJson);

	gradientTree->addItem(perlin);
	gradientTree->addItem(simplex);

	return gradientTree;
}
