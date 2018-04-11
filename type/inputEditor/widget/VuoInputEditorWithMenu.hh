/**
 * @file
 * VuoInputEditorWithMenu interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditor.hh"
#include "VuoInputEditorMenuItem.hh"

/**
 * A base class for input editors that display a menu.
 */
class VuoInputEditorWithMenu : public VuoInputEditor
{
	Q_OBJECT

public:
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues);

protected:
	/**
	 * Creates the tree that models the menu.
	 */
	virtual VuoInputEditorMenuItem *setUpMenuTree(json_object *details);

	/**
	 * Creates the tree that models the menu.
	 */
	virtual VuoInputEditorMenuItem *setUpMenuTree(void);

	bool shouldIncludeValue(json_object *value);

private slots:
	void acceptAction(QAction *action);

private:
	json_object *details;
	json_object *acceptedValue;
};

