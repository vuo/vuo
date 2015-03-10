/**
 * @file
 * VuoInputEditorPointableType implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorPointableType.hh"

extern "C"
{
	#include "VuoLeapPointableType.h"
}

/**
 * Constructs a VuoInputEditorPointableType object.
 */
VuoInputEditor * VuoInputEditorPointableTypeFactory::newInputEditor()
{
	return new VuoInputEditorPointableType();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorPointableType::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for(int i = 0; i < VuoLeapPointableType_Tool+1; i++)
	{
		json_object *optionAsJson = VuoLeapPointableType_jsonFromValue( (VuoLeapPointableType)i );
		char *optionSummary = VuoLeapPointableType_summaryFromValue( (VuoLeapPointableType)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
