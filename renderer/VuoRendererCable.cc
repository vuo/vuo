/**
 * @file
 * VuoRendererCable implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererCable.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoCable.hh"
#include "VuoCompilerCable.hh"
#include "VuoNodeClass.hh"


/// The width of a data-event cable.  (Event-only cable width is determined by @ref VuoRendererCable::getCableSpecs.)
const qreal VuoRendererCable::cableWidth = 5;  // VuoRendererFonts::thickPenWidth / 4.;

/// Radius from the endpoints of the cable in which yanking is permitted.
const qreal VuoRendererCable::cableYankRadius = 20;  // VuoRendererFonts::thickPenWidth;

/**
 * Creates a renderer detail for the specified @c baseCable.
 */
VuoRendererCable::VuoRendererCable(VuoCable *baseCable)
	: VuoBaseDetail<VuoCable>("VuoRendererCable", baseCable)
{
	getBase()->setRenderer(this);

	portConstantsChangedSinceLastCachedOutline = true;

	setZValue(cableZValue);

	VuoNode *fromNode = getBase()->getFromNode();
	VuoRendererNode *fromRN = ((fromNode && fromNode->hasRenderer())? fromNode->getRenderer() : NULL);

	if (fromRN)
		getBase()->getFromPort()->getRenderer()->updateGeometry();

	VuoNode *toNode = getBase()->getToNode();
	VuoRendererNode * toRN = ((toNode && toNode->hasRenderer())? toNode->getRenderer() : NULL);

	if (toRN)
		getBase()->getToPort()->getRenderer()->updateGeometry();

	floatingEndpointLoc = QPointF();
	floatingEndpointPreviousToPort = NULL;
	previouslyAlwaysEventOnly = false;
	floatingEndpointAboveEventPort = false;
	isHovered = false;
	_eligibilityHighlight = VuoRendererColors::noHighlight;

	this->setAcceptHoverEvents(true);
	this->setFlags(QGraphicsItem::ItemIsSelectable);
	resetTimeLastEventPropagated();
}

/**
 * Returns a rectangle containing the rendered cable (including thick cable width).
 */
QRectF VuoRendererCable::boundingRect(void) const
{
	if (paintingDisabled())
		return QRectF();

	QPainterPath cablePath = getCablePath();
	QRectF r = cablePath.boundingRect();

	if (r.isNull())
		return r;

	// Include thick width of cable.
	r.adjust(-cableWidth/2., -cableWidth/2., cableWidth/2., cableWidth/2.);

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Returns the shape of the rendered cable, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererCable::shape() const
{
	if (paintingDisabled())
		return QPainterPath();

	QPainterPath p;
	p.addPath(getCablePath());

	QPainterPathStroker s;
	s.setWidth(cableWidth);

	return s.createStroke(p);
}

/**
 * Returns the cable's starting point
 */
QPointF VuoRendererCable::getStartPoint(void) const
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoPort *fromPort = getBase()->getFromPort();

	if (fromPort && fromPort->hasRenderer())
	{
		VuoRendererPublishedPort *sidebarPort = dynamic_cast<VuoRendererPublishedPort *>(fromPort->getRenderer());
		if (sidebarPort)
		{
			QList<QGraphicsView *> views = scene()->views();
			return (views.empty()? QPointF(0,0) : views[0]->mapToScene(sidebarPort->getCompositionViewportPos()));
		}

		else if (fromNode && fromNode->hasRenderer())
			return fromNode->getRenderer()->pos() + fromPort->getRenderer()->pos() + QPointF(0, -.35);
	}

	return floatingEndpointLoc;
}

/**
 * Returns the cable's ending point
 */
