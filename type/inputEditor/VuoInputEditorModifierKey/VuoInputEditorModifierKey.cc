/**
 * @file
 * VuoInputEditorModifierKey implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorModifierKey.hh"

extern "C"
{
	#include "VuoModifierKey.h"
}

/**
 * Constructs a VuoInputEditorModifierKey object.
 */
VuoInputEditor * VuoInputEditorModifierKeyFactory::newInputEditor()
{
	return new VuoInputEditorModifierKey();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorModifierKey::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	VuoInputEditorMenuItem *optionItem_Any = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_Any),
																		VuoModifierKey_jsonFromValue(VuoModifierKey_Any));
	VuoInputEditorMenuItem *optionItem_Command = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_Command),
																			VuoModifierKey_jsonFromValue(VuoModifierKey_Command));
	VuoInputEditorMenuItem *optionItem_Option = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_Option),
																		   VuoModifierKey_jsonFromValue(VuoModifierKey_Option));
	VuoInputEditorMenuItem *optionItem_Control = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_Control),
																			VuoModifierKey_jsonFromValue(VuoModifierKey_Control));
	VuoInputEditorMenuItem *optionItem_Shift = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_Shift),
																		  VuoModifierKey_jsonFromValue(VuoModifierKey_Shift));
	VuoInputEditorMenuItem *optionItem_None = new VuoInputEditorMenuItem(VuoModifierKey_summaryFromValue(VuoModifierKey_None),
																		 VuoModifierKey_jsonFromValue(VuoModifierKey_None));

	optionsTree->addItem(optionItem_Any);
	optionsTree->addItem(optionItem_Command);
	optionsTree->addItem(optionItem_Option);
	optionsTree->addItem(optionItem_Control);
	optionsTree->addItem(optionItem_Shift);
	optionsTree->addItem(optionItem_None);

	return optionsTree;
}
