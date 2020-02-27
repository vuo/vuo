/**
 * @file
 * VuoInputEditorNamedEnum implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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

	int len = json_object_array_length(menuItemsValue);
	for (int i = 0; i < len; ++i)
	{
		json_object *menuItem = json_object_array_get_idx(menuItemsValue, i);
		if (json_object_is_type(menuItem, json_type_object))
		{
			json_object *value;
			json_object_object_get_ex(menuItem, "value", &value);
			json_object *name;
			json_object_object_get_ex(menuItem, "name", &name);

			optionsTree->addItem(new VuoInputEditorMenuItem(json_object_get_string(name), value, NULL, true));
		}
		else if (json_object_is_type(menuItem, json_type_string))
		{
			if (strcmp(json_object_get_string(menuItem), "---") == 0)
				optionsTree->addSeparator();
			else
				optionsTree->addItem(new VuoInputEditorMenuItem(json_object_get_string(menuItem), 0, NULL, false));
		}
	}

	return optionsTree;
}
