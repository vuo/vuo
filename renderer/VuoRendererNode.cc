/**
 * @file
 * VuoRendererNode implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererNode.hh"
#include<math.h>

#include "VuoRendererComposition.hh"
#include "VuoRendererMakeListNode.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererCable.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoPort.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererFonts.hh"
#include "VuoStringUtilities.hh"

const qreal VuoRendererNode::subcompositionBulge = VuoRendererFonts::thickPenWidth/3.0;
const qreal VuoRendererNode::cornerRadius = VuoRendererFonts::thickPenWidth/8.0;
const qreal VuoRendererNode::nodeTitleHeight = VuoRendererFonts::thickPenWidth;	///< The height of the node's title.
const qreal VuoRendererNode::nodeClassHeight = VuoRendererFonts::thickPenWidth*3./5.;

/**
 * Creates a renderer detail for the specified base node.
 */
VuoRendererNode::VuoRendererNode(VuoNode * baseNode, VuoRendererSignaler *signaler)
	: VuoBaseDetail<VuoNode>("VuoRendererNode from VuoCompilerNode", baseNode)
{
	getBase()->setRenderer(this);

	// Set up signaler after node has been positioned to avoid sending a spurious nodeMoved signal.
	this->signaler = NULL;

	VuoNodeClass * nodeClass = baseNode->getNodeClass();
	this->proxyNode = NULL;
	this->nodeType = VuoRendererNode::node;
	this->nodeClass = QString::fromUtf8(nodeClass->getClassName().c_str());
	this->nodeIsStateful = nodeClass->hasCompiler() && nodeClass->getCompiler()->getInstanceDataClass();
	this->nodeIsSubcomposition = false;  /// @todo support subcompositions - https://b33p.net/kosada/node/2639
	this->nodeIsMissing = !nodeClass->hasCompiler();
	resetTimeLastExecuted();

	this->setFlags(QGraphicsItem::ItemIsMovable |
				   QGraphicsItem::ItemIsSelectable |
				   QGraphicsItem::ItemIsFocusable |
				   QGraphicsItem::ItemSendsGeometryChanges);

	this->setAcceptHoverEvents(true);

	{
		vector<VuoRendererPort *> inputs;
		vector<VuoPort *> inputPorts = baseNode->getInputPorts();
		for(vector<VuoPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
		{
			bool isRefreshPort = (*it == baseNode->getRefreshPort());
			inputs.push_back(new VuoRendererPort(*it, signaler, false, isRefreshPort, false, false));
		}
		setInputPorts(inputs);
	}

	{
		vector<VuoRendererPort *> outputs;
		vector<VuoPort *> outputPorts = baseNode->getOutputPorts();
		for(vector<VuoPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		{
			bool isDonePort = (*it == baseNode->getDonePort());
			bool isFunctionPort = false;
			outputs.push_back(new VuoRendererPort(*it, signaler, true, false, isDonePort, isFunctionPort));
		}
		setOutputPorts(outputs);
	}

	this->frameRect = getNodeFrameRect();
	setPos(baseNode->getX(), baseNode->getY());
	layoutPorts();

	this->signaler = signaler;
}

void VuoRendererNode::setInputPorts(vector<VuoRendererPort *> inputPorts)
{
	this->inputPorts = new VuoRendererPortList(this);

	foreach (VuoRendererPort *p, inputPorts)
		this->inputPorts->addToGroup(p);
}

void VuoRendererNode::setOutputPorts(vector<VuoRendererPort *> outputPorts)
{
	this->outputPorts = new VuoRendererPortList(this);

	// @@@ remove when functionPort is implemented and part of the outputPorts list.
	// At that time, make sure that VuoNodeClass::unreservedOutputPortStartIndex is updated accordingly.
	functionPort = new VuoRendererPort(new VuoPort(new VuoPortClass("fake function port", VuoPortClass::notAPort))
									   , signaler, false, false, false, true);

	// We're not rendering the function port at the moment anyway, so don't add it to the outputPorts list --
	// the discrepancy in the unreserved output port start index between the renderer and base makes things messy.
	//this->outputPorts->addToGroup(functionPort);

	foreach (VuoRendererPort *p, outputPorts)
		this->outputPorts->addToGroup(p);
}

/**
  * Calculates and sets the positions of the node's child ports relative to the node.
  */
void VuoRendererNode::layoutPorts(void)
{
	int i=0;
	foreach (VuoRendererPort *p, this->inputPorts->childItems())
	{
		QPointF point = getPortPoint(p, i++);
		p->setPos(point);

		VuoRendererTypecastPort * tp = dynamic_cast<VuoRendererTypecastPort *>(p);
		if (tp)
			tp->getChildPort()->setPos(point - QPointF(tp->getChildPortXOffset(),0));
	}

	i=0;
	foreach (VuoRendererPort *p, this->outputPorts->childItems())
		p->setPos(getPortPoint(p, i++));
}

/**
 * Calculates and sets the positions of the node's connected input drawers relative to the node.
 */
void VuoRendererNode::layoutConnectedInputDrawers(void)
{
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	if (inputPorts.size() < 1)
		return;

	VuoPort *finalInputPort = inputPorts[inputPorts.size()-1];
	layoutConnectedInputDrawersAtAndAbovePort(finalInputPort->getRenderer());
}

/**
 * Calculates and sets the positions of any input drawers connected to this node's ports
 * beginning with the provided @c port and iterating through the ports rendered
 * above (i.e., with indices lower than) that port.
 */
void VuoRendererNode::layoutConnectedInputDrawersAtAndAbovePort(VuoRendererPort *port)
{
	bool reachedTargetPort = false;

	vector<VuoPort *> inputPorts = getBase()->getInputPorts();
	for (int i = inputPorts.size()-1; i >= 0; --i)
	{
		if (inputPorts[i] == port->getBase())
			reachedTargetPort = true;

		if (reachedTargetPort)
			layoutConnectedInputDrawer(i);
	}

	if (dynamic_cast<VuoRendererMakeListNode *>(this))
	{
		VuoRendererPort *hostPort = ((VuoRendererMakeListNode *)this)->getHostInputPort();
		if (hostPort)
		{
			VuoRendererNode *hostNode = hostPort->getRenderedParentNode();
			if (hostNode)
				hostNode->layoutConnectedInputDrawersAtAndAbovePort(hostPort);
		}
	}
}

/**
 * Calculates and sets the positions of the node's connected input drawer at the provided index @c i.
 */
void VuoRendererNode::layoutConnectedInputDrawer(unsigned int i)
{
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();

	if (i >= inputPorts.size())
		return;

	VuoRendererPort *p = inputPorts[i]->getRenderer();
	VuoRendererMakeListNode *drawer = p->getAttachedInputDrawer();
	if (drawer)
	{
		QPointF point = getPortPoint(p, i);
		drawer->updateGeometry();
		drawer->setHorizontalDrawerOffset(getInputDrawerOffset(i));
		drawer->setPos(mapToScene(point-QPointF(getInputDrawerOffset(i),0)));
		drawer->getBase()->setTintColor(this->getBase()->getTintColor());
	}
}

/**
 * Appends a line (or move) and curve to @c path, to produce a clockwise-wound rounded corner near @c sharpCornerPoint.
 */
void VuoRendererNode::addRoundedCorner(QPainterPath &path, bool drawLine, QPointF sharpCornerPoint, qreal radius, bool isTop, bool isLeft)
{
	QPointF p(
				sharpCornerPoint.x() + (isTop ? (isLeft ? 0 : -radius) : (isLeft ? radius : 0)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? radius : 0) : (isLeft ? 0 : -radius))
				);
	if (drawLine)
		path.lineTo(p);
	else
		path.moveTo(p);

	path.cubicTo(
				sharpCornerPoint.x() + (isTop ? (isLeft ? 0 : -radius/2.) : (isLeft ? radius/2. : 0)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? radius/2. : 0) : (isLeft ? 0 : -radius/2.)),

				sharpCornerPoint.x() + (isTop ? (isLeft ? radius/2. : 0) : (isLeft ? 0 : -radius/2.)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? 0 : radius/2.) : (isLeft ? -radius/2. : 0)),

				sharpCornerPoint.x() + (isTop ? (isLeft ? radius : 0) : (isLeft ? 0 : -radius)),
				sharpCornerPoint.y() + (isTop ? (isLeft ? 0 : radius) : (isLeft ? -radius : 0))
				);
}