QPointF VuoRendererCable::getEndPoint(void) const
{
	VuoCable *cable = getBase();
	VuoNode *toNode = cable->getToNode();
	VuoPort *toPort = cable->getToPort();

	if (toNode && toPort && toPort->hasRenderer())
	{
		VuoRendererPublishedPort *sidebarPort = dynamic_cast<VuoRendererPublishedPort *>(toPort->getRenderer());
		if (sidebarPort)
		{
			QList<QGraphicsView *> views = scene()->views();
			return (views.empty()? QPointF(0,0) : views[0]->mapToScene(sidebarPort->getCompositionViewportPos()));
		}

		else if (toNode->hasRenderer())
		{
			VuoRendererNode *rn = toNode->getRenderer();
			VuoRendererNode *toNodeProxy = rn->getProxyNode();
			if (toNodeProxy)
				return toNodeProxy->pos() + toPort->getRenderer()->pos() + QPointF(0, -.35);
			else
				return rn->pos() + toPort->getRenderer()->pos() + QPointF(0, -.35);
		}
	}

	return floatingEndpointLoc;
}

/**
 * Returns the curve along which this cable is drawn.
 */
QPainterPath VuoRendererCable::getCablePath(void) const
{
	if (paintingDisabled())
		return QPainterPath();

	return getCablePathForEndpoints(getStartPoint(), getEndPoint());
}

/**
 * Adds to `cablePath` a line with circular arcs on the ends.
 * Tries to escape touching the node when possible.
 * To this end, takes into account the vertical boundaries of the "From" node,
 * "From" port, "To" node, and "To" port, in scene coordinates.
 */
void VuoRendererCable::arclineTo(QPainterPath &cablePath, QPointF to, float radius, float fromCableStandoff) const
{
	QPointF from = cablePath.currentPosition();
	float direction = from.y() < to.y() ? 1 : -1;

	QRectF fromRect(from - QPointF(radius, -radius*2*direction), from + QPointF(radius, 0));

	if (to.x() >= from.x() + radius*2)
	{
		// Cable is going left-to-right.
		// Draw an arc at the cable's start and end.

		// Cable start:
		float stopAngle = -atan2(to.y() - from.y(), to.x() - from.x()) * 180/M_PI;
		cablePath.arcTo(fromRect, -90, -stopAngle*direction);

		// Cable end:
		QRectF toRect(to - QPointF(radius, radius*2*direction), to + QPointF(radius, 0));
		cablePath.arcTo(toRect, -90 + stopAngle*direction, -stopAngle*direction);
	}
	else
	{
		// Cable is going right-to-left.
		// At the cable's start and end, draw an arc, then draw a line to escape the node height, then draw another arc.

		// Cable start:
		cablePath.arcTo(fromRect, -90, 90);

		float fromNodeEscape = 0;
		{
			VuoNode *fromNode = getBase()->getFromNode();
			VuoPort *fromPort = getBase()->getFromPort();
			if (fromNode && fromNode->hasRenderer()
			 && fromPort && fromPort->hasRenderer())
			{
				VuoRendererNode *rn = fromNode->getRenderer();
				if (rn->getProxyNode())
					rn = rn->getProxyNode();

				QRectF nodeRect = rn->sceneBoundingRect();
				QRectF portRect = fromPort->getRenderer()->sceneBoundingRect();
				if (from.y() < to.y())
					// Cable is going downward, so escape the part of the node below the `from` port.
					fromNodeEscape = nodeRect.bottom() - portRect.bottom();
				else
					// Cable is going upward.
					fromNodeEscape = portRect.top() - nodeRect.top();
			}

			fromNodeEscape += fromCableStandoff;
		}

		float toNodeEscape = 0;
		VuoNode *toNode = getBase()->getToNode();
		{
			VuoPort *toPort = getBase()->getToPort();
			if (toNode && toNode->hasRenderer()
			 && toPort && toPort->hasRenderer())
			{
				VuoRendererNode *rn = toNode->getRenderer();
				if (rn->getProxyNode())
					rn = rn->getProxyNode();

				QRectF nodeRect = rn->sceneBoundingRect();
				QRectF portRect = toPort->getRenderer()->sceneBoundingRect();
				if (from.y() < to.y())
					// Cable is going downward, so escape the part of the node above the `to` port.
					toNodeEscape = portRect.top() - nodeRect.top();
				else
					// Cable is going upward.
					toNodeEscape = nodeRect.bottom() - portRect.bottom();
			}
		}

		// When a cable connects 2 nodes that are 1 grid spacing apart,
		// make the horizontal section of the cable exactly horizontal.
		fromNodeEscape += direction>0 ? 4.85 : 3.15;
		if (toNode && toNode->getNodeClass()->isDrawerNodeClass())
			toNodeEscape   += direction>0 ? 8 : -1.75;
		else
			toNodeEscape   += direction>0 ? 4.15 : 4.85;

		// If the ports are so close that the escapement would look loopy, forget it.
		if (fromNodeEscape + toNodeEscape + radius*3 >= fabs(from.y() - to.y()))
			fromNodeEscape = toNodeEscape = 0;

		QPointF fromEscaped = cablePath.currentPosition() + QPointF(0, fromNodeEscape*direction);
		QPointF toEscaped = to - QPointF(radius, (radius + toNodeEscape)*direction);

		QRectF fromEscapedRect(fromEscaped - QPointF(radius*2, radius*direction),
							   fromEscaped + QPointF(0, radius*direction));

		float stopAngle = atan2(fromEscaped.x() - toEscaped.x(), fromEscaped.y() - toEscaped.y()) * 180/M_PI;
		if (to.y() > from.y())
			stopAngle = 180-stopAngle;

		cablePath.arcTo(fromEscapedRect, 0, -stopAngle);


		// Cable end:
		QRectF toEscapedRect(toEscaped - QPointF(0, radius*direction), toEscaped + QPointF(radius*2, radius*direction));
		cablePath.arcTo(toEscapedRect, 180 - stopAngle, stopAngle);

		QRectF toRect(to-QPointF(radius, radius*2*direction), to + QPointF(radius, 0));
		cablePath.arcTo(toRect, 180, 90);
	}
}

