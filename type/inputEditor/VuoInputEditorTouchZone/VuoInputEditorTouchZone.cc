/**
 * @file
 * VuoInputEditorTouchZone implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorTouchZone.hh"

extern "C"
{
	#include "VuoLeapTouchZone.h"
}

/**
 * Constructs a VuoInputEditorTouchZone object.
 */
VuoInputEditor * VuoInputEditorTouchZoneFactory::newInputEditor()
{
	return new VuoInputEditorTouchZone();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorTouchZone::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	for(int i = 0; i < VuoLeapTouchZone_Touching+1; i++)
	{
		json_object *optionAsJson = VuoLeapTouchZone_jsonFromValue( (VuoLeapTouchZone)i );
		char *optionSummary = VuoLeapTouchZone_summaryFromValue( (VuoLeapTouchZone)i );
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(optionSummary, optionAsJson);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
