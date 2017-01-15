/**
 * @file
 * VuoInputEditorWithEnumMenu interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORWITHENUMMENU_HH
#define VUOINPUTEDITORWITHENUMMENU_HH

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

#endif // VUOINPUTEDITORWITHENUMMENU_HH