/**
 * Draws lines forming the outer frame of the node.
 */
void VuoRendererNode::drawNodeFrame(QPainter *painter, QRectF nodeInnerFrameRect, VuoRendererColors *colors) const
{
	// First, build a path containing the ports, which we can subtract from the filled areas.
	QPainterPath ports;
	QPainterPath portsInset;
	{
		// Input ports
		foreach(VuoRendererPort *p, inputPorts->childItems())
		{
			QPainterPath outsetPath;
			QPainterPath insetPath;

			VuoRendererTypecastPort * tp = dynamic_cast<VuoRendererTypecastPort *>(p);
			if (tp)
				insetPath = tp->getPortPath(true,false,&outsetPath);
			else
			{
				insetPath = p->getPortPath(VuoRendererPort::portInset);
				outsetPath = p->getPortPath(0);
			}

			ports.addPath(outsetPath.translated(p->pos()));
			portsInset.addPath(p->getRefreshPort() ? outsetPath.translated(p->pos()) : insetPath.translated(p->pos()));
		}
	}
	{
		// Output ports
		foreach(VuoRendererPort *p, outputPorts->childItems())
		{
			QPainterPath outsetPath = p->getPortPath(0).translated(p->pos());
			QPainterPath insetPath = p->getPortPath(VuoRendererPort::portInset).translated(p->pos());
			ports.addPath(outsetPath);
			portsInset.addPath(p->getFunctionPort() || p->getDonePort() ? outsetPath : insetPath);
		}
	}


	// Calculate the bounds of the outer frame (header background, edges around the inner frame, optional footer).
	QRectF nodeOuterFrameRect;
	{
		qreal nodeHeaderHeight = nodeTitleHeight + nodeClassHeight;
		qreal nodeOuterFrameTopY = nodeInnerFrameRect.top() - nodeHeaderHeight;
		nodeOuterFrameRect = QRectF(
					nodeInnerFrameRect.left() - 1.,
					nodeOuterFrameTopY,
					nodeInnerFrameRect.width() + 2.,
					nodeInnerFrameRect.height() + nodeHeaderHeight + 1.
					);

		// Add footer for interface nodes.
		if (getBase()->getNodeClass()->isInterface())
			nodeOuterFrameRect.adjust(0,0,0,VuoRendererFonts::thickPenWidth);
	}


	// Paint the outer and inner frames.
	{
		QPainterPath nodeOuterFrame;
		if (nodeIsSubcomposition)
		{
			// If it's a subcomposition, the outer frame is a rounded rectangle with top and bottom bulges.
			addRoundedCorner(nodeOuterFrame, false, nodeOuterFrameRect.topLeft(),     cornerRadius, true,  true);
			nodeOuterFrame.cubicTo(
						nodeOuterFrameRect.center().x(), nodeOuterFrameRect.top() - VuoRendererNode::subcompositionBulge,
						nodeOuterFrameRect.center().x(), nodeOuterFrameRect.top() - VuoRendererNode::subcompositionBulge,
						nodeOuterFrameRect.right() - cornerRadius, nodeOuterFrameRect.top()
						);
			addRoundedCorner(nodeOuterFrame, true,  nodeOuterFrameRect.topRight(),    cornerRadius, true,  false);
			addRoundedCorner(nodeOuterFrame, true,  nodeOuterFrameRect.bottomRight(), cornerRadius, false, false);
			nodeOuterFrame.cubicTo(
						nodeOuterFrameRect.center().x(), nodeOuterFrameRect.bottom() + VuoRendererNode::subcompositionBulge,
						nodeOuterFrameRect.center().x(), nodeOuterFrameRect.bottom() + VuoRendererNode::subcompositionBulge,
						nodeOuterFrameRect.left() + cornerRadius, nodeOuterFrameRect.bottom()
						);
			addRoundedCorner(nodeOuterFrame, true,  nodeOuterFrameRect.bottomLeft(),  cornerRadius, false, true);
			nodeOuterFrame.closeSubpath();
		}
		else
			// If it's not a subcomposition, the outer frame is just a rounded rectangle.
			nodeOuterFrame.addRoundedRect(nodeOuterFrameRect, cornerRadius, cornerRadius);

		QPainterPath nodeInnerFrame;
		if (getBase()->getNodeClass()->isInterface())
			// If there's a footer, the inner frame is a simple rectangle.
			nodeInnerFrame.addRect(nodeInnerFrameRect);
		else
		{
			// If there's no footer, the bottom of the inner frame has rounded corners, to match the outer frame.
			nodeInnerFrame.moveTo(nodeInnerFrameRect.topLeft());
			nodeInnerFrame.lineTo(nodeInnerFrameRect.topRight());

			qreal innerCornerRadius = cornerRadius - 1.;
			addRoundedCorner(nodeInnerFrame, true, nodeInnerFrameRect.bottomRight(), innerCornerRadius, false, false);
			addRoundedCorner(nodeInnerFrame, true, nodeInnerFrameRect.bottomLeft(), innerCornerRadius, false, true);

			nodeInnerFrame.closeSubpath();
		}

		// Carve out the ports.
		nodeOuterFrame = nodeOuterFrame.subtracted(portsInset);
		nodeInnerFrame = nodeInnerFrame.subtracted(ports);

		// Carve out the inner frame (so the outer frame doesn't overdraw it).
		nodeOuterFrame = nodeOuterFrame.subtracted(nodeInnerFrame);

		// Carve out and paint the stateful indicator cable.
		if (nodeIsStateful)
		{
			QPainterPath cablePath;
			cablePath.moveTo(nodeInnerFrameRect.bottomRight() - QPointF(VuoRendererFonts::thickPenWidth/2., 0));
			cablePath.lineTo(nodeInnerFrameRect.bottomLeft()  + QPointF(VuoRendererFonts::thickPenWidth/2., 0));

			QPainterPathStroker cableStroker;
			cableStroker.setWidth(VuoRendererCable::cableWidth);
			cableStroker.setCapStyle(Qt::RoundCap);
			QPainterPath cableOutline = cableStroker.createStroke(cablePath);

			nodeInnerFrame = nodeInnerFrame.subtracted(cableOutline);
			nodeOuterFrame = nodeOuterFrame.subtracted(cableOutline);

			// Simplified version of VuoRendererCable::paint() —

			QPainterPath highlightPath;
			highlightPath.moveTo(nodeInnerFrameRect.bottomRight() + QPointF(-VuoRendererFonts::thickPenWidth/2., VuoRendererCable::cableHighlightOffset));
			highlightPath.lineTo(nodeInnerFrameRect.bottomLeft()  + QPointF( VuoRendererFonts::thickPenWidth/2., VuoRendererCable::cableHighlightOffset));

			QPainterPathStroker highlightStroker;
			highlightStroker.setWidth(VuoRendererCable::cableHighlightWidth);
			highlightStroker.setCapStyle(Qt::RoundCap);
			QPainterPath highlightOutline = highlightStroker.createStroke(highlightPath);

			// Fill the part of the main cable that doesn't overlap with the highlight.
			painter->fillPath(cableOutline.subtracted(highlightOutline), QBrush(colors->cableMain()));

			// Fill the highlight.
			painter->fillPath(highlightOutline, QBrush(colors->cableUpper()));

		}

		// Paint the outer frame.
		painter->fillPath(nodeOuterFrame, colors->nodeFrame());

		// Paint the inner frame.
		if(nodeIsMissing)
		{
			// Draw background crosshatch using QLinearGradient instead of Qt::DiagCrossPattern, since Qt::DiagCrossPattern is a bitmap and doesn't scale cleanly.

			QLinearGradient nodeBackgroundGradient(QPointF(0,0), QPointF(3.5,3.5));
			nodeBackgroundGradient.setSpread(QGradient::RepeatSpread);
			nodeBackgroundGradient.setColorAt(0.0, Qt::transparent);
			nodeBackgroundGradient.setColorAt(0.8, Qt::transparent);
			nodeBackgroundGradient.setColorAt(0.9, colors->nodeFrame());
			nodeBackgroundGradient.setColorAt(1.0, Qt::transparent);
			painter->fillPath(nodeInnerFrame, QBrush(nodeBackgroundGradient));

			nodeBackgroundGradient.setStart(QPointF(0,3.5));
			nodeBackgroundGradient.setFinalStop(QPointF(3.5,0));
			painter->fillPath(nodeInnerFrame, QBrush(nodeBackgroundGradient));
		}
		else
			painter->fillPath(nodeInnerFrame, colors->nodeFill());
	}

	// Paint the antenna's transmission wake.
	if (getBase()->getNodeClass()->isInterface())
	{
		for (int i=1; i<4; ++i)
		{
			QPainterPath antenna;
			QLineF bottomLine(nodeInnerFrameRect.bottomLeft() + QPointF(4.,4.), nodeInnerFrameRect.bottomRight() + QPointF(-4.,4.));
			antenna.moveTo(bottomLine.pointAt((3.-i)/6.));
			antenna.cubicTo(bottomLine.pointAt((3.-i)/6. + i*.1).x(), bottomLine.y2() + VuoRendererFonts::thickPenWidth*(i-.5)/3.,
							bottomLine.pointAt((i+3.)/6. - i*.1).x(), bottomLine.y2() + VuoRendererFonts::thickPenWidth*(i-.5)/3.,
							bottomLine.pointAt((i+3.)/6.).x(), bottomLine.y2());

			painter->strokePath(antenna, QPen(colors->nodeClass(), VuoRendererFonts::midPenWidth*i*2./3., Qt::SolidLine, Qt::RoundCap));
		}
	}
}

