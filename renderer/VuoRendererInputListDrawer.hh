/**
 * @file
 * VuoRendererInputListDrawer interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoRendererInputDrawer.hh"

/**
 * Represents the resizable, compact form of a "Make List" node.
 */
class VuoRendererInputListDrawer : public VuoRendererInputDrawer
{
public:
	VuoRendererInputListDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QRectF boundingRect(void) const;
	QRectF getOuterNodeFrameBoundingRect(void) const;
	QPainterPath shape() const;
	void updateGeometry();

	QRectF getExtendedDragHandleRect() const;
	void setDragInProgress(bool inProgress);

	void extendedHoverEnterEvent(QPointF scenePos);
	void extendedHoverMoveEvent(QPointF scenePos);
	void extendedHoverLeaveEvent();

private:
	QPainterPath getMakeListNodePath() const;
	QPainterPath getMakeListDragHandlePath() const;
	QRectF getDragHandleRect() const;
	void triggerPortCountAdjustment(int portCountDelta, QGraphicsSceneMouseEvent *event);

	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	// User interaction with drag handle
	bool dragInProgress;
	QPointF mousePositionAtLastPortCountAdjustment;
	bool dragHandleIsHovered;

	QRectF cachedBoundingRect;
	void updateCachedBoundingRect();
};
