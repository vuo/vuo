/**
 * @file
 * VuoRendererPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoStringUtilities.hh"

#include "VuoRendererComposition.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererSignaler.hh"

#include "VuoCompilerCable.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerTriggerPort.hh"

#include "VuoGenericType.hh"

extern "C" {
#include "VuoColor.h"
#include "VuoImage.h"
#include "VuoScreen.h"
#include "VuoTransform.h"
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
	#include <json-c/json.h>
#pragma clang diagnostic pop
#include <sstream>

#include "VuoPort.hh"

#include "VuoHeap.h"

const qreal VuoRendererPort::portRadius = VuoRendererFonts::thickPenWidth*0.3625;
const qreal VuoRendererPort::portSpacing = VuoRendererFonts::thickPenWidth*3.0/4.0;
const qreal VuoRendererPort::portContainerMargin = VuoRendererFonts::thickPenWidth / 6.;
const qreal VuoRendererPort::portInset = 1.4;
const qreal VuoRendererPort::portInsetTriangular = 2.4;
const qreal VuoRendererPort::constantFlagHeight = VuoRendererFonts::thickPenWidth*0.6;

/**
 * Creates a renderer detail for the specified base port.
 */
VuoRendererPort::VuoRendererPort(VuoPort * basePort, VuoRendererSignaler *signaler,
								 bool isOutput, bool isRefreshPort, bool isFunctionPort)
	: VuoBaseDetail<VuoPort>("VuoRendererPort standard", basePort)
{
	getBase()->setRenderer(this);

	this->signaler = signaler;

	this->isOutput = isOutput;
	this->isRefreshPort = isRefreshPort;
	this->isFunctionPort = isFunctionPort;
	this->isEligibleForDirectConnection = false;
	this->isEligibleForConnectionViaTypecast = false;
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

	setFlag(QGraphicsItem::ItemIsFocusable, true);  // allow delivery of key events
	setAcceptHoverEvents(true);  // allow delivery of mouse-hover events
	updateNameRect();
	updateEnabledStatus();

	// Create a hybrid rect having the width of the port's inset rect and the customized
	// height of a constant flag, so that the constant flag has the desired height but
	// directly adjoins the inset port shape.
	QRectF portInnerRect = VuoRendererPort::getPortPath(getInset()).boundingRect();
	this->portHybridRect = QRectF(portInnerRect.x(),
								   -0.5*VuoRendererPort::constantFlagHeight,
								   portInnerRect.width(),
								   VuoRendererPort::constantFlagHeight);
	updatePortConstantPath();
}

/**
 * Calculates and updates the cached constant path for this port based on its current attributes.
 */
void VuoRendererPort::updatePortConstantPath()
{
	QString constantText = QString::fromUtf8(getConstantAsStringToRender().c_str());
	this->portConstantInsetPath = getPortConstantPath(this->portHybridRect, constantText, &this->portConstantPath);
}

/**
 * Returns a path representing the frame of a port constant.
 * If @c outsetPath is not NULL, it is given a path that can be stroked with a pen of width 1 without overlapping the returned path.
 */
