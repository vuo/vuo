/**
 * @file
 * VuoInputEditorMouseButton implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorMouseButton.hh"

extern "C"
{
	#include "VuoMouseButton.h"
}

/**
 * Constructs a VuoInputEditorMouseButton object.
 */
VuoInputEditor * VuoInputEditorMouseButtonFactory::newInputEditor()
{
	return new VuoInputEditorMouseButton();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorMouseButton::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	VuoInputEditorMenuItem *optionItem_Left = new VuoInputEditorMenuItem(VuoMouseButton_summaryFromValue(VuoMouseButton_Left),
																		 VuoMouseButton_jsonFromValue(VuoMouseButton_Left));
	VuoInputEditorMenuItem *optionItem_Middle = new VuoInputEditorMenuItem(VuoMouseButton_summaryFromValue(VuoMouseButton_Middle),
																		   VuoMouseButton_jsonFromValue(VuoMouseButton_Middle));
	VuoInputEditorMenuItem *optionItem_Right = new VuoInputEditorMenuItem(VuoMouseButton_summaryFromValue(VuoMouseButton_Right),
																		  VuoMouseButton_jsonFromValue(VuoMouseButton_Right));
	VuoInputEditorMenuItem *optionItem_Any = new VuoInputEditorMenuItem(VuoMouseButton_summaryFromValue(VuoMouseButton_Any),
																		VuoMouseButton_jsonFromValue(VuoMouseButton_Any));

	optionsTree->addItem(optionItem_Left);
	optionsTree->addItem(optionItem_Middle);
	optionsTree->addItem(optionItem_Right);
	optionsTree->addItem(optionItem_Any);

	return optionsTree;
}
