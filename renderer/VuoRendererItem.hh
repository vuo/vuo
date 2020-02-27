/**
 * @file
 * VuoRendererItem interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Subclass of QGraphicsItem, which can show bounding rects for debugging.
 */
class VuoRendererItem : public QGraphicsItem
{
public:
	/**
	 * Special composition component activity states.
	 */
	enum activityState
	{
		notTrackingActivity = -1,
		activityInProgress = 0
	};

	/**
	 * Stacking order for canvas elements.
	 */
	enum zValues
	{
		triggerAnimationZValue = 1,
		nodeZValue       = 0,
		portZValue       = -1, ///< So the slight antialiasing overlap isn't visible.
		attachmentZValue = -2,	///< Render attachments behind the port they're connected to, so the port hover highlight is consistently visible.
		cableZValue      = -3,
		errorMarkZValue  = -4,
		commentZValue = -5
	};

	static void setSnapToGrid(bool snap);
	static void setDrawBoundingRects(bool drawBoundingRects);
	static bool shouldDrawBoundingRects(void);
	static void drawRect(QPainter *painter, QRectF rect);
	static bool getSnapToGrid();

	VuoRendererItem();

	void setSelectable(bool selectable);

private:
	static bool drawBoundingRects;
	static bool snapToGrid;

protected:
	void drawBoundingRect(QPainter *painter);
	static void addRoundedCorner(QPainterPath &path, bool drawLine, QPointF sharpCornerPoint, qreal radius, bool isTop, bool isLeft);
	bool getRenderActivity() const;
	bool getRenderHiddenCables() const;
	QGraphicsItem::CacheMode getCurrentDefaultCacheMode() const;
};

