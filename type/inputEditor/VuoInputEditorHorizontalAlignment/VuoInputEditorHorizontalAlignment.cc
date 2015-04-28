/**
 * @file
 * VuoInputEditorHorizontalAlignment implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorHorizontalAlignment.hh"

extern "C"
{
	#include "VuoHorizontalAlignment.h"
}

/**
 * Constructs a VuoInputEditorHorizontalAlignment object.
 */
VuoInputEditor * VuoInputEditorHorizontalAlignmentFactory::newInputEditor()
{
	return new VuoInputEditorHorizontalAlignment();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorHorizontalAlignment::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem(VuoHorizontalAlignment_summaryFromValue(VuoHorizontalAlignment_Left), VuoHorizontalAlignment_jsonFromValue(VuoHorizontalAlignment_Left)));
	optionsTree->addItem(new VuoInputEditorMenuItem(VuoHorizontalAlignment_summaryFromValue(VuoHorizontalAlignment_Center), VuoHorizontalAlignment_jsonFromValue(VuoHorizontalAlignment_Center)));
	optionsTree->addItem(new VuoInputEditorMenuItem(VuoHorizontalAlignment_summaryFromValue(VuoHorizontalAlignment_Right), VuoHorizontalAlignment_jsonFromValue(VuoHorizontalAlignment_Right)));

	return optionsTree;
}
