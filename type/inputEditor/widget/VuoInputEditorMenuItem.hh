/**
 * @file
 * VuoInputEditorMenuItem interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <QtGui/QIcon>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

/**
 * Interface of VuoInputEditorMenuItem. This class can be used recursively to construct a tree.
 */
class VuoInputEditorMenuItem
{
public:
	/**
	 * Constructs a menu item with the given human-readable title, representing the given value.
	 */
	VuoInputEditorMenuItem(string title = "", json_object *value = NULL, const QIcon *icon = NULL, bool isEnabled = true)
	{
		this->value = value;
		this->title = title;
		this->icon = icon;
		this->isSeparator = false;
		this->isEnabled = isEnabled;
	}

	/**
	 * Destroys the menu item.
	 */
	~VuoInputEditorMenuItem()
	{
		if (icon)
			delete icon;
	}

	/**
	 * Adds a child to a given VuoInputEditorMenuItem.
	 */
	void addItem(VuoInputEditorMenuItem *item)
	{
		childItems.push_back(item);
	}

	/**
	 * Adds a menu separator (horizontal line) to a given VuoInputEditorMenuItem.
	 */
	void addSeparator(void)
	{
		VuoInputEditorMenuItem *sep = new VuoInputEditorMenuItem;
		sep->isSeparator = true;
		childItems.push_back(sep);
	}

	/**
	 * Builds a menu and adds any actions to an action group.
	 */
	static void buildMenu(QMenu *menu, QActionGroup *actionGroup, VuoInputEditorMenuItem *item)
	{
		for(std::vector<VuoInputEditorMenuItem *>::iterator it = item->childItems.begin(); it != item->childItems.end(); ++it)
		{
			QString title = QString::fromStdString((*it)->title);

			// Qt uses '&' as a prefix for keyboard shortcuts.
			// These menus never have shortcuts, so double the ampersands to ensure they're visible.
			// Example: VuoTimeFormat
			// https://doc.qt.io/qt-5/qmenubar.html#details
			title.replace("&", "&&");

			if ((*it)->childItems.size() > 0)
			{
				QMenu *submenu = new QMenu(menu);
				submenu->setTitle(title);
				if ((*it)->icon)
					submenu->setIcon(*(*it)->icon);
				menu->addMenu(submenu);
				buildMenu(submenu, actionGroup, *it);
			}
			else
			{
				if ((*it)->isSeparator)
				{
					menu->addSeparator();
					continue;
				}

				QAction *action = new QAction(title, menu);
				if ((*it)->icon)
					action->setIcon(*(*it)->icon);
				action->setEnabled((*it)->isEnabled);
				action->setCheckable((*it)->isEnabled);
				action->setData(QVariant::fromValue((*it)->value));
				menu->addAction(action);
				action->setActionGroup(actionGroup);
			}
		}
	}

private:
	json_object *value;
	string title;
	const QIcon *icon;
	vector<VuoInputEditorMenuItem *> childItems;
	bool isSeparator;
	bool isEnabled;
};
