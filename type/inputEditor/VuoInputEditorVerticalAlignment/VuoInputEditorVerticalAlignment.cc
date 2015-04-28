/**
 * @file
 * VuoInputEditorVerticalAlignment implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorVerticalAlignment.hh"

extern "C"
{
	#include "VuoVerticalAlignment.h"
}

/**
 * Constructs a VuoInputEditorVerticalAlignment object.
 */
VuoInputEditor * VuoInputEditorVerticalAlignmentFactory::newInputEditor()
{
	return new VuoInputEditorVerticalAlignment();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorVerticalAlignment::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem(VuoVerticalAlignment_summaryFromValue(VuoVerticalAlignment_Top), VuoVerticalAlignment_jsonFromValue(VuoVerticalAlignment_Top)));
	optionsTree->addItem(new VuoInputEditorMenuItem(VuoVerticalAlignment_summaryFromValue(VuoVerticalAlignment_Center), VuoVerticalAlignment_jsonFromValue(VuoVerticalAlignment_Center)));
	optionsTree->addItem(new VuoInputEditorMenuItem(VuoVerticalAlignment_summaryFromValue(VuoVerticalAlignment_Bottom), VuoVerticalAlignment_jsonFromValue(VuoVerticalAlignment_Bottom)));

	return optionsTree;
}