QPainterPath VuoRendererPort::getPortConstantPath(QRectF innerPortRect, QString text, QPainterPath *outsetPath, bool isTypecast)
{
	qreal portConstantTextMargin = VuoRendererFonts::thickPenWidth*3./20. + (isTypecast ? 2 : 0);
	qreal innerOffsetForTypecast = isTypecast ? 1. : 0.;
	qreal outerCornerRadius = VuoRendererFonts::thickPenWidth/8.0;
	qreal innerCornerRadius = outerCornerRadius - 1;

	QPainterPath p;
	QRectF textRect = getPortConstantTextRectForText(text);

	QPointF topLeftCorner(textRect.x() - portConstantTextMargin - (isTypecast ? 1. : 0.), innerPortRect.y() + innerOffsetForTypecast);
	QPointF topRightCorner(innerPortRect.x() - innerPortRect.width()/2 - portInset, innerPortRect.y() + innerOffsetForTypecast);
	QPointF rightPoint(innerPortRect.x() - portInset, innerPortRect.center().y());
	QPointF bottomRightCorner(innerPortRect.x() - innerPortRect.width()/2. - portInset, innerPortRect.bottom() - innerOffsetForTypecast);
	QPointF bottomLeftCorner(textRect.x() - portConstantTextMargin - (isTypecast ? 1. : 0.), innerPortRect.bottom() - innerOffsetForTypecast);

	p.moveTo(topRightCorner);
	p.lineTo(rightPoint);
	p.lineTo(bottomRightCorner);
	addRoundedCorner(p, true, bottomLeftCorner, innerCornerRadius, false, true);
	addRoundedCorner(p, true, topLeftCorner, innerCornerRadius, true, true);
	p.lineTo(topRightCorner);

	if (outsetPath)
	{
		QPointF outsetTopLeftCorner(textRect.x() - portConstantTextMargin - (isTypecast ? 1. : 0.) - 1., innerPortRect.y() - 1.);
		QPointF outsetTopRightCorner(innerPortRect.x() - innerPortRect.width()/2. - portInset + .2, innerPortRect.y() - 1.);
		QPointF outsetRightPoint(innerPortRect.x() + .2, innerPortRect.center().y());
		QPointF outsetBottomRightCorner(innerPortRect.x() - innerPortRect.width()/2. - portInset + .2, innerPortRect.bottom() + 1.);
		QPointF outsetBottomLeftCorner(textRect.x() - portConstantTextMargin - (isTypecast ? 1. : 0.) - 1., innerPortRect.bottom() + 1.);

		*outsetPath = QPainterPath();
		outsetPath->moveTo(outsetTopRightCorner);
		outsetPath->lineTo(outsetRightPoint);
		outsetPath->lineTo(outsetBottomRightCorner);
		addRoundedCorner(*outsetPath, true, outsetBottomLeftCorner, outerCornerRadius, false, true);
		addRoundedCorner(*outsetPath, true, outsetTopLeftCorner, outerCornerRadius, true, true);
		outsetPath->lineTo(outsetTopRightCorner);
	}

	return p;
}

/**
 * Returns the inset that should be passed to @ref getPortPath.
 */
qreal VuoRendererPort::getInset(void) const
{
	if (getDataType() || isRefreshPort)
		return portInset;

	return portInsetTriangular;
}

/**
 * Returns a closed path representing the port's circle/triangle.  Does not include constant flag (see @c getPortConstantPath).
 */
QPainterPath VuoRendererPort::getPortPath(qreal inset) const
{
	bool carriesData = (getBase()->getClass()->hasCompiler() &&
						((VuoCompilerPortClass *)getBase()->getClass()->getCompiler())->getDataVuoType());

	return getPortPath(inset,
					   getBase()->getClass()->getPortType(),
					   getInput(),
					   carriesData
					   );
}

/**
 * Returns a closed path representing the circle/triangle for a port with type @c portType.
 * Does not include constant flag (see @c getPortConstantPath).
 */
QPainterPath VuoRendererPort::getPortPath(qreal inset,
										  VuoPortClass::PortType portType,
										  bool isInputPort,
										  bool carriesData
										 )
{
	QPainterPath p;
	QRectF outerPortRect = getPortRect();
	QRectF innerPortRect = outerPortRect.adjusted(inset,inset,-inset,-inset);

	if (carriesData)
		// Circle.
		p.addEllipse(innerPortRect);
	else
	{
		// Triangle.

		// Align the port rect to integer pixels, so lines are sharp.
		outerPortRect = outerPortRect.toRect();

		// Move right 1 pixel, so the triangle's center-of-gravity better matches circular ports.
		outerPortRect = outerPortRect.adjusted(1,0,1,0);

		// Adjust the inset so that the amount of whitespace padding the triangle's
		// vertical left edge matches the amount of whitespace padding its diagonal edges.
		qreal h = outerPortRect.height();
		qreal w = outerPortRect.width();
		qreal triangleLeftInset = (inset*(w-0.5*h))/sqrt(pow(0.5*h,2)+pow(w,2));

		// Round the left inset to the nearest whole pixel to eliminate blur for input ports.
		if (isInputPort)
			triangleLeftInset = qRound(triangleLeftInset);

		qreal triangleVerticalInset = inset-(0.5*h/w)*(inset-triangleLeftInset);

		innerPortRect = outerPortRect.adjusted(triangleLeftInset, triangleVerticalInset, -inset, -triangleVerticalInset);

		p.moveTo(innerPortRect.right(), innerPortRect.center().y());
		p.lineTo(innerPortRect.topLeft());
		p.lineTo(innerPortRect.bottomLeft());
		p.closeSubpath();
	}

	return p;
}


