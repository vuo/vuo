/**
 * @file
 * VuoRendererInputListDrawer implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererSignaler.hh"

/**
 * Creates a collapsed "Make List" node, which takes the form of an input drawer.
 */
VuoRendererInputListDrawer::VuoRendererInputListDrawer(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererInputDrawer(baseNode, signaler)
{
	dragInProgress = false;

	this->dragHandleIsHovered = false;
	mousePositionAtLastPortCountAdjustment = QPointF(NAN,NAN);

	updateCachedBoundingRect();
}

/**
 * Draws a collapsed "Make List" node.
 *
 * The node is drawn with (0,0) as the top left inner margin, so you should use a QPainter transform if you want to position the node.
 */
void VuoRendererInputListDrawer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	drawBoundingRect(painter);
	VuoRendererColors::SelectionType selectionType = (isEffectivelySelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	qint64 timeOfLastActivity = (getRenderActivity()? timeLastExecutionEnded : VuoRendererItem::notTrackingActivity);
	VuoRendererColors *drawerColors = new VuoRendererColors(getBase()->getTintColor(),
													  selectionType,
													  false,
													  _eligibilityHighlight,
													  timeOfLastActivity);

	VuoRendererColors *dragHandleColors = new VuoRendererColors(getBase()->getTintColor(),
													  selectionType,
													  dragHandleIsHovered,
													  _eligibilityHighlight,
													  timeOfLastActivity);

	// Drawer frame
	QPainterPath drawerFrame = getDrawerPath(true);
	painter->fillPath(drawerFrame, drawerColors->nodeFill());

	// Drag handle
	if (dragHandleIsHovered)
	{
		painter->setClipPath(drawerFrame);
		painter->fillRect(getDragHandleRect(), dragHandleColors->nodeFill());
		painter->setClipping(false);
	}
	painter->fillPath(getMakeListDragHandlePath(), dragHandleColors->nodeFrame());

	// Child input ports
	layoutPorts();

	delete drawerColors;
	delete dragHandleColors;
}

/**
 * Returns a closed path representing the collapsed "Make List" node.
 */
QPainterPath VuoRendererInputListDrawer::getMakeListNodePath() const
{
	QPainterPath p;
	p.addPath(getDrawerPath(true));
	return p;
}

/**
 * Returns a closed path representing the drag handle of the collapsed "Make List" node.
 */
QPainterPath VuoRendererInputListDrawer::getMakeListDragHandlePath() const
{
	QPainterPath p;
	QRectF r = getDragHandleRect();
	QRectF r2(floor(r.center().x()) - 3, r.top()+2, 7, 1);
	p.addRoundedRect(r2, 1,1);
	return p;
}

/**
 * Returns the bounding rectangle of this collapsed "Make List" node.
 */
QRectF VuoRendererInputListDrawer::boundingRect(void) const
{
	return cachedBoundingRect;
}

void VuoRendererInputListDrawer::updateCachedBoundingRect()
{
	cachedBoundingRect = getMakeListNodePath().boundingRect();
	cachedBoundingRect = cachedBoundingRect.united(getExtendedDragHandleRect());

	// Antialiasing bleed
	cachedBoundingRect.adjust(-1,-1,1,1);

	cachedBoundingRect = cachedBoundingRect.toAlignedRect();
}

/**
 * Schedules a redraw of this drawer.
 */
void VuoRendererInputListDrawer::updateGeometry()
{
	VuoRendererInputDrawer::updateGeometry();
	updateCachedBoundingRect();
}

/**
 * Returns the shape of the rendered "Make List" node, for use in collision detection,
 * hit tests, and QGraphicsScene::items() functions.
 */
QPainterPath VuoRendererInputListDrawer::shape() const
{
	QPainterPath p;
	p.addPath(getDrawerPath(true));
	p.addRect(getExtendedDragHandleRect());
	p.setFillRule(Qt::WindingFill);

	return p;
}

/**
 * Returns a rectangle encompassing the node's drag handle.
 */
QRectF VuoRendererInputListDrawer::getDragHandleRect() const
{
	const qreal handleVerticalOffset = 6;
	const qreal handleHeight = 6;
	qreal drawerBottomExtensionWidth = getMaxDrawerLabelWidth();

	return QRectF(VuoRendererPort::portRadius - 1,
				  drawerBottomExtensionHeight + handleVerticalOffset,
				  qRound(drawerBottomExtensionWidth) - VuoRendererPort::portRadius + 1,
				  handleHeight);
}

/**
 * Returns a rectangle encompassing the drag handle's extended hover and drag range.
 */
QRectF VuoRendererInputListDrawer::getExtendedDragHandleRect() const
{
	return getDragHandleRect().adjusted(-3,-5,3,5);
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
		if (isnan(mousePositionAtLastPortCountAdjustment.y()))
			mousePositionAtLastPortCountAdjustment = event->pos();

		QPointF delta = event->pos() - mousePositionAtLastPortCountAdjustment;
		double steps = delta.y() / VuoRendererPort::portSpacing;

		// If the user has dragged the drag handle down, add 1 or more input ports.
		if (steps >= 1.)
			triggerPortCountAdjustment(round(steps), event);

		// If the user has dragged the drag handle up, remove 1 or more input ports.
		else if (steps <= -1. && drawerPorts.size() >= 1)
			triggerPortCountAdjustment(fmax(-(int)drawerPorts.size(), round(steps)), event);
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

	mousePositionAtLastPortCountAdjustment = QPointF(NAN,NAN);
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
	return getDrawerPath(true).boundingRect();
}
