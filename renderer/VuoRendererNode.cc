/**
 * @file
 * VuoRendererNode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererNode.hh"
#include "VuoRendererComposition.hh"
#include "VuoRendererSignaler.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerPort.hh"
#include "VuoFileUtilities.hh"
#include "VuoGenericType.hh"
#include "VuoNodeClass.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoStringUtilities.hh"
#include "VuoCompilerSpecializedNodeClass.hh"

const qreal VuoRendererNode::cornerRadius              = 10 /*VuoRendererFonts::thickPenWidth/2.0*/;  ///< The radius of rounded corners.
const qreal VuoRendererNode::outerBorderWidth          = 1.;
const qreal VuoRendererNode::nodeTitleHeight           = 18 /*round(VuoRendererFonts::nodeTitleFontSize + VuoRendererFonts::thickPenWidth*1./8.) + 2*/;  ///< The height of the node's title.
const qreal VuoRendererNode::nodeTitleHorizontalMargin = 12 /*VuoRendererFonts::thickPenWidth/2.0 + 2*/;                                                 ///< The margin to the left and right of the node title.
const qreal VuoRendererNode::nodeClassHeight           = 12 /*round(VuoRendererFonts::thickPenWidth*3./5.)*/;
const qreal VuoRendererNode::nodeHeaderYOffset         = -nodeTitleHeight - nodeClassHeight;  ///< The vertical offset of the node header's top left corner from the origin.
const qreal VuoRendererNode::iconRightOffset           = 11.;

/**
 * Creates a renderer detail for the specified base node.
 */
VuoRendererNode::VuoRendererNode(VuoNode * baseNode, VuoRendererSignaler *signaler)
	: VuoBaseDetail<VuoNode>("VuoRendererNode from VuoCompilerNode", baseNode)
{
	getBase()->setRenderer(this);

	setZValue(nodeZValue);

	// Set up signaler after node has been positioned to avoid sending a spurious nodeMoved signal.
	this->signaler = NULL;

	VuoNodeClass * nodeClass = baseNode->getNodeClass();
	this->proxyNode = NULL;
	this->nodeType = VuoRendererNode::node;

	if (nodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()))
		this->nodeClass = QString::fromUtf8(dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler())->getOriginalGenericNodeClassName().c_str());
	else
		this->nodeClass = QString::fromUtf8(nodeClass->getClassName().c_str());

	// QPainter::drawText expects strings to be canonically composed,
	// else it renders diacritics next to (instead of superimposed upon) their base glyphs.
	this->nodeClass = this->nodeClass.normalized(QString::NormalizationForm_C);

	this->nodeIsSubcomposition = nodeClass->hasCompiler() && nodeClass->getCompiler()->isSubcomposition();
	this->nodeIsMissing = !nodeClass->hasCompiler();
	this->alwaysDisplayPortNames = false;
	this->_eligibilityHighlight = VuoRendererColors::noHighlight;

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
			inputs.push_back(new VuoRendererPort(*it, signaler, false, isRefreshPort, false));
		}
		setInputPorts(inputs);
	}

	{
		vector<VuoRendererPort *> outputs;
		vector<VuoPort *> outputPorts = baseNode->getOutputPorts();
		for(vector<VuoPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		{
			bool isFunctionPort = false;
			outputs.push_back(new VuoRendererPort(*it, signaler, true, false, isFunctionPort));
		}
		setOutputPorts(outputs);
	}

	setPos(baseNode->getX(), baseNode->getY());
	updateNodeFrameRect();
	layoutPorts();

	this->signaler = signaler;
}

void VuoRendererNode::setInputPorts(vector<VuoRendererPort *> inputPorts)
{
	for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
	{
		this->inputPorts.push_back(*it);
		(*it)->setParentItem(this);

		if ((*it)->isHiddenRefreshPort())
			continue;

		// Now that the port knows what node it belongs to, it may need to update
		// its cached display name and related structures to reflect the fact that
		// the port name may be unambiguous within the node, and therefore not displayed.
		(*it)->updateNameRect();
	}
}

void VuoRendererNode::setOutputPorts(vector<VuoRendererPort *> outputPorts)
{
	// @@@ remove when functionPort is implemented and part of the outputPorts list.
	// At that time, make sure that VuoNodeClass::unreservedOutputPortStartIndex is updated accordingly.
	functionPort = new VuoRendererPort(new VuoPort(new VuoPortClass("fake function port", VuoPortClass::notAPort))
									   , signaler, false, false, true);

	// We're not rendering the function port at the moment anyway, so don't add it to the outputPorts list --
	// the discrepancy in the unreserved output port start index between the renderer and base makes things messy.
	//this->outputPorts->addToGroup(functionPort);

	for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
	{
		this->outputPorts.push_back(*it);

		(*it)->setParentItem(this);

		// Now that the port knows what node it belongs to, it may need to update
		// its cached display name and related structures to reflect the fact that
		// the port name may be unambiguous within the node, and therefore not displayed.
		(*it)->updateNameRect();
	}
}

