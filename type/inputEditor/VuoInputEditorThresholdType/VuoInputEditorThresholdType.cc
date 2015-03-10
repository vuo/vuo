/**
 * @file
 * VuoInputEditorThresholdType implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorThresholdType.hh"

extern "C"
{
	#include "VuoThresholdType.h"
}

/**
 * Constructs a VuoInputEditorThresholdType object.
 */
VuoInputEditor * VuoInputEditorThresholdTypeFactory::newInputEditor()
{
	return new VuoInputEditorThresholdType();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorThresholdType::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for(int i = 0; i < VuoThresholdType_Alpha+1; i++)
	{
		json_object *optionAsJson = VuoThresholdType_jsonFromValue( (VuoThresholdType)i );
		char *optionSummary = VuoThresholdType_summaryFromValue( (VuoThresholdType)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
