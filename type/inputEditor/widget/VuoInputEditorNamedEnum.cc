/**
 * @file
 * VuoInputEditorNamedEnum implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */
#include "VuoInputEditorNamedEnum.hh"

extern "C"
{
	#include "VuoInteger.h"
	#include "VuoText.h"
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorNamedEnum::setUpMenuTree(json_object *details)
{
	VuoInputEditorMenuItem *optionsTree = new VuoInputEditorMenuItem("root");

	json_object *menuItemsValue = NULL;
	json_object_object_get_ex(details, "menuItems", &menuItemsValue);

	json_object_object_foreach(menuItemsValue, key, val)
	{
		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(VuoText_makeFromJson(val),
																		VuoInteger_getJson(atoi(key)));
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
