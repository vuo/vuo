/**
 * @file
 * VuoRendererCable interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERCABLE_HH
#define VUORENDERERCABLE_HH

#include "VuoBaseDetail.hh"
#include "VuoCable.hh"
#include "VuoRendererItem.hh"

/**
 * Renders a cable in a @c VuoRendererComposition.
 */
class VuoRendererCable : public VuoRendererItem, public VuoBaseDetail<VuoCable>
{
public:
	VuoRendererCable(VuoCable * baseCable);

	static const qreal cableWidth;
	static const qreal cableHighlightWidth;
	static const qreal cableHighlightOffset;
	static const qreal cableYankRadius;

	QRectF boundingRect(void) const;
	QPainterPath shape(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPointF getFloatingEndpointLoc();
	void setFloatingEndpointLoc(QPointF loc);
	void removeFromScene();
	void extendedHoverEnterEvent();
	void extendedHoverMoveEvent();
	void extendedHoverLeaveEvent();
	bool yankZoneIncludes(QPointF scenePos);
	VuoPort * getFloatingEndpointPreviousToPort();
	void setFloatingEndpointPreviousToPort(VuoPort *port);
	void setHovered(bool hovered);
	void updateGeometry(void);
	void resetTimeLastEventPropagated();
	VuoNode::TintColor getTintColor(void);
	bool paintingDisabled(void) const;

private:
	// Drawing configuration
	QPointF floatingEndpointLoc;
	VuoPort *floatingEndpointPreviousToPort;
	bool isHovered;
	qint64 timeLastEventPropagated;

	// Internal methods
	QPointF getStartPoint(void) const;
	QPointF getEndPoint(void) const;
	QPainterPath getCablePath(void) const;
	bool isConnectedToSelectedNode(void);
};

#endif // VUORENDERERCABLE_HH
