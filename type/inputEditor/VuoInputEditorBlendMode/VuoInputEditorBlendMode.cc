/**
 * @file
 * VuoInputEditorBlendMode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
		addBlendMode(optionsTree, VuoBlendMode_Subtract);
		addBlendMode(optionsTree, VuoBlendMode_Multiply);
		addBlendMode(optionsTree, VuoBlendMode_DarkerComponents);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_LinearDodge);
		addBlendMode(optionsTree, VuoBlendMode_LighterComponents);
	}
	else
	{
		addBlendMode(optionsTree, VuoBlendMode_Normal);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Multiply);
		addBlendMode(optionsTree, VuoBlendMode_DarkerComponents);
		addBlendMode(optionsTree, VuoBlendMode_DarkerColor);
		addBlendMode(optionsTree, VuoBlendMode_LinearBurn);
		addBlendMode(optionsTree, VuoBlendMode_ColorBurn);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Screen);
		addBlendMode(optionsTree, VuoBlendMode_LighterComponents);
		addBlendMode(optionsTree, VuoBlendMode_LighterColor);
		addBlendMode(optionsTree, VuoBlendMode_LinearDodge);
		addBlendMode(optionsTree, VuoBlendMode_ColorDodge);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Overlay);
		addBlendMode(optionsTree, VuoBlendMode_SoftLight);
		addBlendMode(optionsTree, VuoBlendMode_HardLight);
		addBlendMode(optionsTree, VuoBlendMode_VividLight);
		addBlendMode(optionsTree, VuoBlendMode_LinearLight);
		addBlendMode(optionsTree, VuoBlendMode_PinLight);
		addBlendMode(optionsTree, VuoBlendMode_HardMix);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Difference);
		addBlendMode(optionsTree, VuoBlendMode_Exclusion);
		addBlendMode(optionsTree, VuoBlendMode_Subtract);
		addBlendMode(optionsTree, VuoBlendMode_Divide);
		addBlendMode(optionsTree, VuoBlendMode_Power);
		optionsTree->addSeparator();
		addBlendMode(optionsTree, VuoBlendMode_Hue);
		addBlendMode(optionsTree, VuoBlendMode_Saturation);
		addBlendMode(optionsTree, VuoBlendMode_Color);
		addBlendMode(optionsTree, VuoBlendMode_Luminosity);
	}

	return optionsTree;
}
