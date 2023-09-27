/**
 * @file
 * VuoInputEditorNamedEnum interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithMenu.hh"

/**
 * An input editor that displays a menu of named enum values, as extracted from the "menuItems" key of the JSON details object.
 *
 * @see VuoInputData
 */
class VuoInputEditorNamedEnum : public VuoInputEditorWithMenu
{
	Q_OBJECT

protected:
	VuoInputEditorMenuItem * setUpMenuTree(json_object *details);
};
