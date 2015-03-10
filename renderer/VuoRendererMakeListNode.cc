/**
 * @file
 * VuoRendererMakeListNode implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererMakeListNode.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoCable.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererTypecastPort.hh"

const qreal VuoRendererMakeListNode::drawerHorizontalSpacing = VuoRendererFonts::thickPenWidth*1./4.;
const qreal VuoRendererMakeListNode::drawerInteriorHorizontalPadding = VuoRendererFonts::thickPenWidth*3./4.+1;

/**
 * Creates a collapsed "Make List" node, which takes the form of an input drawer.
 */
VuoRendererMakeListNode::VuoRendererMakeListNode(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererNode(baseNode, signaler)
{
	vector<VuoPort *> inputPorts = this->getBase()->getInputPorts();
	for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
		drawerPorts.push_back(inputPorts[i]->getRenderer());

	QPainterPath outsetPath;
	VuoRendererPort::getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(""),&outsetPath);
	qreal minimumDrawerArmLength = outsetPath.boundingRect().width();

	this->dragHandleIsHovered = false;
	this->horizontalDrawerOffset = 0.5*VuoRendererPort::getPortRect().width() + minimumDrawerArmLength + getMaxDrawerLabelWidth();
	this->drawerBottomExtensionHeight = VuoRendererPort::portSpacing*(fmax(1,drawerPorts.size())-1);
	this->mousePositionAtLastPortCountAdjustment = QPointF(getDragHandleRect().center().x(),
														   getDragHandleRect().center().y() - (drawerPorts.size() < 1? VuoRendererPort::portSpacing: 0));
	layoutPorts();
}

/**
 * Returns the vector of the input ports whose values will be incorporated into the output list.
 */
vector<VuoRendererPort *> VuoRendererMakeListNode::getDrawerPorts(void) const
{
	return drawerPorts;
}

/**
  * Returns the input port to which this collapsed "Make List" node is attached.
  */
VuoRendererPort * VuoRendererMakeListNode::getHostInputPort(void) const
{
	VuoPort * listOutPort = getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
	vector<VuoCable *> outCables = listOutPort->getConnectedCables(true);
	if (outCables.size() < 1)
		return NULL;

	VuoCable *outCable = outCables[0];
	VuoPort *toPort = outCable->getToPort();
	if (! (toPort && toPort->hasRenderer()))
		return NULL;

	return toPort->getRenderer();
}

/**
 * Returns the maximum width in pixels of the port labels within this drawer.
 */
qreal VuoRendererMakeListNode::getMaxDrawerLabelWidth(void) const
{
	qreal maxLabelWidth = 0;
	QFont labelFont = VuoRendererFonts::getSharedFonts()->nodePortTitleFont();
	foreach (VuoRendererPort *port, drawerPorts)
	{
		string portTitle = port->getBase()->getClass()->getName();
		qreal labelWidth = QFontMetricsF(labelFont).boundingRect(QString::fromUtf8(portTitle.c_str())).width();

		if (labelWidth > maxLabelWidth)
			maxLabelWidth = labelWidth;
	}

	return maxLabelWidth + drawerInteriorHorizontalPadding;
}

/**
 * Returns the maximum width in pixels of the port labels within this drawer,
 * also accounting for the width of the child port as well as any constant flags
 * or collapsed typecasts attached to the child port.
 */
qreal VuoRendererMakeListNode::getMaxDrawerChainedLabelWidth(void) const
{
	qreal maxLabelWidth = 0;
	QFont labelFont = VuoRendererFonts::getSharedFonts()->nodePortTitleFont();
	foreach (VuoRendererPort *port, drawerPorts)
	{
		string portTitle = port->getBase()->getClass()->getName();
		qreal labelWidth = QFontMetricsF(labelFont).boundingRect(QString::fromUtf8(portTitle.c_str())).width();

		// Accommodate the width of the child port.
		labelWidth += 0.5*VuoRendererPort::getPortRect().width();

		// Accommodate the width of any collapsed typecast attached to the child port.
		if (dynamic_cast<VuoRendererTypecastPort *>(port->getBase()->getRenderer()))
		{
			VuoRendererTypecastPort *tp = (VuoRendererTypecastPort *)(port->getBase()->getRenderer());
			QPainterPath outsetPath;
			tp->getPortPath(false, true, &outsetPath);
			labelWidth += outsetPath.boundingRect().width();

			// Accommodate the width of the attached typecast's child port.
			labelWidth += 0.5*VuoRendererPort::getPortRect().width();
		}
		else

		// Accommodate the width of any constant flag attached to the child port.
		if (port->isConstant())
		{
			QPainterPath outsetPath;
			port->getPortConstantPath(VuoRendererPort::getPortRect(), QString::fromUtf8(port->getConstantAsStringToRender().c_str()),&outsetPath);
			labelWidth += outsetPath.boundingRect().width();
		}

		if (labelWidth > maxLabelWidth)
			maxLabelWidth = labelWidth;
	}

	return maxLabelWidth + drawerInteriorHorizontalPadding;
}