/**
  * Calculates and sets the positions of the node's child ports relative to the node.
  */
void VuoRendererNode::layoutPorts(void)
{
	int i=0;
	foreach (VuoRendererPort *p, this->inputPorts)
	{
		QPointF point = getPortPoint(p, i++);
		p->setPos(point);

		VuoRendererTypecastPort * tp = dynamic_cast<VuoRendererTypecastPort *>(p);
		if (tp)
			tp->getChildPort()->setPos(point - QPointF(tp->getChildPortXOffset(),0));
	}

	i=0;
	foreach (VuoRendererPort *p, this->outputPorts)
		p->setPos(getPortPoint(p, i++));

	this->nodeFrames = getNodeFrames(this->frameRect);
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

	if (finalInputPort->hasRenderer())
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

	VuoRendererInputDrawer *thisDrawer = dynamic_cast<VuoRendererInputDrawer *>(this);
	if (thisDrawer)
	{
		VuoNode *hostNode = thisDrawer->getRenderedHostNode();
		VuoPort *hostPort = thisDrawer->getRenderedHostPort();
		if (hostNode && hostNode->hasRenderer() && hostPort && hostPort->hasRenderer())
			hostNode->getRenderer()->layoutConnectedInputDrawersAtAndAbovePort(hostPort->getRenderer());
	}
}

/**
 * Calculates and sets the position and tint of the node's connected input drawer
 * at the provided index @c i.
 */
void VuoRendererNode::layoutConnectedInputDrawer(unsigned int i)
{
	vector<VuoPort *> inputPorts = getBase()->getInputPorts();

	if (i >= inputPorts.size())
		return;

	VuoRendererPort *p = inputPorts[i]->getRenderer();
	VuoRendererInputDrawer *drawer = p->getAttachedInputDrawer();
	if (drawer)
	{
		QPointF point = getPortPoint(p, i);
		drawer->updateGeometry();

		QPoint inputDrawerOffset(getInputDrawerOffset(i) + 0.5 - 2, 0); // Add 0.5 in order to round, not truncate.
		drawer->setHorizontalDrawerOffset(inputDrawerOffset.x());
		drawer->setPos(mapToScene(point-inputDrawerOffset));
		drawer->getBase()->setTintColor(this->getBase()->getTintColor());

		// Update the list's cached bounding rect, so long arms don't disappear when the ports scroll offscreen.
		auto listDrawer = dynamic_cast<VuoRendererInputListDrawer *>(drawer);
		if (listDrawer)
			listDrawer->updateGeometry();
	}
}

/**
 * Draws lines forming the outer frame of the node.
 */
void VuoRendererNode::drawNodeFrame(QPainter *painter, QRectF nodeInnerFrameRect, VuoRendererColors *colors) const
{
	QPainterPath nodeOuterFrame = this->nodeFrames.first;
	QPainterPath nodeInnerFrame = this->nodeFrames.second;

	// Paint the node body background.
	painter->fillPath(nodeInnerFrame, colors->nodeFill());

	// Paint the node header background.
	// This gets painted after the body, so the parts outside the
	// header's rounded bottom corners show through to the body.
	painter->fillPath(nodeOuterFrame, colors->nodeFrame());

	// Paint the subcomposition icon.
	if (nodeIsSubcomposition)
		painter->fillPath(this->subcompositionIndicatorPath, QBrush(QColor::fromRgb(255,255,255,128)));
}

/**
 * Returns the outer and inner node frames, in that order, for a node with the provided `nodeInnerFrameRect` value.
 */