/**
 * Returns the horizontal offset necessary to prevent the drawer at @c portIndex from overlapping any drawers beneath it.
 */
qreal VuoRendererNode::getInputDrawerOffset(unsigned int portIndex) const
{
	qreal cumulativeDrawerOffset = 0;
	qreal maxConstantOffset = 0;
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();

	for (unsigned int i = portIndex+1; i < inputPorts.size(); ++i)
	{
		VuoRendererPort *port = inputPorts[i]->getRenderer();

		if (port->getAttachedInputDrawer())
		{
			qreal currentDrawerChainedOffset = port->getAttachedInputDrawer()->getMaxDrawerChainedLabelWidth() +
					VuoRendererMakeListNode::drawerHorizontalSpacing;
			cumulativeDrawerOffset += currentDrawerChainedOffset;
		}
		else if (dynamic_cast<VuoRendererTypecastPort *>(port))
		{
			VuoRendererTypecastPort *tp = (VuoRendererTypecastPort *)port;
			QPainterPath outsetPath;
			tp->getPortPath(false, true, &outsetPath);
			qreal currentTypecastOffset = outsetPath.boundingRect().width();
			currentTypecastOffset += 0.5*VuoRendererPort::getPortRect().width(); // Account for typecast child port.
			currentTypecastOffset += VuoRendererMakeListNode::drawerHorizontalSpacing;

			if (currentTypecastOffset > maxConstantOffset)
				maxConstantOffset = currentTypecastOffset;
		}
		else if (port->isConstant())
		{
			QPainterPath outsetPath;
			port->getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(port->getConstantAsStringToRender().c_str()),&outsetPath);
			qreal currentConstantOffset = outsetPath.boundingRect().width();
			currentConstantOffset += VuoRendererMakeListNode::drawerHorizontalSpacing;

			if (currentConstantOffset > maxConstantOffset)
				maxConstantOffset = currentConstantOffset;
		}
	}

	qreal targetDrawerOffset = inputPorts[portIndex]->getRenderer()->getAttachedInputDrawer()->getMaxDrawerLabelWidth();

	// Accommodate the width of the host port circle.
	targetDrawerOffset += 0.5*VuoRendererPort::getPortRect().width();

	// Accommodate the width of the drawer arm, which we would like by default to match
	// the width of an empty constant flag.
	QPainterPath outsetPath;
	inputPorts[0]->getRenderer()->getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(""),&outsetPath);
	qreal drawerArmLength = outsetPath.boundingRect().width();

	targetDrawerOffset += drawerArmLength;

	return fmax(maxConstantOffset+targetDrawerOffset-VuoRendererPort::getPortRect().width(),
				cumulativeDrawerOffset+targetDrawerOffset);
}