/**
 * Draws a collapsed "Make List" node.
 *
 * The node is drawn with (0,0) as the top left inner margin, so you should use a QPainter transform if you want to position the node.
 */
void VuoRendererMakeListNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	VuoRendererPort *hostPort = getHostInputPort();
	if (hostPort)
	{
		VuoRendererNode *hostNode = hostPort->getRenderedParentNode();
		if (hostNode && hostNode->isSelected() && !isSelected())
			this->setSelected(true);
	}

	drawBoundingRect(painter);

	VuoRendererColors::SelectionType selectionType = (isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity = (getRenderActivity()? timeLastExecutionEnded : VuoRendererItem::notTrackingActivity);
	VuoRendererColors *drawerColors = new VuoRendererColors(getBase()->getTintColor(),
													  selectionType,
													  false,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);

	VuoRendererColors *dragHandleColors = new VuoRendererColors(getBase()->getTintColor(),
													  selectionType,
													  dragHandleIsHovered,
													  VuoRendererColors::noHighlight,
													  timeOfLastActivity);

	// Drawer frame
	painter->setPen(QPen(drawerColors->nodeFrame(),1,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));
	painter->setBrush(drawerColors->nodeFill());
	QPainterPath drawerPath = getMakeListDrawerPath();
	painter->drawPath(drawerPath);

	// Drag handle frame
	painter->setPen(QPen(dragHandleColors->nodeFrame(),1,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));
	painter->setBrush(dragHandleColors->nodeFill());
	QPainterPath dragHandlePath = getMakeListDragHandlePath();
	painter->drawPath(dragHandlePath);

	// Child input ports
	layoutPorts();

	// Drag handle grip
	qreal drawerBottomExtensionWidth = getMaxDrawerLabelWidth();
	qreal handleGripLength = VuoRendererFonts::thickPenWidth/2.0;

	QPointF handleGripLeft(
				0.5*drawerBottomExtensionWidth - 0.5*handleGripLength,
				getDragHandleRect().center().y()
				);
	painter->setPen(QColor(0,0,0,32));
	painter->drawLine(
				handleGripLeft,
				handleGripLeft+QPointF(handleGripLength, 0)
				);

	delete drawerColors;
	delete dragHandleColors;
}

/**
 * Returns a closed path representing the collapsed "Make List" node.
 */
QPainterPath VuoRendererMakeListNode::getMakeListNodePath() const
{
	QPainterPath p;
	p.addPath(getMakeListDrawerPath());
	p.addPath(getMakeListDragHandlePath());

	return p;
}

/**
 * Returns a closed path representing the drawer of the collapsed "Make List" node.
 */
