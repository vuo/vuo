/**
 * @file
 * VuoInputEditorWrapMode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWrapMode.hh"

extern "C"
{
	#include "VuoWrapMode.h"
}

#include "../VuoInputEditorCurve/VuoInputEditorCurveRenderer.hh"

/**
 * Constructs a VuoInputEditorWrapMode object.
 */
VuoInputEditor * VuoInputEditorWrapModeFactory::newInputEditor()
{
	return new VuoInputEditorWrapMode();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorWrapMode::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	optionsTree->addItem(new VuoInputEditorMenuItem(VuoWrapMode_getSummary(VuoWrapMode_Wrap),		VuoWrapMode_getJson(VuoWrapMode_Wrap),		renderMenuIconWithWrapMode(VuoWrapMode_Wrap)));
	optionsTree->addItem(new VuoInputEditorMenuItem(VuoWrapMode_getSummary(VuoWrapMode_Saturate),	VuoWrapMode_getJson(VuoWrapMode_Saturate),	renderMenuIconWithWrapMode(VuoWrapMode_Saturate)));

	return optionsTree;
}
