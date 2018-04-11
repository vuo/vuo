/**
 * @file
 * VuoInputEditorNamedEnum interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithMenu.hh"

/**
 * An input editor that displays a menu of named enum values, as extracted from the JSON details object.
 *
 * @eg{
 *   {
 *		"menuItems":{
 *			"0":"Red",
 *			"1":"Green",
 *			"2":"Blue"
 *		}
 *   }
 * }
 */
class VuoInputEditorNamedEnum : public VuoInputEditorWithMenu
{
	Q_OBJECT

protected:
	VuoInputEditorMenuItem * setUpMenuTree(json_object *details);
};

