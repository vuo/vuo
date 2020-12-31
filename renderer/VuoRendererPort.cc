/**
 * @file
 * VuoRendererPort implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoStringUtilities.hh"

#include "VuoRendererComposition.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererTypecastPort.hh"

#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPortClass.hh"

#include "VuoGenericType.hh"
#include "VuoNodeClass.hh"

extern "C" {
#include "VuoAnchor.h"
#include "VuoAudioInputDevice.h"
#include "VuoAudioOutputDevice.h"
#include "VuoImage.h"
#include "VuoIntegerRange.h"
#include "VuoRange.h"
#include "VuoScreen.h"
#include "VuoTransform.h"
#include "VuoUrl.h"
#include "VuoSpeechVoice.h"

char *VuoHid_getUsageText(uint32_t usagePage, uint32_t usage);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
	#include <json-c/json.h>
#pragma clang diagnostic pop


#include "VuoHeap.h"

const qreal VuoRendererPort::portRadius              = 8;         // VuoRendererFonts::thickPenWidth*8./20.
const qreal VuoRendererPort::portSpacing             = 15;        // VuoRendererFonts::thickPenWidth*3.0/4.0
const qreal VuoRendererPort::portContainerMargin     = 3.333333;  // VuoRendererFonts::thickPenWidth / 6.
const qreal VuoRendererPort::portInset               = 1.4;
const qreal VuoRendererPort::portBarrierWidth        = 5.5;  // VuoRendererFonts::thickPenWidth*5.5/20.
const qreal VuoRendererPort::portConstantTextPadding = 6.5;  // VuoRendererFonts::thickPenWidth*6.5/20.

/**
 * Creates a renderer detail for the specified base port.
 */
VuoRendererPort::VuoRendererPort(VuoPort * basePort, VuoRendererSignaler *signaler,
								 bool isOutput, bool isRefreshPort, bool isFunctionPort)
	: VuoBaseDetail<VuoPort>("VuoRendererPort standard", basePort)
{
	getBase()->setRenderer(this);

	setZValue(portZValue);

	this->signaler = signaler;

	this->isOutput = isOutput;
	this->isRefreshPort = isRefreshPort;
	this->isFunctionPort = isFunctionPort;
	this->_eligibilityHighlight = VuoRendererColors::noHighlight;
	this->isEligibleForSelection = false;
	setAnimated(false);
	this->typecastParentPort = NULL;
	this->customizedPortName = getDefaultPortNameToRender();

	resetTimeLastEventFired();

	const int maxAnimationsPerPort = 4;

	if (getInput() || (getBase()->getClass()->getPortType() == VuoPortClass::triggerPort))
	{
		for (int i = 0; i < maxAnimationsPerPort; ++i)
		{
			QGraphicsItemAnimation *animation = new QGraphicsItemAnimation();
			animation->setTimeLine(new QTimeLine(VuoRendererColors::activityAnimationFadeDuration));
			animations.push_back(animation);
		}
	}

	if (! isHiddenRefreshPort())
	{
		setFlag(QGraphicsItem::ItemIsFocusable, true);  // allow delivery of key events
		setAcceptHoverEvents(true);  // allow delivery of mouse-hover events
	}

	updateNameRect();
	updateCachedPortPath();
	updateCachedBoundingRect();
	updateEnabledStatus();

	if (getConstantAsStringToRender() != getConstantAsTruncatedStringToRender())
		setToolTip(QString("<span></span>") + getConstantAsStringToRender().c_str());
}

/**
 * Inscribes an equilateral triangle in the circle described by `center` and `radius`,
 * and rounds its 3 corners.
 */
void VuoRendererPort::addRoundedTriangle(QPainterPath &p, QPointF center, qreal radius, qreal cornerRadius)
{
	p.moveTo(center + QPointF(radius,0));
	for (int theta = 0; theta <= 360; theta += 120)
	{
		QRectF rect(center.x() + (radius - cornerRadius)*cos(theta*M_PI/180.) - cornerRadius,
					center.y() - (radius - cornerRadius)*sin(theta*M_PI/180.) - cornerRadius,
					cornerRadius*2,
					cornerRadius*2);
		bool first = theta==0;
		bool last = theta==360;

		p.arcTo(rect,
				theta - (first ? 0 : 60),
				(first||last ? 60 : 120));
	}
}

/**
 * Returns a closed path representing the port's circle/triangle and its constant value.
 */
QPainterPath VuoRendererPort::getPortPath() const
{
	return cachedPortPath;
}

/**
 * Recalculates `cachedPortPath`.
 */
void VuoRendererPort::updateCachedPortPath()
{
	if (isHiddenRefreshPort())
	{
		cachedPortPath = QPainterPath();
		return;
	}

	cachedPortPath = getPortPath(VuoRendererPort::portInset,
					   getBase()->getClass()->getPortType(),
					   isConstant() ? QString::fromUtf8(getConstantAsTruncatedStringToRender().c_str()) : "",
					   getInput(),
					   carriesData()
					   );
}

/**
 * Returns a path representing the circle/triangle and its constant value,
 * for a port with type `portType`.
 */
QPainterPath VuoRendererPort::getPortPath(qreal inset,
										  VuoPortClass::PortType portType,
										  QString constantText,
										  bool isInputPort,
										  bool carriesData
										 )
{
	QPainterPath p;
	QRectF outerPortRect = getPortRect();
	QRectF innerPortRect = outerPortRect.adjusted(inset,inset,-inset,-inset);

	QRectF textRect = getPortConstantTextRectForText(constantText);

	qreal left = textRect.x() - portConstantTextPadding + inset - VuoRendererPort::portInset;
	QPointF     topLeftCorner(left                  + .5, innerPortRect.top()    - .24);
	QPointF    topRightCorner(innerPortRect.right() + .5, innerPortRect.top()    - .24);
	QPointF bottomRightCorner(innerPortRect.right() + .5, innerPortRect.bottom() - .43);
	QPointF  bottomLeftCorner(left                  + .5, innerPortRect.bottom() - .43);

	p.moveTo(innerPortRect.right() + .5, innerPortRect.center().y());
	qreal adjustedPortRadius = portRadius - 2;
	addRoundedCorner(p, true, bottomRightCorner, adjustedPortRadius, false, false);
	addRoundedCorner(p, true,  bottomLeftCorner, adjustedPortRadius, false, true);
	addRoundedCorner(p, true,     topLeftCorner, adjustedPortRadius, true, true);
	addRoundedCorner(p, true,    topRightCorner, adjustedPortRadius, true, false);

	return p;
}

/**
 * Returns a rectangle encompassing the port's circle.
 */
QRectF VuoRendererPort::getPortRect(void)
{
	return QRectF(
		-portRadius,
		-portRadius,
		portRadius*2.0,
		portRadius*2.0
	);
}

/**
 * Returns a rectangle encompassing the port's event barrier.
 */
QRectF VuoRendererPort::getEventBarrierRect(void) const
{
	QRectF barrierRect = QRectF();

	bool sidebarPaintMode = dynamic_cast<const VuoRendererPublishedPort *>(this);
	VuoPortClass::PortType type = getBase()->getClass()->getPortType();
	VuoPortClass::EventBlocking eventBlocking = getBase()->getClass()->getEventBlocking();

	if (!isAnimated &&
			((!isOutput && !sidebarPaintMode && eventBlocking != VuoPortClass::EventBlocking_None)
			|| (isOutput && type == VuoPortClass::triggerPort))
		)
	{
		QRectF portRect = getPortRect();
		if (isOutput)
			barrierRect = QRectF(portRect.topLeft()  + QPointF( 2 - VuoRendererPort::portBarrierWidth, 2), portRect.bottomLeft()  + QPointF( 2, -3));
		else
			barrierRect = QRectF(portRect.topRight() + QPointF(-1 + VuoRendererPort::portBarrierWidth, 2), portRect.bottomRight() + QPointF(-1, -3));
	}

	return barrierRect;
}

/**
 * Returns the path of the antenna that represents any connected wireless cables, or an empty path if not applicable.
 */
QPainterPath VuoRendererPort::getWirelessAntennaPath() const
{
	bool paintDataAntenna = hasConnectedWirelessDataCable(true);
	bool paintEventAntenna = hasConnectedWirelessEventCable(true);
	if (!paintDataAntenna && !paintEventAntenna)
		return QPainterPath();

	// Mast
	qreal cableWidth;
	VuoRendererCable::getCableSpecs(paintDataAntenna, cableWidth);

	const qreal constantWidth = fmax(0, getPortConstantTextRect().width() - 3);
	const qreal mastLength = portRadius * 2.;
	const qreal pixelOffset = -.3;
	QPointF startPoint = (getInput()? -QPointF(mastLength - (paintDataAntenna ? 0.5 : 0 ) + constantWidth, -pixelOffset) : QPointF(0, pixelOffset));
	QPointF endPoint = (getInput()? QPointF(-constantWidth, pixelOffset) : QPointF(mastLength + (paintDataAntenna ? 0.5 : 0 ), pixelOffset));

	VuoCable cableBase(NULL, NULL, NULL, NULL);
	VuoRendererCable cableRenderer(&cableBase);
	QPainterPath mastPath = cableRenderer.getCablePathForEndpoints(startPoint, endPoint);

	QPainterPathStroker mastStroker;
	mastStroker.setWidth(cableWidth);
	mastStroker.setCapStyle(Qt::RoundCap);
	QPainterPath antennaOutline = mastStroker.createStroke(mastPath);

	// Crossbars
	qreal outerCrossbarXOffset = (paintDataAntenna? 1 : 0.5);
	const QPointF outerCrossbarPos = (getInput()? startPoint - QPointF(outerCrossbarXOffset, 0) :
												  endPoint + QPointF(outerCrossbarXOffset, 0));
	const int crossbarSpacing = 5;
	const int crossbarHeight = VuoRendererFonts::midPenWidth*5;

	QPainterPath crossbars;
	for (int i = 0; i < 2; ++i)
	{
		crossbars.moveTo((outerCrossbarPos +
						 QPointF(QPointF(0, -0.5*crossbarHeight) +
						 QPointF((getInput()? 1 : -1)*crossbarSpacing*i, 0))));
		crossbars.lineTo((outerCrossbarPos +
						 QPointF(QPointF(0, 0.5*crossbarHeight) +
						 QPointF((getInput()? 1 : -1)*crossbarSpacing*i, 0))));
	}

	// Union the mast and crossbars.
	QPainterPathStroker crossbarStroker;
	crossbarStroker.setWidth(cableWidth*3/5);
	crossbarStroker.setCapStyle(Qt::RoundCap);
	antennaOutline += crossbarStroker.createStroke(crossbars);

	return antennaOutline;
}