QPainterPath VuoRendererMakeListNode::getMakeListDrawerPath() const
{
	// Create a hybrid rect having the width of the port's inset rect and the customized
	// height of a constant flag, so that the "Make List" arm has the desired height but
	// directly adjoins the inset port shape.
	QRectF hostPortRect = VuoRendererPort::getPortRect();
	vector<VuoRendererPort *> drawerPorts = getDrawerPorts();
	QRectF hostPortHybridRect = QRectF(hostPortRect.x(), -0.5*VuoRendererPort::constantFlagHeight, hostPortRect.width(), VuoRendererPort::constantFlagHeight);

	qreal hostPortContactPointX = horizontalDrawerOffset -0.5*hostPortRect.width() + VuoRendererPort::portInset;
	qreal drawerBottomExtensionWidth = getMaxDrawerLabelWidth();

	QPainterPath p;

	// Drawer top left
	p.moveTo(0, -0.5*hostPortHybridRect.height());
	// Arm top edge
	p.lineTo(hostPortContactPointX-0.5*hostPortHybridRect.height(), -0.5*hostPortHybridRect.height());
	// Arm right triangular point
	p.lineTo(hostPortContactPointX, 0);
	p.lineTo(hostPortContactPointX-0.5*hostPortHybridRect.height(), 0.5*hostPortHybridRect.height());
	// Arm bottom edge
	p.lineTo(drawerBottomExtensionWidth, 0.5*hostPortHybridRect.height());
	// Right drawer wall
	p.lineTo(drawerBottomExtensionWidth, 0.5*hostPortHybridRect.height() + drawerBottomExtensionHeight);
	// Far drawer bottom
	p.lineTo(0, 0.5*hostPortHybridRect.height() + drawerBottomExtensionHeight);
	p.closeSubpath();

	// Carve the input child ports out of the frame.
	QPainterPath drawerPortsPath;

	for (int i=0; i < drawerPorts.size(); ++i)
	{
		VuoRendererPort *p = drawerPorts[i];
		VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p);
		QPainterPath insetPath;

		if (tp)
			insetPath = tp->getPortPath(true, false, NULL);
		else
			insetPath = p->getPortPath(VuoRendererPort::portInset);

		drawerPortsPath.addPath(insetPath.translated(p->pos()));
	}

	p = p.subtracted(drawerPortsPath);

	return p;
}

/**
 * Returns a closed path representing the drag handle of the collapsed "Make List" node.
 */
QPainterPath VuoRendererMakeListNode::getMakeListDragHandlePath() const
{
	QPainterPath p;
	QRectF dragHandleRect = getDragHandleRect();

	p.moveTo(dragHandleRect.topLeft());
	p.lineTo(dragHandleRect.topRight());
	addRoundedCorner(p, true, dragHandleRect.bottomRight(), VuoRendererNode::cornerRadius, false, false);
	addRoundedCorner(p, true, dragHandleRect.bottomLeft(), VuoRendererNode::cornerRadius, false, true);
	p.lineTo(dragHandleRect.topLeft());

	return p;
}

/**
 * Returns the bounding rectangle of this collapsed "Make List" node.
 */
QRectF VuoRendererMakeListNode::boundingRect(void) const
{
	QRectF r = getMakeListNodePath().boundingRect();

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Returns the shape of the rendered "Make List" node, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererMakeListNode::shape() const
{	
	QPainterPath p;
	p.addPath(getMakeListDrawerPath());
	p.addRect(getExtendedDragHandleRect());

	return p;
}

/**
  * Sets the distance, in pixels, left of its attached port that the rightmost point of this drawer should be rendered.
  */
void VuoRendererMakeListNode::setHorizontalDrawerOffset(qreal offset)
{
	this->horizontalDrawerOffset = offset;
}

/**
  * Calculates and sets the positions of the node's child ports relative to the node.
  */
void VuoRendererMakeListNode::layoutPorts(void)
{
	QList<VuoRendererPort *> inputPorts = this->inputPorts->childItems();
	for (unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		VuoRendererPort *p = inputPorts[i];

		if (i < VuoNodeClass::unreservedInputPortStartIndex)
			p->setVisible(false);

		else
		{
			p->setVisible(true);
			int adjustedPortIndex = i-VuoNodeClass::unreservedInputPortStartIndex;
			qreal portPointY = adjustedPortIndex*VuoRendererPort::portSpacing;
			p->setPos(QPointF(0,portPointY));

			VuoRendererTypecastPort *tp = dynamic_cast<VuoRendererTypecastPort *>(p);
			if (tp)
				tp->getChildPort()->setPos(QPointF(-tp->getChildPortXOffset(),portPointY));
		}
	}

	// Do not display output ports.
	QList<VuoRendererPort *> outputPorts = this->outputPorts->childItems();
	for (unsigned int i = 0; i < outputPorts.size(); ++i)
		outputPorts[i]->setVisible(false);
}

/**
 * Returns a rectangle encompassing the node's drag handle.
 */
QRectF VuoRendererMakeListNode::getDragHandleRect() const
{
	const qreal handleVerticalOffset = 2.5;
	const qreal handleHeight = 4;
	qreal drawerBottomExtensionWidth = getMaxDrawerLabelWidth();

	return QRectF(0,
				  0.5*VuoRendererPort::constantFlagHeight + drawerBottomExtensionHeight + handleVerticalOffset,
				  drawerBottomExtensionWidth,
				  handleHeight);
}

/**
 * Returns a rectangle encompassing the drag handle's extended hover and drag range.
 */
QRectF VuoRendererMakeListNode::getExtendedDragHandleRect() const
{
	return getDragHandleRect().adjusted(0,0,0,2);
}

/**
 * Handle mouse hover start events.
 */
void VuoRendererMakeListNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	bool cursorAboveDragHandle = (getExtendedDragHandleRect().contains(event->pos()));
	prepareGeometryChange();
	dragHandleIsHovered = cursorAboveDragHandle;

	QGraphicsItem::hoverEnterEvent(event);
}