/**
 * Returns a vector containing all of the drawers attached to any of this node's input ports.
 */
vector<VuoRendererMakeListNode *> VuoRendererNode::getAttachedInputDrawers(void) const
{
	vector<VuoRendererMakeListNode *> drawers;
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for(unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		VuoRendererMakeListNode *drawer = inputPorts[i]->getRenderer()->getAttachedInputDrawer();
		if (drawer)
			drawers.push_back(drawer);
	}

	return drawers;
}

/**
 * Determines and returns the size of the inner frame of a node having the specified attributes.
 *
 * The "inner frame" is the light grey rect in the middle of the node.  It doesn't include the node's header or footer.
 */
QRectF VuoRendererNode::getNodeFrameRect(void) const
{
	QRectF frameRect;
	QString nodeTitle = QString::fromUtf8(getBase()->getTitle().c_str());

	// Width is the longest string or combined input+output port row.
	qreal maxPortRowWidth=0;

	for (int portRow=0; portRow < max(inputPorts->childItems().size(),outputPorts->childItems().size()); ++portRow)
	{
		// Skip port rows containing reserved (refresh/function/done) ports, which don't affect the node's width.
		int adjustedInputPortRow = portRow + VuoNodeClass::unreservedInputPortStartIndex;
		int adjustedOutputPortRow = portRow + VuoNodeClass::unreservedOutputPortStartIndex;

		qreal thisRowWidth = 0;
		if (adjustedInputPortRow < inputPorts->childItems().size())
			thisRowWidth += QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodePortTitleFont()).boundingRect(QString::fromUtf8(inputPorts->childItems()[adjustedInputPortRow]->getBase()->getClass()->getName().c_str())).width();

		if (adjustedOutputPortRow < outputPorts->childItems().size())
			thisRowWidth += QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodePortTitleFont()).boundingRect(QString::fromUtf8(outputPorts->childItems()[adjustedOutputPortRow]->getBase()->getClass()->getName().c_str())).width();

		if(thisRowWidth > maxPortRowWidth)
			maxPortRowWidth = thisRowWidth;
	}

	qreal nodeTitleWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodeTitleFont()).boundingRect(nodeTitle).width();
	qreal nodeClassWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodeClassFont()).boundingRect(nodeClass).width();
	frameRect.setWidth(
		floor(max(
			max(
				nodeTitleWidth + VuoRendererFonts::thickPenWidth,
				nodeClassWidth + VuoRendererFonts::thickPenWidth
			),
			maxPortRowWidth + VuoRendererFonts::thickPenWidth*2.0
		))
	);

	// Height depends only on the number of input/output ports.
	frameRect.setHeight(
		floor(
			max(
				inputPorts->childItems().size() - VuoNodeClass::unreservedInputPortStartIndex,	// don't count the refresh port
				outputPorts->childItems().size() - VuoNodeClass::unreservedOutputPortStartIndex	// don't count the done or function port
			) * VuoRendererPort::portSpacing
			+ VuoRendererPort::portContainerMargin * 2.
		)
	);

	return frameRect;
}