QPair<QPainterPath, QPainterPath> VuoRendererNode::getNodeFrames(QRectF nodeInnerFrameRect)
{
	// Calculate the bounds of the outer frame (header background).
	QRectF nodeOuterFrameRect;
	{
		qreal nodeOuterFrameTopY = nodeInnerFrameRect.top() + VuoRendererNode::nodeHeaderYOffset;
		nodeOuterFrameRect = QRectF(
					nodeInnerFrameRect.left() - VuoRendererNode::outerBorderWidth,
					nodeOuterFrameTopY,
					nodeInnerFrameRect.width() + 2*VuoRendererNode::outerBorderWidth,
					-VuoRendererNode::nodeHeaderYOffset// + VuoRendererNode::outerBorderWidth
					);
	}

	// Calculate the outer and inner frames.
	QPainterPath nodeOuterFrame;
	qreal nodeHeaderBottomCornerRadius = VuoRendererNode::cornerRadius/4;
	{
		// The top of the outer frame (header) is rounded; the bottom is slightly rounded.
		nodeOuterFrame.moveTo(nodeOuterFrameRect.right(), nodeOuterFrameRect.center().y());
		addRoundedCorner(nodeOuterFrame, true, nodeOuterFrameRect.bottomRight(), nodeHeaderBottomCornerRadius, false, false);
		addRoundedCorner(nodeOuterFrame, true, nodeOuterFrameRect.bottomLeft(), nodeHeaderBottomCornerRadius, false, true);
		addRoundedCorner(nodeOuterFrame, true, nodeOuterFrameRect.topLeft(),  VuoRendererNode::cornerRadius, true, true);
		addRoundedCorner(nodeOuterFrame, true, nodeOuterFrameRect.topRight(), VuoRendererNode::cornerRadius, true, false);
	}

	QPainterPath nodeInnerFrame;
	{
		// The bottom of the inner frame has rounded corners, to match the outer frame.
		qreal bw = VuoRendererNode::outerBorderWidth;
		nodeInnerFrame.moveTo(nodeInnerFrameRect.topLeft()  - QPoint(bw, 0) - QPoint(0, nodeHeaderBottomCornerRadius));
		nodeInnerFrame.lineTo(nodeInnerFrameRect.topRight() + QPoint(bw, 0) - QPoint(0, nodeHeaderBottomCornerRadius));
		addRoundedCorner(nodeInnerFrame, true, nodeInnerFrameRect.bottomRight() + QPoint( bw,bw), VuoRendererNode::cornerRadius, false, false);
		addRoundedCorner(nodeInnerFrame, true, nodeInnerFrameRect.bottomLeft()  + QPoint(-bw,bw), VuoRendererNode::cornerRadius, false, true);
	}

	return qMakePair(nodeOuterFrame, nodeInnerFrame);
}

/**
 * Returns the path of the subcomposition icon for a node with the provided @c nodeInnerFrameRect,
 * or an empty path if @c isSubcomposition is false.
 */
QPainterPath VuoRendererNode::getSubcompositionIndicatorPath(QRectF nodeInnerFrameRect, bool isSubcomposition)
{
	if (!isSubcomposition)
		return QPainterPath();

	const double rectSideLength = VuoRendererFonts::midPenWidth*2.5;

	// The icon should start at the node class label's baseline, and extend to match its ascender height.
	QPainterPath rectPath;
	rectPath.addRect(QRectF(-0.5 /* pixel-align */ + nodeInnerFrameRect.right() - iconRightOffset,
							-4. - 1.5*rectSideLength,
							rectSideLength,
							rectSideLength));

	// Stroke the top-left rectangle.
	QPainterPathStroker rectStroker;
	QPainterPath topLeftStroke = rectStroker.createStroke(rectPath);

	// Stroke the bottom-right rectangle.
	rectPath.translate(-0.5 /* pixel-align */ + rectSideLength/2,
					   -0.5 /* pixel-align */ + rectSideLength/2);
	QPainterPath bottomRightStroke = rectStroker.createStroke(rectPath);

	// Unite the two rectangles (rather than overdrawing semitransparent strokes), so the icon is a uniform color.
	return topLeftStroke.united(bottomRightStroke);
}

/**
 * Returns the horizontal offset necessary to prevent the drawer at @c portIndex from overlapping any drawers beneath it.
 */
qreal VuoRendererNode::getInputDrawerOffset(unsigned int portIndex) const
{
	qreal maxOffset = 0;
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();

	for (unsigned int i = inputPorts.size()-1; i > portIndex; --i)
	{
		VuoRendererPort *port = inputPorts[i]->getRenderer();

		if (port->getAttachedInputDrawer())
			maxOffset += port->getAttachedInputDrawer()->getMaxDrawerChainedLabelWidth() - 5;
		else if (dynamic_cast<VuoRendererTypecastPort *>(port))
		{
			VuoRendererTypecastPort *tp = (VuoRendererTypecastPort *)port;
			qreal currentTypecastOffset = tp->getPortPath(true, true).boundingRect().width()
					+ (VuoRendererPort::portRadius-2) // Account for typecast child port.
					+ VuoRendererInputDrawer::drawerHorizontalSpacing;

			if (currentTypecastOffset > maxOffset)
				maxOffset = currentTypecastOffset;
		}
		else if (port->isConstant())
		{
			qreal currentConstantOffset = port->getPortPath().boundingRect().width()
					- (VuoRendererPort::portRadius-1)
					+ VuoRendererInputDrawer::drawerHorizontalSpacing;

			if (currentConstantOffset > maxOffset)
				maxOffset = currentConstantOffset;
		}
		else if (!port->getBase()->getConnectedCables(true).empty()) // Simple circle port with cable(s) attached (leave room for the cable's rounded corner).
			maxOffset = fmax(maxOffset, VuoRendererPort::portRadius * 2 + 3);
		else // Simple circle port with no cables.
			maxOffset = fmax(maxOffset, VuoRendererPort::portRadius );
	}

	qreal targetDrawerOffset = inputPorts[portIndex]->getRenderer()->getAttachedInputDrawer()->getMaxDrawerLabelWidth()
			+ VuoRendererInputDrawer::drawerHorizontalSpacing;

	return targetDrawerOffset + maxOffset;
}

