/**
 * @file
 * VuoComboBox implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoComboBox.hh"

/**
 * Creates a new combo box.
 */
VuoComboBox::VuoComboBox(QWidget *parent)
	: QComboBox(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

/**
 * Enables or disables an item in a combo box menu.
 */
void VuoComboBox::setItemEnabled(int index, bool enabled)
{
	if (enabled)
		setItemData(index, QVariant(), Qt::UserRole - 1);
	else
		setItemData(index, true, Qt::UserRole - 1);
}
