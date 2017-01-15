/**
 * @file
 * VuoInputEditorScreen implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorScreen.hh"

extern "C"
{
	#include "VuoScreen.h"
	#include "VuoScreenCommon.h"
}

/**
 * Constructs a VuoInputEditorScreen object.
 */
VuoInputEditor * VuoInputEditorScreenFactory::newInputEditor()
{
	return new VuoInputEditorScreen();
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorScreen::setUpMenuTree()
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	VuoScreen mainScreen      = {VuoScreenType_Active,     0,"",false,{0,0},0,0,0,0};
	VuoScreen primaryScreen   = {VuoScreenType_Primary,  0,"",false,{0,0},0,0,0,0};
	VuoScreen secondaryScreen = {VuoScreenType_Secondary,0,"",false,{0,0},0,0,0,0};

	optionsTree->addItem(new VuoInputEditorMenuItem("Screen with Active Window", VuoScreen_getJson(mainScreen)));
	optionsTree->addItem(new VuoInputEditorMenuItem("Primary Screen", VuoScreen_getJson(primaryScreen)));
	optionsTree->addItem(new VuoInputEditorMenuItem("Secondary Screen", VuoScreen_getJson(secondaryScreen)));
	optionsTree->addSeparator();
	optionsTree->addItem(new VuoInputEditorMenuItem("Screens on this computer", NULL, NULL, false));

	VuoList_VuoScreen screens = VuoScreen_getList();

	unsigned long screenCount = VuoListGetCount_VuoScreen(screens);
	if (screenCount)
		for (unsigned long i = 1; i <= screenCount; ++i)
		{
			VuoScreen screen = (VuoScreen)VuoListGetValue_VuoScreen(screens, i);
			optionsTree->addItem(new VuoInputEditorMenuItem(VuoText_format("      %s", screen.name), VuoScreen_getJson(screen)));
		}
	else
		optionsTree->addItem(new VuoInputEditorMenuItem("      (no screens found)", NULL, NULL, false));

	return optionsTree;
}