/**
 * Returns a vector containing all of the drawers attached to any of this node's input ports.
 */
vector<VuoRendererInputDrawer *> VuoRendererNode::getAttachedInputDrawers(void) const
{
	vector<VuoRendererInputDrawer *> drawers;
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for(unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		VuoRendererInputDrawer *drawer = inputPorts[i]->getRenderer()->getAttachedInputDrawer();
		if (drawer)
			drawers.push_back(drawer);
	}

	return drawers;
}

/**
 * Calculates and updates the cached inner frame of a node based on its current attributes.
 *
 * The "inner frame" is the light grey rect in the middle of the node.  It doesn't include the node's header or footer.
 */
void VuoRendererNode::updateNodeFrameRect(void)
{
	QRectF updatedFrameRect;
	QString nodeTitle = QString::fromUtf8(getBase()->getTitle().c_str());

	// QPainter::drawText expects strings to be canonically composed,
	// else it renders diacritics next to (instead of superimposed upon) their base glyphs.
	nodeTitle = nodeTitle.normalized(QString::NormalizationForm_C);

	// Width is the longest string or combined input+output port row.
	qreal maxPortRowWidth=0;

	size_t inputPortCount = inputPorts.size();
	size_t outputPortCount = outputPorts.size();
	size_t portRowCount = max(inputPortCount, outputPortCount);
	for (int portRow = 0; portRow < portRowCount; ++portRow)
	{
		// Skip port rows containing reserved (refresh/function) ports, which don't affect the node's width.
		int adjustedInputPortRow = portRow + VuoNodeClass::unreservedInputPortStartIndex;
		int adjustedOutputPortRow = portRow + VuoNodeClass::unreservedOutputPortStartIndex;

		qreal thisRowWidth = 0;
		if (adjustedInputPortRow < inputPortCount)
		{
			VuoRendererPort *port = inputPorts[adjustedInputPortRow];
			thisRowWidth += port->getNameRect().x() + port->getNameRect().width();

			if (port->hasPortAction())
				thisRowWidth += port->getActionIndicatorRect().width();
		}

		if (adjustedOutputPortRow < outputPortCount)
			thisRowWidth += outputPorts[adjustedOutputPortRow]->getNameRect().width();

		if(thisRowWidth > maxPortRowWidth)
			maxPortRowWidth = thisRowWidth;
	}

	qreal nodeTitleWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodeTitleFont()).boundingRect(nodeTitle).width();
	qreal nodeClassWidth = QFontMetricsF(VuoRendererFonts::getSharedFonts()->nodeClassFont()).boundingRect(nodeClass).width();

	if (this->nodeIsSubcomposition)
		nodeClassWidth += VuoRendererNode::iconRightOffset;

	bool hasInputPorts = inputPortCount > VuoNodeClass::unreservedInputPortStartIndex;
	bool hasOutputPorts = outputPortCount > VuoNodeClass::unreservedOutputPortStartIndex;
	int unalignedFrameWidth =
		floor(max(
			max(
				nodeTitleWidth
					  + nodeTitleHorizontalMargin * 2.
					  // Line up right edge of text with right edge of subcomposition icon
					  - 3,

				nodeClassWidth
					  // Padding between left edge of node frame and left edge of text
					  + VuoRendererFonts::thickPenWidth/2.
					  // Padding between right edge of text and right edge of node frame
					  + VuoRendererFonts::thickPenWidth/2.
					  // Line up right edge of text with where the right edge of the subcomposition icon would be if it were present
					  - 3
			),
			maxPortRowWidth

				  // Leave some space between the right edge of the input port name and the left edge of the output port name.
				  + ((hasInputPorts && hasOutputPorts) ? (nameDisplayEnabledForInputPorts() && nameDisplayEnabledForOutputPorts()?
															  VuoRendererFonts::thickPenWidth :
															  VuoRendererFonts::thickPenWidth/4.0) :
														 0.)


				  + 1.
		));

	// Snap to the next-widest horizontal grid position so that node columns can be right-justified.
	int alignedFrameWidth = VuoRendererComposition::quantizeToNearestGridLine(QPointF(unalignedFrameWidth, 0), VuoRendererComposition::minorGridLineSpacing).x()
							- 2*VuoRendererNode::outerBorderWidth;

	if (alignedFrameWidth < unalignedFrameWidth)
		alignedFrameWidth += VuoRendererComposition::minorGridLineSpacing;

	updatedFrameRect.setWidth(alignedFrameWidth);

	// Height depends only on the number of input/output ports.
	updatedFrameRect.setHeight(
		floor(
			max(
				inputPortCount - VuoNodeClass::unreservedInputPortStartIndex,   // don't count the refresh port
				outputPortCount - VuoNodeClass::unreservedOutputPortStartIndex  // don't count the function port
			) * VuoRendererPort::portSpacing
			+ VuoRendererPort::portContainerMargin * 2.
		)
	);

	this->frameRect = updatedFrameRect;
	this->subcompositionIndicatorPath = getSubcompositionIndicatorPath(this->frameRect, this->nodeIsSubcomposition);
	this->nodeFrames = getNodeFrames(this->frameRect);
}

