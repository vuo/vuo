/**
 * @file
 * VuoInputEditorSizingMode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorSizingMode.hh"

extern "C"
{
	#include "VuoSizingMode.h"
}

#include "VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorSizingMode object.
 */
VuoInputEditor * VuoInputEditorSizingModeFactory::newInputEditor()
{
	return new VuoInputEditorSizingMode();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorSizingMode::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for (int i = 0; i < VuoSizingMode_Proportional + 1; ++i)
	{
		json_object *optionAsJson = VuoSizingMode_getJson( (VuoSizingMode)i );
		char *optionSummary = VuoSizingMode_getSummary( (VuoSizingMode)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson, renderMenuIconWithSizingMode((VuoSizingMode)i, isInterfaceDark()));
		free(optionSummary);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
