/**
 * @file
 * VuoInputEditorWrapMode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWrapMode.hh"

extern "C"
{
	#include "VuoWrapMode.h"
}

/**
 * Constructs a VuoInputEditorWrapMode object.
 */
VuoInputEditor * VuoInputEditorWrapModeFactory::newInputEditor()
{
	return new VuoInputEditorWrapMode();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorWrapMode::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	json_object *optionAsJson_Wrap = VuoWrapMode_jsonFromValue(VuoWrapMode_Wrap);
	const char *optionSummary_Wrap = VuoWrapMode_summaryFromValue(VuoWrapMode_Wrap);

	json_object *optionAsJson_Saturate = VuoWrapMode_jsonFromValue(VuoWrapMode_Saturate);
	const char *optionSummary_Saturate = VuoWrapMode_summaryFromValue(VuoWrapMode_Saturate);

	VuoInputEditorMenuItem *optionItem_Wrap = new VuoInputEditorMenuItem(optionSummary_Wrap, optionAsJson_Wrap);
	VuoInputEditorMenuItem *optionItem_Saturate = new VuoInputEditorMenuItem(optionSummary_Saturate, optionAsJson_Saturate);

	optionsTree->addItem(optionItem_Wrap);
	optionsTree->addItem(optionItem_Saturate);

	return optionsTree;
}
