/**
 * @file
 * VuoInputEditorNoise implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorNoise.hh"

extern "C"
{
	#include "VuoNoise.h"
}

/**
 * Constructs a VuoInputEditorNoise object.
 */
VuoInputEditor * VuoInputEditorNoiseFactory::newInputEditor()
{
	return new VuoInputEditorNoise();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorNoise::setUpMenuTree()
{
	VuoInputEditorMenuItem *noiseOptionsTree = new VuoInputEditorMenuItem("root");

	json_object *optionAsJson_White = VuoNoise_jsonFromValue(VuoNoise_White);
	const char *optionSummary_White = VuoNoise_summaryFromValue(VuoNoise_White);

	json_object *optionAsJson_Grey = VuoNoise_jsonFromValue(VuoNoise_Grey);
	const char *optionSummary_Grey = VuoNoise_summaryFromValue(VuoNoise_Grey);

	json_object *optionAsJson_Pink = VuoNoise_jsonFromValue(VuoNoise_Pink);
	const char *optionSummary_Pink = VuoNoise_summaryFromValue(VuoNoise_Pink);

	json_object *optionAsJson_Brown = VuoNoise_jsonFromValue(VuoNoise_Brown);
	const char *optionSummary_Brown = VuoNoise_summaryFromValue(VuoNoise_Brown);

	json_object *optionAsJson_Blue = VuoNoise_jsonFromValue(VuoNoise_Blue);
	const char *optionSummary_Blue = VuoNoise_summaryFromValue(VuoNoise_Blue);

	json_object *optionAsJson_Violet = VuoNoise_jsonFromValue(VuoNoise_Violet);
	const char *optionSummary_Violet = VuoNoise_summaryFromValue(VuoNoise_Violet);


	VuoInputEditorMenuItem *optionItem_White = new VuoInputEditorMenuItem(optionSummary_White, optionAsJson_White);
	VuoInputEditorMenuItem *optionItem_Grey = new VuoInputEditorMenuItem(optionSummary_Grey, optionAsJson_Grey);
	VuoInputEditorMenuItem *optionItem_Pink = new VuoInputEditorMenuItem(optionSummary_Pink, optionAsJson_Pink);
	VuoInputEditorMenuItem *optionItem_Brown = new VuoInputEditorMenuItem(optionSummary_Brown, optionAsJson_Brown);
	VuoInputEditorMenuItem *optionItem_Blue = new VuoInputEditorMenuItem(optionSummary_Blue, optionAsJson_Blue);
	VuoInputEditorMenuItem *optionItem_Violet = new VuoInputEditorMenuItem(optionSummary_Violet, optionAsJson_Violet);

	noiseOptionsTree->addItem(optionItem_White);
	noiseOptionsTree->addItem(optionItem_Grey);
	noiseOptionsTree->addItem(optionItem_Pink);
	noiseOptionsTree->addItem(optionItem_Brown);
	noiseOptionsTree->addItem(optionItem_Blue);
	noiseOptionsTree->addItem(optionItem_Violet);

	return noiseOptionsTree;
}
