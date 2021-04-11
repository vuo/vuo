/**
 * @file
 * VuoRendererComment interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoHeap.h"
#include "VuoRendererColors.hh"
#include "VuoRendererItem.hh"

class VuoRendererSignaler;
class VuoComment;

/**
 * Renders a comment in a @c VuoRendererComposition.
 */
class VuoRendererComment : public VuoRendererItem, public VuoBaseDetail<VuoComment>
{
public:
	VuoRendererComment(VuoComment *baseComment, VuoRendererSignaler *signaler);

	QRectF boundingRect(void) const;
	QPainterPath shape(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void updateGeometry(void);
	void updateColor(void);

	void setContent(string content);
	void setBodySelectable(bool bodySelectable);

protected:
	bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) VuoWarnUnusedResult;
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	VuoRendererSignaler *signaler; ///< The object that sends signals on behalf of this renderer comment.

private:
	// Drawing configuration
	static const qreal cornerRadius;
	static const qreal borderWidth;
	static const qreal textMargin;

	QRectF frameRect;
	QPainterPath commentFrame;
	QGraphicsTextItem *textItem;

	bool resizeDragInProgress;
	QPointF resizeDeltaIgnored;
	bool dragHandleHovered;
	bool titleHandleHovered;
	bool bodySelectable;

	// Internal methods
	void updateFrameRect(void);
	QPainterPath getCommentFrame(QRectF frameRect) const;
	QPainterPath getTitleHandlePath(QRectF frameRect) const;
	QPainterPath getDragHandlePath(QRectF frameRect) const;
	QRectF extendedTitleHandleBoundingRect(void) const;
	QRectF extendedDragHandleBoundingRect(void) const;

	bool titleHandleActiveForEventPos(QPointF pos);
	bool titleHandleHoveredForEventPos(QPointF pos);
	bool dragHandleHoveredForEventPos(QPointF pos);

	void updateFormattedCommentText();
	QString generateTextStyleString();

	void drawCommentFrame(QPainter *painter, VuoRendererColors *colors) const;
	void drawTextContent(QPainter *painter) const;
	void drawTitleHandle(QPainter *painter, VuoRendererColors *colors) const;
	void drawDragHandle(QPainter *painter, VuoRendererColors *colors) const;
};

