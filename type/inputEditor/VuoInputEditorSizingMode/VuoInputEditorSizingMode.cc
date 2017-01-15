/**
 * @file
 * VuoInputEditorSizingMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorSizingMode.hh"

extern "C"
{
	#include "VuoSizingMode.h"
}

#include "../VuoInputEditorCurve/VuoInputEditorCurveRenderer.hh"

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

	for (int i = 0; i < VuoSizingMode_Fill+1; ++i)
	{
		json_object *optionAsJson = VuoSizingMode_getJson( (VuoSizingMode)i );
		char *optionSummary = VuoSizingMode_getSummary( (VuoSizingMode)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson, renderMenuIconWithSizingMode((VuoSizingMode)i));
		free(optionSummary);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
