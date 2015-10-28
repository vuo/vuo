/**
 * @file
 * VuoRendererCable interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	QPainterPath getCablePath(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPointF getFloatingEndpointLoc();
	void setFloatingEndpointLoc(QPointF loc);
	void setFloatingEndpointAboveEventPort(bool aboveEventPort);
	void setFrom(VuoNode *fromNode, VuoPort *fromPort);
	void setTo(VuoNode *toNode, VuoPort *toPort);
	bool effectivelyCarriesData(void) const;
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
	static const qreal publishedCableStubLength;

	// Drawing configuration
	QPainterPath getOutline(QPointF startPoint,
							QPointF endPoint,
							bool cableCarriesData);

	void getYankZonePaths(QPointF startPoint,
						QPointF endPoint,
						bool cableCarriesData,
						bool toPortSupportsYanking,
						QPainterPath &yankZone,
						QPainterPath &invertedYankZone
						);
	QPainterPath getPublishedAntennaCrossbarPath() const;

	QPointF floatingEndpointLoc;
	VuoPort *floatingEndpointPreviousToPort;
	bool floatingEndpointAboveEventPort;
	bool isHovered;
	qint64 timeLastEventPropagated;

	// Cached outline, and the cached attribute values used to calculate them.
	QPainterPath cachedOutline;
	QPair<QPointF, QPointF> cachedEndpointsForOutlines;
	bool cachedCarriesDataValueForOutlines;

	// Cached yank zones, and the cached attribute values used to calculate them.
	QPair<QPainterPath, QPainterPath> cachedYankZonePaths; // (yankZone, invertedYankZone)
	QPair<QPointF, QPointF> cachedEndpointsForYankZone;
	bool cachedCarriesDataValueForYankZone;
	bool cachedToPortSupportsYankingValueForYankZone;

	// Internal methods
	QPointF getStartPoint(void) const;
	QPointF getEndPoint(void) const;
	bool isPublishedInputCableWithoutVisiblePublishedPort() const;
	bool isPublishedOutputCableWithoutVisiblePublishedPort() const;
	static QPainterPath getCablePathForEndpoints(QPointF from, QPointF to);
	static void getCableSpecs(	bool cableCarriesData,
								qreal &cableWidth,
								qreal &cableHighlightWidth,
								qreal &cableHighlightOffset);
	bool isConnectedToSelectedNode(void);
};

#endif // VUORENDERERCABLE_HH