/**
 * Returns true if this port has a connected data+event cable that should currently be rendered wirelessly.
 */
bool VuoRendererPort::hasConnectedWirelessDataCable(bool includePublishedCables) const
{
	vector<VuoCable *> connectedCables = getBase()->getConnectedCables(includePublishedCables);
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		if ((*cable)->hasRenderer() && (*cable)->getRenderer()->effectivelyCarriesData() &&
				(*cable)->getRenderer()->getEffectivelyWireless() &&
				(*cable)->getRenderer()->paintingDisabled())
			return true;
	return false;
}

/**
 * Returns true if this port has a connected event-only cable that should currently be rendered wirelessly.
 */
bool VuoRendererPort::hasConnectedWirelessEventCable(bool includePublishedCables) const
{
	vector<VuoCable *> connectedCables = getBase()->getConnectedCables(includePublishedCables);
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		if ((*cable)->hasRenderer() && !(*cable)->getRenderer()->effectivelyCarriesData() &&
				(*cable)->getRenderer()->getEffectivelyWireless() &&
				(*cable)->getRenderer()->paintingDisabled())
			return true;
	return false;
}

/**
 * Returns the tint color to be used in rendering the port shape.
 */
VuoNode::TintColor VuoRendererPort::getPortTint() const
{
	VuoRendererNode *renderedParentNode = getRenderedParentNode();
	if (renderedParentNode)
		return renderedParentNode->getBase()->getTintColor();
	else
	{
		// Tint protocol ports the same color as the protocol.
		if (dynamic_cast<const VuoRendererPublishedPort *>(this) &&
				(dynamic_cast<VuoPublishedPort *>(this->getBase()))->isProtocolPort())
		{
			// @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
			return VuoRendererColors::getActiveProtocolTint(0, !isOutput);
		}
	}

	return VuoNode::TintNone;
}

/**
 * Returns the tint color of the wireless antenna.
 * Tint output antennas the same color as their parent nodes.
 * Tint input antennas the color of their wirelessly connected "From" ports, or leave untinted if there are multiple
 * wirelessly connected "From" ports of conflicting tints.
 */
VuoNode::TintColor VuoRendererPort::getWirelessAntennaTint() const
{
	if (!getInput())
		return getPortTint();

	set<VuoNode::TintColor> connectedPortTints;
	foreach (VuoRendererPort *port, getPortsConnectedWirelessly(true))
	{
		connectedPortTints.insert(port->getPortTint());
		if (connectedPortTints.size() > 1)
			return VuoNode::TintNone;
	}

	if (connectedPortTints.size() == 1)
		return *connectedPortTints.begin();
	else
		return VuoNode::TintNone;
}

/**
 * Returns the set of ports that have antennas connected by wireless cable to this one.
 */
set<VuoRendererPort *> VuoRendererPort::getPortsConnectedWirelessly(bool includePublishedCables) const
{
	set<VuoRendererPort *> connectedPorts;
	foreach (VuoCable *cable, getBase()->getConnectedCables(includePublishedCables))
	{
		if (cable->hasRenderer() &&
				cable->getRenderer()->getEffectivelyWireless() &&
				cable->getRenderer()->paintingDisabled())
		{
			VuoPort *connectedPort = (getInput()? cable->getFromPort() : cable->getToPort());
			if (connectedPort && connectedPort->hasRenderer())
				connectedPorts.insert(connectedPort->getRenderer());
		}
	}

	return connectedPorts;
}

/**
 * Returns the input drawer that is rendered as if it is attached to this port
 * (whether or not it is in the underlying composition), or NULL if none.
 */
VuoRendererInputDrawer * VuoRendererPort::getAttachedInputDrawer(void) const
{
	return getAttachedInputDrawerRenderedWithHostPort(this);
}

/**
 * Returns the input drawer that is rendered as if it is attached to `targetHostPort`
 * (whether or not it is in the underlying composition), or NULL if none.
 */
VuoRendererInputDrawer * VuoRendererPort::getAttachedInputDrawerRenderedWithHostPort(const VuoRendererPort *targetHostPort) const
{
	VuoRendererInputAttachment *underlyingAttachment = getUnderlyingInputAttachment();
	if (underlyingAttachment &&
			underlyingAttachment->getRenderedHostPort() &&
			underlyingAttachment->getRenderedHostPort()->hasRenderer() &&
			(underlyingAttachment->getRenderedHostPort()->getRenderer() == targetHostPort) &&
			(dynamic_cast<VuoRendererInputDrawer *>(underlyingAttachment)))
		return dynamic_cast<VuoRendererInputDrawer *>(underlyingAttachment);

	else if (underlyingAttachment)
	{
		// The drawer might not be directly connected in the underlying composition.  Find it anyway.
		foreach (VuoPort *port, underlyingAttachment->getBase()->getInputPorts())
		{
			VuoRendererInputDrawer *upstreamDrawer = port->getRenderer()->getAttachedInputDrawerRenderedWithHostPort(targetHostPort);
			if (upstreamDrawer)
				return upstreamDrawer;
		}
	}

	return NULL;
}

/**
 * Returns the input drawer attached to this port in the underlying composition
 * (whether or not it is rendered as if it is), or NULL if none.
 */
VuoRendererInputAttachment * VuoRendererPort::getUnderlyingInputAttachment(void) const
{
	if (! getInput())
		return NULL;

	vector<VuoCable *> inCables = getBase()->getConnectedCables(false);
	foreach (VuoCable *cable, inCables)
	{
		VuoNode *fromNode = cable->getFromNode();
		if (fromNode && fromNode->hasRenderer() &&
				dynamic_cast<VuoRendererInputAttachment *>(fromNode->getRenderer()) &&
				dynamic_cast<VuoRendererInputAttachment *>(fromNode->getRenderer())->getUnderlyingHostPort()->getRenderer() == this)
			return dynamic_cast<VuoRendererInputAttachment *>(fromNode->getRenderer());
	}

	return NULL;
}

/**
 * Returns a set containing the input attachments connected directly or indirectly
 * to this port in the underlying composition.
 * Assumption: A given port can have at most one input attachment.
 */
set<VuoRendererInputAttachment *> VuoRendererPort::getAllUnderlyingUpstreamInputAttachments(void) const
{
	set<VuoRendererInputAttachment *> allUpstreamAttachments;
	VuoRendererInputAttachment *directUpstreamAttachment = getUnderlyingInputAttachment();
	if (!directUpstreamAttachment)
		return allUpstreamAttachments;

	allUpstreamAttachments.insert(directUpstreamAttachment);

	vector<VuoPort *> inputPorts = directUpstreamAttachment->getBase()->getInputPorts();
	foreach (VuoPort *port, inputPorts)
	{
		set<VuoRendererInputAttachment *> indirectUpstreamAttachments = port->getRenderer()->getAllUnderlyingUpstreamInputAttachments();
		allUpstreamAttachments.insert(indirectUpstreamAttachments.begin(), indirectUpstreamAttachments.end());
	}

	return allUpstreamAttachments;
}

/**
 * Returns a rect enclosing the string representation of the port's constant value.
 * If the port does not have a constant value, returns a null rect.
 */
QRectF VuoRendererPort::getPortConstantTextRect(void) const
{
	return (isConstant()?
				getPortConstantTextRectForText(QString::fromUtf8(getConstantAsTruncatedStringToRender().c_str())) :
				QRectF());
}

/**
 * Returns the width of `text` as rendered using VuoRendererFonts::nodePortConstantFont.
 *
 * @threadMain
 */
int VuoRendererPort::getTextWidth(QString text)
{
	static QHash<QString, int> cachedTextWidths;
	QHash<QString, int>::iterator i = cachedTextWidths.find(text);
	if (i != cachedTextWidths.end())
		return i.value();

	int textWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodePortConstantFont())
		.boundingRect(QRectF(0,0,0,0), Qt::TextIncludeTrailingSpaces, text)
		.width();
	cachedTextWidths.insert(text, textWidth);
	return textWidth;
}

/**
 * Returns a rect enclosing the specified @c text.
 */
QRectF VuoRendererPort::getPortConstantTextRectForText(QString text)
{
	int textWidth = getTextWidth(text) + 1;

	QRectF textRect(
		-textWidth - portConstantTextPadding + 8,
		-VuoRendererFonts::thickPenWidth/3.0,
		textWidth,
		(VuoRendererPort::portRadius - 1)*2 - 1
	);

	return textRect.toAlignedRect();
}


/**
 * Returns the cached bounding box of the port's label.
 */
QRectF VuoRendererPort::getNameRect() const
{
	return this->nameRect;
}

/**
 * Updates the cached bounding box of the port's label.
 */
void VuoRendererPort::updateNameRect()
{
	QString text = QString::fromUtf8(getPortNameToRender().c_str());
	QFont font = getPortNameFont();
	QSizeF textSize = QFontMetricsF(font).size(0,text);

	bool isPortOnDrawer = dynamic_cast<VuoRendererInputAttachment *>(getUnderlyingParentNode());

	this->nameRect = QRectF(
		(isOutput? -VuoRendererFonts::thickPenWidth/2.0 - textSize.width() - VuoRendererPort::portRadius :
										 VuoRendererFonts::thickPenWidth/2.0
											 - VuoRendererFonts::getHorizontalOffset(font, text)
											 + (isPortOnDrawer ? 2 : VuoRendererPort::portRadius) + 2.
										 ),
		-VuoRendererFonts::thickPenWidth/3.0 - (isPortOnDrawer ? 0 : 1),
		textSize.width(),
		textSize.height()
	);
}

/**
 * Returns a pointer to the node to which this port belongs in the underlying
 * Graphviz (.dot/.vuo) representation of the composition.
 *
 * For a given port, this value will never change.
 * For typecast ports, this method will always return the original parent typecast node,
 * regardless of whether the typecast is currently free-standing or collapsed.
 */
VuoRendererNode * VuoRendererPort::getUnderlyingParentNode(void) const
{
	VuoRendererNode *node;
	if (getTypecastParentPort())
		node = (((VuoRendererTypecastPort *)(getTypecastParentPort()))->getUncollapsedTypecastNode());
	else
		node = getRenderedParentNode();

	return node;
}

/**
 * Returns a pointer to the node currently rendered as this port's parent.
 *
 * For ports belonging to typecast nodes, this value will change depending
 * whether the typecast is currently free-standing (in which case that is the node that will
 * be returned) or collapsed (in which case the downstream node to which the typecast is
 * attached is the one that will be returned).
 */
VuoRendererNode * VuoRendererPort::getRenderedParentNode(void) const
{
	if (!parentItem())
		return NULL;

	if (!parentItem()->parentItem())
		return (VuoRendererNode *)(parentItem());

	return (VuoRendererNode *)(parentItem()->parentItem());
}

