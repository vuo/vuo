/**
 * @file
 * VuoLineEdit implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLineEdit.hh"

/**
 * Creates an instance of a line edit with a custom color specified for placeholder text.
 */
VuoLineEdit::VuoLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	updatePlaceholderTextStyle();
	connect(this, &QLineEdit::textChanged, this, &VuoLineEdit::updatePlaceholderTextStyle);
}

/**
  * Updates the text color of the line edit according to whether or not
  * the line edit is currently displaying placeholder text.
  */
void VuoLineEdit::updatePlaceholderTextStyle()
{
	// Placeholder text
	if (text().isEmpty())
		setStyleSheet("color: #808080;"); // #c0c0c0 after 50% opacity reduction

	// Normal text
	else
		setStyleSheet("color: black;");
}

VuoLineEdit::~VuoLineEdit()
{
}
