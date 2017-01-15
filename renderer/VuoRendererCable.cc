/**
 * @file
 * VuoRendererCable implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererCable.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerCable.hh"


/// The width of a data-event cable.  (Event-only cables are half this width.)
const qreal VuoRendererCable::cableWidth = VuoRendererFonts::thickPenWidth / 5.;
/// The width of the highlight on a data-event cable.
const qreal VuoRendererCable::cableHighlightWidth = VuoRendererCable::cableWidth / 3.;
/// How far from the center of the data-event cable the highlight should be rendered.
const qreal VuoRendererCable::cableHighlightOffset = VuoRendererCable::cableHighlightWidth * 5./8.;
/// Radius from the endpoints of the cable in which yanking is permitted.
const qreal VuoRendererCable::cableYankRadius = VuoRendererFonts::thickPenWidth;

/**
 * Creates a renderer detail for the specified @c baseCable.
 */
VuoRendererCable::VuoRendererCable(VuoCable *baseCable)
	: VuoBaseDetail<VuoCable>("VuoRendererCable", baseCable)
{
	getBase()->setRenderer(this);

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
	floatingEndpointAboveEventPort = false;
	isHovered = false;

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
			return fromNode->getRenderer()->pos() + fromPort->getRenderer()->pos();
	}

	return floatingEndpointLoc;
}

/**
 * Returns the cable's ending point
 */