/**
 * Returns a cable's path, given its start and end points.
 * Also takes into account:
 * - Other cables originating from this cable's "From" node, including their wireless status;
 * - Constant input flags attached to this cable's "To" node.
 */
QPainterPath VuoRendererCable::getCablePathForEndpoints(QPointF from, QPointF to) const
{
	QPainterPath cablePath(from);

	// The `from` node's other output ports (besides the port it's connected to)
	// may have cables, so escape them to avoid overlapping.
	float fromStandoff = 0; // Total standoff
	float fromCableStandoff = 0; // Standoff due to routed cables (excluding hidden cables)
	VuoNode *fromNode = getBase()->getFromNode();
	VuoPort *fromPort = getBase()->getFromPort();
	if (fromNode && fromPort)
	{
		vector<VuoPort *> outputs = (from.y() < to.y())
				? fromNode->getOutputPortsAfterPort(fromPort)
				: fromNode->getOutputPortsBeforePort(fromPort);
		for (vector<VuoPort *>::iterator i = outputs.begin(); i != outputs.end(); ++i)
		{
			vector<VuoCable *> cables = (*i)->getConnectedCables();
			if ((*i)->hasRenderer() && !cables.empty())
			{
				bool hasOnlyWireless = true;
				for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
					if ((*cable)->hasRenderer())
					{
						VuoRendererCable *cableR = (*cable)->getRenderer();
						if (!cableR->getEffectivelyWireless())
						{
							hasOnlyWireless = false;
							break;
						}
					}

				if (hasOnlyWireless)
					// We only need to escape the wireless antenna once (we don't need to escape every port's antenna).
					fromStandoff = fmax(fromStandoff, VuoRendererPort::portSpacing);
				else
				{
					fromStandoff += VuoRendererPort::portSpacing;
					fromCableStandoff += VuoRendererPort::portSpacing;
				}
			}
		}
	}
	fromStandoff += VuoRendererPort::portSpacing / 2.;

	// The `to` node's other input ports (besides the port it's connected to)
	// may have constant flags, so escape them.
	float toStandoff = 0;
	VuoNode *toNode = getBase()->getToNode();
	VuoPort *toPort = getBase()->getToPort();
	if (toNode && toPort)
	{
		vector<VuoPort *> inputs = (from.y() < to.y())
				? toNode->getInputPortsBeforePort(toPort)
				: toNode->getInputPortsAfterPort(toPort);
		for (vector<VuoPort *>::iterator i = inputs.begin(); i != inputs.end(); ++i)
			if ((*i)->hasRenderer() && (*i)->getRenderer()->isConstant())
			{
				float constantFlagWidth = (*i)->getRenderer()->getPortConstantTextRect().width();
				toStandoff = fmax(toStandoff, constantFlagWidth);
			}
	}

	// Move the cable attachment point to the left of the port constant.
	if (!effectivelyCarriesData() && toPort)
		toStandoff = fmax(toStandoff, toPort->getRenderer()->getPortConstantTextRect().width());

	toStandoff += VuoRendererPort::portSpacing / 2.;

	QPointF fromWithStandoff(from.x() + fromStandoff, from.y());
	QPointF   toWithStandoff(  to.x() -   toStandoff,   to.y());

	float length = (from - to).manhattanLength();
	if (length < fromStandoff + toStandoff + VuoRendererPort::portSpacing * 3)
	{
		// Cable is very short; just draw a single line.
	}
	else
	{
		cablePath.lineTo(fromWithStandoff);
		arclineTo(cablePath, toWithStandoff, VuoRendererPort::portSpacing / 2., fromCableStandoff);
	}

	cablePath.lineTo(to);

	return cablePath;
}