/**
 * Returns this port's typecast parent port, or NULL if it has none.
 */
VuoRendererPort * VuoRendererPort::getTypecastParentPort() const
{
	return typecastParentPort;
}

/**
 * Sets this port's typecast parent port.
 */
void VuoRendererPort::setTypecastParentPort(VuoRendererPort *typecastParentPort)
{
	this->typecastParentPort = typecastParentPort;
}

/**
 * Returns the bounding rectangle of this port (and its optional name and plug).
 */
QRectF VuoRendererPort::boundingRect(void) const
{
	return cachedBoundingRect;
}

/**
 * Recalculates `cachedBoundingRect`.
 */
void VuoRendererPort::updateCachedBoundingRect()
{
	VuoRendererNode *renderedParentNode = getRenderedParentNode();
	if ((renderedParentNode && renderedParentNode->paintingDisabled()) || isHiddenRefreshPort())
	{
		cachedBoundingRect = QRectF();
		return;
	}

	QRectF r = getPortPath().boundingRect();

	r = r.united(getEventBarrierRect());

	if (portNameRenderingEnabled())
		r = r.united(getNameRect());

	if (hasPortAction())
		r = r.united(getActionIndicatorRect());

	r = r.united(getWirelessAntennaPath().boundingRect());

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	cachedBoundingRect = r.toAlignedRect();
}

/**
 * Returns the shape of the rendered port, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererPort::shape() const
{
	QPainterPath p;
	p.addRect(boundingRect());
	return p;
}

/**
 * Paints the port's event wall or door.
 */
void VuoRendererPort::paintEventBarrier(QPainter *painter, VuoRendererColors *colors)
{
	bool sidebarPaintMode = dynamic_cast<VuoRendererPublishedPort *>(this);

	VuoPortClass::PortType type = getBase()->getClass()->getPortType();
	VuoPortClass::EventBlocking eventBlocking = getBase()->getClass()->getEventBlocking();

	if (
			!isAnimated &&
			((!isOutput && !sidebarPaintMode && eventBlocking != VuoPortClass::EventBlocking_None)
			|| (isOutput && type == VuoPortClass::triggerPort))
			)
	{
		QRectF barrierRect = getEventBarrierRect();
		QColor eventBlockingBarrierColor = (isAnimated? colors->animatedeventBlockingBarrier() : colors->eventBlockingBarrier());
		painter->setPen(QPen(eventBlockingBarrierColor, VuoRendererPort::portBarrierWidth, Qt::SolidLine, Qt::RoundCap));

		if (eventBlocking == VuoPortClass::EventBlocking_Wall || type == VuoPortClass::triggerPort)
			painter->drawLine(barrierRect.center() + QPointF(0, -barrierRect.height()/2. + VuoRendererPort::portBarrierWidth/2. - .83),
							  barrierRect.center() + QPointF(0,  barrierRect.height()/2. - VuoRendererPort::portBarrierWidth/2. +1.16));
		else // VuoPortClass::EventBlocking_Door
		{
			painter->drawPoint(barrierRect.center() + QPointF(0, -barrierRect.height()/2. + 1.75 + .17));
			painter->drawPoint(barrierRect.center() + QPointF(0,  barrierRect.height()/2. - 1.75 + .17));
		}
	}
}

/**
 * Returns the font to be used to render this port's label.
 */
QFont VuoRendererPort::getPortNameFont(void) const
{
	if (dynamic_cast<VuoRendererInputAttachment *>(getUnderlyingParentNode()))
		// Use a smaller font for port labels on drawers.
		return VuoRendererFonts::getSharedFonts()->nodePortConstantFont();
	else
		return VuoRendererFonts::getSharedFonts()->nodePortTitleFont();
}

/**
 * Paints the port's label.
 */
void VuoRendererPort::paintPortName(QPainter *painter, VuoRendererColors *colors)
{
	if (!portNameRenderingEnabled())
		return;

	VuoRendererPublishedPort *rpp = dynamic_cast<VuoRendererPublishedPort *>(this);

	string name = getPortNameToRender();

	if (rpp)
		painter->setPen((rpp->isSelected() && rpp->getCurrentlyActive())
						? Qt::white
						: (dynamic_cast<VuoPublishedPort *>(rpp->getBase())->isProtocolPort() ? colors->publishedProtocolPortTitle() : colors->publishedPortTitle()));
	else
		painter->setPen(colors->portTitle());

	painter->setFont(getPortNameFont());
	painter->drawText(getNameRect(), isOutput? Qt::AlignRight : Qt::AlignLeft, QString::fromStdString(name));
}

/**
 * Returns the name of the port as it should be rendered. This will be the empty string
 * if name rendering is currently disabled for this port.
 */
string VuoRendererPort::getPortNameToRender() const
{
	bool displayPortName = (getRenderedParentNode()? getRenderedParentNode()->nameDisplayEnabledForPort(this) : true);

	return (!displayPortName? "": getPortNameToRenderWhenDisplayed());
}

/**
 * Returns the name of the port as it should be rendered when it is to be rendered at all.
 */
string VuoRendererPort::getPortNameToRenderWhenDisplayed() const
{
	const VuoRendererPublishedPort *publishedPort = dynamic_cast<const VuoRendererPublishedPort *>(this);
	return (publishedPort? getBase()->getClass()->getName() :
							  (!customizedPortName.empty()? customizedPortName :
															getBase()->getClass()->getName()));
}

/**
 * Sets the name of the port as it should be rendered. If set, it will override
 * the default name of the port class.
 */
void VuoRendererPort::setPortNameToRender(string name)
{
	this->customizedPortName = name;
	updateNameRect();
}

/**
 * Returns the default display name of the port. This may be overridden.
 * (See VuoRendererPort::setPortNameToRender(string name)).
 */
string VuoRendererPort::getDefaultPortNameToRender()
{
	VuoPortClass *pc = getBase()->getClass();
	if (pc->hasCompiler())
		return static_cast<VuoCompilerPortClass *>(pc->getCompiler())->getDisplayName();
	else
		return "";
}

/**
 * Returns true if this port has a port action.
 */
bool VuoRendererPort::hasPortAction(void) const
{
	return getBase()->getClass()->hasPortAction();
}

/**
 * Returns the bounding rectangle of the port action symbol. Assumes this port has a port action.
 */
QRectF VuoRendererPort::getActionIndicatorRect(void) const
{
	QFontMetricsF fontMetrics = QFontMetricsF(getPortNameFont());
	const qreal marginFromPortName = 4;
	const qreal triangleSize = 6;
	qreal triangleLeft = qRound( getNameRect().right() + marginFromPortName );
	qreal triangleTop = qRound( getNameRect().bottom() - fontMetrics.descent() - fontMetrics.xHeight() );

	return QRectF(triangleLeft, triangleTop, triangleSize, triangleSize);
}

/**
 * Draws the port action symbol (a triangle to the right of the port name) if this port has a port action.
 */
void VuoRendererPort::paintActionIndicator(QPainter *painter, VuoRendererColors *colors)
{
	if (hasPortAction())
	{
		QRectF rect = getActionIndicatorRect();

		QPainterPath p;
		addRoundedTriangle(p, rect.center() + QPointF(-1,-.75), qRound(rect.width()/2.) + .5, VuoRendererNode::cornerRadius/9.);

		QColor color = colors->actionIndicator();
		painter->fillPath(p, color);
	}
}

/**
 * Draws the wireless antenna to represent any wireless cables connected to this port.
 */
void VuoRendererPort::paintWirelessAntenna(QPainter *painter, VuoRendererColors *colors)
{
	painter->fillPath(getWirelessAntennaPath(), QBrush(colors->cableMain()));
}

/**
 * Draws an input or output port (both standard ports and refresh/function ports).
 */
void VuoRendererPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (isHiddenRefreshPort())
		return;

	VuoRendererNode *renderedParentNode = getRenderedParentNode();
	if (renderedParentNode && renderedParentNode->paintingDisabled())
		return;

	VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(this);

	// Workaround to prevent items that have been removed from the scene from being painted on the scene anyway.
	// https://b33p.net/kosada/node/7938
	if (!(scene() || publishedPort))
		return;

	painter->setRenderHint(QPainter::Antialiasing, true);
	drawBoundingRect(painter);

	bool isColorInverted = isRefreshPort || isFunctionPort;

	VuoRendererColors::SelectionType selectionType = ((renderedParentNode && renderedParentNode->isSelected())? VuoRendererColors::directSelection :
													   VuoRendererColors::noSelection);

	bool isHovered = isEligibleForSelection;
	qint64 timeOfLastActivity = getTimeOfLastActivity();

	// If an attached drawer does have eligible ports, ensure this host port isn't faded out, so the port name is legible.
	VuoRendererColors::HighlightType effectiveHighlight = _eligibilityHighlight;
	bool effectivelyHovered = isHovered;
	VuoRendererInputDrawer *drawer = getAttachedInputDrawer();
	if (drawer)
	{
		VuoRendererColors::HighlightType drawerHighlight = drawer->getEligibilityHighlight();
		if (drawerHighlight < _eligibilityHighlight)
		{
			effectiveHighlight = drawerHighlight;

			if (_eligibilityHighlight == VuoRendererColors::ineligibleHighlight)
				effectivelyHovered = false;
		}
	}

	VuoRendererColors *colors = new VuoRendererColors(getPortTint(),
													  selectionType,
													  effectivelyHovered,
													  effectiveHighlight,
													  timeOfLastActivity);
	VuoRendererColors *antennaColors = new VuoRendererColors((getWirelessAntennaTint()),
													  selectionType,
													  isHovered,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);

	// Draw the port circle / constant flag
	QPainterPath portPath = getPortPath();

	QBrush portBrush;

	if (isColorInverted)
		portBrush = colors->portTitlebarFill();
	else if (isAnimated)
		portBrush = colors->animatedPortFill();
	else if (publishedPort && dynamic_cast<VuoPublishedPort *>(publishedPort->getBase())->isProtocolPort())
		portBrush = colors->portTitlebarFill();
	else if (publishedPort)
		portBrush = colors->publishedPortFill();
	else
		portBrush = colors->portFill();


	if (!isConstant())
	{
		bool showRightHalfOnly = false;
		VuoRendererInputDrawer *drawer = getAttachedInputDrawer();
		if (drawer)
		{
			VuoRendererPort *drawerChildPort = (drawer && (drawer->getInputPorts().size() >= VuoNodeClass::unreservedInputPortStartIndex+1)?
													drawer->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex] :
												NULL);

			// Prevent neighboring semi-transparent highlights from overlapping in a misleading way
			// Essentially: Paint the whole circle if the circle is meant to be more opaque than the drawer handle it intersects.
			qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
			const double fadeThreshold = 0.3; // Tuned visually.
			qint64 childTimeOfLastActivity = drawerChildPort? drawerChildPort->getTimeOfLastActivity() : VuoRendererItem::notTrackingActivity;
			bool showingActiveEvent = (timeOfLastActivity != VuoRendererItem::notTrackingActivity) &&
									  (((timeNow - timeOfLastActivity) < VuoRendererColors::activityAnimationFadeDuration*fadeThreshold) ||
									   (timeOfLastActivity == VuoRendererItem::activityInProgress));
			bool childShowingActiveEvent = (childTimeOfLastActivity != VuoRendererItem::notTrackingActivity) &&
										   (((timeNow - childTimeOfLastActivity) < VuoRendererColors::activityAnimationFadeDuration*fadeThreshold) ||
										   (childTimeOfLastActivity == VuoRendererItem::activityInProgress));
			showRightHalfOnly = !effectivelyHovered && drawerChildPort &&
									 (drawerChildPort->eligibilityHighlight() <= effectiveHighlight) &&
									 !(showingActiveEvent && !childShowingActiveEvent);
		}

		if (showRightHalfOnly)
			painter->setClipRect(QRectF(-0.39, -portRadius, portRadius, portRadius*2.));

		painter->fillPath(portPath, portBrush);

		if (showRightHalfOnly)
			painter->setClipping(false);
	}
	else
	{
		// Display a color swatch for VuoColor data.
		/// @todo Implement with input viewers (https://b33p.net/kosada/node/5700)
		bool isColorPort = getDataType() && getDataType()->getModuleKey()=="VuoColor";
		if (isColorPort)
		{
			string colorRGBAJsonString = getConstantAsString();
			VuoColor c = VuoColor_makeFromString(colorRGBAJsonString.c_str());
			QColor swatchColor = QColor(c.r*255, c.g*255, c.b*255, c.a*255 * portBrush.color().alphaF());
			VuoReal h,s,l,a;
			VuoColor_getHSLA(c, &h, &s, &l, &a);

			bool isDark = colors->isDark();

			// Two possible swatches:

			// 1. Semitransparent color, or solid color that matches the canvas background:  Draw a background+border, then draw the swatch.
			if ((a < 1) || (isDark ? l < .25 : l > .75))
			{
				// Fill the entire background with a color distinct from the canvas.
				QColor topLeftColor = colors->nodeFrame();
				painter->fillPath(portPath, topLeftColor);

				// Fill the bottom right of the background with the opposite color.
				// Use a slightly smaller circle, so the topLeftColor acts as a border.
				QColor bottomRightColor = isDark ? Qt::black : Qt::white;
				QTransform transform;
				transform.scale(0.87, 0.87);
				QPainterPath smallerCircle = portPath * transform;
				{
					QRectF r = portPath.boundingRect();
					QPainterPath bottomRight;
					bottomRight.moveTo(r.bottomLeft());
					bottomRight.lineTo(r.topRight());
					bottomRight.lineTo(r.bottomRight());
					painter->setClipPath(smallerCircle);
					painter->fillPath(bottomRight, bottomRightColor);
					painter->setClipping(false);
				}

				// Draw the swatch.
				painter->fillPath(smallerCircle, swatchColor);
			}

			// 2. Solid color that's distinct from the canvas background:  Just draw the swatch.
			else
				painter->fillPath(portPath, swatchColor);
		}
		else
		{
			painter->fillPath(portPath, portBrush);

			QString constantText = QString::fromUtf8(getConstantAsTruncatedStringToRender().c_str());
			QBrush constantFlagBackgroundBrush = colors->constantFill();

			// Constant string
			QRectF textRect = getPortConstantTextRectForText(constantText);
			painter->setPen(colors->constantText());
			painter->setFont(VuoRendererFonts::getSharedFonts()->nodePortConstantFont());
			painter->drawText(textRect, Qt::AlignLeft, constantText);
		}
	}

	if (! carriesData())
	{
		QRectF r = getPortRect();
		QPainterPath p;
		addRoundedTriangle(p, r.center() + QPointF(.5, -.3), (r.width() - VuoRendererPort::portInset*2)/2.-2, VuoRendererNode::cornerRadius/6.);
		painter->fillPath(p, colors->portIcon());
	}

	paintEventBarrier(painter, colors);
	paintPortName(painter, colors);
	paintActionIndicator(painter, colors);
	paintWirelessAntenna(painter, antennaColors);

	delete colors;
	delete antennaColors;
}

/**
 * Returns the time of the port's latest activity for purposes of "Show Events" mode event tracking.
 */
qint64 VuoRendererPort::getTimeOfLastActivity()
{
	VuoPortClass::PortType type = getBase()->getClass()->getPortType();
	bool isTriggerPort = (type == VuoPortClass::triggerPort);

	VuoRendererComposition *composition = dynamic_cast<VuoRendererComposition *>(scene());
	bool renderNodeActivity = composition && composition->getRenderNodeActivity();
	bool renderPortActivity = composition && composition->getRenderPortActivity();
	VuoRendererNode *renderedParentNode = getRenderedParentNode();

	return ((! renderNodeActivity)? VuoRendererItem::notTrackingActivity :
									((isTriggerPort && renderPortActivity)? timeLastEventFired :
									(getTypecastParentPort()? static_cast<VuoRendererTypecastPort *>(getTypecastParentPort())->getUncollapsedTypecastNode()->getTimeLastExecutionEnded() :
									(renderedParentNode? renderedParentNode->getTimeLastExecutionEnded() :
									VuoRendererItem::notTrackingActivity))));
}

/**
 * Returns a boolean indicating whether this port has been deemed
 * eligible for selection based on its proximity to the cursor.
 */
bool VuoRendererPort::getEligibleForSelection(void)
{
	return isEligibleForSelection;
}

/**
 * Returns a boolean indicating whether this port is eligible for direct or
 * typecast-assisted connection to the cable currently being dragged between ports.
 */
bool VuoRendererPort::isEligibleForConnection(void)
{
	return _eligibilityHighlight == VuoRendererColors::standardHighlight
		|| _eligibilityHighlight == VuoRendererColors::subtleHighlight;
}

/**
 * Sets whether this port is eligible for direct or typecast-assisted connection to the cable currently being dragged between ports.
 */
void VuoRendererPort::setEligibilityHighlight(VuoRendererColors::HighlightType eligibility)
{
	_eligibilityHighlight = eligibility;
}

/**
 * Returns the status of this port's ability to connect to the cable currently being dragged.
 */
VuoRendererColors::HighlightType VuoRendererPort::eligibilityHighlight(void)
{
	return _eligibilityHighlight;
}

/**
 * Handle mouse hover start events generated by custom code making use of an extended hover range.
 */
void VuoRendererPort::extendedHoverEnterEvent(bool cableDragUnderway, bool disableConnectedCableHighlight)
{
	extendedHoverMoveEvent(cableDragUnderway, disableConnectedCableHighlight);
}

/**
 * Handle mouse hover move events generated by custom code making use of an extended hover range.
 * If the optional @c highlightOnlyIfConnectable boolean is set to true, enable hover highlighting
 * for this port only if the port is eligible for connection to the cable currently being connected.
 */
void VuoRendererPort::extendedHoverMoveEvent(bool cableDragUnderway, bool disableConnectedCableHighlight)
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);

	prepareGeometryChange();
	isEligibleForSelection = (cableDragUnderway? isEligibleForConnection() : true);

	setCacheMode(normalCacheMode);

	setFocus();

	if (!cableDragUnderway && !disableConnectedCableHighlight)
	{
		vector<VuoCable *> connectedCables = getBase()->getConnectedCables(false);
		if (supportsDisconnectionByDragging() && (! connectedCables.empty()))
		{
			VuoRendererCable *cableToDisconnect = connectedCables.back()->getRenderer();
			cableToDisconnect->updateGeometry();
			cableToDisconnect->setHovered(true);
		}
	}
}

/**
 * Handle mouse hover leave events generated by custom code making use of an extended hover range.
 */
void VuoRendererPort::extendedHoverLeaveEvent()
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);

	prepareGeometryChange();
	isEligibleForSelection = false;

	setCacheMode(normalCacheMode);

	clearFocus();

	vector<VuoCable *> connectedCables = getBase()->getConnectedCables(false);
	if (supportsDisconnectionByDragging() && (! connectedCables.empty()))
	{
		VuoRendererCable *cableToDisconnect = connectedCables.back()->getRenderer();
		cableToDisconnect->updateGeometry();
		cableToDisconnect->setHovered(false);
	}
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from this port to @c toPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types).
 *
 * The @c eventOnlyConnection argument should indicate whether the cable
 * potentially connecting the two ports would be an always-event-only cable.
 *
 * If the connection would require one or both ports to be specialized, returns false.
 * (But see @c VuoRendererPort::canConnectDirectlyWithSpecializationTo(...).)
 */
bool VuoRendererPort::canConnectDirectlyWithoutSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection)
{
	bool fromPortIsEnabledOutput = (this->getOutput() && this->isEnabled());
	bool toPortIsEnabledInput = (toPort && toPort->getInput() && toPort->isEnabled());

	if (!(fromPortIsEnabledOutput && toPortIsEnabledInput))
			return false;

	// OK: Any connection made using an event-only cable.
	if (eventOnlyConnection)
		return true;

	VuoType *fromDataType = this->getDataType();
	VuoType *toDataType = toPort->getDataType();

	// OK: Event-only to event+data.
	// OK: Event-only to event-only.
	// OK: Event+data to event-only.
	if (!fromDataType || !toDataType)
		return true;

	// OK: Event+data to event+data, if types are non-generic and identical.
	if (! dynamic_cast<VuoGenericType *>(fromDataType) && (fromDataType == toDataType))
		return true;

	// OK: Event+data to event+data, if types are generic and compatible.
	if (dynamic_cast<VuoGenericType *>(fromDataType) && dynamic_cast<VuoGenericType *>(toDataType))
	{
		/// @todo (https://b33p.net/kosada/node/7032)
		if (VuoType::isListTypeName(fromDataType->getModuleKey()) != VuoType::isListTypeName(toDataType->getModuleKey()))
			return false;

		return (dynamic_cast<VuoGenericType *>(fromDataType)->isGenericTypeCompatible(dynamic_cast<VuoGenericType *>(toDataType)));
	}

	return false;
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from this port to @c toPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), and the possibility that one
 * port may be specialized in preparation for the connection.
 *
 * The @c eventOnlyConnection argument should indicate whether the cable
 * potentially connecting the two ports would be an always-event-only cable.
 *
 * Convenience function for VuoRendererPort::canConnectDirectlyWithSpecializationTo(const VuoRendererPort *toPort,
 * VuoRendererPort **portToSpecialize, string &specializedTypeName), for use
 * when only the returned boolean and none of the other output parameter values are needed.
 *
 */