/**
 * Returns a rectangle that completely encloses the rendered node (accounting for the thick line width).
 */
QRectF VuoRendererNode::boundingRect(void) const
{
	if (proxyNode)
		return QRectF();

	QRectF r = getNodeFrameRect();

	// Header
	r.adjust(0, -nodeTitleHeight - nodeClassHeight, 0, 0);

	// "Antenna" footer
	if (getBase()->getNodeClass()->isInterface())
		r.adjust(0, 0, 0, VuoRendererFonts::thickPenWidth);
	else if (nodeIsStateful)
		// If there's no footer, and the node is stateful, leave room for the stateful indicator's thick line width.
		r.adjust(0, 0, 0, VuoRendererCable::cableWidth/2.);

	if (nodeIsSubcomposition)
		r.adjust(0, -subcompositionBulge*3./4., 0, subcompositionBulge*3./4.);

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}


/**
 * Returns the center point of the specified input/output port circle.
 *
 * @param port The port.
 * @param portIndex The index of the port in the list of input/output ports (including refresh/done/function ports).
 */
QPointF VuoRendererNode::getPortPoint(VuoRendererPort *port, unsigned int portIndex) const
{
	qreal x = (port->getOutput() ? getNodeFrameRect().width()+1. : -1.);
	qreal y;

	if (port->getRefreshPort() || port->getDonePort())
	{
		y = -VuoRendererFonts::thickPenWidth / 2.;
	}
	else if (port->getFunctionPort())
	{
		y = -VuoRendererFonts::thickPenWidth / 2. + VuoRendererPort::portSpacing;
	}
	else
	{
		qreal adjustedPortIndex = (qreal)portIndex - (port->getOutput() ?
														  VuoNodeClass::unreservedOutputPortStartIndex :
														  VuoNodeClass::unreservedInputPortStartIndex);  // Skip the refresh/done/function port.

		qreal portOffset = adjustedPortIndex + 0.5;  // Center the port within its space.
		y = VuoRendererPort::portContainerMargin + portOffset * VuoRendererPort::portSpacing;
	}

	return QPointF(x, y);
}