/**
 * Returns a path representing the glyph for function ports.
 */
QPainterPath VuoRendererPort::getFunctionPortGlyph(void) const
{
	// Started with the "f" glyph from Signika, then mirrored the top to the bottom using TouchDraw (see functionPort.t2d),
	// then converted the SVG points manually to QPainterPath statements.
	QPainterPath p;
	p.moveTo(4.43432694097478,7.85885865921973);
	p.cubicTo(4.54036026626481,8.63606869973077,4.74441887553019,9.56484019503971,4.75274533523895,10.1928227448389);
	p.cubicTo(4.72128346257729,11.5340886975457,4.07231883670452,11.9700122913261,3.0456930331924,11.9990782089261);
	p.cubicTo(2.48601155026458,12.0156314445099,1.53237634536436,11.6260438763205,0.942586240509103,11.5907867879313);
	p.cubicTo(0.691548665364582,12.0521443684896,0.443317511968114,12.628296671768,0.447553573494685,13.2469575591589);
	p.cubicTo(1.12910328803351,13.4954516035728,2.61869803131267,13.8462628915561,3.48793785656784,13.8437212530216);
	p.cubicTo(5.53069082864804,13.8377482873112,7.17526649001644,12.3276012820681,7.18981660610081,10.1534969961992);
	p.cubicTo(7.16594715465797,9.45320361017714,6.983790429683,8.62373810487953,6.88077734147409,7.85885865921973);
	p.lineTo(8.86491502201942,7.8528224278457);
	p.cubicTo(8.93434118668425,7.47692149366752,8.94681182167128,7.16303272511104,8.94430567454254,6.84815303891249);
	p.cubicTo(8.9422358996167,6.58810043794526,8.89028454897106,6.27550508860728,8.82579627591599,6.02535784537993);
	p.lineTo(6.71122119472517,6.09063125766457);
	p.cubicTo(6.61433216601297,5.38818431265963,6.54166550997983,4.6857373676547,6.54166550997983,4.05595804896617);
	p.cubicTo(6.54166550997983,2.74795371137328,7.21988917296796,2.02128576427494,8.33411415816619,2.02128576427494);
	p.cubicTo(9.01233782115432,2.02128576427494,9.78745097485828,2.19084109499369,10.4172298924888,2.3846201998277);
	p.cubicTo(10.7805640966619,1.99706199016,10.9985649887684,1.27039311905448,10.9985649887684,0.689059500581475);
	p.cubicTo(10.2476746697465,0.398390843330462,8.98811544847555,0.13194596019471,7.89811283595644,0.13194596019471);
	p.cubicTo(5.52432955349385,0.13194596019471,4.16788176551395,1.75483903312481,4.16788176551395,3.91062464434798);
	p.cubicTo(4.16788176551395,4.58884873924487,4.240548652549,5.31551761035038,4.31321553958377,6.06640840755649);
	p.lineTo(2.35121074464882,6.04218648145591);
	p.cubicTo(2.27854391536436,6.35707614080034,2.25432162931124,6.64774387404386,2.25432162931124,6.96263353338827);
	p.cubicTo(2.25432162931124,7.27752319273238,2.27854391536436,7.59241285207678,2.35121074464882,7.90730343542837);
	p.closeSubpath();

	// Center
	p.translate(-p.boundingRect().center());

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
		if (type == VuoPortClass::dataAndEventPort)
		{
			// Circular port
			barrierRect = getPortRect().adjusted(-1.5,-1.5,1.5,1.5);
		}

		else if (type == VuoPortClass::eventOnlyPort)
		{
			// Triangular port
			barrierRect = getPortRect().adjusted(-3.5,-3.5,3.5,3.5);
		}

		else if (type == VuoPortClass::triggerPort)
		{
			// Exploding port

			bool carriesData = (getBase()->getClass()->hasCompiler() &&
								((VuoCompilerPortClass *)getBase()->getClass()->getCompiler())->getDataVuoType());

			if (carriesData)
			{
				barrierRect = getPortRect().adjusted(-1.5,-1.5,1.5,1.5);;
			}
			else
			{
				// Triangular port
				barrierRect = getPortRect().adjusted(-.5,2.5,.5,-1.5);
			}
		}
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
	qreal cableWidth, cableHighlightWidth, cableHighlightOffset;
	VuoRendererCable::getCableSpecs(paintDataAntenna, cableWidth, cableHighlightWidth, cableHighlightOffset);

	const qreal mastLength = 15;
	QPointF startPoint = (getInput()? -QPointF(mastLength - (paintDataAntenna ? 0.5 : 0 ), 0) : QPointF(0,0));
	QPointF endPoint = (getInput()? QPointF(0,0) : QPointF(mastLength + (paintDataAntenna ? 0.5 : 0 ), 0));

	QPainterPath mastPath = VuoRendererCable::getCablePathForEndpoints(startPoint, endPoint);

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
			return VuoRendererColors::getActiveProtocolTint(0);
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
				getPortConstantTextRectForText(QString::fromUtf8(getConstantAsStringToRender().c_str())) :
				QRectF());
}