QPointF VuoRendererCable::getEndPoint(void) const
{
	VuoNode *toNode = getBase()->getToNode();
	VuoPort *toPort = getBase()->getToPort();

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
			VuoRendererNode * toNodeProxy = getBase()->getToNode()->getRenderer()->getProxyNode();
			if (toNodeProxy)
				return toNodeProxy->pos() + getBase()->getToPort()->getRenderer()->pos();
			else
				return getBase()->getToNode()->getRenderer()->pos() + getBase()->getToPort()->getRenderer()->pos();
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
 * Returns a cable's path, given its start and end points.
 */
QPainterPath VuoRendererCable::getCablePathForEndpoints(QPointF from, QPointF to)
{
	QPainterPath cablePath(from);

	// Cables going left to right are straight lines.
	// Cables going right to left have biased control points and are thus rendered curved.
	// Curve opposite direction of cable, to try to avoid overdrawing the connected nodes.
	QPointF bias = QPointF(
				fmin(240.,fmax(4.,from.x()-to.x()+16.)),
				fmin(240.,100.*fmax(0.,from.x()-to.x())/fmax(4.,fabs(from.y()-to.y()))) * (to.y()>from.y() ? 1. : -1.)
				);
	// Limit bias for very short cables.
	bias *= fmin(1., (to-from).manhattanLength() / (VuoRendererFonts::thickPenWidth*16.));

	cablePath.cubicTo(
		// Slight upward bias on output
		from + bias,
		// Slight downward bias on input
		to   - bias,
		to
	);
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
 * either directly or by way of a collapsed typecast.
 */
bool VuoRendererCable::isConnectedToSelectedNode(void)
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoNode *toNode = getBase()->getToNode();

	bool fromNodeIsSelected = (fromNode && fromNode->hasRenderer() && fromNode->getRenderer()->isSelected());
	bool toNodeIsSelected = (toNode && toNode->hasRenderer() && toNode->getRenderer()->isSelected());
	bool toNodeViaTypeconverterIsSelected = toNode && toNode->hasRenderer() && toNode->getRenderer()->getProxyNode() && toNode->getRenderer()->getProxyNode()->isSelected();
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
	VuoRendererColors *colors = new VuoRendererColors(tintColor,
													  selectionType,
													  (selectionType != VuoRendererColors::noSelection? false : hoverHighlightingEnabled),
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity
													  );

	VuoRendererColors *yankZoneColors = new VuoRendererColors(tintColor,
															  VuoRendererColors::noSelection,
															  false,
															  VuoRendererColors::standardHighlight,
															  timeOfLastActivity);

	QPointF startPoint = getStartPoint();
	QPointF endPoint = getEndPoint();
	bool renderAsIfCableCarriesData = effectivelyCarriesData() && !floatingEndpointAboveEventPort;

	// Etch the highlight out of the main cable.
	QPainterPath outline = getOutline(startPoint, endPoint, renderAsIfCableCarriesData);
	if (outline.isEmpty())
		return;

	// Highlight the yank zone when hovering.
	if (hoverHighlightingEnabled)
	{
		bool toPortSupportsYanking = getBase()->getToPort()->getRenderer()->supportsDisconnectionByDragging();

		QPainterPath yankZone, invertedYankZone;
		getYankZonePaths(startPoint, endPoint, renderAsIfCableCarriesData, toPortSupportsYanking, yankZone, invertedYankZone);

		painter->setClipPath(yankZone);
		painter->fillPath(outline, QBrush(yankZoneColors->cableUpper()));

		// When drawing the main cable below, don't overdraw the yank zone.
		painter->setClipPath(invertedYankZone);
	}

	// Fill the cable.
	painter->fillPath(outline, QBrush(colors->cableMain()));

	if (hoverHighlightingEnabled)
		painter->setClipping(false);

	delete colors;
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
	if ((qMakePair(startPoint, endPoint) != this->cachedEndpointsForOutlines) ||
		(cableCarriesData != this->cachedCarriesDataValueForOutlines))
	{
		qreal cableWidth, cableHighlightWidth, cableHighlightOffset;
		getCableSpecs(cableCarriesData, cableWidth, cableHighlightWidth, cableHighlightOffset);

		QPainterPath cablePath = getCablePathForEndpoints(startPoint, endPoint);

		QPainterPathStroker mainStroker;
		mainStroker.setWidth(cableWidth);
		mainStroker.setCapStyle(Qt::RoundCap);
		QPainterPath mainOutline = mainStroker.createStroke(cablePath);

		this->cachedEndpointsForOutlines = qMakePair(startPoint, endPoint);
		this->cachedCarriesDataValueForOutlines = cableCarriesData;
		this->cachedOutline = mainOutline;
	}

	return this->cachedOutline;
}

/**
 * Determines the cable's @c yankZonePath and @c invertedYankZonePath, given the
 * provided @c startPoint, @c endPoint, @c toPortSupportsYanking,
 * and @c cableCarriesData attribute values.
 * Retrieves cached versions of the yank zone paths, if available.
 */
void VuoRendererCable::getYankZonePaths(QPointF startPoint,
										QPointF endPoint,
										bool cableCarriesData,
										bool toPortSupportsYanking,
										QPainterPath &yankZone,
										QPainterPath &invertedYankZone
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

		// Calculate the inverted yank zone.
		QPainterPath invertedYankZone;

		qreal cableWidth, cableHighlightWidth, cableHighlightOffset;
		getCableSpecs(cableCarriesData, cableWidth, cableHighlightWidth, cableHighlightOffset);

		QPainterPath cablePath = getCablePathForEndpoints(startPoint, endPoint);
		invertedYankZone.addRect(cablePath.controlPointRect().adjusted(-cableWidth, -cableWidth ,cableWidth, cableWidth));

		this->cachedEndpointsForYankZone = qMakePair(startPoint, endPoint);
		this->cachedCarriesDataValueForYankZone = cableCarriesData;
		this->cachedToPortSupportsYankingValueForYankZone = toPortSupportsYanking;
		this->cachedYankZonePaths = qMakePair(yankZone, invertedYankZone);
	}

	yankZone = this->cachedYankZonePaths.first;
	invertedYankZone = this->cachedYankZonePaths.second;
}

/**
 * Calculates the @c cableWidth, @c cableHighlightWidth, and @c cableHighlightOffset,
 * for a cable, dependent on whether that cable carries data.
 */