/**
 * Returns the bounding rect for the node title box.
 */
QRectF VuoRendererNode::getNodeTitleBoundingRect() const
{
	return nodeTitleBoundingRect;
}

/**
 * Draws a standard node, including rectangular frame, and input and output ports.
 *
 * The node is drawn with (0,0) as the top left inner margin, so you should use a QPainter transform if you want to position the node.
 */
void VuoRendererNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (proxyNode)
		return;

	frameRect = getNodeFrameRect();
	drawBoundingRect(painter);

	qint64 timeOfLastActivity = (getRenderActivity()? timeLastExecutionEnded : VuoRendererItem::notTrackingActivity);
	VuoRendererColors *colors = new VuoRendererColors(getBase()->getTintColor(), isSelected(), false, false, timeOfLastActivity);
	drawNodeFrame(painter, frameRect, colors);


	// Node Title
	QString nodeTitle = QString::fromUtf8(getBase()->getTitle().c_str());
	{
		nodeTitleBoundingRect = QRectF(
			VuoRendererFonts::thickPenWidth/2. - 1.,	// Move left 1 point to line up node title text with port title text.
			-nodeTitleHeight - nodeClassHeight,
			frameRect.width() + 2. - 2. * VuoRendererFonts::thickPenWidth/2.,	// Leave room for in-event and out-event ports.
			nodeTitleHeight + 2.
		);
		painter->setPen(colors->nodeTitle());
		painter->setFont(VuoRendererFonts::getSharedFonts()->nodeTitleFont());
		painter->drawText(nodeTitleBoundingRect,Qt::AlignLeft,nodeTitle);
	}

	// Node Class
	{
		QRectF r(
			VuoRendererFonts::thickPenWidth/2. - 1.,	// Move left 1 point to line up node title text with port title text.
			-nodeClassHeight - 2.,
			frameRect.width() + 2. - 2. * VuoRendererFonts::thickPenWidth/2.,	// Leave room for in-event and out-event ports.
			nodeClassHeight + 2.
		);
		painter->setPen(colors->nodeClass());
		painter->setFont(VuoRendererFonts::getSharedFonts()->nodeClassFont());
		painter->drawText(r,Qt::AlignLeft,nodeClass);
	}

	layoutPorts();

	delete colors;
}

/**
 * Sets whether the node is rendered as though its implementation is missing.
 */
void VuoRendererNode::setMissingImplementation(bool missingImplementation)
{
	this->nodeIsMissing = missingImplementation;
}

/**
 * If set, this node will not be drawn; its drawing will be handled by @c proxyNode.  Useful for, e.g., attached typecast nodes.
 */
void VuoRendererNode::setProxyNode(VuoRendererNode * proxyNode)
{
	this->proxyNode = proxyNode;
}

/**
 * Returns this node's rendering proxy.
 */
VuoRendererNode * VuoRendererNode::getProxyNode(void) const
{
	return proxyNode;
}

/**
 * Returns the collapsed typecast port rendered in place of this node, or NULL if none.
 */
VuoRendererTypecastPort * VuoRendererNode::getProxyCollapsedTypecast(void) const
{
	if (!proxyNode)
		return NULL;

	foreach (VuoPort *p, proxyNode->getBase()->getInputPorts())
	{
		VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p->getRenderer());
		if (tp && (tp->getUncollapsedTypecastNode()->getBase() == this->getBase()))
			return tp;
	}

	return NULL;
}

/**
 * Updates the node and its connected cables to reflect changes in state.
 */