/**
 * Returns a rect enclosing the specified @c text.
 */
QRectF VuoRendererPort::getPortConstantTextRectForText(QString text)
{
	qreal textWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodePortConstantFont()).boundingRect(QRectF(0,0,0,0), Qt::TextIncludeTrailingSpaces, text).width()+1.0;
	QRectF textRect(
		-textWidth - getPortRect().width(),
		-VuoRendererFonts::thickPenWidth/3.0 + 1,
		textWidth,
		VuoRendererFonts::thickPenWidth
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
		(isOutput? -VuoRendererFonts::thickPenWidth/2.0 - textSize.width() - 2. :
										 VuoRendererFonts::thickPenWidth/2.0
											 - VuoRendererFonts::getHorizontalOffset(font, text)
											 + 3.
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
	VuoRendererNode *renderedParentNode = getRenderedParentNode();
	if (renderedParentNode && renderedParentNode->paintingDisabled())
		return QRectF();

	QRectF r = getPortPath(1.5).boundingRect();

	r = r.united(getEventBarrierRect());

	if (portNameRenderingEnabled())
		r = r.united(getNameRect());

	if (hasPortAction())
		r = r.united(getActionIndicatorRect());

	if (isConstant())
		r = r.united(this->portConstantPath.controlPointRect());

	r = r.united(getWirelessAntennaPath().boundingRect());

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
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
		painter->setPen(QPen(eventBlockingBarrierColor, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

		if (type == VuoPortClass::dataAndEventPort)
		{
			// Circular port
			if (eventBlocking == VuoPortClass::EventBlocking_Wall)
				painter->drawArc(barrierRect,40*16,-80*16);
			else // VuoPortClass::EventBlocking_Door
			{
				painter->drawArc(barrierRect,40*16,-15*16);
				painter->drawArc(barrierRect,-40*16,15*16);
			}
		}
		else if (type == VuoPortClass::eventOnlyPort)
		{
			// Triangular port
			QLineF topLine = QLineF(barrierRect.topLeft(),QPointF(barrierRect.right(),barrierRect.center().y()));
			QLineF bottomLine = QLineF(barrierRect.bottomLeft(),QPointF(barrierRect.right(),barrierRect.center().y()));

			if (eventBlocking == VuoPortClass::EventBlocking_Wall)
			{
				QPainterPath p;
				p.moveTo(topLine.pointAt(0.65));
				p.lineTo(topLine.p2());
				p.lineTo(bottomLine.pointAt(0.65));
				painter->drawPath(p);
			}
			else // VuoPortClass::EventBlocking_Door
			{
				QPainterPath p;
				p.moveTo(topLine.pointAt(0.65));
				p.lineTo(topLine.pointAt(0.75));
				p.moveTo(bottomLine.pointAt(0.75));
				p.lineTo(bottomLine.pointAt(0.65));
				painter->drawPath(p);
			}
		}
		else if (type == VuoPortClass::triggerPort)
		{
			// Exploding port

			bool carriesData = (getBase()->getClass()->hasCompiler() &&
								((VuoCompilerPortClass *)getBase()->getClass()->getCompiler())->getDataVuoType());

			if (carriesData)
			{
				painter->drawArc(barrierRect,145*16,70*16);
			}
			else
			{
				// Triangular port

				QPainterPath p;
				p.moveTo(barrierRect.topLeft());
				p.lineTo(barrierRect.bottomLeft());
				painter->drawPath(p);
			}
		}
	}

#if 0
	{
		// Draw the function port's glyph
		painter->save();
		painter->scale(.6,.6);
		painter->fillPath(getFunctionPortGlyph(), colors->nodeTitle());
		painter->restore();
	}
#endif
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
	const qreal triangleHeight = 6;
	const qreal triangleWidth = 5;
	qreal triangleLeft = qRound( getNameRect().right() + marginFromPortName );
	qreal triangleTop = qRound( getNameRect().bottom() - fontMetrics.descent() - fontMetrics.xHeight());

	return QRectF(triangleLeft, triangleTop, triangleWidth, triangleHeight);
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
		p.moveTo(rect.topLeft());
		p.lineTo(rect.bottomLeft());
		p.lineTo(rect.right(), rect.center().y());
		p.closeSubpath();

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
													   ((publishedPort && publishedPort->isSelected() && !publishedPort->getCurrentlyActive())?
														  VuoRendererColors::sidebarSelection :
														  VuoRendererColors::noSelection));

	bool isHovered = isEligibleForSelection;
	VuoRendererColors::HighlightType highlightType = (isEligibleForDirectConnection? VuoRendererColors::standardHighlight :
												(isEligibleForConnectionViaTypecast? VuoRendererColors::subtleHighlight :
																					 VuoRendererColors::noHighlight));

	VuoPortClass::PortType type = getBase()->getClass()->getPortType();
	bool isTriggerPort = (type == VuoPortClass::triggerPort);

	qint64 timeOfLastActivity =		((! getRenderActivity())? VuoRendererItem::notTrackingActivity :
									(isTriggerPort? timeLastEventFired :
									(getTypecastParentPort()? static_cast<VuoRendererTypecastPort *>(getTypecastParentPort())->getUncollapsedTypecastNode()->getTimeLastExecutionEnded() :
									(renderedParentNode? renderedParentNode->getTimeLastExecutionEnded() :
									VuoRendererItem::notTrackingActivity))));

	VuoRendererColors *colors = new VuoRendererColors(getPortTint(),
													  selectionType,
													  isHovered,
													  highlightType,
													  timeOfLastActivity);
	VuoRendererColors *antennaColors = new VuoRendererColors((getWirelessAntennaTint()),
													  selectionType,
													  isHovered,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);

	// Draw the port circle / constant flag
	QPainterPath portPath = getPortPath(getInset());

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

	painter->fillPath(portPath, portBrush);

	if(isConstant())
	{
		// Display a color swatch for VuoColor data.
		/// @todo Implement with input viewers (https://b33p.net/kosada/node/5700)
		bool isColorPort = getDataType() && getDataType()->getModuleKey()=="VuoColor";
		if (isColorPort)
		{
			// Paint an opaque 4-layer color swatch:
			//   _________
			//  /  ______ \
			// |  |    / \  \
			// |  |  /    ⟩  ⟩
			// |  |/_____/  /
			//  \_________/
			// (Provide an opaque background, so help distinguish transparent colors from opaque colors.)


			// First, fill the outermost area to match the node frame (when in Light Interface Mode) :
			painter->fillPath(this->portConstantPath, colors->nodeFrame());


			// Second, fill the inner area with solid black (when in Light Interface Mode) :
			bool isDark = colors->isDark();
			painter->fillPath(this->portConstantInsetPath, isDark ? Qt::white : Qt::black);


			// Third, fill the bottom-right half of the inner area with solid white (when in Light Interface Mode) :
			{
				QPainterPath p = this->portConstantInsetPath;
				// getPortConstantPath() is assumed to give us points in this order:
				QPointF topRight = p.elementAt(0);
				QPointF rightTip = p.elementAt(1);
				QPointF bottomRight = p.elementAt(2);
				QPointF bottomLeftRoundedCornerBegin = p.elementAt(3);
				QPointF bottomLeftRoundedCornerEnd = p.elementAt(6);
				QPointF topLeftRoundedCornerBegin = p.elementAt(7);
				QPointF topLeftRoundedCornerEnd = p.elementAt(10);

				QPointF topLeftSharpEdge(topLeftRoundedCornerBegin.x(), topLeftRoundedCornerEnd.y());
				QPointF bottomLeftSharpEdge(bottomLeftRoundedCornerEnd.x(), bottomLeftRoundedCornerBegin.y());

				QPainterPath bottomHalf;
				bottomHalf.moveTo(bottomLeftSharpEdge);
				bottomHalf.lineTo(topRight);
				bottomHalf.lineTo(rightTip);
				bottomHalf.lineTo(bottomRight);
				bottomHalf.closeSubpath();
				painter->setClipPath(p);
				painter->fillPath(bottomHalf, isDark ? Qt::black : Qt::white);
				painter->setClipping(false);
			}


			// Fourth, fill the entire inner area with the (possibly-transparent) color:
			string colorRGBAJsonString = getConstantAsString();
			VuoColor c = VuoColor_makeFromString(colorRGBAJsonString.c_str());
			painter->fillPath(this->portConstantInsetPath, QColor(c.r*255, c.g*255, c.b*255, c.a*255));
		}
		else
		{
			QString constantText = QString::fromUtf8(getConstantAsStringToRender().c_str());
			QBrush constantFlagBackgroundBrush = colors->constantFill();

			// Constant flag
			painter->fillPath(this->portConstantPath, constantFlagBackgroundBrush);

			// Constant string
			QRectF textRect = getPortConstantTextRectForText(constantText);
			painter->setPen(colors->constantText());
			painter->setFont(VuoRendererFonts::getSharedFonts()->nodePortConstantFont());
			painter->drawText(textRect, Qt::AlignLeft, constantText);
		}
	}

	paintEventBarrier(painter, colors);
	paintPortName(painter, colors);
	paintActionIndicator(painter, colors);
	paintWirelessAntenna(painter, antennaColors);

	delete colors;
	delete antennaColors;
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
	return isEligibleForDirectConnection || isEligibleForConnectionViaTypecast;
}

/**
 * Sets the boolean indicating whether this port is eligible
 * for direct connection to the cable currently being dragged between ports.
 */
void VuoRendererPort::setEligibleForDirectConnection(bool eligible)
{
	this->isEligibleForDirectConnection = eligible;
}

/**
 * Sets the boolean indicating whether this port is eligible
 * for typecast-assisted connection to the cable currently being dragged between ports.
 */
void VuoRendererPort::setEligibleForConnectionViaTypecast(bool eligible)
{
	this->isEligibleForConnectionViaTypecast = eligible;
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
	isEligibleForSelection = (cableDragUnderway? (isEligibleForDirectConnection || isEligibleForConnectionViaTypecast) : true);

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
	bool toPortIsEnabledInput = (toPort->getInput() && toPort->isEnabled());

	if (fromPortIsEnabledOutput && toPortIsEnabledInput)
	{
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
		else
			return (! dynamic_cast<VuoGenericType *>(fromDataType) && (fromDataType == toDataType));
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
	bool toPortIsEnabledInput = (toPort->getInput() && toPort->isEnabled());

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

	// Case: The 'From' and 'To' ports are both generics.
	if (currentFromGenericType && currentToGenericType)
		return (currentFromGenericType->isGenericTypeCompatible(currentToGenericType));

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
VuoCable * VuoRendererPort::getCableConnectedTo(VuoRendererPort *toPort)
{
	vector<VuoCable *> cables = this->getBase()->getConnectedCables(false);
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
	if (isConstant() && event->key() == Qt::Key_Return)
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
 * Schedules a redraw of this port.
 */
void VuoRendererPort::updateGeometry()
{
	this->prepareGeometryChange();
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
	if (!getBase()->hasCompiler())
		return NULL;

	VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(getBase()->getCompiler());
	return compilerPort->getDataVuoType();
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
 * Returns the string representation of this port's constant data value as it should be rendered
 * in its constant data flag, or an empty string if it has no currently assigned constant data value.
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

		// Don't display constant input values for list ports.
		if (VuoType::isListTypeName(getDataType()->getModuleKey()))
			return "";

		if (getDataType()->getModuleKey()=="VuoColor")
		{
			return "   ";
		}
		if (getDataType()->getModuleKey()=="VuoReal")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			double real = json_object_get_double(js);
			json_object_put(js);

			return getStringForRealValue(real);
		}

		if (getDataType()->getModuleKey()=="VuoPoint2d")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			double x = 0, y = 0;
			json_object *o = NULL;
			if (json_object_object_get_ex(js, "x", &o))
				x = json_object_get_double(o);
			if (json_object_object_get_ex(js, "y", &o))
				y = json_object_get_double(o);
			json_object_put(js);

			QList<double> pointList = QList<double>() << x << y;
			return getPointStringForCoords(pointList);
		}
		if (getDataType()->getModuleKey()=="VuoPoint3d")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			double x = 0, y = 0, z = 0;
			json_object *o = NULL;
			if (json_object_object_get_ex(js, "x", &o))
				x = json_object_get_double(o);
			if (json_object_object_get_ex(js, "y", &o))
				y = json_object_get_double(o);
			if (json_object_object_get_ex(js, "z", &o))
				z = json_object_get_double(o);
			json_object_put(js);

			QList<double> pointList = QList<double>() << x << y << z;
			return getPointStringForCoords(pointList);
		}
		if (getDataType()->getModuleKey()=="VuoPoint4d")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			double x = 0, y = 0, z = 0, w = 0;
			json_object *o = NULL;
			if (json_object_object_get_ex(js, "x", &o))
				x = json_object_get_double(o);
			if (json_object_object_get_ex(js, "y", &o))
				y = json_object_get_double(o);
			if (json_object_object_get_ex(js, "z", &o))
				z = json_object_get_double(o);
			if (json_object_object_get_ex(js, "w", &o))
				w = json_object_get_double(o);
			json_object_put(js);

			QList<double> pointList = QList<double>() << x << y << z << w;
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

			const char *outputString = "";
			if (fontName)
				outputString = QString("%1 %2pt%3").arg(fontName).arg(pointSize).arg(underlineString).toUtf8().data();

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
						{
							VuoText t = VuoText_truncateWithEllipsis(json_object_get_string(itemObject), 30);
							VuoRetain(t);
							expression = strdup(t);
							VuoRelease(t);
						}
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
				return strdup("");

			return VuoText_format("%lu×%lu", value->pixelsWide, value->pixelsHigh);
		}
		if (getDataType()->getModuleKey()=="VuoTransform")
		{
			VuoTransform value = VuoTransform_makeFromString(getConstantAsString().c_str());

			if (VuoTransform_isIdentity(value))
				return strdup("≡");

			if (value.type == VuoTransformTypeTargeted)
				return VuoText_format("(%g,%g,%g) toward (%g,%g,%g)",
									  value.translation.x, value.translation.y, value.translation.z, value.rotationSource.target.x, value.rotationSource.target.y, value.rotationSource.target.z);

			char *rotation;
			if (value.type == VuoTransformTypeQuaternion)
				rotation = VuoText_format("‹%g,%g,%g,%g›",
										  value.rotationSource.quaternion.x, value.rotationSource.quaternion.y, value.rotationSource.quaternion.z, value.rotationSource.quaternion.w);
			else
			{
				VuoPoint3d r = VuoPoint3d_multiply(value.rotationSource.euler, 180./M_PI);
				rotation = VuoText_format("(%g°,%g°,%g°)",
										  r.x, r.y, r.z);
			}

			char *valueAsString = VuoText_format("(%g,%g,%g) %s %g×%g×%g",
												 value.translation.x, value.translation.y, value.translation.z, rotation, value.scale.x, value.scale.y, value.scale.z);
			free(rotation);
			return valueAsString;
		}
		if (getDataType()->getModuleKey()=="VuoTransform2d")
		{
			VuoTransform2d value = VuoTransform2d_makeFromString(getConstantAsString().c_str());

			if (VuoTransform2d_isIdentity(value))
				return strdup("≡");

			VuoReal rotationInDegrees = value.rotation * 180./M_PI;
			return VuoText_format("(%g,%g) %g° %g×%g",
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
				if (strcmp(imageEncoding, "H264") == 0)
					imageEncoding = "H.264";
				else if (strcmp(imageEncoding, "ProRes4444") == 0)
					imageEncoding = "ProRes 4444";
				else if (strcmp(imageEncoding, "ProRes422") == 0)
					imageEncoding = "ProRes 422";
			}

			const char *audioEncoding = NULL;
			if (json_object_object_get_ex(js, "audioEncoding", &o))
			{
				audioEncoding = json_object_get_string(o);
				if (strcmp(audioEncoding, "LinearPCM") == 0)
					audioEncoding = "Linear PCM";
			}

			const char *outputString = "";
			if (imageEncoding && audioEncoding)
				outputString = QString("%1, %2").arg(imageEncoding).arg(audioEncoding).toUtf8().data();

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoScreen")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *label = NULL;

			VuoScreenType type = VuoScreenType_Primary;
			if (json_object_object_get_ex(js, "type", &o))
			{
				type = VuoScreen_typeFromCString(json_object_get_string(o));

				if (type == VuoScreenType_Active)
					label = "(active)";
				else if (type == VuoScreenType_Primary)
					label = "(primary)";
				else if (type == VuoScreenType_Secondary)
					label = "(secondary)";
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

			const char *outputString = "";
			if (label)
				outputString = strdup(label);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoSerialDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);

			const char *outputString = "(first)";
			if (name && strlen(name))
				outputString = strdup(name);

			json_object_put(js);

			return outputString;
		}
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
			json_object_put(js);

			return strdup(outputString.toUtf8().data());
		}
		if (getDataType()->getModuleKey()=="VuoOscInputDevice"
		 || getDataType()->getModuleKey()=="VuoOscOutputDevice")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			json_object *o = NULL;

			const char *name = NULL;
			if (json_object_object_get_ex(js, "name", &o))
				name = json_object_get_string(o);

			const char *outputString = "(auto)";
			if (name && strlen(name))
				outputString = strdup(name);

			json_object_put(js);

			return outputString;
		}
		if (getDataType()->getModuleKey()=="VuoTempoRange")
		{
			json_object *js = json_tokener_parse(getConstantAsString().c_str());
			const char *tempoRange = json_object_get_string(js);
			if (strcmp(tempoRange, "andante") == 0)
				return strdup("70–110 BPM");
			else if (strcmp(tempoRange, "moderato") == 0)
				return strdup("100–140 BPM");
			else if (strcmp(tempoRange, "allegro") == 0)
				return strdup("120–180 BPM");
		}
	}

	// If it's a JSON string (e.g., VuoText or an enum identifier), unescape and truncate it.
	json_object *js = json_tokener_parse(getConstantAsString().c_str());
	if (json_object_get_type(js) == json_type_string)
	{
		string textWithoutQuotes = json_object_get_string(js);
		json_object_put(js);
		VuoText t = VuoText_truncateWithEllipsis(textWithoutQuotes.c_str(), 30);
		VuoRetain(t);
		char *valueAsString = strdup(t);
		VuoRelease(t);
		return valueAsString;
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

		updatePortConstantPath();
		setCacheMode(normalCacheMode);
		getRenderedParentNode()->layoutConnectedInputDrawersAtAndAbovePort(eventPort->getBase()->getRenderer());
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

	if (name.empty() || isAnimated)
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
string VuoRendererPort::getPointStringForCoords(QList<double> coordList) const
{
	const QString coordSeparator = QString(QLocale::system().decimalPoint() != ','? QChar(',') : QChar(';')).append(" ");
	QStringList coordStringList;

	foreach (double coord, coordList)
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
	QString valueAsStringInUserLocale = QLocale::system().toString(value);
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
												 (this->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName() == "vuo.math.calculate"));


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
				dynamic_cast<VuoRendererPublishedPort *>(this)));
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
}