void VuoRendererCable::getCableSpecs(bool cableCarriesData,
										   qreal &cableWidth,
										   qreal &cableHighlightWidth,
										   qreal &cableHighlightOffset)
{
	const qreal dataCableToEventCableWidthRatio = 2.5;

	cableWidth = (cableCarriesData? VuoRendererCable::cableWidth :
									 VuoRendererCable::cableWidth/dataCableToEventCableWidthRatio);

	cableHighlightWidth = (cableCarriesData? VuoRendererCable::cableHighlightWidth :
											  VuoRendererCable::cableHighlightWidth/dataCableToEventCableWidthRatio);

	cableHighlightOffset = (cableCarriesData? VuoRendererCable::cableHighlightOffset :
											   VuoRendererCable::cableHighlightOffset/dataCableToEventCableWidthRatio);
}

/**
 * Returns a boolean indicating whether painting is currently disabled for this cable.
 */
bool VuoRendererCable::paintingDisabled() const
{
	// If the cable is an outgoing cable from a collapsed typecast, disable painting.
	if (getBase()->getFromNode() && getBase()->getFromNode()->hasRenderer() && getBase()->getFromNode()->getRenderer()->getProxyNode())
		return true;

	// If the cable is an outgoing cable from an input port attachment, disable painting.
	if (getBase()->getFromNode() && getBase()->getFromNode()->hasRenderer() && dynamic_cast<VuoRendererInputAttachment *>(getBase()->getFromNode()->getRenderer()))
		return true;

	// If the cable is a published cable and published port sidebars are not currently displayed, disable painting.
	if (isPublishedInputCableWithoutVisiblePublishedPort() || isPublishedOutputCableWithoutVisiblePublishedPort())
		return true;

	// If the cable is an internal wireless cable, disable painting unless the composition is currently
	// in show-hidden-cables mode.
	if (getBase()->getCompiler()->getHidden() && !getRenderHiddenCables())
		return true;

	return false;
}

/**
 * Returns @c true if this is a published input cable whose connected published
 * input port is not currently displayed (e.g., if published port sidebars are hidden).
 */
bool VuoRendererCable::isPublishedInputCableWithoutVisiblePublishedPort() const
{
	// If the cable is connected to a published input port withn the published input sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (getBase()->getFromPort())
	{
		VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(getBase()->getFromPort()->getRenderer());

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
	// If the cable is connected to a published output port within the published output sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (getBase()->getToPort())
	{
		VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(getBase()->getToPort()->getRenderer());

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
 *
 * Note that the returned value may conflict with that returned by VuoCompilerCable::carriesData()
 * for cables connected to published ports.  This is because the vuoPsuedoPorts associated
 * with, e.g., published input ports are always technically event-only trigger ports even if the
 * published ports themselves have data.  This method treats the vuoPseudoPort as if it
 * has the same data type as its associated VuoPublishedPort, whereas VuoCompilerCable::carriesData()
 * uses the vuoPseudoPort's technical data type.
 */
bool VuoRendererCable::effectivelyCarriesData(void) const
{
	if (getBase()->getCompiler()->getAlwaysEventOnly())
		return false;

	VuoRendererPort *fromPort = (getBase()->getFromPort()? getBase()->getFromPort()->getRenderer() : NULL);
	VuoRendererPort *toPort = (getBase()->getToPort()? getBase()->getToPort()->getRenderer() : NULL);

	VuoType *fromPortDataType = (fromPort? fromPort->getDataType() : NULL);
	VuoType *toPortDataType  = (toPort? toPort->getDataType() : NULL);

	bool cableCarriesData = false;
	if (fromPort && !toPort)
		cableCarriesData = fromPortDataType;
	else if (toPort && !fromPort)
		cableCarriesData = toPortDataType;
	else if (fromPort && toPort)
		cableCarriesData = (fromPortDataType && toPortDataType);

	return cableCarriesData;
}
