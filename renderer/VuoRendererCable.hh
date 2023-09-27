/**
 * @file
 * VuoRendererCable interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoRendererItem.hh"
#include "VuoRendererColors.hh"

class VuoCable;

/**
 * Renders a cable in a @c VuoRendererComposition.
 */
class VuoRendererCable : public VuoRendererItem, public VuoBaseDetail<VuoCable>
{
public:
	VuoRendererCable(VuoCable * baseCable);

	static const qreal cableWidth;
	static const qreal cableYankRadius;

	QRectF boundingRect(void) const;
	QPainterPath shape(void) const;
	QPainterPath getCablePath(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPointF getFloatingEndpointLoc();
	void setFloatingEndpointLoc(QPointF loc);
	void setFloatingEndpointAboveEventPort(bool aboveEventPort);
	bool isFloatingEndpointAboveEventPort(void);
	void setFrom(VuoNode *fromNode, VuoPort *fromPort);
	void setTo(VuoNode *toNode, VuoPort *toPort);
	bool effectivelyCarriesData(void) const;
	bool getEffectivelyWireless() const;
	void setWireless(bool wireless);
	void removeFromScene();
	void extendedHoverEnterEvent();
	void extendedHoverMoveEvent();
	void extendedHoverLeaveEvent();
	bool yankZoneIncludes(QPointF scenePos);
	VuoPort * getFloatingEndpointPreviousToPort();
	void setFloatingEndpointPreviousToPort(VuoPort *port);
	bool getPreviouslyAlwaysEventOnly();
	void setPreviouslyAlwaysEventOnly(bool eventOnly);
	void setHovered(bool hovered);
	void setEligibilityHighlight(VuoRendererColors::HighlightType eligibility);
	void updateGeometry(void);
	void resetTimeLastEventPropagated();
	VuoNode::TintColor getTintColor(void);
	bool paintingDisabled(void) const;
	QPainterPath getCablePathForEndpoints(QPointF from, QPointF to) const;
	static void getCableSpecs(bool cableCarriesData, qreal &cableWidth);
	void setCacheModeForCableAndConnectedPorts(QGraphicsItem::CacheMode mode);
	void setPortConstantsChanged();

private:
	// Drawing configuration
	QPainterPath getOutline(QPointF startPoint,
							QPointF endPoint,
							bool cableCarriesData);

	void getYankZonePath(QPointF startPoint,
						QPointF endPoint,
						bool cableCarriesData,
						bool toPortSupportsYanking,
						QPainterPath &yankZone);
	void arclineTo(QPainterPath &cablePath, QPointF to, float radius, float fromCableStandoff) const;

	QPointF floatingEndpointLoc;
	VuoPort *floatingEndpointPreviousToPort;
	bool previouslyAlwaysEventOnly;
	bool floatingEndpointAboveEventPort;
	bool isHovered;
	VuoRendererColors::HighlightType _eligibilityHighlight;
	qint64 timeLastEventPropagated;

	// Cached outline, and the cached attribute values used to calculate them.
	QPainterPath cachedOutline;
	QPair<QPointF, QPointF> cachedEndpointsForOutlines;
	bool cachedCarriesDataValueForOutlines;
	bool portConstantsChangedSinceLastCachedOutline;

	// Cached yank zones, and the cached attribute values used to calculate them.
	QPainterPath cachedYankZonePath;
	QPair<QPointF, QPointF> cachedEndpointsForYankZone;
	bool cachedCarriesDataValueForYankZone;
	bool cachedToPortSupportsYankingValueForYankZone;

	// Internal methods
	QPointF getStartPoint(void) const;
	QPointF getEndPoint(void) const;
	bool isPublishedInputCableWithoutVisiblePublishedPort() const;
	bool isPublishedOutputCableWithoutVisiblePublishedPort() const;
	bool isConnectedToSelectedNode(void);
};