/**
 * Returns the tint color of the cable.
 * Tint the cable with the color of its "From" node.
 */
VuoNode::TintColor VuoRendererCable::getTintColor(void)
{
	VuoPort *fromPort = getBase()->getFromPort();
	if (fromPort && fromPort->hasRenderer())
		return fromPort->getRenderer()->getPortTint();

	return VuoNode::TintNone;
}

/**
 * Returns a boolean indicating whether this cable is connected to or from a node that is currently selected,
 * either directly or by way of a collapsed typecast and/or attachment.
 */
bool VuoRendererCable::isConnectedToSelectedNode(void)
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoNode *toNode = getBase()->getToNode();

	bool fromNodeIsSelected = (fromNode && fromNode->hasRenderer() && fromNode->getRenderer()->isSelected());
	bool toNodeIsSelected = (toNode && toNode->hasRenderer() && toNode->getRenderer()->isSelected());
	bool toNodeViaTypeconverterIsSelected = toNode && toNode->hasRenderer() && toNode->getRenderer()->getProxyNode() && toNode->getRenderer()->getProxyNode()->isEffectivelySelected();
	bool toNodeViaAttachmentIsSelected = toNode && toNode->hasRenderer() &&
			dynamic_cast<VuoRendererInputAttachment *>(toNode->getRenderer()) &&
			dynamic_cast<VuoRendererInputAttachment *>(toNode->getRenderer())->getRenderedHostNode() &&
			dynamic_cast<VuoRendererInputAttachment *>(toNode->getRenderer())->getRenderedHostNode()->getRenderer()->isSelected();

	return (fromNodeIsSelected || toNodeIsSelected || toNodeViaTypeconverterIsSelected || toNodeViaAttachmentIsSelected);
}

/**
 * Draws the cable on @c painter.
 */
