/**
 * @file
 * VuoInputEditorWithEnumMenu interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithMenu.hh"

/**
 * A base class for input editors that display a menu of enum values.
 */
class VuoInputEditorWithEnumMenu : public VuoInputEditorWithMenu
{
	Q_OBJECT

	QString type;

public:
	VuoInputEditorWithEnumMenu(QString type);

	VuoInputEditorMenuItem *setUpMenuTree(json_object *details);
};