bool VuoRendererPort::canConnectDirectlyWithSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection)
{
	VuoRendererPort *portToSpecialize = NULL;
	string specializedTypeName = "";

	return (this->canConnectDirectlyWithSpecializationTo(toPort, eventOnlyConnection, &portToSpecialize, specializedTypeName));
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from this port to @c toPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), and the possibility that one
 * port may be specialized in preparation for the connection.
 *
 * @param[in] toPort The port to consider connecting to.
 * @param[in] eventOnlyConnection A boolean indicating whether the connection under consideration would be always-event-only.
 * @param[out] portToSpecialize The port, either this port or @c toPort, that will require specialization in order for the connection to be completed.
 *                              Does not account for potential cascade effects. May be NULL, if the connection may be completed without specialization.
 * @param[out] specializedTypeName The name of the specialized port type with which the generic port type is to be replaced.
 */
bool VuoRendererPort::canConnectDirectlyWithSpecializationTo(VuoRendererPort *toPort, bool eventOnlyConnection, VuoRendererPort **portToSpecialize, string &specializedTypeName)
{
	*portToSpecialize = NULL;
	specializedTypeName = "";

	if (this->canConnectDirectlyWithoutSpecializationTo(toPort, eventOnlyConnection))
		return true;

	bool fromPortIsEnabledOutput = (this->getOutput() && this->isEnabled());
	bool toPortIsEnabledInput = (toPort && toPort->getInput() && toPort->isEnabled());

	if (!(fromPortIsEnabledOutput && toPortIsEnabledInput))
		return false;

	VuoType *originalFromDataType = ((VuoCompilerPortClass *)(this->getBase()->getClass()->getCompiler()))->getDataVuoType();
	VuoType *originalToDataType = ((VuoCompilerPortClass *)(toPort->getBase()->getClass()->getCompiler()))->getDataVuoType();

	VuoType *currentFromDataType = this->getDataType();
	VuoType *currentToDataType = toPort->getDataType();

	if (!(originalFromDataType && originalToDataType && currentFromDataType && currentToDataType))
		return false;

	VuoGenericType *currentFromGenericType = dynamic_cast<VuoGenericType *>(currentFromDataType);
	VuoGenericType *currentToGenericType = dynamic_cast<VuoGenericType *>(currentToDataType);

	/// @todo (https://b33p.net/kosada/node/7032)
	if (VuoType::isListTypeName(currentFromDataType->getModuleKey()) != VuoType::isListTypeName(currentToDataType->getModuleKey()))
		return false;

	// Case: The 'From' port is generic and can be specialized to match the concrete type of the 'To' port.
	if (currentFromGenericType && currentFromGenericType->isSpecializedTypeCompatible(originalToDataType->getModuleKey()))
	{
		*portToSpecialize = this;
		specializedTypeName = originalToDataType->getModuleKey();

		return true;
	}

	// Case: The 'To' port is generic and can be specialized to match the concrete type of the 'From' port.
	else if (currentToGenericType && currentToGenericType->isSpecializedTypeCompatible(originalFromDataType->getModuleKey()))
	{
		*portToSpecialize = toPort;
		specializedTypeName = originalFromDataType->getModuleKey();

		return true;
	}

	return false;
}

/**
 * Returns the cable connecting this port to @c toPort, or NULL if not applicable.
 */
VuoCable * VuoRendererPort::getCableConnectedTo(VuoRendererPort *toPort, bool includePublishedCables)
{
	vector<VuoCable *> cables = this->getBase()->getConnectedCables(includePublishedCables);
	for (vector<VuoCable *>::iterator cable = cables.begin(); cable != cables.end(); ++cable)
		if ((*cable)->getToPort() == toPort->getBase())
			return (*cable);

	return NULL;
}

/**
 * Handle mouse double-click events.
 */
void VuoRendererPort::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (isConstant())
	{
		signaler->signalInputEditorRequested(this);
	}
}

/**
 * Handle key-press events.
 */
