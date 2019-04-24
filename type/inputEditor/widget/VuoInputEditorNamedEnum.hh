/**
 * @file
 * VuoInputEditorNamedEnum interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoInputEditorWithMenu.hh"

/**
 * An input editor that displays a menu of named enum values, as extracted from the JSON details object.
 *
 * To insert a menu separator, use value `"-"`.
 *
 * To disable a menu item (to make a section label), begin its key with non-numeric characters (e.g., `"Label 0"`).
 *
 * @eg{
 * {
 *     "menuItems":{
 *         "RGB Section":"RGB",
 *         "0":"    Red",
 *         "1":"    Green",
 *         "2":"    Blue",
 *
 *         "Separator":"-",
 *
 *         "CMYK Section":"CMYK",
 *         "3":"    Cyan",
 *         "4":"    Magenta",
 *         "5":"    Yellow",
 *         "6":"    Black",
 *     }
 * }
 * }
 */
class VuoInputEditorNamedEnum : public VuoInputEditorWithMenu
{
	Q_OBJECT

protected:
	VuoInputEditorMenuItem * setUpMenuTree(json_object *details);
};

