/**
 * @file
 * VuoComboBox interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * Subclasses QComboBox to enable keyboard/mouse focus, add the ability to enable and disable items.
 */
class VuoComboBox : public QComboBox
{
	Q_OBJECT

public:
	VuoComboBox(QWidget *parent = NULL);
	void setItemEnabled(int index, bool enabled);
};