void VuoRendererCable::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (paintingDisabled())
		return;

	bool hoverHighlightingEnabled = isHovered;
	drawBoundingRect(painter);

	VuoNode::TintColor tintColor = getTintColor();
	VuoRendererColors::SelectionType selectionType =	(isSelected()?					VuoRendererColors::directSelection :
														(isConnectedToSelectedNode()?	VuoRendererColors::indirectSelection :
																						VuoRendererColors::noSelection));

	qint64 timeOfLastActivity = (getRenderActivity()? timeLastEventPropagated : VuoRendererItem::notTrackingActivity);
	VuoRendererColors colors(tintColor,
													  selectionType,
													  (selectionType != VuoRendererColors::noSelection? false : hoverHighlightingEnabled),
													  _eligibilityHighlight,
													  timeOfLastActivity
													  );

	VuoRendererColors yankZoneColors(tintColor,
															  selectionType,
															  true,
															  VuoRendererColors::standardHighlight,
															  timeOfLastActivity);

	QPointF startPoint = getStartPoint();
	QPointF endPoint = getEndPoint();
	bool renderAsIfCableCarriesData = effectivelyCarriesData() && !floatingEndpointAboveEventPort;

	// Etch the highlight out of the main cable.
	QPainterPath outline = getOutline(startPoint, endPoint, renderAsIfCableCarriesData);
	if (outline.isEmpty())
		return;

	// Fill the cable.
	painter->fillPath(outline, QBrush(colors.cableMain()));

	// Highlight the yank zone when hovering.
	if (hoverHighlightingEnabled)
	{
		bool toPortSupportsYanking = getBase()->getToPort() &&
									 getBase()->getToPort()->getRenderer()->supportsDisconnectionByDragging();

		QPainterPath yankZone;
		getYankZonePath(startPoint, endPoint, renderAsIfCableCarriesData, toPortSupportsYanking, yankZone);

		painter->setClipPath(yankZone);
		painter->fillPath(outline, QBrush(yankZoneColors.cableMain()));

		painter->setClipping(false);
	}
}

/**
 * Determines the cable's outline, given the
 * provided @c startPoint, @c endPoint, and @c cableCarriesData attribute values.
 * Retrieves cached versions of the outline, if available.
 */
QPainterPath VuoRendererCable::getOutline(QPointF startPoint,
										  QPointF endPoint,
										  bool cableCarriesData)
{
	if ((qMakePair(startPoint, endPoint) != this->cachedEndpointsForOutlines)
	 || (cableCarriesData != this->cachedCarriesDataValueForOutlines)
	 || portConstantsChangedSinceLastCachedOutline)
	{
		qreal cableWidth;
		getCableSpecs(cableCarriesData, cableWidth);

		QPainterPath cablePath = getCablePathForEndpoints(startPoint, endPoint);

		QPainterPathStroker mainStroker;
		mainStroker.setWidth(cableWidth);
		mainStroker.setCapStyle(Qt::RoundCap);
		QPainterPath mainOutline = mainStroker.createStroke(cablePath);

		this->cachedEndpointsForOutlines = qMakePair(startPoint, endPoint);
		this->cachedCarriesDataValueForOutlines = cableCarriesData;
		this->cachedOutline = mainOutline;
		this->portConstantsChangedSinceLastCachedOutline = false;
	}

	return this->cachedOutline;
}

/**
 * Determines the cable's @c yankZonePath, given the
 * provided @c startPoint, @c endPoint, @c toPortSupportsYanking,
 * and @c cableCarriesData attribute values.
 * Retrieves cached version of the yank zone path, if available.
 */
void VuoRendererCable::getYankZonePath(QPointF startPoint,
										QPointF endPoint,
										bool cableCarriesData,
										bool toPortSupportsYanking,
										QPainterPath &yankZone
										)
{
	if ((qMakePair(startPoint, endPoint) != this->cachedEndpointsForYankZone) ||
		(toPortSupportsYanking != this->cachedToPortSupportsYankingValueForYankZone) ||
		(cableCarriesData != this->cachedCarriesDataValueForYankZone))
	{
		// Calculate the yank zone.
		QPainterPath yankZone;
		if (getBase()->getToPort() && toPortSupportsYanking)
			yankZone.addEllipse(endPoint, VuoRendererCable::cableYankRadius, VuoRendererCable::cableYankRadius);

		this->cachedEndpointsForYankZone = qMakePair(startPoint, endPoint);
		this->cachedCarriesDataValueForYankZone = cableCarriesData;
		this->cachedToPortSupportsYankingValueForYankZone = toPortSupportsYanking;
		this->cachedYankZonePath = yankZone;
	}

	yankZone = this->cachedYankZonePath;
}

/**
 * Calculates the `cableWidth` for a cable, dependent on whether that cable carries data.
 */
void VuoRendererCable::getCableSpecs(bool cableCarriesData, qreal &cableWidth)
{
	const qreal dataCableToEventCableWidthRatio = 4;

	cableWidth = (cableCarriesData? VuoRendererCable::cableWidth :
									 VuoRendererCable::cableWidth/dataCableToEventCableWidthRatio);
}

