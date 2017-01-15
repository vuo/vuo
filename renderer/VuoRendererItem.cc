/**
 * @file
 * VuoRendererItem implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererItem.hh"
#include "VuoRendererComposition.hh"

bool VuoRendererItem::drawBoundingRects = false;
bool VuoRendererItem::snapToGrid = false;

/**
 * Specifies whether bounding rects will be shown the next time the QGraphicsScene is rendered.
 */
void VuoRendererItem::setDrawBoundingRects(bool drawBoundingRects)
{
	VuoRendererItem::drawBoundingRects = drawBoundingRects;
}

/**
 * Specifies whether graphics items should be snapped to a grid.
 */
void VuoRendererItem::setSnapToGrid(bool snap)
{
	VuoRendererItem::snapToGrid = snap;
}

/**
 * Retrieves the boolean indicating whether graphics items should be
 * snapped to a grid.
 */
bool VuoRendererItem::getSnapToGrid()
{
	return VuoRendererItem::snapToGrid;
}

/**
 * Retrieves the composition-wide boolean indicating whether recent activity
 * (e.g., a node execution or event firing) by this item should be reflected
 * in its rendering.
 */
bool VuoRendererItem::getRenderActivity() const
{
	VuoRendererComposition *composition = dynamic_cast<VuoRendererComposition *>(scene());
	return (composition && composition->getRenderActivity());
}

/**
 * Retrieves the composition-wide boolean indicating whether hidden ("wireless")
 * cables should be rendered as if they were not hidden.
 */
bool VuoRendererItem::getRenderHiddenCables() const
{
	VuoRendererComposition *composition = dynamic_cast<VuoRendererComposition *>(scene());
	return (composition && composition->getRenderHiddenCables());
}

/**
 * Returns the current default cache mode for components of this composition.
 */
QGraphicsItem::CacheMode VuoRendererItem::getCurrentDefaultCacheMode() const
{
	VuoRendererComposition *composition = dynamic_cast<VuoRendererComposition *>(scene());
	if (composition)
		return composition->getCurrentDefaultCacheMode();
	else
		return QGraphicsItem::NoCache;
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

/**
 * Appends a line (or move) and curve to @c path, to produce a clockwise-wound rounded corner near @c sharpCornerPoint.
 */
void VuoRendererItem::addRoundedCorner(QPainterPath &path, bool drawLine, QPointF sharpCornerPoint, qreal radius, bool isTop, bool isLeft)
{
	QPointF p(
				sharpCornerPoint.x() + (isTop ? (isLeft ? 0 : -radius) : (isLeft ? radius : 0)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? radius : 0) : (isLeft ? 0 : -radius))
				);
	if (drawLine)
		path.lineTo(p);
	else
		path.moveTo(p);

	path.cubicTo(
				sharpCornerPoint.x() + (isTop ? (isLeft ? 0 : -radius/2.) : (isLeft ? radius/2. : 0)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? radius/2. : 0) : (isLeft ? 0 : -radius/2.)),

				sharpCornerPoint.x() + (isTop ? (isLeft ? radius/2. : 0) : (isLeft ? 0 : -radius/2.)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? 0 : radius/2.) : (isLeft ? -radius/2. : 0)),

				sharpCornerPoint.x() + (isTop ? (isLeft ? radius : 0) : (isLeft ? 0 : -radius)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? 0 : radius) : (isLeft ? -radius : 0))
				);
}
