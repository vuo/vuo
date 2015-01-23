/**
 * @file
 * VuoRendererItem interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

	static void setDrawBoundingRects(bool drawBoundingRects);
	static void drawRect(QPainter *painter, QRectF rect);

	VuoRendererItem();

private:
	static bool drawBoundingRects;

protected:
	void drawBoundingRect(QPainter *painter);
	bool getRenderActivity();
};

#endif // VUORENDERERITEM_HH
