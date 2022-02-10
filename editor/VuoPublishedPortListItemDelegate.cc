/**
 * @file
 * VuoPublishedPortListItemDelegate implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPublishedPortListItemDelegate.hh"
#include "VuoPublishedPortList.hh"
#include "VuoPublishedPortSidebar.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoEditor.hh"

/**
 * Creates a Published Port list rendering delegate.
 */
VuoPublishedPortListItemDelegate::VuoPublishedPortListItemDelegate(VuoEditorComposition *composition, QObject *parent) :
	QStyledItemDelegate(parent)
{
	this->composition = composition;
}

/**
 * Displays a given published port within the list.
 */
void VuoPublishedPortListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	painter->save();

	setUpSelectionHighlighting(painter, option, index);

	VuoRendererPublishedPort *publishedPort = index.data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	if (!publishedPort)
		return;

	QRect r = option.rect;
	painter->setClipRect(r);

	// Center port vertically within row.
	int yAdjusted = r.y() + r.size().height()/2;

	// Left-align published output ports; right-align published input ports.
	// "Published input" ports are *output* ports within the published input node,
	// and vice versa.
	bool isPublishedInput = !publishedPort->getInput();
	int xAdjusted = (isPublishedInput?
						 r.x() + r.size().width():
						 r.x());

	painter->translate(xAdjusted, yAdjusted);
	publishedPort->paint(painter, NULL, NULL);

	painter->restore();
}

/**
 * Sets the published port background color appropriately, depending whether
 * the node class is currently selected within the list.
 */
void VuoPublishedPortListItemDelegate::setUpSelectionHighlighting(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	VuoEditor *editor = (VuoEditor *)qApp;
	bool isDark = editor->isInterfaceDark();

	VuoRendererPublishedPort *publishedPort = index.data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	if (!publishedPort)
		return;

	QColor itemColor;
	if (option.state & QStyle::State_Selected)
	{
		bool isActive = option.state & QStyle::State_Active;
		if (isActive)
			itemColor = isDark ? QColor("#1d6ae5") : option.palette.brush(QPalette::Normal, QPalette::Highlight).color();
		else
			itemColor = Qt::transparent;
	}
	else
	{
		itemColor = dynamic_cast<VuoPublishedPort *>(publishedPort->getBase())->isProtocolPort()
				? VuoPublishedPortSidebar::getActiveProtocolPortColor(0, publishedPort->getInput())
				: QColor(isDark ? "#404040" : option.palette.button().color().name());
	}
	painter->fillRect(option.rect, itemColor);

	publishedPort->setSelected(option.state & QStyle::State_Selected);
	publishedPort->setCurrentlyActive(option.state & QStyle::State_Active);
}

/**
 * Returns the size needed to display the published port specified by @c index.
 */
QSize VuoPublishedPortListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	double heightScalingFactor = 1.5;
	VuoRendererPublishedPort *publishedPort = index.data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	if (!publishedPort)
		return QSize(0,0);

	return QSize(publishedPort->boundingRect().width(), publishedPort->boundingRect().height()*heightScalingFactor);
}