/**
 * Returns a rectangle that completely encloses the rendered node (accounting for the thick line width).
 */
QRectF VuoRendererNode::boundingRect(void) const
{
	if (paintingDisabled())
		return QRectF();

	QRectF r = frameRect;

	// Header
	r.adjust(0, nodeHeaderYOffset, 0, 0);

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Returns a boolean indicating whether painting is currently disabled for this node.
 */
bool VuoRendererNode::paintingDisabled() const
{
	return this->proxyNode;
}

/**
 * Returns a boolean indicating whether the node should be rendered as if it is selected,
 * regardless of whether it has been directly selected.
 *
 * For regular (non-input-attachment) nodes, these two concepts are equivalent.
 */
bool VuoRendererNode::isEffectivelySelected()
{
	return isSelected();
}

/**
 * Returns the center point of the specified input/output port circle.
 *
 * @param port The port.
 * @param portIndex The index of the port in the list of input/output ports (including refresh/function ports).
 */
QPointF VuoRendererNode::getPortPoint(VuoRendererPort *port, unsigned int portIndex) const
{
	qreal x = (port->getOutput() ? frameRect.width() + VuoRendererPort::portRadius - 1 : -VuoRendererPort::portRadius);
	qreal y;

	if (port->getRefreshPort())
	{
		y = VuoRendererPort::portContainerMargin - 1.5 * VuoRendererPort::portSpacing;
	}
	else if (port->getFunctionPort())
	{
		y = -VuoRendererFonts::thickPenWidth / 2. + VuoRendererPort::portSpacing;
	}
	else
	{
		qreal adjustedPortIndex = (qreal)portIndex - (port->getOutput() ?
														  VuoNodeClass::unreservedOutputPortStartIndex :
														  VuoNodeClass::unreservedInputPortStartIndex);  // Skip the refresh/function port.

		qreal portOffset = adjustedPortIndex + 0.5;  // Center the port within its space.
		y = VuoRendererPort::portContainerMargin + portOffset * VuoRendererPort::portSpacing;
	}

	return QPointF(x, y);
}

/**
 * Returns the bounding rect for the node's outer frame (excluding ports).
 */
QRectF VuoRendererNode::getOuterNodeFrameBoundingRect(void) const
{
	if (paintingDisabled())
		return QRectF();

	return nodeFrames.first.boundingRect();
}

/**
 * Draws a standard node, including rectangular frame, and input and output ports.
 *
 * The node is drawn with (0,0) as the top left inner margin, so you should use a QPainter transform if you want to position the node.
 */
void VuoRendererNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (paintingDisabled())
		return;

	drawBoundingRect(painter);

	VuoRendererColors::SelectionType selectionType = (isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity = (getRenderActivity()? timeLastExecutionEnded : VuoRendererItem::notTrackingActivity);
	VuoRendererColors *colors = new VuoRendererColors(getBase()->getTintColor(), selectionType, false, VuoRendererColors::noHighlight, timeOfLastActivity, nodeIsMissing);
	drawNodeFrame(painter, frameRect, colors);

	// Node Title
	{
		QString nodeTitle = QString::fromUtf8(getBase()->getTitle().c_str());

		// QPainter::drawText expects strings to be canonically composed,
		// else it renders diacritics next to (instead of superimposed upon) their base glyphs.
		nodeTitle = nodeTitle.normalized(QString::NormalizationForm_C);

		QFont font = VuoRendererFonts::getSharedFonts()->nodeTitleFont();

		nodeTitleBoundingRect = QRectF(
			nodeTitleHorizontalMargin
					- VuoRendererFonts::getHorizontalOffset(font, nodeTitle),
			nodeHeaderYOffset + 2,
			frameRect.width() + 4. - 2. * VuoRendererFonts::thickPenWidth/2.,	// Leave room for in-event and out-event ports.
			nodeTitleHeight + 2.
		);
		painter->setPen(colors->nodeTitle());
		painter->setFont(font);
		painter->drawText(nodeTitleBoundingRect,Qt::AlignLeft,nodeTitle);
		VuoRendererItem::drawRect(painter, nodeTitleBoundingRect);
	}

	// Node Class
	{
		QFont font = VuoRendererFonts::getSharedFonts()->nodeClassFont();

		QRectF r(
			VuoRendererFonts::thickPenWidth/2.
					+ 2.
					- VuoRendererFonts::getHorizontalOffset(font, nodeClass),
			-nodeClassHeight - 2.,
			frameRect.width() + 4. - 2. * VuoRendererFonts::thickPenWidth/2.,	// Leave room for in-event and out-event ports.
			nodeClassHeight + 2.
		);
		painter->setPen(colors->nodeClass());
		painter->setFont(font);
		painter->drawText(r,Qt::AlignLeft,nodeClass);
		VuoRendererItem::drawRect(painter, r);
	}

	if (nodeIsMissing)
	{
		bool notAvailable = getBase()->isForbidden();
#if VUO_PRO
		notAvailable |= getBase()->getNodeClass()->isPro();
#endif
		QString text = notAvailable ? "Not Available" : "Not Installed";
		QFont font = VuoRendererFonts::getSharedFonts()->nodeTitleFont();

#if VUO_PRO
		if (getBase()->getNodeClass()->isPro())
		{
			QFontMetrics fm(font);
			QRect stickerRect;
			stickerRect.setSize(QSize(16,16));
			stickerRect.moveCenter(frameRect.center().toPoint());
			stickerRect.translate(-fm.boundingRect(text).width()/2 - 12, -2);
			QIcon(":/Icons/menuitem-pro.svg").paint(painter, stickerRect);
		}
#endif

		painter->setPen(colors->portTitle());
		painter->setFont(font);
		painter->drawText(frameRect, Qt::AlignCenter, text);
	}

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
 * Returns whether the node is rendered as though its implementation is missing.
 */
bool VuoRendererNode::isMissingImplementation()
{
	return nodeIsMissing;
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
		if (p->hasRenderer())
		{
			VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p->getRenderer());
			if (tp && (tp->getUncollapsedTypecastNode()->getBase() == this->getBase()))
				return tp;
		}
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
		setCacheModeForConnectedCables(QGraphicsItem::NoCache);

		if (getSnapToGrid() && !dynamic_cast<VuoRendererInputAttachment *>(this))
		{
			// Quantize position to nearest minor gridline.
			newValue = VuoRendererComposition::quantizeToNearestGridLine(value.toPointF(), VuoRendererComposition::minorGridLineSpacing);
		}
		else
		{
			// Quantize position to whole pixels.
			newValue = value.toPoint();
		}

		this->updateConnectedCableGeometry();

		for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
		{
			(*it)->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
			(*it)->updateGeometry();
			(*it)->setCacheModeForPortAndChildren(getCurrentDefaultCacheMode());
		}

		for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		{
			(*it)->setCacheModeForPortAndChildren(QGraphicsItem::NoCache);
			(*it)->updateGeometry();
			(*it)->setCacheModeForPortAndChildren(getCurrentDefaultCacheMode());
		}

		foreach (VuoRendererInputDrawer *n, getAttachedInputDrawers())
			n->updateGeometry();

		setCacheModeForConnectedCables(getCurrentDefaultCacheMode());
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

				// Attachments follow their host nodes' movements automatically.
				// Pushing their moves onto the 'Undo' stack breaks other 'Undo' commands.
				if (!dynamic_cast<VuoRendererInputAttachment *>(this))
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
		setCacheModeForConnectedCables(QGraphicsItem::NoCache);

		// When the node is (de)selected, repaint all ports, attachments, and cables (since they also reflect selection status).
		for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
		{
			(*it)->update();
			foreach (VuoRendererInputAttachment *attachment, (*it)->getAllUnderlyingUpstreamInputAttachments())
			{
				attachment->setCacheModeForConnectedCables(QGraphicsItem::NoCache);
				attachment->update();
				foreach (VuoRendererPort *attachmentInputPort, attachment->getInputPorts())
				{
					attachmentInputPort->update();
					auto rtp = dynamic_cast<VuoRendererTypecastPort *>(attachmentInputPort);
					if (rtp)
						rtp->getChildPort()->update();
				}

				attachment->updateConnectedCableGeometry();
				attachment->setCacheModeForConnectedCables(getCurrentDefaultCacheMode());
			}

			auto rtp = dynamic_cast<VuoRendererTypecastPort *>(*it);
			if (rtp)
				rtp->getChildPort()->update();
		}
		for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
			(*it)->update();

		updateConnectedCableGeometry();
		setCacheModeForConnectedCables(getCurrentDefaultCacheMode());
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
 * Handle mouse release events.
 */
void VuoRendererNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		// Display the node popover, as long as the mouse release did not mark the end of a drag.
		if (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance())
			signaler->signalNodePopoverRequested(this);
	}

	QGraphicsItem::mouseReleaseEvent(event);
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

	// If the node is editable, open the source for editing.
	else
	{
		VuoNodeClass *nodeClass = getBase()->getNodeClass();
		if (nodeClass->hasCompiler() && VuoFileUtilities::fileExists(nodeClass->getCompiler()->getSourcePath()))
			signaler->signalNodeSourceEditorRequested(this);
	}
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

		if ((*inputPort)->hasRenderer())
		{
			VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
			if (typecastPort)
			{
				vector<VuoCable *> childPortInputCables = typecastPort->getChildPort()->getBase()->getConnectedCables(includePublishedCables);
				connectedInputCables.insert(childPortInputCables.begin(), childPortInputCables.end());
			}
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

	for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
		(*it)->updateGeometry();

	for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		(*it)->updateGeometry();
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
 * Returns this node's input ports.
 *
 * @see VuoNode::getInputPorts()
 */
vector<VuoRendererPort *> &VuoRendererNode::getInputPorts(void)
{
	return inputPorts;
}

/**
 * Returns this node's output ports.
 *
 * @see VuoNode::getInputPorts()
 */
vector<VuoRendererPort *> &VuoRendererNode::getOutputPorts(void)
{
	return outputPorts;
}

/**
  * Returns an ordered list of port name-value pairs for the ports that belong to this
  * node and whose values are currently constant.
  */
vector<pair<QString, json_object *> > VuoRendererNode::getConstantPortValues()
{
	vector<pair<QString, json_object *> > portNamesAndValues;

	for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
	{
		VuoRendererPort *port = *it;
		if (port->isConstant())
		{
			QString name = port->getBase()->getClass()->getName().c_str();
			json_object *value = json_tokener_parse(port->getConstantAsString().c_str());
			portNamesAndValues.push_back(make_pair(name, value));
		}
	}

	return portNamesAndValues;
}

/**
 * Replaces input port @c old with @c new.  Affects only renderer detail ports (not base or compiler ports).
 */
void VuoRendererNode::replaceInputPort(VuoRendererPort * oldPort, VuoRendererPort * newPort)
{
	vector<VuoRendererPort *>::iterator it = find(inputPorts.begin(), inputPorts.end(), oldPort);
	*it = newPort;

	newPort->setParentItem(this);
	newPort->stackBefore(oldPort);
	oldPort->updateGeometry();
	scene()->removeItem(oldPort);
	layoutPorts();
	layoutConnectedInputDrawers();
}

/**
 * Adds input port @c newPort to the node, relocating it to the end of the input port list
 * if it was already present somewhere within the list.
 * Affects only renderer detail ports (not base or compiler ports).
 */
void VuoRendererNode::addInputPort(VuoRendererPort * newPort)
{
	vector<VuoRendererPort *>::iterator it = find(inputPorts.begin(), inputPorts.end(), newPort);
	if (it != inputPorts.end())
		inputPorts.erase(it);

	inputPorts.push_back(newPort);
	newPort->setParentItem(this);
	layoutPorts();
	layoutConnectedInputDrawers();
}

/**
 * Returns a boolean indicating whether this node has any ports with generic data types.
 */
bool VuoRendererNode::hasGenericPort(void)
{
	foreach (VuoPort *port, getBase()->getInputPorts())
	{
		if (port->hasCompiler())
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
			if (dynamic_cast<VuoGenericType *>(compilerPort->getDataVuoType()))
				return true;
		}
	}

	foreach (VuoPort *port, getBase()->getOutputPorts())
	{
		if (port->hasCompiler())
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
			if (dynamic_cast<VuoGenericType *>(compilerPort->getDataVuoType()))
				return true;
		}
	}

	return false;
}

