/**
 * @file
 * VuoRendererPortList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererPortList.hh"
#include "VuoRendererPort.hh"

/**
 * Creates a group of ports.
 */
VuoRendererPortList::VuoRendererPortList(VuoRendererNode *parent)
	: QGraphicsItemGroup((QGraphicsItem *)parent)
{
	setHandlesChildEvents(false);
}

/**
 * Returns an empty rectangle, since this class doesn't draw anything.
 */
QRectF VuoRendererPortList::boundingRect(void) const
{
	return QRectF();
}

/**
 * Returns the ports in this port group.
 */
QList<VuoRendererPort *> VuoRendererPortList::childItems() const
{
	// For some reason this doesn't work if the items inherit from QObject.
	QList<QGraphicsItem *> ci = QGraphicsItemGroup::childItems();
	return *(QList<VuoRendererPort *> *)&ci;
}
