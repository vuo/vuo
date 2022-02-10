/**
 * @file
 * VuoLineEdit interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A line edit with a custom color specified for placeholder text.
 */
class VuoLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit VuoLineEdit(QWidget *parent = 0);
	~VuoLineEdit();

private slots:
	void updatePlaceholderTextStyle();
};
