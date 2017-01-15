/**
 * @file
 * VuoInputEditorBlendMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
 * Appends `blendMode` to `menu`.
 */
static void addBlendMode(VuoInputEditorMenuItem *menu, VuoBlendMode blendMode)
{
	json_object *optionAsJson = VuoBlendMode_getJson(blendMode);
	char *optionSummary = VuoBlendMode_getSummary(blendMode);
	VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
	free(optionSummary);
	menu->addItem(optionItem);
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem *VuoInputEditorBlendMode::setUpMenuTree(json_object *details)
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	bool restrictToOpenGlBlendModes = false;
	json_object *o;
	if (json_object_object_get_ex(details, "restrictToOpenGlBlendModes", &o))
			restrictToOpenGlBlendModes = json_object_get_boolean(o);

	if (restrictToOpenGlBlendModes)
	{
		addBlendMode(optionsTree, VuoBlendMode_Normal);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_LinearDodge);
		addBlendMode(optionsTree, VuoBlendMode_LighterComponent);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Subtract);
		addBlendMode(optionsTree, VuoBlendMode_Multiply);
		addBlendMode(optionsTree, VuoBlendMode_DarkerComponent);
	}
	else
	{
		for(int i = 0; i < VuoBlendMode_Luminosity+1; i++)
		{
			addBlendMode(optionsTree, (VuoBlendMode)i);

			// Add separators after the last item in each group.
			if (i == VuoBlendMode_Normal
					|| i == VuoBlendMode_ColorBurn
					|| i == VuoBlendMode_ColorDodge
					|| i == VuoBlendMode_HardMix
					|| i == VuoBlendMode_Divide)
				optionsTree->addSeparator();
		}
	}

	return optionsTree;
}