QVariant VuoRendererNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
	QVariant newValue = value;

	if (change == QGraphicsItem::ItemPositionChange)
	{
		// Quantize position to whole pixels.
		newValue = value.toPoint();

		this->updateConnectedCableGeometry();

		foreach (VuoRendererPort * p, inputPorts->childItems())
			p->updateGeometry();

		foreach (VuoRendererPort * p, outputPorts->childItems())
			p->updateGeometry();

		foreach (VuoRendererMakeListNode *n, getAttachedInputDrawers())
			n->updateGeometry();
	}

	// Node has moved within its parent
	if (change == QGraphicsItem::ItemPositionHasChanged)
	{
		QPointF newPos = value.toPointF();
		if ((getBase()->getX() != newPos.x()) || (getBase()->getY() != newPos.y()))
		{
			qreal dx = newPos.x() - this->getBase()->getX();
			qreal dy = newPos.y() - this->getBase()->getY();

			this->getBase()->setX(newPos.x());
			this->getBase()->setY(newPos.y());

			if (signaler)
			{
				set<VuoRendererNode *> movedNodes;

				// "Make List" nodes follow their host nodes' movements automatically.
				// Pushing their moves onto the 'Undo' stack breaks other 'Undo' commands.
				if (!dynamic_cast<VuoRendererMakeListNode *>(this))
				{
					movedNodes.insert(this);
					signaler->signalNodesMoved(movedNodes, dx, dy, true);
				}
			}

			layoutConnectedInputDrawers();
		}

		return newPos;
	}

	if (change == QGraphicsItem::ItemSelectedHasChanged)
	{
		// When the node is (de)selected, repaint all ports (since they also reflect selection status).
		foreach (VuoRendererPort *p, inputPorts->childItems())
			p->updateGeometry();
		foreach (VuoRendererPort *p, outputPorts->childItems())
			p->updateGeometry();
	}

	return QGraphicsItem::itemChange(change, newValue);
}

/**
 * Handle mouse hover start events.
 */
void VuoRendererNode::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
	hoverMoveEvent(event);
}

/**
 * Handle mouse hover move events.
 */
void VuoRendererNode::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
	// If the cursor is currently positioned over the node title rectangle,
	// give the node keyboard focus.
	if (nodeTitleBoundingRect.contains(event->pos()))
		setFocus();

	// Othewise, release focus.
	else
		clearFocus();
}

/**
 * Handle mouse hover leave events.
 */
void VuoRendererNode::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
	clearFocus();
}

/**
 * Handle mouse double-click events.
 */
void VuoRendererNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	// If the double-click occurs on the node title box,
	// open up a text editor to modify the title.
	if (nodeTitleBoundingRect.contains(event->pos()))
		signaler->signalNodeTitleEditorRequested(this);
}

/**
 * Handle key-press events.
 */
void VuoRendererNode::keyPressEvent(QKeyEvent *event)
{
	// If the user presses 'Return' while the node has keyboard focus,
	// open up a text editor to modify the title.
	if (event->key() == Qt::Key_Return)
	{
		signaler->signalNodeTitleEditorRequested(this);
		setFocus();
	}
}

/**
 * Returns the set of cables connected to this node.  This includes:
 * - Cables connected to any of the node's input ports.
 * - Cables connected to the child port of any collapsed typecast attached to an input port.
 * - Cables connected to any of the node's output ports.
 */
set<VuoCable *> VuoRendererNode::getConnectedCables(bool includePublishedCables)
{
	set<VuoCable *> connectedInputCables = getConnectedInputCables(includePublishedCables);
	set<VuoCable *> connectedOutputCables = getConnectedOutputCables(includePublishedCables);

	set<VuoCable *> connectedCables;
	connectedCables.insert(connectedInputCables.begin(), connectedInputCables.end());
	connectedCables.insert(connectedOutputCables.begin(), connectedOutputCables.end());

	return connectedCables;
}

/**
 * Returns the set of input cables connected to this node.  This includes:
 * - Cables connected to any of the node's input ports.
 * - Cables connected to the child port of any collapsed typecast attached to an input port.
 */
set<VuoCable *> VuoRendererNode::getConnectedInputCables(bool includePublishedCables)
{
	set<VuoCable *> connectedInputCables;

	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for(vector<VuoPort *>::iterator inputPort = inputPorts.begin(); inputPort != inputPorts.end(); ++inputPort)
	{
		vector<VuoCable *> inputCables = (*inputPort)->getConnectedCables(includePublishedCables);
		connectedInputCables.insert(inputCables.begin(), inputCables.end());

		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
		if (typecastPort)
		{
			vector<VuoCable *> childPortInputCables = typecastPort->getChildPort()->getBase()->getConnectedCables(includePublishedCables);
			connectedInputCables.insert(childPortInputCables.begin(), childPortInputCables.end());
		}
	}

	return connectedInputCables;
}

/**
 * Returns the set of output cables connected to this node.  This includes:
 * - Cables connected to any of the node's output ports.
 */
