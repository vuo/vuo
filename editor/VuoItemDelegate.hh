/**
 * @file
 * VuoItemDelegate interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Draws list items with menu item separators that look like the separators in macOS menus.
 */
class VuoItemDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	explicit VuoItemDelegate(QObject *parent = 0);

protected:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
