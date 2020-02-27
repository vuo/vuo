/**
 * @file
 * VuoNodeClassListItemDelegate interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Displays a given node class name within the node library.
 */
class VuoNodeClassListItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit VuoNodeClassListItemDelegate(QObject *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	bool getHumanReadable() const;
	void setHumanReadable(bool humanReadable);
	void setFilterText(QString filterText);

	static const int humanReadableNameIndex; ///< The node's title.
	static const int classNameIndex; ///< The node's class name.
	static const int nodeClassPointerIndex; ///< A @c VuoCompilerNodeClass instance.
	static const int baseTitleFontPt; ///< The size (in typographic points) to render the node title.
	static const int disambiguatingTitleFontPt; ///< The size (in typographic points) to render the node class name.

signals:

public slots:

private:
	void paintTextDocument(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setUpSelectionHighlighting(QPainter *painter, QColor &baseTextColor, QColor &disambiguatingTitleFontColor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QTextDocument *generateTextDocument(const QModelIndex &index, QColor baseTextColor=Qt::black, QColor disambiguatingTitleFontColor=Qt::gray) const;

	bool humanReadable;
	QString filterText;
};