/**
 * Handle mouse hover move events.
 */
void VuoRendererMakeListNode::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	bool cursorAboveDragHandle = (getExtendedDragHandleRect().contains(event->pos()));
	prepareGeometryChange();
	dragHandleIsHovered = cursorAboveDragHandle;

	QGraphicsItem::hoverMoveEvent(event);
}

/**
 * Handle mouse hover leave events.
 */
void VuoRendererMakeListNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	prepareGeometryChange();
	dragHandleIsHovered = false;
	QGraphicsItem::hoverLeaveEvent(event);
}

/**
 * Handle mouse press events.
 */
void VuoRendererMakeListNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	bool leftButtonPressed = (event->button() == Qt::LeftButton);
	bool pressedOnDragHandle = (getExtendedDragHandleRect().contains(event->pos()));
	if (leftButtonPressed && pressedOnDragHandle)
	{
		setDragInProgress(true);
		mousePositionAtLastPortCountAdjustment = event->pos();
	}

	else
		QGraphicsItem::mousePressEvent(event);
}

/**
 * Handle mouse move events.
 */
void VuoRendererMakeListNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	bool leftMouseButtonPressed = (event->buttons() & Qt::LeftButton);

	if (leftMouseButtonPressed && dragInProgress)
	{
		QPointF delta = event->pos() - mousePositionAtLastPortCountAdjustment;

		// If the user has dragged the drag handle down far enough, add an input port.
		if (delta.y() >= VuoRendererPort::portSpacing ||
				((delta.y() > 0) && (drawerPorts.size() == 0)))
		{
			triggerPortCountAdjustment(1, event);
		}

		// If the user has dragged the drag handle up far enough, remove an input port.
		else if (((delta.y() < 0) && (drawerPorts.size() >= 2)) ||
				 ((delta.y() <= -VuoRendererPort::portSpacing) && (drawerPorts.size() == 1)))
		{
			triggerPortCountAdjustment(-1, event);
		}
	}

	else
		QGraphicsItem::mouseMoveEvent(event);
}

/**
 * Handle mouse release events.
 */
void VuoRendererMakeListNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (scene() && (scene()->mouseGrabberItem() == this))
			ungrabMouse();

		setDragInProgress(false);
	}

	else
		QGraphicsItem::mouseReleaseEvent(event);
}

/**
  * Sets the boolean indicating whether this node's drag handle is currently being dragged.
  */
void VuoRendererMakeListNode::setDragInProgress(bool inProgress)
{
	updateGeometry();
	this->dragInProgress = inProgress;

	if (signaler)
		signaler->signalDisableDragStickiness(inProgress);
}

/**
 * Prepares to add/remove the number of input ports specified by @c portCountDelta
 * in response to the provided mouse @c event.
 */
void VuoRendererMakeListNode::triggerPortCountAdjustment(int portCountDelta, QGraphicsSceneMouseEvent *event)
{
	mousePositionAtLastPortCountAdjustment = event->pos();
	dragHandleIsHovered = false;

	if (signaler)
		signaler->signalInputPortCountAdjustmentRequested(this, portCountDelta, true);
}

/**
 * Returns the bounding rect for the node title box.
 * Since MakeList nodes don't display their titles, return the
 * entire node boundingRect instead.
 */
QRectF VuoRendererMakeListNode::getNodeTitleBoundingRect() const
{
	return boundingRect();
}