set<VuoCable *> VuoRendererNode::getConnectedOutputCables(bool includePublishedCables)
{
	set<VuoCable *> connectedOutputCables;

	vector<VuoPort *> outputPorts = this->getBase()->getOutputPorts();
	for(vector<VuoPort *>::iterator outputPort = outputPorts.begin(); outputPort != outputPorts.end(); ++outputPort)
	{
		vector<VuoCable *> outputCables = (*outputPort)->getConnectedCables(includePublishedCables);
		connectedOutputCables.insert(outputCables.begin(), outputCables.end());
	}

	return connectedOutputCables;
}

/**
 * Schedules a redraw of this node.
 */
void VuoRendererNode::updateGeometry(void)
{
	this->prepareGeometryChange();
	this->updateConnectedCableGeometry();
}

/**
 * Schedules a redraw of this node's connected cables.
 */
void VuoRendererNode::updateConnectedCableGeometry(void)
{
	set<VuoCable *> connectedCables = getConnectedCables(true);
	for (set<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
	{
		if (! (*cable)->hasRenderer())
			continue;  // in case cable has not yet been added to the composition, when constructing a VuoRendererComposition from an existing set of nodes and cables

		VuoRendererCable *rc = (*cable)->getRenderer();
		rc->updateGeometry();
	}
}

/**
 * Returns this node's input ports, as a @c VuoRendererPortList.
 *
 * @see VuoNode::getInputPorts()
 */
VuoRendererPortList * VuoRendererNode::getInputPorts(void)
{
	return inputPorts;
}

/**
 * Returns this node's output ports, as a @c VuoRendererPortList.
 *
 * @see VuoNode::getInputPorts()
 */
VuoRendererPortList * VuoRendererNode::getOutputPorts(void)
{
	return outputPorts;
}

/**
 * Replaces input port @c old with @c new.  Affects only renderer detail ports (not base or compiler ports).
 */
void VuoRendererNode::replaceInputPort(VuoRendererPort * oldPort, VuoRendererPort * newPort)
{
	inputPorts->addToGroup(newPort);
	newPort->stackBefore(oldPort);
	inputPorts->removeFromGroup(oldPort);
	oldPort->updateGeometry();
	scene()->removeItem(oldPort);
	layoutPorts();
	layoutConnectedInputDrawers();
}

/**
 * Adds input port @c port to the node.  Affects only renderer detail ports (not base or compiler ports).
 */
void VuoRendererNode::addInputPort(VuoRendererPort * newPort)
{
	inputPorts->addToGroup(newPort);
	layoutPorts();
	layoutConnectedInputDrawers();
}

/**
 * Sets the @c title for this node; re-lays-out its ports to accommodate the new name.
 */
void VuoRendererNode::setTitle(string title)
{
	updateGeometry();
	getBase()->setTitle(title);
	layoutPorts();
}

/**
 * Generates a formatted title to be incorporated into the tooltip for the input @c nodeClass.
 */
QString VuoRendererNode::generateNodeClassToolTipTitle(VuoNodeClass *nodeClass)
{
	string humanReadableName = nodeClass->getDefaultTitle();
	return QString("<h2>%1</h2>").arg(humanReadableName.c_str());
}

/**
 * Generates a formatted description to be incorporated into the tooltip for the input @c nodeClass.
 */
QString VuoRendererNode::generateNodeClassToolTipTextBody(VuoNodeClass *nodeClass)
{
	string className = nodeClass->getClassName();
	string description = VuoStringUtilities::generateHtmlFromMarkdown(nodeClass->getDescription());
	string version = nodeClass->getVersion();

	QString tooltipBody;
	tooltipBody.append(QString("<font size=+1 color=\"gray\">%1</font>").arg(description.c_str()));
	tooltipBody.append(QString("<p><font color=\"gray\">%1</font><br>").arg(className.c_str()));
	tooltipBody.append(QString("<font color=\"gray\">Version %1</font></p>").arg(version.c_str()));

	return tooltipBody;
}

/**
 * Resets the time last executed to a value that causes the node
 * to be painted as if activity-rendering were disabled.
 */
void VuoRendererNode::resetTimeLastExecuted()
{
	this->timeLastExecutionEnded = VuoRendererColors::getVirtualNodeExecutionOrigin();
}

/**
 * Updates the node's execution state to indicate that it is currently executing.
 */
void VuoRendererNode::setExecutionBegun()
{
	this->timeLastExecutionEnded = VuoRendererItem::activityInProgress;
}

/**
 * Updates the node's execution state to indicate that it has just finished executing.
 */
void VuoRendererNode::setExecutionEnded()
{
	this->timeLastExecutionEnded = QDateTime::currentMSecsSinceEpoch();
}

/**
 * Returns the time, in ms since epoch, that this node's most recent node execution ended,
 *  or if applicable, a special VuoRendererItem::activityState.
 */
qint64 VuoRendererNode::getTimeLastExecutionEnded()
{
	return this->timeLastExecutionEnded;
}
