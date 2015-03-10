/**
 * @file
 * VuoInputEditorImageWrapMode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorImageWrapMode.hh"

extern "C"
{
	#include "VuoImageWrapMode.h"
}

/**
 * Constructs a VuoInputEditorImageWrapMode object.
 */
VuoInputEditor * VuoInputEditorImageWrapModeFactory::newInputEditor()
{
	return new VuoInputEditorImageWrapMode();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorImageWrapMode::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for(int i = 0; i < VuoImageWrapMode_MirroredRepeat+1; i++)
	{
		json_object *optionAsJson = VuoImageWrapMode_jsonFromValue( (VuoImageWrapMode)i );
		char *optionSummary = VuoImageWrapMode_summaryFromValue( (VuoImageWrapMode)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
		
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
