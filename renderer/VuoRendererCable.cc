/**
 * @file
 * VuoRendererCable implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererCable.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererMakeListNode.hh"
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
	if (getBase()->getFromNode() && getBase()->getFromNode()->hasRenderer() && getBase()->getFromNode()->getRenderer()->getProxyNode())
		return QRectF();

	QPainterPath cablePath = getCablePath();
	QRectF r = cablePath.controlPointRect();

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

	QPainterPathStroker s;
	s.setWidth(cableWidth);
	return s.createStroke(getCablePath());
}

/**
 * Returns the cable's starting point
 */
QPointF VuoRendererCable::getStartPoint(void) const
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoPort *fromPort = getBase()->getFromPort();

	if (fromNode && fromPort && fromPort->hasRenderer())
	{
		VuoRendererPublishedPort *sidebarPort = fromPort->getRenderer()->getProxyPublishedSidebarPort();
		if (sidebarPort)
		{
			QList<QGraphicsView *> views = scene()->views();
			return (views.empty()? QPointF(0,0) : views[0]->mapToScene(views[0]->mapFromGlobal(sidebarPort->getGlobalPos().toPoint())));
		}

		else if (fromNode->hasRenderer())
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
		VuoRendererPublishedPort *sidebarPort = toPort->getRenderer()->getProxyPublishedSidebarPort();
		if (sidebarPort)
		{
			QList<QGraphicsView *> views = scene()->views();
			return (views.empty()? QPointF(0,0) : views[0]->mapToScene(views[0]->mapFromGlobal(sidebarPort->getGlobalPos().toPoint())));
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

QPainterPath VuoRendererCable::getCablePath(void) const
{
	QPointF from = getStartPoint();
	QPointF to = getEndPoint();

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
 * If the nodes on both sides of the cable have the same tint color, tint the cable with that color, too.
 */
VuoNode::TintColor VuoRendererCable::getTintColor(void)
{
	VuoNode *fromNode = getBase()->getFromNode();
	VuoNode *toNode = getBase()->getToNode();
	if (fromNode && toNode)
	{
		while (toNode->hasRenderer() && toNode->getRenderer()->getProxyNode())
			toNode = toNode->getRenderer()->getProxyNode()->getBase();

		if (fromNode->getTintColor() == toNode->getTintColor())
			return fromNode->getTintColor();
	}

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
	bool toNodeViaDrawerIsSelected = toNode && toNode->hasRenderer() &&
			dynamic_cast<VuoRendererMakeListNode *>(toNode->getRenderer()) &&
			((VuoRendererMakeListNode *)(toNode->getRenderer()))->getHostInputPort() &&
			((VuoRendererMakeListNode *)(toNode->getRenderer()))->getHostInputPort()->getRenderedParentNode() &&
			((VuoRendererMakeListNode *)(toNode->getRenderer()))->getHostInputPort()->getRenderedParentNode()->isSelected();

	return (fromNodeIsSelected || toNodeIsSelected || toNodeViaTypeconverterIsSelected || toNodeViaDrawerIsSelected);
}

/**
 * Draws the cable on @c painter.
 */
void VuoRendererCable::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (paintingDisabled())
		return;

	bool hoverHighlightingEnabled = (isHovered && (! getBase()->isPublishedCable()));

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

	QPainterPath cablePath = getCablePath();

	qreal cableWidth = this->cableWidth;
	qreal cableHighlightWidth = this->cableHighlightWidth;
	qreal cableHighlightOffset = this->cableHighlightOffset;
	// Render event-only cables half-size.
	if (!getBase()->getCompiler()->carriesData())
	{
		cableWidth /= 2.;
		cableHighlightWidth /= 2.;
		cableHighlightOffset /= 2.;
	}

	// Convert curve into line segments.
	QList<QPolygonF> cablePolygons = cablePath.toSubpathPolygons();
	if (cablePolygons.length() != 1)
		return;
	QPolygonF flattenedPath = cablePolygons[0];

	QPainterPath upperLines;
	for (int i=1; i<flattenedPath.count(); ++i)
	{
		// For each line segment...
		QLineF l = QLineF(flattenedPath[i-1], flattenedPath[i]);

		// ...make a parallel line segment, displaced along the normal.
		QLineF normal = l.normalVector();
		normal.setLength(cableHighlightOffset);
		if (i==1)
			upperLines.moveTo(normal.p2());

		upperLines.lineTo(QPointF(l.dx(), l.dy()) + normal.p2());
	}

	// Turn each series of line segments into a thick outline.
	QPainterPathStroker upperStroker;
	upperStroker.setWidth(cableHighlightWidth);
	upperStroker.setCapStyle(Qt::RoundCap);
	QPainterPath upperOutline = upperStroker.createStroke(upperLines);

	QPainterPathStroker mainStroker;
	mainStroker.setWidth(cableWidth);
	mainStroker.setCapStyle(Qt::RoundCap);
	QPainterPath mainOutline = mainStroker.createStroke(cablePath);

	// Etch the highlight out of the main cable.
	QPainterPath mainOutlineMinusHighlight = mainOutline.subtracted(upperOutline);

	// Highlight the yank zone when hovering.
	if (hoverHighlightingEnabled)
	{
		QPainterPath yankZone;
		if (getBase()->getFromPort() && getBase()->getFromPort()->getRenderer()->supportsDisconnectionByDragging())
			yankZone.addEllipse(QPointF(getStartPoint()), cableYankRadius, cableYankRadius);
		if (getBase()->getToPort() && getBase()->getToPort()->getRenderer()->supportsDisconnectionByDragging())
			yankZone.addEllipse(QPointF(getEndPoint()), cableYankRadius, cableYankRadius);

		painter->setClipPath(yankZone);
		painter->fillPath(mainOutlineMinusHighlight, QBrush(yankZoneColors->cableUpper()));
		painter->fillPath(upperOutline, QBrush(yankZoneColors->cableMain()));

		QPainterPath invertedYankZone;
		invertedYankZone.addRect(cablePath.controlPointRect().adjusted(-cableWidth,-cableWidth,cableWidth,cableWidth));
		invertedYankZone = invertedYankZone.subtracted(yankZone);

		// When drawing the main cable below, don't overdraw the yank zone.
		painter->setClipPath(invertedYankZone);
	}

	// Fill the cable.
	painter->fillPath(mainOutlineMinusHighlight, QBrush(colors->cableMain()));
	painter->fillPath(upperOutline, QBrush(colors->cableUpper()));

	if (hoverHighlightingEnabled)
		painter->setClipping(false);

	delete colors;
}

/**
 * Returns a boolean indicating whether painting is currently disabled for this cable.
 */
bool VuoRendererCable::paintingDisabled() const
{
	// If the cable is an outgoing cable from a collapsed typecast, disable painting.
	if (getBase()->getFromNode() && getBase()->getFromNode()->hasRenderer() && getBase()->getFromNode()->getRenderer()->getProxyNode())
		return true;

	// If the cable is an outgoing cable from a collapsed "Make List" node, disable painting.
	if (getBase()->getFromNode() && getBase()->getFromNode()->hasRenderer() && dynamic_cast<VuoRendererMakeListNode *>(getBase()->getFromNode()->getRenderer()))
		return true;

	// If the cable is connected to a published output port withn the published output sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (getBase()->getFromPort())
	{
		VuoRendererPublishedPort *proxyPublishedInputSidebarPort = getBase()->getFromPort()->getRenderer()->getProxyPublishedSidebarPort();

		if (proxyPublishedInputSidebarPort)
		{
			bool proxyPublishedInputSidebarPortHidden = (! proxyPublishedInputSidebarPort->isVisible());
			if (proxyPublishedInputSidebarPortHidden)
				return true;
		}
	}

	// If the cable is connected to a published input port within the published input sidebar,
	// but the sidebar is not currently displayed, disable painting.
	if (getBase()->getToPort())
	{
		VuoRendererPublishedPort *proxyPublishedOutputSidebarPort = getBase()->getToPort()->getRenderer()->getProxyPublishedSidebarPort();

		if (proxyPublishedOutputSidebarPort)
		{
			bool proxyPublishedOutputSidebarPortHidden = (! proxyPublishedOutputSidebarPort->isVisible());
			if (proxyPublishedOutputSidebarPortHidden)
				return true;
		}
	}

	return false;
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
 * Sets the coordinates of the cable's floating endpoint.
 */
void VuoRendererCable::setFloatingEndpointLoc(QPointF loc)
{
	floatingEndpointLoc = loc;
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
		this->getBase()->setFrom(NULL, NULL);
		this->getBase()->setTo(NULL, NULL);

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
	prepareGeometryChange();
	isHovered = true;
}

/**
 * Handle mouse hover leave events generated by custom code making use of an extended hover range.
 */
void VuoRendererCable::extendedHoverLeaveEvent()
{
	prepareGeometryChange();
	isHovered = false;
}

/**
 * Returns a boolean indicating whether the cable may be disconnected by
 * by dragging from @c scenePos.
 */
bool VuoRendererCable::yankZoneIncludes(QPointF scenePos)
{
	if (getBase()->isPublishedCable())
		return false;

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
	this->isHovered = hovered;
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