/**
 * Returns a boolean indicating whether painting is currently disabled for this cable.
 */
bool VuoRendererCable::paintingDisabled() const
{
	VuoCable *cable = getBase();
	VuoNode *fromNode = cable->getFromNode();

	// If the cable is an outgoing cable from a collapsed typecast, disable painting.
	if (fromNode && fromNode->hasRenderer() && fromNode->getRenderer()->getProxyNode())
		return true;

	// If the cable is an outgoing cable from an input port attachment, disable painting.
	if (fromNode && fromNode->hasRenderer() && dynamic_cast<VuoRendererInputAttachment *>(fromNode->getRenderer()))
		return true;

	// If the cable is a published cable and published port sidebars are not currently displayed, disable painting.
	if (isPublishedInputCableWithoutVisiblePublishedPort() || isPublishedOutputCableWithoutVisiblePublishedPort())
		return true;

	// If the cable is an internal wireless cable, disable painting unless the composition is currently
	// in show-hidden-cables mode.
	if (cable->getCompiler()->getHidden() && !getRenderHiddenCables())
		return true;

	return false;
}

/**
 * Returns @c true if this is a published input cable whose connected published
 * input port is not currently displayed (e.g., if published port sidebars are hidden).
 */
bool VuoRendererCable::isPublishedInputCableWithoutVisiblePublishedPort() const
{
	VuoPort *fromPort = getBase()->getFromPort();

	// If the cable is connected to a published input port withn the published input sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (fromPort)
	{
		VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(fromPort->getRenderer());

		if (publishedPort)
		{
			bool proxyPublishedInputSidebarPortHidden = (! publishedPort->isVisible());
			if (proxyPublishedInputSidebarPortHidden)
				return true;
		}
	}

	return false;
}

/**
 * Returns @c true if this is a published output cable whose connected published
 * output port is not currently displayed (e.g., if published port sidebars are hidden).
 */
bool VuoRendererCable::isPublishedOutputCableWithoutVisiblePublishedPort() const
{
	VuoPort *toPort = getBase()->getToPort();

	// If the cable is connected to a published output port within the published output sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (toPort)
	{
		VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(toPort->getRenderer());

		if (publishedPort)
		{
			bool proxyPublishedOutputSidebarPortHidden = (! publishedPort->isVisible());
			if (proxyPublishedOutputSidebarPortHidden)
				return true;
		}
	}

	return false;
}

/**
 * Returns @c true if this cable is to be rendered as if it is wireless.
 */
bool VuoRendererCable::getEffectivelyWireless() const
{
	if (isPublishedInputCableWithoutVisiblePublishedPort() || isPublishedOutputCableWithoutVisiblePublishedPort())
		return true;

	return getBase()->getCompiler()->getHidden();
}

/**
 * Sets the boolean indicating whether this cable has been explicitly marked
 * by the user to be rendered as if it is wireless.
 */
void VuoRendererCable::setWireless(bool wireless)
{
	VuoRendererPort *fromPort = ((getBase()->getFromPort() && getBase()->getFromPort()->hasRenderer())?
									getBase()->getFromPort()->getRenderer() : NULL);
	VuoRendererPort *toPort = ((getBase()->getToPort() && getBase()->getToPort()->hasRenderer())?
									getBase()->getToPort()->getRenderer() : NULL);

	updateGeometry();

	if (fromPort)
		fromPort->updateGeometry();

	if (toPort)
		toPort->updateGeometry();

	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheModeForCableAndConnectedPorts(QGraphicsItem::NoCache);

	getBase()->getCompiler()->setHidden(wireless);

	setCacheModeForCableAndConnectedPorts(normalCacheMode);
}

/**
 * Sets the cache mode of this cable and its connected ports to @c mode.
 */