/**
 * Sets the @c title for this node; re-lays-out its ports to accommodate the new name.
 */
void VuoRendererNode::setTitle(string title)
{
	updateGeometry();
	getBase()->setTitle(title);
	updateNodeFrameRect();
	layoutPorts();
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

/**
 * Sets the cache mode of this node and its child ports to @c mode.
 */
void VuoRendererNode::setCacheModeForNodeAndPorts(QGraphicsItem::CacheMode mode)
{
	// Caching is currently disabled for VuoRendererInputAttachments; see
	// https://b33p.net/kosada/node/6286 and https://b33p.net/kosada/node/6064 .
	if (dynamic_cast<VuoRendererInputAttachment *>(this))
		this->setCacheMode(QGraphicsItem::NoCache);
	else
		this->setCacheMode(mode);

	for (vector<VuoRendererPort *>::iterator it = inputPorts.begin(); it != inputPorts.end(); ++it)
		(*it)->setCacheModeForPortAndChildren(mode);

	for (vector<VuoRendererPort *>::iterator it = outputPorts.begin(); it != outputPorts.end(); ++it)
		(*it)->setCacheModeForPortAndChildren(mode);
}

/**
 * Sets the cache mode of this node's connected cables to @c mode.
 */
void VuoRendererNode::setCacheModeForConnectedCables(QGraphicsItem::CacheMode mode)
{
	set<VuoCable *> connectedCables = getConnectedCables(true);
	for (set<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
	{
		if (! (*cable)->hasRenderer())
			continue;  // in case cable has not yet been added to the composition, when constructing a VuoRendererComposition from an existing set of nodes and cables

		VuoRendererCable *rc = (*cable)->getRenderer();
		rc->setCacheMode(mode);
	}
}

/**
 * Sets the boolean indicating whether this node's ports, including unambiguous ones,
 * should always have their names displayed.
 *
 * Updates the relevant cached data structures.
 */
void VuoRendererNode::setAlwaysDisplayPortNames(bool displayPortNames)
{
	if (this->alwaysDisplayPortNames != displayPortNames)
	{
		this->alwaysDisplayPortNames = displayPortNames;

		foreach (VuoPort *p, getBase()->getInputPorts())
			p->getRenderer()->updateNameRect();

		foreach (VuoPort *p, getBase()->getOutputPorts())
			p->getRenderer()->updateNameRect();

		updateNodeFrameRect();
		layoutPorts();
	}
}

/**
 * Returns a boolean indicating whether the name of the provided @c port should be rendered within the node.
 */
bool VuoRendererNode::nameDisplayEnabledForPort(const VuoRendererPort *port)
{
	if (port->getOutput())
		return nameDisplayEnabledForOutputPorts();

	else // if (!port->getOutput())
		return nameDisplayEnabledForInputPorts();
}

/**
 * Returns a boolean indicating whether the names of input ports should be rendered within the node.
 */
bool VuoRendererNode::nameDisplayEnabledForInputPorts()
{
	if (alwaysDisplayPortNames)
		return true;

	if (!nameDisplayEnabledForOutputPorts())
		return true;

	bool nodeHasUnambiguousInput = ((getBase()->getInputPorts().size() == VuoNodeClass::unreservedInputPortStartIndex + 1) &&
												  (!getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex]->getRenderer()->hasPortAction()));
	if (!nodeHasUnambiguousInput)
		return true;

	// Don't hide port labels if the node contains any trigger ports.
	foreach (VuoPort *outputPort, getBase()->getOutputPorts())
	{
		if (outputPort->getClass()->getPortType() == VuoPortClass::triggerPort)
			return true;
	}

	return false;
}

/**
 * Returns a boolean indicating whether the names of output ports should be rendered within the node.
 */
bool VuoRendererNode::nameDisplayEnabledForOutputPorts()
{
	if (alwaysDisplayPortNames)
		return true;

	bool nodeHasUnambiguousOutput = ((getBase()->getOutputPorts().size() == VuoNodeClass::unreservedOutputPortStartIndex + 1) &&
												  (getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex]->getClass()->getPortType() != VuoPortClass::triggerPort));

	return !nodeHasUnambiguousOutput;
}

/**
 * Returns the highlight that should be used to indicate whether this node
 * contains ports eligible for the current cable drag.
 */
VuoRendererColors::HighlightType VuoRendererNode::getEligibilityHighlight()
{
	return _eligibilityHighlight;
}

/**
 * Sets whether this node should be highlighted to indicate that it contains ports
 * eligible for the current cable drag.
 */
void VuoRendererNode::setEligibilityHighlight(VuoRendererColors::HighlightType eligibility)
{
	_eligibilityHighlight = eligibility;
}
