/**
 * @file
 * VuoPublishedPortNameEditor interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoTitleEditor.hh"

/**
 * A line edit that pops up in its own dialog and lets the user edit some text.
 * The text must meet the requirements of a published port identifier.
 */
class VuoPublishedPortNameEditor : public VuoTitleEditor
{
protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details=0);
	json_object * getAcceptedValue(void);
	json_object *originalValue;	///< The port's name prior to editing.
};