void VuoRendererPort::keyPressEvent(QKeyEvent *event)
{
	if (isConstant() &&
		(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
	{
		signaler->signalInputEditorRequested(this);
	}
}

/**
 * Returns a boolean indicating whether this port supports cable disconnection
 * by dragging from the port.
 */
bool VuoRendererPort::supportsDisconnectionByDragging(void)
{
	return (getInput() &&
			(! dynamic_cast<VuoRendererTypecastPort *>(this)) &&
			(! getAttachedInputDrawer()));
}

/**
 * Returns a boolean indicating whether this port is an input port.
 */
bool VuoRendererPort::getInput(void) const
{
	return ((! isOutput) && (! isFunctionPort));
}

/**
 * Returns a boolean indicating whether this port is an output port.
 */
bool VuoRendererPort::getOutput(void) const
{
	return isOutput;
}

/**
 * Returns true if this port is a refresh port.
 */
bool VuoRendererPort::getRefreshPort(void) const
{
	return isRefreshPort;
}

/**
 * Returns true if this port is a function port.
 */
bool VuoRendererPort::getFunctionPort(void) const
{
	return isFunctionPort;
}

/**
 * Returns true if this port is a refresh port that should be hidden because it has no connected cables.
 */
bool VuoRendererPort::isHiddenRefreshPort(void)
{
	return isRefreshPort && getBase()->getConnectedCables(true).empty();
}

/**
 * Returns true if this port is a data+event port (regular or trigger).
 */
bool VuoRendererPort::carriesData(void) const
{
	return (getBase()->getClass()->getPortType() == VuoPortClass::dataAndEventPort ||
			(getBase()->getClass()->hasCompiler() &&
			 static_cast<VuoCompilerPortClass *>(getBase()->getClass()->getCompiler())->getDataVuoType()));
}

/**
 * Schedules a redraw of this port.
 */
void VuoRendererPort::updateGeometry()
{
	updateCachedPortPath();
	this->prepareGeometryChange();
	updateCachedBoundingRect();
}

/**
 * Updates the port to reflect changes in state.
 */
QVariant VuoRendererPort::itemChange(GraphicsItemChange change, const QVariant &value)
{
	// Port has moved relative to its parent
	if (change == QGraphicsItem::ItemPositionHasChanged)
	{
		VuoRendererNode *parentNode = getRenderedParentNode();
		if (parentNode)
			parentNode->layoutConnectedInputDrawersAtAndAbovePort(this);
	}

	return QGraphicsItem::itemChange(change, value);
}

/**
 * Returns the data type associated with this port,
 * or NULL if there is no associated data type.
 */
VuoType * VuoRendererPort::getDataType(void) const
{
	if (!(getBase() && getBase()->hasCompiler()))
		return NULL;

	VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(getBase()->getCompiler());
	return compilerPort->getDataVuoType();
}

/**
 * Returns a boolean indicating whether this port expects a URL as input.
 * @todo https://b33p.net/kosada/node/9204 Just check whether it has a VuoUrl type.
 * For now, use hard-coded rules.
 */
bool VuoRendererPort::hasURLType() const
{
	// For now, ports with URLs are expected to be of type "VuoText".
	if ( !(getDataType() && getDataType()->getModuleKey() == "VuoText") )
		return false;

	string portName = getBase()->getClass()->getName();

	// Case: Port is titled "url"
	if (portName == "url")
		return true;

	// Case: Port is titled "folder"
	// Relevant for vuo.file.list node.
	if (portName == "folder")
		return true;

	// Case: Port is an input port on a drawer attached to a port titled "urls"
	// Relevant for vuo.image.fetch.list, vuo.scene.fetch.list nodes.
	VuoRendererInputDrawer *drawer = dynamic_cast<VuoRendererInputDrawer *>(getRenderedParentNode());
	if (drawer)
	{
		VuoPort *hostPort = drawer->getRenderedHostPort();
		if (hostPort && (hostPort->getClass()->getName() == "urls"))
			return true;
	}

	return false;
}

/**
 * Returns a boolean indicating whether this port currently has
 * a relative input file path as a constant value.
 *
 * Returns `false` if the port has an output file path -- a URL that will be written to
 * rather than read from, as indicated by the port's `isSave:true` port detail.
 */
bool VuoRendererPort::hasRelativeReadURLConstantValue() const
{
	if ( !(isConstant() && hasURLType()) )
		return false;

	json_object *details = static_cast<VuoCompilerInputEventPortClass *>(getBase()->getClass()->getCompiler())->getDataClass()->getDetails();
	json_object *isSaveValue = NULL;
	if (details && json_object_object_get_ex(details, "isSave", &isSaveValue) && json_object_get_boolean(isSaveValue))
		return false;

	VuoUrl url = VuoUrl_makeFromString(getConstantAsString().c_str());
	VuoRetain(url);
	bool isRelativePath = VuoUrl_isRelativePath(url);
	VuoRelease(url);
	return isRelativePath;
}

/**
 * Returns true if this port has a constant data value.
 */
bool VuoRendererPort::isConstant(void) const
{
	return ((getInput() && getDataType()) &&						// input port with data...
			(!effectivelyHasConnectedDataCable(true)));	// ... that has no incoming data cable (published or unpublished).
}

/**
 * Returns true if this port has a connected cable that effectively carries data.
 * For details on what it means to effectively carry data, see VuoRendererCable::effectivelyCarriesData().
 */
bool VuoRendererPort::effectivelyHasConnectedDataCable(bool includePublishedCables) const
{
	vector<VuoCable *> connectedCables = getBase()->getConnectedCables(includePublishedCables);
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		if ((*cable)->hasRenderer() && (*cable)->getRenderer()->effectivelyCarriesData())
			return true;
	return false;
}

/**
 * Returns a new string formatted using the printf-style `format` string.
 */
string VuoRendererPort::format(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int size = vsnprintf(NULL, 0, format, args);
	va_end(args);

	char *formattedString = (char *)malloc(size+1);
	va_start(args, format);
	vsnprintf(formattedString, size+1, format, args);
	va_end(args);

	string s(formattedString);
	free(formattedString);
	return s;
}

/**
 * Creates an std::string from `strz`, then frees it.
 */
string VuoRendererPort::stringAndFree(char *strz)
{
	string s(strz);
	free(strz);
	return s;
}

/**
 * Returns the string representation of this port's constant data value, or an empty string if it has none.
 */
string VuoRendererPort::getConstantAsString(void) const
{
	if (!(getInput() && getDataType()))
		return "";

	VuoCompilerInputEventPort *compilerEventPort = dynamic_cast<VuoCompilerInputEventPort *>(getBase()->getCompiler());
	if (! compilerEventPort)
		return "";

	return compilerEventPort->getData()->getInitialValue();
}

/**
 * Returns the truncated string representation of this port's constant data value as it should be rendered
 * in its constant data flag, or an empty string if it has no currently assigned constant data value.
 */
string VuoRendererPort::getConstantAsTruncatedStringToRender(void) const
{
	VuoText fullString = VuoText_make(getConstantAsStringToRender().c_str());
	VuoLocal(fullString);

	if (getDataType() && (getDataType()->getModuleKey() == "VuoColor"))
		return "";

	bool truncateFromBeginning = (getDataType() &&
								  (getDataType()->getModuleKey()=="VuoArtNetInputDevice" ||
								   getDataType()->getModuleKey()=="VuoArtNetOutputDevice" ||
								   hasURLType()));

	size_t maxLength = strlen("Matches wildcard (not case-sensitive)");
	VuoText t = VuoText_truncateWithEllipsis(fullString, maxLength, truncateFromBeginning?
												 VuoTextTruncation_Beginning :
												 VuoTextTruncation_End);
	VuoLocal(t);
	return string(t);
}

/**
 * If the port's data type is equal to the specified `type`,
 * creates an object from the port's current value,
 * then returns the output of `VuoType_getShortSummary()`.
 */
#define RETURN_SHORT_SUMMARY(type)                                           \
    if (getDataType()->getModuleKey() == #type)                              \
    {                                                                        \
        type value = type ## _makeFromString(getConstantAsString().c_str()); \
        type ## _retain(value);                                              \
        string s = stringAndFree(type ## _getShortSummary(value));           \
        type ## _release(value);                                             \
        return s;                                                            \
    }

/**
 * Returns the untruncated string representation of this port's constant data value as it should be rendered
 * in a port tooltip, or an empty string if it has no currently assigned constant data value.
 */
string VuoRendererPort::getConstantAsStringToRender(void) const
{
	if (!(getInput() && getDataType()))
		return "";

	/// @todo Implement with input viewers (https://b33p.net/kosada/node/5700)
	if (getDataType())
	{
		// Don't display constant input values for generic ports.
		if (dynamic_cast<VuoGenericType *>(getDataType()))
			return "";

		string typeName = getDataType()->getModuleKey();

		// Don't display constant input values for list ports.
		if (VuoType::isListTypeName(typeName))
			return "";

		// Don't display constant input values for types that don't have input editors.
		if (typeName == "VuoData")
			return "";

		if (getDataType()->getModuleKey()=="VuoColor")
		{
			string colorRGBAJsonString = getConstantAsString();
			VuoColor c = VuoColor_makeFromString(colorRGBAJsonString.c_str());
			return stringAndFree(VuoColor_getSummary(c));
		}
		if (getDataType()->getModuleKey()=="VuoBoolean")
			return stringAndFree(VuoBoolean_getSummary(VuoBoolean_makeFromString(getConstantAsString().c_str())));
		if (getDataType()->getModuleKey()=="VuoReal")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			double real = json_object_get_double(js);
			json_object_put(js);

			if (getBase()->getClass()->hasCompiler() && !dynamic_cast<VuoPublishedPort *>(getBase()))
			{
				json_object *portDetails = static_cast<VuoCompilerEventPortClass *>(getBase()->getClass()->getCompiler())->getDataClass()->getDetails();
				json_object *autoObject = NULL;
				if (json_object_object_get_ex(portDetails, "auto", &autoObject))
					if (real == json_object_get_double(autoObject))
						return "Auto";
			}

			return getStringForRealValue(real);
		}
		if (getDataType()->getModuleKey()=="VuoInteger")
		{
			// Retrieve the port's JSON details object.
			json_object *details = NULL;
			VuoCompilerInputEventPortClass *portClass = dynamic_cast<VuoCompilerInputEventPortClass *>(getBase()->getClass()->getCompiler());
			if (portClass)
				details = portClass->getDataClass()->getDetails();

			// Case: Port type is named enum
			json_object *menuItemsValue = NULL;
			if (details && json_object_object_get_ex(details, "menuItems", &menuItemsValue))
			{
				string portValue = getConstantAsString();
				// Support upgrading a VuoBoolean port to a named enum.
				if (portValue == "false")
					portValue = "0";
				else if (portValue == "true")
					portValue = "1";

				int len = json_object_array_length(menuItemsValue);
				for (int i = 0; i < len; ++i)
				{
					json_object *menuItem = json_object_array_get_idx(menuItemsValue, i);
					if (json_object_is_type(menuItem, json_type_object))
					{
						json_object *value = NULL;
						if (json_object_object_get_ex(menuItem, "value", &value))
							if ((json_object_is_type(value, json_type_string) && portValue == json_object_get_string(value))
							 || (json_object_is_type(value, json_type_int   ) && atol(portValue.c_str()) == json_object_get_int64(value)))
							{
								json_object *name = NULL;
								if (json_object_object_get_ex(menuItem, "name", &name))
								{
									VuoText t = VuoText_makeFromJson(name);
									VuoLocal(t);
									VuoText tr = VuoText_trim(t);  // Trim off leading indentation, if any.
									VuoLocal(tr);
									return string(tr);
								}
							}
					}
				}
			}

			// Case: Port type is a regular VuoInteger
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			VuoInteger i = json_object_get_int64(js);
			json_object_put(js);

			if (getBase()->getClass()->hasCompiler() && !dynamic_cast<VuoPublishedPort *>(getBase()))
			{
				json_object *portDetails = static_cast<VuoCompilerEventPortClass *>(getBase()->getClass()->getCompiler())->getDataClass()->getDetails();
				json_object *autoObject = NULL;
				if (json_object_object_get_ex(portDetails, "auto", &autoObject))
					if (i == json_object_get_int64(autoObject))
						return "Auto";
			}

			return format("%lld", i);
		}
		if (getDataType()->getModuleKey()=="VuoPoint2d")
		{
			VuoPoint2d p = VuoPoint2d_makeFromString(getConstantAsString().c_str());
			QList<float> pointList = QList<float>() << p.x << p.y;
			return getPointStringForCoords(pointList);
		}
		if (getDataType()->getModuleKey()=="VuoPoint3d")
		{
			VuoPoint3d p = VuoPoint3d_makeFromString(getConstantAsString().c_str());
			QList<float> pointList = QList<float>() << p.x << p.y << p.z;
			return getPointStringForCoords(pointList);
		}
		if (getDataType()->getModuleKey()=="VuoPoint4d")
		{
			VuoPoint4d p = VuoPoint4d_makeFromString(getConstantAsString().c_str());
			QList<float> pointList = QList<float>() << p.x << p.y << p.z << p.w;
			return getPointStringForCoords(pointList);
		}
		if (getDataType()->getModuleKey()=="VuoFont")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *fontName = NULL;
			if (json_object_object_get_ex(js, "fontName", &o))
				fontName = json_object_get_string(o);

			double pointSize = 0;
			if (json_object_object_get_ex(js, "pointSize", &o))
				pointSize = json_object_get_double(o);

			bool underline = false;
			if (json_object_object_get_ex(js, "underline", &o))
				underline = json_object_get_boolean(o);
			const char *underlineString = underline ? " [U]" : "";

			string outputString;
			if (fontName)
				outputString = format("%s %gpt%s", fontName, pointSize, underlineString);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoMathExpressionList")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *expressionsObject = NULL;

			string expression;
			if (json_object_object_get_ex(js, "expressions", &expressionsObject))
			{
				if (json_object_get_type(expressionsObject) == json_type_array)
				{
					int itemCount = json_object_array_length(expressionsObject);
					if (itemCount > 0)
					{
						json_object *itemObject = json_object_array_get_idx(expressionsObject, 0);
						if (json_object_get_type(itemObject) == json_type_string)
							expression = json_object_get_string(itemObject);
					}
				}
			}

			json_object_put(js);

			return expression;
		}
		if (getDataType()->getModuleKey()=="VuoRealRegulation")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			string outputString;
			if (json_object_object_get_ex(js, "name", &o))
				outputString = json_object_get_string(o);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoImage")
		{
			VuoImage value = VuoImage_makeFromString(getConstantAsString().c_str());
			if (!value)
				return "";

			VuoLocal(value);
			return format("%lu×%lu", value->pixelsWide, value->pixelsHigh);
		}
		if (getDataType()->getModuleKey()=="VuoTransform")
		{
			VuoTransform value = VuoTransform_makeFromString(getConstantAsString().c_str());

			if (VuoTransform_isIdentity(value))
				return "≡";

			if (value.type == VuoTransformTypeTargeted)
				return format("(%g,%g,%g) toward (%g,%g,%g)",
									  value.translation.x, value.translation.y, value.translation.z, value.rotationSource.target.x, value.rotationSource.target.y, value.rotationSource.target.z);

			string rotation;
			if (value.type == VuoTransformTypeQuaternion)
				rotation = format("‹%g,%g,%g,%g›",
										  value.rotationSource.quaternion.x, value.rotationSource.quaternion.y, value.rotationSource.quaternion.z, value.rotationSource.quaternion.w);
			else
			{
				VuoPoint3d r = VuoPoint3d_multiply(value.rotationSource.euler, 180./M_PI);
				rotation = format("(%g°,%g°,%g°)",
										  r.x, r.y, r.z);
			}

			return format("(%g,%g,%g) %s %g×%g×%g",
												 value.translation.x, value.translation.y, value.translation.z, rotation.c_str(), value.scale.x, value.scale.y, value.scale.z);
		}
		if (getDataType()->getModuleKey()=="VuoTransform2d")
		{
			VuoTransform2d value = VuoTransform2d_makeFromString(getConstantAsString().c_str());

			if (VuoTransform2d_isIdentity(value))
				return "≡";

			VuoReal rotationInDegrees = value.rotation * 180./M_PI;
			return format("(%g,%g) %g° %g×%g",
								  value.translation.x, value.translation.y, rotationInDegrees, value.scale.x, value.scale.y);
		}
		if (getDataType()->getModuleKey()=="VuoMovieFormat")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *imageEncoding = NULL;
			if (json_object_object_get_ex(js, "imageEncoding", &o))
			{
				imageEncoding = json_object_get_string(o);
				if (strcasecmp(imageEncoding, "jpeg") == 0)
					imageEncoding = "JPEG";
				else if (strcasecmp(imageEncoding, "h264") == 0)
					imageEncoding = "H.264";
				else if (strcasecmp(imageEncoding, "prores4444") == 0)
					imageEncoding = "ProRes 4444";
				else if (strcasecmp(imageEncoding, "prores422") == 0)
					imageEncoding = "ProRes 422";
				else if (strcasecmp(imageEncoding, "prores422-hq") == 0)
					imageEncoding = "ProRes 422 HQ";
				else if (strcasecmp(imageEncoding, "prores422-lt") == 0)
					imageEncoding = "ProRes 422 LT";
				else if (strcasecmp(imageEncoding, "prores422-proxy") == 0)
					imageEncoding = "ProRes 422 Proxy";
				else if (strcmp(imageEncoding, "hevc") == 0)
					imageEncoding = "HEVC";
				else if (strcmp(imageEncoding, "hevc-alpha") == 0)
					imageEncoding = "HEVC+Alpha";
			}

			const char *audioEncoding = NULL;
			if (json_object_object_get_ex(js, "audioEncoding", &o))
			{
				audioEncoding = json_object_get_string(o);
				if (strcmp(audioEncoding, "LinearPCM") == 0)
					audioEncoding = "Linear PCM";
			}

			string outputString;
			if (imageEncoding && audioEncoding)
				outputString = format("%s, %s", imageEncoding, audioEncoding);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoScreen")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			string label;
			if (json_object_object_get_ex(js, "type", &o))
			{
				VuoScreenType type = VuoScreen_typeFromCString(json_object_get_string(o));

				if (type == VuoScreenType_Active)
					label = "Active";
				else if (type == VuoScreenType_Primary)
					label = "Primary";
				else if (type == VuoScreenType_Secondary)
					label = "Secondary";
				else if (type == VuoScreenType_MatchName)
				{
					if (json_object_object_get_ex(js, "name", &o))
						label = json_object_get_string(o);
				}
				else if (type == VuoScreenType_MatchId)
				{
					VuoScreen screen = VuoScreen_makeFromJson(js);
					VuoScreen realizedScreen;
					if (VuoScreen_realize(screen, &realizedScreen))
						label = realizedScreen.name;
				}
			}
			json_object_put(js);

			return label;
		}
		if (getDataType()->getModuleKey()=="VuoSerialDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);
			else if (json_object_object_get_ex(js, "path", &o))
				name = json_object_get_string(o);

			string outputString = "First";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoMidiInputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);

			string outputString = "First";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoMidiOutputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);

			string outputString = "First";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoSyphonServerDescription")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "serverName", &o))
			{
				const char *n = json_object_get_string(o);
				if (strcmp(n, "*"))
					name = n;
			}
			if (!name && json_object_object_get_ex(js, "applicationName", &o))
			{
				const char *n = json_object_get_string(o);
				if (strcmp(n, "*"))
					name = n;
			}

			string outputString = "First";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoVideoInputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);
			else if (json_object_object_get_ex(js, "id", &o))
				name = json_object_get_string(o);

			string outputString = "Default";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		RETURN_SHORT_SUMMARY(VuoAudioInputDevice);
		RETURN_SHORT_SUMMARY(VuoAudioOutputDevice);
		if (getDataType()->getModuleKey()=="VuoHidDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			QString outputString;
			if (json_object_object_get_ex(js, "name", &o))
			{
				outputString = json_object_get_string(o);

				// Trim off the parenthetical vendor/class.
				outputString = outputString.section(" (", 0, 0);
			}
			else if (json_object_object_get_ex(js, "matchType", &o))
			{
				const char *matchType = json_object_get_string(o);
				if (strcmp(matchType, "usage") == 0)
				{
					int usagePage = 0;
					if (json_object_object_get_ex(js, "usagePage", &o))
						usagePage = json_object_get_int(o);
					int usage = 0;
					if (json_object_object_get_ex(js, "usage", &o))
						usage = json_object_get_int(o);
					json_object_put(js);
					return stringAndFree(VuoHid_getUsageText(usagePage, usage));
				}
			}
			json_object_put(js);

			return outputString.toStdString();
		}
		if (getDataType()->getModuleKey()=="VuoOscInputDevice"
		 || getDataType()->getModuleKey()=="VuoOscOutputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);

			string outputString = "Auto";
			if (name && strlen(name))
				outputString = name;

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoArtNetInputDevice"
		 || getDataType()->getModuleKey()=="VuoArtNetOutputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);
			else
			{
				if (getDataType()->getModuleKey()=="VuoArtNetInputDevice")
					name = "Any";
				else
					name = "Broadcast";
			}

			VuoInteger net=0, subNet=0, universe=0;
			if (json_object_object_get_ex(js, "net", &o))
				net = json_object_get_int64(o);
			if (json_object_object_get_ex(js, "subNet", &o))
				subNet = json_object_get_int64(o);
			if (json_object_object_get_ex(js, "universe", &o))
				universe = json_object_get_int64(o);

			string outputString = format("%s (%lld:%lld:%lld)", name, net, subNet, universe);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoTempoRange")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			const char *tempoRange = json_object_get_string(js);
			if (!tempoRange)
				return "Unknown";
			if (strcmp(tempoRange, "andante") == 0)
				return "70–110 BPM";
			else if (strcmp(tempoRange, "moderato") == 0)
				return "100–140 BPM";
			else if (strcmp(tempoRange, "allegro") == 0)
				return "120–180 BPM";
			else if (strcmp(tempoRange, "presto") == 0)
				return "170–250 BPM";
			else if (strcmp(tempoRange, "prestissimo") == 0)
				return "220–320 BPM";
		}
		if (getDataType()->getModuleKey()=="VuoEdgeBlend")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());

			double cutoff = 0, gamma = 0, crop = 0;
			json_object *o = NULL;

			if (json_object_object_get_ex(js, "cutoff", &o))
				cutoff = json_object_get_double(o);

			if (json_object_object_get_ex(js, "gamma", &o))
				gamma = json_object_get_double(o);

			if (json_object_object_get_ex(js, "crop", &o))
				crop = json_object_get_double(o);

			json_object_put(js);

			double cropPercent = -crop * 100;
			double cutoffPercent = cutoff * 100;
			if (VuoReal_areEqual(crop, 0) && VuoReal_areEqual(cutoff, 0))
				return "≡";
			else if (VuoReal_areEqual(cutoff, 0))
				return format("%.0f%%", cropPercent);
			else if (VuoReal_areEqual(gamma, 1))
				return format("%.0f%% %.0f%%", cropPercent, cutoffPercent);
			else
				return format("%.0f%% %.0f%% @ %.2gγ", cropPercent, cutoffPercent, gamma);
		}
		if (getDataType()->getModuleKey()=="VuoRange")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());

			double minimum = VuoRange_NoMinimum, maximum = VuoRange_NoMaximum;

			json_object *o = NULL;

			if (json_object_object_get_ex(js, "minimum", &o))
				minimum = json_object_get_double(o);

			if (json_object_object_get_ex(js, "maximum", &o))
				maximum = json_object_get_double(o);

			json_object_put(js);

			if (minimum != VuoRange_NoMinimum && maximum != VuoRange_NoMaximum)
				return format("%.4g to %.4g", minimum, maximum);
			else if (minimum != VuoRange_NoMinimum)
				return format("%.4g to ∞", minimum);
			else if (maximum != VuoRange_NoMaximum)
				return format("-∞ to %.4g", maximum);
			else
				return format("-∞ to ∞");
		}
		if (getDataType()->getModuleKey()=="VuoIntegerRange")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());

			VuoInteger minimum = VuoIntegerRange_NoMinimum, maximum = VuoIntegerRange_NoMaximum;

			json_object *o = NULL;

			if (json_object_object_get_ex(js, "minimum", &o))
				minimum = json_object_get_int64(o);

			if (json_object_object_get_ex(js, "maximum", &o))
				maximum = json_object_get_int64(o);

			json_object_put(js);

			if (minimum != VuoIntegerRange_NoMinimum && maximum != VuoIntegerRange_NoMaximum)
				return format("%lld to %lld", minimum, maximum);
			else if (minimum != VuoIntegerRange_NoMinimum)
				return format("%lld to ∞", minimum);
			else if (maximum != VuoIntegerRange_NoMaximum)
				return format("-∞ to %lld", maximum);
			else
				return format("-∞ to ∞");
		}
		if (getDataType()->getModuleKey() == "VuoAnchor")
		{
			VuoAnchor value = VuoAnchor_makeFromString(getConstantAsString().c_str());
			return stringAndFree(VuoAnchor_getSummary(value));
		}
		if (getDataType()->getModuleKey() == "VuoTextComparison")
		{
			VuoTextComparison value = VuoTextComparison_makeFromString(getConstantAsString().c_str());
			return stringAndFree(VuoTextComparison_getSummary(value));
		}
		if (getDataType()->getModuleKey() == "VuoBlackmagicInputDevice"
		 || getDataType()->getModuleKey() == "VuoBlackmagicOutputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o;
			if (json_object_object_get_ex(js, "name", &o))
				return json_object_get_string(o);
			else
				return "First";
		}
		if (getDataType()->getModuleKey() == "VuoSpeechVoice")
		{
			VuoSpeechVoice value = VuoSpeechVoice_makeFromString(getConstantAsString().c_str());
			return stringAndFree(VuoSpeechVoice_getSummary(value));
		}
		if (getDataType()->getModuleKey() == "VuoRectangle")
		{
			VuoRectangle value = VuoRectangle_makeFromString(getConstantAsString().c_str());
			return stringAndFree(VuoRectangle_getSummary(value));
		}
		if (getDataType()->getModuleKey() == "VuoNdiSource")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o, *o2;
			if (json_object_object_get_ex(js, "name", &o))
				return json_object_get_string(o);
			else if (json_object_object_get_ex(js, "ipAddress", &o)
				  && json_object_object_get_ex(js, "port", &o2))
				return format("%s:%lld", json_object_get_string(o), json_object_get_int(o2));
			else if (json_object_object_get_ex(js, "ipAddress", &o))
				return json_object_get_string(o);
			else
				return "First";
		}
	}

	// If it's a JSON string (e.g., VuoText or an enum identifier), unescape and optionally capitalize it.
	json_object *js = json_tokener_parse(getConstantAsString().c_str());
	if (json_object_get_type(js) == json_type_string)
	{
		string textWithoutQuotes = json_object_get_string(js);
		json_object_put(js);

		// Show linebreaks as a glyph (rather than causing the following text to move to the next line, which gets cut off).
		VuoStringUtilities::replaceAll(textWithoutQuotes, "\n", "⏎");

		string type;
		if (getDataType())
			type = getDataType()->getModuleKey();

		// Leave text as-is.
		if (type == "VuoText"
		 || type == "VuoImageFormat"
		 || type == "VuoAudioEncoding"
		 || type == "VuoBlackmagicConnection"
		 || type == "VuoBlackmagicVideoMode"
		 || type == "VuoMovieImageEncoding")
			return textWithoutQuotes;

		// All-caps.
		if (type == "VuoTableFormat")
		{
			std::transform(textWithoutQuotes.begin(), textWithoutQuotes.end(), textWithoutQuotes.begin(), ::toupper);
			return textWithoutQuotes;
		}

		// Convert hyphenations to camelcase.
		// Example: VuoTimeFormat
		for (auto it = textWithoutQuotes.begin(); it != textWithoutQuotes.end();)
			if (*it == '-')
			{
				textWithoutQuotes.erase(it);
				*it = toupper(*it);
			}
			else
				++it;

		return VuoStringUtilities::expandCamelCase(textWithoutQuotes);
	}
	json_object_put(js);

	return getConstantAsString();
}