void VuoRendererCable::setCacheModeForCableAndConnectedPorts(QGraphicsItem::CacheMode mode)
{
	this->setCacheMode(mode);

	VuoRendererPort *fromPort = ((getBase()->getFromPort() && getBase()->getFromPort()->hasRenderer())?
									getBase()->getFromPort()->getRenderer() : NULL);
	VuoRendererPort *toPort = ((getBase()->getToPort() && getBase()->getToPort()->hasRenderer())?
									getBase()->getToPort()->getRenderer() : NULL);

	if (fromPort)
		fromPort->setCacheMode(mode);

	if (toPort)
		toPort->setCacheMode(mode);
}

/**
 * Call this when the constants on this cable's node's other ports have changed,
 * so the cable path gets updated to escape them.
 */
void VuoRendererCable::setPortConstantsChanged()
{
	portConstantsChangedSinceLastCachedOutline = true;
}

/**
 * Returns the coordinates of the cable's floating endpoint, or a NULL
 * QPointF if none.
 */
QPointF VuoRendererCable::getFloatingEndpointLoc()
{
	return floatingEndpointLoc;
}

/**
 * Sets the node and port from which this cable is output, updating the
 * connected renderer and base components.
 */
void VuoRendererCable::setFrom(VuoNode *fromNode, VuoPort *fromPort)
{
	VuoPort *origFromPort = getBase()->getFromPort();

	QGraphicsItem::CacheMode normalCacheMode = cacheMode();

	// Prepare affected ports and cable for geometry changes
	updateGeometry();

	if (origFromPort && origFromPort->hasRenderer())
	{
		origFromPort->getRenderer()->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
		origFromPort->getRenderer()->updateGeometry();
	}

	if (fromPort && fromPort->hasRenderer())
	{
		fromPort->getRenderer()->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
		fromPort->getRenderer()->updateGeometry();
	}

	getBase()->setFrom(fromNode, fromPort);

	if (origFromPort && origFromPort->hasRenderer())
		origFromPort->getRenderer()->setCacheModeForPortAndChildren(normalCacheMode);

	if (fromPort && fromPort->hasRenderer())
		fromPort->getRenderer()->setCacheModeForPortAndChildren(normalCacheMode);
}

/**
 * Sets the node and port to which this cable is input, updating the
 * connected renderer and base components.
 */
void VuoRendererCable::setTo(VuoNode *toNode, VuoPort *toPort)
{
	VuoPort *origToPort = getBase()->getToPort();

	QGraphicsItem::CacheMode normalCacheMode = cacheMode();

	// Prepare affected ports and cable for geometry changes
	updateGeometry();

	if (origToPort && origToPort->hasRenderer())
	{
		origToPort->getRenderer()->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
		origToPort->getRenderer()->updateGeometry();
	}

	if (toPort && toPort->hasRenderer())
	{
		toPort->getRenderer()->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
		toPort->getRenderer()->updateGeometry();
	}

	getBase()->setTo(toNode, toPort);

	// Re-lay out any drawers connected to the original and updated "To" nodes.
	if (origToPort && origToPort->hasRenderer())
	{
		origToPort->getRenderer()->updateGeometry();

		if (origToPort->getRenderer()->getRenderedParentNode())
			origToPort->getRenderer()->getRenderedParentNode()->layoutConnectedInputDrawersAtAndAbovePort(origToPort->getRenderer());

		origToPort->getRenderer()->setCacheModeForPortAndChildren(normalCacheMode);
	}

	if (toPort && toPort->hasRenderer())
	{
		toPort->getRenderer()->updateGeometry();

		if (toPort->getRenderer()->getRenderedParentNode())
			toPort->getRenderer()->getRenderedParentNode()->layoutConnectedInputDrawersAtAndAbovePort(toPort->getRenderer());

		toPort->getRenderer()->setCacheModeForPortAndChildren(normalCacheMode);
	}
}

/**
 * Sets the coordinates of the cable's floating endpoint.
 */
void VuoRendererCable::setFloatingEndpointLoc(QPointF loc)
{
	floatingEndpointLoc = loc;
}

/**
 * Sets the boolean indicating whether this cable's floating endpoint is currently
 * hovered over an event-only port.
 */
void VuoRendererCable::setFloatingEndpointAboveEventPort(bool aboveEventPort)
{
	floatingEndpointAboveEventPort = aboveEventPort;
}

