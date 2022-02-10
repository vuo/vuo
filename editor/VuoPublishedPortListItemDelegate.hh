/**
 * @file
 * VuoPublishedPortListItemDelegate interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorComposition;

/**
 * Displays a given published port within the published port sidebar.
 */
class VuoPublishedPortListItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit VuoPublishedPortListItemDelegate(VuoEditorComposition *composition, QObject *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setUpSelectionHighlighting(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:

public slots:

private:
	VuoEditorComposition *composition;

};

