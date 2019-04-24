/**
 * @file
 * VuoInputEditorWithMenu implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWithMenu.hh"

/**
 * Displays a menu.
 */
json_object * VuoInputEditorWithMenu::show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues)
{
	acceptedValue = originalValue;
	this->details = details;
	VuoInputEditorMenuItem *menuTree = setUpMenuTree(details);

	QMenu *menu = new QMenu();
	menu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menu->setFont(getDefaultFont());
	QActionGroup *menuActionGroup;
	menuActionGroup = new QActionGroup(menu);

	connect(menuActionGroup, SIGNAL(triggered(QAction *)), this, SLOT(acceptAction(QAction *)));

	VuoInputEditorMenuItem::buildMenu(menu, menuActionGroup, menuTree);
	menuActionGroup->setExclusive(true);

	// Select the menu item corresponding to the original value.
	int checkIndex = 0;
	int index = 0;
	string originalValueAsString = json_object_to_json_string_ext(originalValue, JSON_C_TO_STRING_PLAIN);
	foreach (QAction *action, menuActionGroup->actions())
	{
		json_object *actionValue = (json_object *)action->data().value<void *>();
		string actionValueAsString = json_object_to_json_string_ext(actionValue, JSON_C_TO_STRING_PLAIN);
		bool isOriginalValue = (actionValueAsString == originalValueAsString);
		action->setChecked(isOriginalValue);
		if (isOriginalValue)
			checkIndex = index;
		++index;
	}

	// Position the right center of the selected menu item at the left center of the port.
	int xOffset = menu->sizeHint().width();
	int yOffset = (checkIndex + 0.5) / menu->actions().size() * menu->sizeHint().height();
	portLeftCenter = portLeftCenter - QPoint(xOffset, yOffset);
	menu->exec(portLeftCenter);

	return acceptedValue;
}

VuoInputEditorMenuItem *VuoInputEditorWithMenu::setUpMenuTree(json_object *details)
{
	return setUpMenuTree();
}

VuoInputEditorMenuItem *VuoInputEditorWithMenu::setUpMenuTree(void)
{
	return new VuoInputEditorMenuItem("root");
}

/**
 * Saves the selected menu item to be able to use it after the menu has closed.
 */
void VuoInputEditorWithMenu::acceptAction(QAction *action)
{
	acceptedValue = (json_object *)action->data().value<void *>();
}

/**
 * Returns true if `value` is in the port's `includeValues` list,
 * or if the port doesn't have an `includeValues` list (and thus all values should be included in the menu).
 */
bool VuoInputEditorWithMenu::shouldIncludeValue(json_object *value)
{
	json_object *includeValues = NULL;
	json_object_object_get_ex(details, "includeValues", &includeValues);
	if (!includeValues)
		return true;

	bool includeThisValue = false;
	int includeValuesCount = json_object_array_length(includeValues);
	const char *valueString = json_object_to_json_string(value);
	for (int j = 0; j < includeValuesCount; ++j)
	{
		const char *includeValueString = json_object_to_json_string(json_object_array_get_idx(includeValues, j));
		if (strcmp(valueString, includeValueString) == 0)
		{
			includeThisValue = true;
			break;
		}
	}

	return includeThisValue;
}