/**
 * Sets this port's constant data value to that represented by the provided @c constantValue string.
 */
void VuoRendererPort::setConstant(string constantValue)
{
	VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>(getBase()->getCompiler());
	if (eventPort)
	{
		QGraphicsItem::CacheMode normalCacheMode = cacheMode();
		setCacheMode(QGraphicsItem::NoCache);
		updateGeometry();

		eventPort->getData()->setInitialValue(constantValue);

		if (getConstantAsStringToRender() != getConstantAsTruncatedStringToRender())
			setToolTip(QString("<span></span>") + getConstantAsStringToRender().c_str());
		else
			setToolTip("");

		setCacheMode(normalCacheMode);
		updateCachedPortPath();
		updateCachedBoundingRect();
		getRenderedParentNode()->layoutConnectedInputDrawersAtAndAbovePort(eventPort->getBase()->getRenderer());

		// Ensure this node's cable paths are updated to escape the new constant's flag.
		set<VuoCable *> cables = getRenderedParentNode()->getConnectedInputCables(true);
		for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
		{
			(*i)->getRenderer()->setPortConstantsChanged();
			(*i)->getRenderer()->updateGeometry();
		}
	}
}

/**
  * Returns a boolean indicating whether the port name should be rendered along with this port,
  * taking into account the port's own attributes as well as whether the port will be
  * rendered within a published port sidebar.
  */
