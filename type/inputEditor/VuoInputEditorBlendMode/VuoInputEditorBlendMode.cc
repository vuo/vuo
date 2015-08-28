/**
 * @file
 * VuoInputEditorBlendMode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorBlendMode.hh"

extern "C"
{
	#include "VuoBlendMode.h"
}

/**
 * Constructs a VuoInputEditorBlendMode object.
 */
VuoInputEditor * VuoInputEditorBlendModeFactory::newInputEditor()
{
	return new VuoInputEditorBlendMode();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorBlendMode::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for(int i = 0; i < VuoBlendMode_Luminosity+1; i++)
	{
		json_object *optionAsJson = VuoBlendMode_jsonFromValue( (VuoBlendMode)i );
		char *optionSummary = VuoBlendMode_summaryFromValue( (VuoBlendMode)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
		free(optionSummary);
		optionsTree->addItem(optionItem);

		// Add separators after the last item in each group.
		if (i == VuoBlendMode_Normal
				|| i == VuoBlendMode_ColorBurn
				|| i == VuoBlendMode_ColorDodge
				|| i == VuoBlendMode_HardMix
				|| i == VuoBlendMode_Divide)
			optionsTree->addSeparator();
	}

	return optionsTree;
}
