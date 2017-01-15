/**
 * @file
 * VuoRendererItem interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERITEM_HH
#define VUORENDERERITEM_HH

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

	static void setSnapToGrid(bool snap);
	static void setDrawBoundingRects(bool drawBoundingRects);
	static void drawRect(QPainter *painter, QRectF rect);
	static bool getSnapToGrid();

	VuoRendererItem();

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

#endif // VUORENDERERITEM_HH
