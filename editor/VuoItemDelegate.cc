/**
 * @file
 * VuoItemDelegate implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoItemDelegate.hh"

/**
 * Creates a list item delegate.
 */
VuoItemDelegate::VuoItemDelegate(QObject *parent)
	: QItemDelegate(parent)
{
}

/**
 * Renders a list item.
 */
void VuoItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
	{
		painter->setPen(QPen(QColor("#ddd"), 2));
		painter->drawLine(option.rect.left(), option.rect.center().y(), option.rect.right(), option.rect.center().y());
	}
	else
		QItemDelegate::paint(painter, option, index);
}

/**
 * Returns the size of a list item.
 */
QSize VuoItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QString type = index.data(Qt::AccessibleDescriptionRole).toString();
	if (type == QLatin1String("separator"))
		return QSize(0, 20);

	return QItemDelegate::sizeHint(option, index);
}