bool VuoRendererPort::portNameRenderingEnabled() const
{
	bool sidebarPaintMode = dynamic_cast<const VuoRendererPublishedPort *>(this);
	string name = getPortNameToRender();
	VuoRendererNode *parent = getRenderedParentNode();

	if (name.empty() || isAnimated)
		return false;
	else if (parent && parent->isMissingImplementation())
		return false;
	else if (sidebarPaintMode)
		return true;
	else if (isRefreshPort || isFunctionPort || typecastParentPort)
		return false;

	return true;
}

/**
  * Given a list of coordinates, returns the string representation of the point
  * consisting of those coordinate values as it should be rendered within a
  * constant data flag.
  *
  * Helper function for @c VuoRendererPort::getConstantAsStringToRender().
  */
string VuoRendererPort::getPointStringForCoords(QList<float> coordList) const
{
	const QString coordSeparator = QString(QLocale::system().decimalPoint() != ','? QChar(',') : QChar(';')).append(" ");
	QStringList coordStringList;

	foreach (float coord, coordList)
		coordStringList.append(getStringForRealValue(coord).c_str());

	QString pointString = QString("(").append(coordStringList.join(coordSeparator).append(")"));
	return pointString.toStdString();
}

/**
  * Given a real number, returns the string representation of the number
  * as it should be rendered within a constant data flag.
  */
string VuoRendererPort::getStringForRealValue(double value) const
{
	// See VuoDoubleSpinBox::textFromValue.
	QString valueAsStringInUserLocale = QLocale::system().toString(value, 'g', 11);
	if (qAbs(value) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale.toStdString();
}

/**
  * Given a real number, returns the string representation of the number
  * as it should be rendered within a constant data flag.
  */
string VuoRendererPort::getStringForRealValue(float value) const
{
	// Like the `double` version, but reduced to 7 so we don't display bogus precision when rendering VuoPoint*d (whose components are `float`, not `double` like VuoReal).
	QString valueAsStringInUserLocale = QLocale::system().toString(value, 'g', 7);
	if (qAbs(value) >= 1000.0)
		valueAsStringInUserLocale.remove(QLocale::system().groupSeparator());

	return valueAsStringInUserLocale.toStdString();
}

/**
 * Returns a boolean indicating whether this port is publishable.
 */
bool VuoRendererPort::getPublishable() const
{
	// @todo: Allow generic published ports (https://b33p.net/kosada/node/7655).
	bool isGeneric = bool(dynamic_cast<VuoGenericType *>(this->getDataType()));

	// @todo: Allow published dictionary ports (https://b33p.net/kosada/node/8524).
	bool hasDictionaryType = (this->getDataType() && VuoStringUtilities::beginsWith(this->getDataType()->getModuleKey(), "VuoDictionary_"));

	// @todo: Allow published math expression ports for "Calculate" nodes (https://b33p.net/kosada/node/8550).
	bool isMathExpressionInputToCalculateNode = (this->getDataType() &&
												 (this->getDataType()->getModuleKey() == "VuoMathExpressionList") &&
												 this->getUnderlyingParentNode() &&
												 VuoStringUtilities::beginsWith(this->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.math.calculate"));


	// @todo: Allow direct connections between external published inputs and external published outputs
	// (https://b33p.net/kosada/node/7756).
	return (!isGeneric && !hasDictionaryType && !isMathExpressionInputToCalculateNode && !dynamic_cast<const VuoRendererPublishedPort *>(this));
}

/**
 * Returns a vector of pointers to the externally visible published ports
 * connected to this port.
 */
vector<VuoRendererPublishedPort *> VuoRendererPort::getPublishedPorts(void) const
{
	vector <VuoRendererPublishedPort *> publishedPorts;
	foreach (VuoCable *cable, getBase()->getConnectedCables(true))
	{
		if (getInput() && cable->isPublishedInputCable())
			publishedPorts.push_back(dynamic_cast<VuoRendererPublishedPort *>(cable->getFromPort()->getRenderer()));
		else if (getOutput() && cable->isPublishedOutputCable())
			publishedPorts.push_back(dynamic_cast<VuoRendererPublishedPort *>(cable->getToPort()->getRenderer()));
	}

	return publishedPorts;
}

/**
 * Returns a vector of pointers to the externally visible published ports
 * connected to this port by data-carrying cables.
 */
vector<VuoRendererPublishedPort *> VuoRendererPort::getPublishedPortsConnectedByDataCarryingCables(void) const
{
	vector <VuoRendererPublishedPort *> publishedPorts;
	foreach (VuoCable *cable, getBase()->getConnectedCables(true))
	{
		if (getInput() && cable->isPublishedInputCable() && cable->getRenderer()->effectivelyCarriesData())
			publishedPorts.push_back(dynamic_cast<VuoRendererPublishedPort *>(cable->getFromPort()->getRenderer()));
		else if (getOutput() && cable->isPublishedOutputCable() && cable->getRenderer()->effectivelyCarriesData())
			publishedPorts.push_back(dynamic_cast<VuoRendererPublishedPort *>(cable->getToPort()->getRenderer()));
	}

	return publishedPorts;
}

/**
 * Resets the time that the last event was fired to a value that causes
 * the port to be painted as if activity-rendering were disabled.
 */
void VuoRendererPort::resetTimeLastEventFired()
{
	this->timeLastEventFired = VuoRendererColors::getVirtualFiredEventOrigin();
}

/**
 * Updates the port's state to indicate that it has just fired an event.
 */
void VuoRendererPort::setFiredEvent()
{
	this->timeLastEventFired = QDateTime::currentMSecsSinceEpoch();
}

/**
 * Updates the port's state to indicate that it fired an event at such a time
 * that its fade percentage should now be equal to @c percentage.
 */
void VuoRendererPort::setFadePercentageSinceEventFired(qreal percentage)
{
	this->timeLastEventFired = VuoRendererColors::getVirtualFiredEventOriginForAnimationFadePercentage(percentage);
}

/**
 * Returns the 'Show Events'-mode animations associated with this port.
 */
vector<QGraphicsItemAnimation *> VuoRendererPort::getAnimations()
{
	return this->animations;
}

/**
 * Sets the boolean indicating whether this port is an animation, and not
 * itself a component of the base composition.
 */
void VuoRendererPort::setAnimated(bool animated)
{
	this->isAnimated = animated;
	updateEnabledStatus();
}

/**
 * Sets the cache mode of this port, and any child ports, to @c mode.
 */
void VuoRendererPort::setCacheModeForPortAndChildren(QGraphicsItem::CacheMode mode)
{
	this->setCacheMode(mode);

	VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(this);
	if (typecastPort)
		typecastPort->getChildPort()->setCacheMode(mode);
}

/**
 * Determines whether this port will accept mouse events based on
 * the port's current attributes.
 */
void VuoRendererPort::updateEnabledStatus()
{
	// Port animations, and ports without compilers, shouldn't accept mouse events.
	setEnabled(!isAnimated &&
			   ((getBase()->hasCompiler() && getBase()->getClass()->hasCompiler()) ||
				dynamic_cast<VuoRendererPublishedPort *>(this)) &&
			   !isHiddenRefreshPort());
}

/**
 * Returns a string representation of the regular expression that describes valid
 * port identifiers.
 *
 * See also VuoRendererPort::sanitizePortIdentifier(QString portID).
 */
QString VuoRendererPort::getPortIdentifierRegExp()
{
	// A published port name must:
	// - Contain only alphanumeric characters; and
	// - Either be entirely numeric or begin with an alphabetic character; and
	// - Have a total length of 1-31 characters.
	return QString("[A-Za-z][A-Za-z0-9]{0,30}")
			.append("|")
			.append("[0-9]{1,31}");
}

/**
 * Sanitizes the provided @c portID to meet the requirements of a
 * valid port identifier. Sanitizes only by removing characters, never adding,
 * so it is possible for sanitization to fail, in which case this function
 * returns the empty string.
 *
 * See also VuoRendererPort::getPortIdentifierRegExp().
 */
QString VuoRendererPort::sanitizePortIdentifier(QString portID)
{
	// Remove non-alphanumeric characters.
	portID.remove(QRegExp("[^A-Za-z0-9]"));

	// Unless the identifier is purely numeric, remove non-alphabetic first characters.
	if (!portID.contains(QRegExp("^[0-9]+$")))
	{
		while (!portID.isEmpty() && !portID.contains(QRegExp("^[A-Za-z]")))
			portID = portID.right(portID.size()-1);
	}

	// Remove characters beyond the 31st.
	portID = portID.left(31);

	return portID;
}

VuoRendererPort::~VuoRendererPort()
{
	foreach (QGraphicsItemAnimation *animation, animations)
		animation->clear();

	animations.clear();
}
