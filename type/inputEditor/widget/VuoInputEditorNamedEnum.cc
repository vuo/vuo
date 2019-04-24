/**
 * @file
 * VuoInputEditorNamedEnum implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
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
		if (strcmp(json_object_get_string(val), "-") == 0)
		{
			optionsTree->addSeparator();
			continue;
		}

		long intkey = (int)strtol(key, (char **)NULL, 10);
		int intkeyResult = (intkey==0 ? errno : 0);
		bool enabled = true;

		if (intkeyResult == EINVAL)
			enabled = false;

		VuoInputEditorMenuItem *optionItem = new VuoInputEditorMenuItem(VuoText_makeFromJson(val),
																		VuoInteger_getJson(intkey),
																		NULL,
																		enabled);
		optionsTree->addItem(optionItem);
	}

	return optionsTree;
}