/**
 * Returns true if this cable's floating endpoint is currently hovered over an event-only port.
 */
bool VuoRendererCable::isFloatingEndpointAboveEventPort(void)
{
	return floatingEndpointAboveEventPort;
}

/**
 * Removes a cable from the canvas and performs other necessary cleanup.
 */
void VuoRendererCable::removeFromScene()
{
	if (scene())
	{
		this->prepareGeometryChange();

		VuoPort *rcFromPort = this->getBase()->getFromPort();
		VuoPort *rcToPort = this->getBase()->getToPort();

		if (rcFromPort && rcFromPort->hasRenderer())
		{
			rcFromPort->getRenderer()->updateGeometry();
		}

		if (rcToPort && rcToPort->hasRenderer())
		{
			rcToPort->getRenderer()->updateGeometry();
		}

		scene()->removeItem(this);
	}
}

/**
 * Handle mouse hover start events generated by custom code making use of an extended hover range.
 */
void VuoRendererCable::extendedHoverEnterEvent()
{
	extendedHoverMoveEvent();
}

/**
 * Handle mouse hover move events generated by custom code making use of an extended hover range.
 */
void VuoRendererCable::extendedHoverMoveEvent()
{
	setHovered(true);
}

/**
 * Handle mouse hover leave events generated by custom code making use of an extended hover range.
 */
void VuoRendererCable::extendedHoverLeaveEvent()
{
	setHovered(false);
}

/**
 * Returns a boolean indicating whether the cable may be disconnected by
 * by dragging from @c scenePos.
 */
bool VuoRendererCable::yankZoneIncludes(QPointF scenePos)
{
	qreal yankDistance = QLineF(scenePos, getEndPoint()).length();
	return (yankDistance <= cableYankRadius);
}

/**
 * Returns the cable's previous 'To' port (if the cable has since been
 * disconnected from that port by dragging), or NULL if not applicable.
 */
VuoPort * VuoRendererCable::getFloatingEndpointPreviousToPort()
{
	return floatingEndpointPreviousToPort;
}

/**
 * Sets the cable's previous 'To' port (if the cable has since been
 * disconnected from that port by dragging), or NULL if not applicable.
 */
void VuoRendererCable::setFloatingEndpointPreviousToPort(VuoPort *port)
{
	this->floatingEndpointPreviousToPort = port;
}

/**
 * Returns the boolean indicating whether the cable was in forced event-only
 * mode before it was most recently disconnected by dragging.
 */
bool VuoRendererCable::getPreviouslyAlwaysEventOnly()
{
	return this->previouslyAlwaysEventOnly;
}

/**
 * Sets the boolean indicating whether the cable was in forced event-only
 * mode before it was most recently disconnected by dragging.
 */
void VuoRendererCable::setPreviouslyAlwaysEventOnly(bool eventOnly)
{
	this->previouslyAlwaysEventOnly = eventOnly;
}

/**
 * Sets the boolean indicating that the cable is tinted to represent mouse hovering.
 */
void VuoRendererCable::setHovered(bool hovered)
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);

	prepareGeometryChange();

	this->isHovered = hovered;

	setCacheMode(normalCacheMode);
}

/**
 * Sets whether this cable is currently being dragged.
 */
void VuoRendererCable::setEligibilityHighlight(VuoRendererColors::HighlightType eligibility)
{
	_eligibilityHighlight = eligibility;
}

/**
 * Schedules a redraw of this cable.
 */
void VuoRendererCable::updateGeometry(void)
{
	this->prepareGeometryChange();
}

/**
 * Resets the time that the last event was propagated through this cable to a value
 * that causes the cable to be painted as if activity-rendering were disabled.
 */
void VuoRendererCable::resetTimeLastEventPropagated()
{
	this->timeLastEventPropagated = VuoRendererColors::getVirtualPropagatedEventOrigin();
}

/**
 * Returns a boolean indicating whether this cable should be rendered as if it carries data.
 */
bool VuoRendererCable::effectivelyCarriesData(void) const
{
	return (getBase()->hasCompiler() ?
				getBase()->getCompiler()->carriesData() :
				false);
}
