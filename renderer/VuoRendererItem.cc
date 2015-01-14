/**
 * @file
 * VuoRendererItem implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererItem.hh"
#include "VuoRendererComposition.hh"

bool VuoRendererItem::drawBoundingRects = false;

/**
 * Specifies whether bounding rects will be shown the next time the QGraphicsScene is rendered.
 */
void VuoRendererItem::setDrawBoundingRects(bool drawBoundingRects)
{
	VuoRendererItem::drawBoundingRects = drawBoundingRects;
}

/**
 * Retrieves the composition-wide boolean indicating whether recent activity
 * (e.g., a node execution or event firing) by this item should be reflected
 * in its rendering.
 */
bool VuoRendererItem::getRenderActivity()
{
	VuoRendererComposition *composition = dynamic_cast<VuoRendererComposition *>(scene());
	return (composition && composition->getRenderActivity());
}

VuoRendererItem::VuoRendererItem()
{
}

/**
 * Draws a rect, for debugging.
 */
void VuoRendererItem::drawRect(QPainter *painter, QRectF rect)
{
	if (!drawBoundingRects)
		return;

	painter->setPen(QPen(QColor(255,0,0),0));
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(rect);
}

/**
 * Draws this item's bounding rect, for debugging.
 */
void VuoRendererItem::drawBoundingRect(QPainter *painter)
{
	drawRect(painter, boundingRect());
}
