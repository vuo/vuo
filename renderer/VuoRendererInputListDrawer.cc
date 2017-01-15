/**
 * @file
 * VuoRendererInputListDrawer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererNode.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererFonts.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoCable.hh"
#include "VuoRendererColors.hh"
#include "VuoRendererTypecastPort.hh"

/**
 * Creates a collapsed "Make List" node, which takes the form of an input drawer.
 */
VuoRendererInputListDrawer::VuoRendererInputListDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputDrawer(baseNode, signaler)
{
	dragInProgress = false;

	this->dragHandleIsHovered = false;
	this->mousePositionAtLastPortCountAdjustment = QPointF(getDragHandleRect().center().x(),
														   getDragHandleRect().center().y() - (drawerPorts.size() < 1? VuoRendererPort::portSpacing: 0));
}

/**
 * Draws a collapsed "Make List" node.
 *
 * The node is drawn with (0,0) as the top left inner margin, so you should use a QPainter transform if you want to position the node.
 */
void VuoRendererInputListDrawer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	VuoNode *hostNode = getUnderlyingHostNode();
	if (hostNode && hostNode->hasRenderer() && hostNode->getRenderer()->isSelected() && !isSelected())
		this->setSelected(true);

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
QPainterPath VuoRendererInputListDrawer::getMakeListNodePath() const
{
	QPainterPath p;
	p.addPath(getMakeListDrawerPath());
	p.addPath(getMakeListDragHandlePath());

	return p;
}

/**
 * Returns a closed path representing the drawer of the collapsed "Make List" node.
 */
QPainterPath VuoRendererInputListDrawer::getMakeListDrawerPath() const
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
QPainterPath VuoRendererInputListDrawer::getMakeListDragHandlePath() const
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
QRectF VuoRendererInputListDrawer::boundingRect(void) const
{
	QRectF r = getMakeListNodePath().boundingRect();
	r = r.united(getExtendedDragHandleRect());

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
 * Returns the shape of the rendered "Make List" node, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererInputListDrawer::shape() const
{
	QPainterPath p;
	p.addPath(getMakeListDrawerPath());
	p.addRect(getExtendedDragHandleRect());

	return p;
}

/**
 * Returns a rectangle encompassing the node's drag handle.
 */
QRectF VuoRendererInputListDrawer::getDragHandleRect() const
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
QRectF VuoRendererInputListDrawer::getExtendedDragHandleRect() const
{
	return getDragHandleRect().adjusted(0,0,0,10);
}

/**
 * Handle mouse hover start events.
 */
void VuoRendererInputListDrawer::extendedHoverEnterEvent(QPointF scenePos)
{
	bool cursorAboveDragHandle = (getExtendedDragHandleRect().translated(this->scenePos()).contains(scenePos));
	prepareGeometryChange();
	dragHandleIsHovered = cursorAboveDragHandle;
}

/**
 * Handle mouse hover move events.
 */
void VuoRendererInputListDrawer::extendedHoverMoveEvent(QPointF scenePos)
{
	bool cursorAboveDragHandle = (getExtendedDragHandleRect().translated(this->scenePos()).contains(scenePos));
	prepareGeometryChange();
	dragHandleIsHovered = cursorAboveDragHandle;
}

/**
 * Handle mouse hover leave events.
 */
void VuoRendererInputListDrawer::extendedHoverLeaveEvent()
{
	prepareGeometryChange();
	dragHandleIsHovered = false;
}

/**
 * Handle mouse press events.
 */
void VuoRendererInputListDrawer::mousePressEvent(QGraphicsSceneMouseEvent *event)
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
void VuoRendererInputListDrawer::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
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
void VuoRendererInputListDrawer::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (scene() && (scene()->mouseGrabberItem() == this))
			ungrabMouse();

		setDragInProgress(false);

		VuoRendererNode::mouseReleaseEvent(event);
	}

	else
		QGraphicsItem::mouseReleaseEvent(event);
}

/**
  * Sets the boolean indicating whether this node's drag handle is currently being dragged.
  */
void VuoRendererInputListDrawer::setDragInProgress(bool inProgress)
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
void VuoRendererInputListDrawer::triggerPortCountAdjustment(int portCountDelta, QGraphicsSceneMouseEvent *event)
{
	mousePositionAtLastPortCountAdjustment = event->pos();
	dragHandleIsHovered = false;

	if (signaler)
		signaler->signalInputPortCountAdjustmentRequested(this, portCountDelta, true);
}

/**
 * Returns the bounding rect for just the drawer, not including the drag handle.
 */
QRectF VuoRendererInputListDrawer::getOuterNodeFrameBoundingRect(void) const
{
	return getMakeListDrawerPath().boundingRect();
}
