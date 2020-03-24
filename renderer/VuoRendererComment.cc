/**
 * @file
 * VuoRendererComment implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererComment.hh"
#include "VuoRendererComposition.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererFonts.hh"
#include "VuoStringUtilities.hh"
#include "VuoComment.hh"

const qreal VuoRendererComment::cornerRadius = 10 /*VuoRendererFonts::thickPenWidth/2.0*/;  ///< The radius of rounded corners.
const qreal VuoRendererComment::borderWidth  = 5.5 /*VuoRendererPort::portBarrierWidth*/;   ///< The width of the border (drag handle).
const qreal VuoRendererComment::textMargin   = 5 /*VuoRendererFonts::thickPenWidth/4.0*/;   ///< The margin around the comment text.

/**
 * Creates a renderer detail for the specified base comment.
 */
VuoRendererComment::VuoRendererComment(VuoComment *baseComment, VuoRendererSignaler *signaler)
	: VuoBaseDetail<VuoComment>("VuoRendererComment from VuoCompilerComment", baseComment)
{
	getBase()->setRenderer(this);
	setZValue(commentZValue);
	this->resizeDragInProgress = false;
	this->resizeDeltaIgnored = QPointF(0,0);
	this->dragHandleHovered = false;
	this->titleHandleHovered = false;
	this->bodySelectable = true;

	this->textItem = new QGraphicsTextItem(this);

	VuoRendererColors *colors = new VuoRendererColors(getBase()->getTintColor(), VuoRendererColors::noSelection);
	textItem->setDefaultTextColor(colors->commentText());

	textItem->setPos(QPoint(textMargin, textMargin));
	textItem->setTextInteractionFlags(Qt::TextBrowserInteraction); // necessary for clickable links
	textItem->setFlag(QGraphicsItem::ItemIsSelectable, false); // disable dotted border
	textItem->setFlag(QGraphicsItem::ItemIsFocusable, false);  // disable dotted border
	textItem->setOpenExternalLinks(true);
	updateFormattedCommentText();

	// Set up signaler after comment has been positioned to avoid sending a spurious commentMoved signal.
	this->signaler = NULL;

	this->setFlags(QGraphicsItem::ItemIsMovable |
				   QGraphicsItem::ItemIsSelectable |
				   QGraphicsItem::ItemIsFocusable |
				   QGraphicsItem::ItemSendsGeometryChanges);

	this->setAcceptHoverEvents(true);

	setPos(baseComment->getX(), baseComment->getY());
	updateFrameRect();

	this->signaler = signaler;
}

/**
 * Returns the comment frame for a comment with the provided `commentFrameRect` value.
 */
QPainterPath VuoRendererComment::getCommentFrame(QRectF commentFrameRect) const
{
	// Rounded rectangle
	QPainterPath frame;
	frame.moveTo(commentFrameRect.right(), commentFrameRect.center().y());
	addRoundedCorner(frame, true, commentFrameRect.bottomRight(), cornerRadius, false, false);
	addRoundedCorner(frame, true, commentFrameRect.bottomLeft(), cornerRadius, false, true);
	addRoundedCorner(frame, true, commentFrameRect.topLeft(), cornerRadius, true, true);
	addRoundedCorner(frame, true, commentFrameRect.topRight(), cornerRadius, true, false);

	return frame;
}

/**
 * Returns a rectangle that completely encloses the rendered title handle, with some extra buffer.
 */
QRectF VuoRendererComment::extendedTitleHandleBoundingRect(void) const
{
	QRectF r = getTitleHandlePath(frameRect).boundingRect();

	r.setLeft(boundingRect().left());
	r.setRight(boundingRect().right());
	r.adjust(0,0,0,textMargin-r.bottom());

	return r.toAlignedRect();
}

/**
 * Returns a rectangle that completely encloses the rendered drag handle, with some extra buffer.
 */
QRectF VuoRendererComment::extendedDragHandleBoundingRect(void) const
{
	QRectF r = getDragHandlePath(frameRect).boundingRect();
	r.adjust(-borderWidth/2.0, -borderWidth/2.0, borderWidth/2.0, borderWidth/2.0);
	r.adjust(-10, -10, 10, 10);

	return r.toAlignedRect();
}

/**
 * Returns a rectangle that completely encloses the rendered comment.
 */
QRectF VuoRendererComment::boundingRect(void) const
{
	QRectF r = this->frameRect;

	// Antialiasing bleed
	r.adjust(-1,-1,1,1);

	return r.toAlignedRect();
}

/**
+ * Returns the shape of the rendered comment, for use in collision detection,
+ * hit tests, and QGraphicsScene::items() functions.
+ */
QPainterPath VuoRendererComment::shape() const
{
	   if (bodySelectable || isSelected())
	   {
		   QPainterPath p;
		   p.addRect(boundingRect());
		   return p;
	   }
	   else
	   {
		   QPainterPath p;
		   p.addPath(getTitleHandlePath(frameRect));

		   QPainterPathStroker s;
		   s.setWidth(textMargin);

		   return s.createStroke(p);
	   }
}

/**
 * Draws a standard comment, including a rectangular frame with rounded corners.
 */
void VuoRendererComment::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter->setRenderHint(QPainter::Antialiasing, true);
	drawBoundingRect(painter);

	VuoRendererColors::SelectionType selectionType = (isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	VuoRendererColors *colors = new VuoRendererColors(getBase()->getTintColor(), selectionType);

	// @todo https://b33p.net/kosada/node/9986 Quantize ?
	drawCommentFrame(painter, colors);
	drawTextContent(painter);
	drawTitleHandle(painter, colors);
	drawDragHandle(painter, colors);

	delete colors;
}

/**
 * Draws a filled rounded rectangle representing the comment background.
 */
void VuoRendererComment::drawCommentFrame(QPainter *painter, VuoRendererColors *colors) const
{
	// Filled rounded rectangle
	painter->fillPath(commentFrame, colors->commentFill());
}

/**
 * Draws the text content of the comment.
 */
void VuoRendererComment::drawTextContent(QPainter *painter) const
{
	// The actual QGraphicsTextItem child will paint itself.
	QRectF textContentBoundingRect = QRectF(textMargin,
											textMargin,
											frameRect.width() - 2*textMargin,
											frameRect.height() - 2*textMargin);
	VuoRendererItem::drawRect(painter, textContentBoundingRect);
}

/**
 * Draws the title handle.
 */
void VuoRendererComment::drawTitleHandle(QPainter *painter, VuoRendererColors *colors) const
{
	bool paintTitleHandle = (titleHandleHovered || isSelected());
	painter->setPen(QPen(paintTitleHandle? colors->commentFrame() : colors->commentFill(), borderWidth, Qt::SolidLine, Qt::RoundCap));
	painter->drawPath(getTitleHandlePath(frameRect));
	VuoRendererItem::drawRect(painter, extendedTitleHandleBoundingRect());
}

/**
 * Constructs the path for the comment title handle for a comment with the provided `commentFrameRect` value.
 */
QPainterPath VuoRendererComment::getTitleHandlePath(QRectF commentFrameRect) const
{
	QPainterPath p;
	p.moveTo(commentFrameRect.topLeft()+QPointF(0.5*borderWidth+cornerRadius, 0.5*borderWidth));
	p.lineTo(commentFrameRect.topRight()+QPointF(-0.5*borderWidth-cornerRadius, 0.5*borderWidth));

	return p;
}

/**
 * Draws the comment resize handle.
 */
void VuoRendererComment::drawDragHandle(QPainter *painter, VuoRendererColors *colors) const
{
	painter->setPen(QPen(dragHandleHovered? colors->commentFrame() : colors->commentFill(), borderWidth, Qt::SolidLine, Qt::RoundCap));
	painter->drawPath(getDragHandlePath(frameRect));
	VuoRendererItem::drawRect(painter, extendedDragHandleBoundingRect());
}

/**
 * Constructs the path for the comment resize handle for a comment with the provided `commentFrameRect` value.
 */
QPainterPath VuoRendererComment::getDragHandlePath(QRectF commentFrameRect) const
{
	QPainterPath p;
	addRoundedCorner(p, false, commentFrameRect.bottomRight()-QPointF(0.5*borderWidth, 0.5*borderWidth), cornerRadius, false, false);

	return p;
}

/**
 * Updates the comment to reflect changes in state.
 */
QVariant VuoRendererComment::itemChange(GraphicsItemChange change, const QVariant &value)
{
	QVariant newValue = value;

	if (change == QGraphicsItem::ItemSceneHasChanged)
	{
		// Scene event filters can only be installed on graphics items after they have been added to a scene.
		if (scene())
			textItem->installSceneEventFilter(this);
	}

	if (change == QGraphicsItem::ItemPositionChange)
	{
		if (getSnapToGrid())
		{
			// Quantize position to nearest minor gridline.
			newValue = VuoRendererComposition::quantizeToNearestGridLine(value.toPointF(), VuoRendererComposition::minorGridLineSpacing);
		}
		else
		{
			// Quantize position to whole pixels.
			newValue = value.toPoint();
		}
	}

	// Comment has moved within its parent
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
				set<VuoRendererComment *> movedComments;
				movedComments.insert(this);
				signaler->signalCommentsMoved(movedComments, dx, dy, true);
			}
		}

		return newPos;
	}

	if (change == QGraphicsItem::ItemSelectedHasChanged)
	{
		updateGeometry();
		updateColor();
	}

	return QGraphicsItem::itemChange(change, newValue);
}

/**
 * Filter events on watched graphics items.
 */
bool VuoRendererComment::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if (watched == this->textItem)
	{
		// Respond to double-clicks in our own VuoRendererComment event handler even if the
		// double-click occurred over the comment text.
		if (event->type() == QEvent::GraphicsSceneMouseDoubleClick)
		{
			QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);

			// Map the event position from child coordinates to local coordinates.
			mouseEvent->setPos(mapFromItem(this->textItem, mouseEvent->pos()));

			mouseDoubleClickEvent(mouseEvent);
			return true; // Comment text does not need to know about mouse double-click events.
		}

		// Disable text selection via mouse-drag within comment text.
		{
			if (event->type() == QEvent::GraphicsSceneMouseMove)
			{
				QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);

				// Map the event position from child coordinates to local coordinates.
				mouseEvent->setPos(mapFromItem(this->textItem, mouseEvent->pos()));

				mouseMoveEvent(mouseEvent);
				return true; // Comment text does not need to know about mouse move events.
			}
			if (event->type() == QEvent::GraphicsSceneMousePress)
			{
				QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);

				// Map the event position from child coordinates to local coordinates.
				mouseEvent->setPos(mapFromItem(this->textItem, mouseEvent->pos()));

				// Remove Shift modifier from mouse-press events to prevent it from triggering unpredictable text selection.
				Qt::KeyboardModifiers modifiersOtherThanShift = (mouseEvent->modifiers() & ~Qt::ShiftModifier);
				mouseEvent->setModifiers(modifiersOtherThanShift);

				mousePressEvent(mouseEvent);

				// Remap the event position to child coordinates before passing it along to the child.
				mouseEvent->setPos(mapToItem(this->textItem, mouseEvent->pos()));

				return false; // Comment text needs to know about mouse presses on links.
			}
			if (event->type() == QEvent::GraphicsSceneMouseRelease)
			{
				QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);

				// Map the event position from child coordinates to local coordinates.
				mouseEvent->setPos(mapFromItem(this->textItem, mouseEvent->pos()));

				mouseReleaseEvent(mouseEvent);

				// Remap the event position to child coordinates before passing it along to the child.
				mouseEvent->setPos(mapToItem(this->textItem, mouseEvent->pos()));

				return false; // Comment text needs to know about mouse releases on links.
			}
		}

		// Make sure this comment's hover highlighting is updated when its child text item
		// is receiving the hover move events.
		if (event->type() == QEvent::GraphicsSceneHoverMove)
		{
			QGraphicsSceneHoverEvent *hoverEvent = static_cast<QGraphicsSceneHoverEvent *>(event);

			// Map the event position from child coordinates to local coordinates.
			hoverEvent->setPos(mapFromItem(this->textItem, hoverEvent->pos()));

			hoverMoveEvent(hoverEvent);
			return true; // Comment text does not need to know about hover events.
		}
	}

	return false;
}

/**
 * Handle mouse hover start events.
 */
void VuoRendererComment::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	hoverMoveEvent(event);
}

/**
 * Handle mouse hover move events.
 */
void VuoRendererComment::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	// Update hover highlighting for the drag handle.
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);
	prepareGeometryChange();
	dragHandleHovered = dragHandleHoveredForEventPos(event->pos());
	titleHandleHovered = titleHandleHoveredForEventPos(event->pos());
	setCacheMode(normalCacheMode);
}

/**
 * Handle mouse hover leave events.
 */
void VuoRendererComment::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);
	prepareGeometryChange();
	dragHandleHovered = false;
	titleHandleHovered = false;
	setCacheMode(normalCacheMode);
}

/**
 * Handle mouse double-click events.
 */
void VuoRendererComment::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->modifiers() & Qt::ShiftModifier)
		signaler->signalCommentZoomRequested(this);
	else
		signaler->signalCommentEditorRequested(this);
}

/**
 * Handle mouse press events.
 */
void VuoRendererComment::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if ((event->button() == Qt::LeftButton) && dragHandleHoveredForEventPos(event->pos()))
	{
		resizeDragInProgress = true;
		resizeDeltaIgnored = QPointF(0,0);
	}
	else if ((event->button() == Qt::LeftButton) && titleHandleActiveForEventPos(event->pos()))
		QGraphicsItem::mousePressEvent(event);
	else
		event->ignore();
}

/**
 * Handle mouse move events.
 */
void VuoRendererComment::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if ((event->buttons() & Qt::LeftButton) && resizeDragInProgress)
	{
		const qreal minSize = VuoRendererComposition::minorGridLineSpacing*2.0; ///< The minimum width and height of a comment.
		const QPointF eventDelta = event->scenePos().toPoint() - event->lastScenePos().toPoint();

		QPointF requestedDelta = eventDelta;

		// Wait to start expanding until the mouse passes back over the point where clamping began.
		if ((requestedDelta.x() > 0) && (resizeDeltaIgnored.x() < 0))
		{
			requestedDelta.setX(requestedDelta.x() + resizeDeltaIgnored.x());
			resizeDeltaIgnored.setX(0);
		}

		if ((requestedDelta.y() > 0) && (resizeDeltaIgnored.y() < 0))
		{
			requestedDelta.setY(requestedDelta.y() + resizeDeltaIgnored.y());
			resizeDeltaIgnored.setY(0);
		}

		qreal widthUnderflow = minSize - (getBase()->getWidth()+requestedDelta.x());
		qreal heightUnderflow = minSize - (getBase()->getHeight()+requestedDelta.y());

		// Don't allow the user to resize the comment below the minimum comment dimensions.
		QPointF adjustedDelta((widthUnderflow <= 0? requestedDelta.x() : requestedDelta.x()+widthUnderflow),
							  (heightUnderflow <= 0? requestedDelta.y() : requestedDelta.y()+heightUnderflow));

		updateGeometry();
		getBase()->setWidth(getBase()->getWidth()+adjustedDelta.x());
		getBase()->setHeight(getBase()->getHeight()+adjustedDelta.y());
		updateFrameRect();

		resizeDeltaIgnored += requestedDelta - adjustedDelta;

		// Don't allow the user to resize the comment so that it no longer encloses its text content.
		int yTextOverflow = textItem->boundingRect().height()+2*textMargin - frameRect.height();
		if ((yTextOverflow > 0) && ((adjustedDelta.y() < 0) || adjustedDelta.x() < 0))
		{
			updateGeometry();
			getBase()->setWidth(getBase()->getWidth()-adjustedDelta.x());
			getBase()->setHeight(getBase()->getHeight()-adjustedDelta.y());
			updateFrameRect();

			resizeDeltaIgnored += adjustedDelta;
		}

		else if (signaler)
			signaler->signalCommentResized(this, adjustedDelta.x(), adjustedDelta.y());
	}

	else if ((event->buttons() & Qt::LeftButton) && titleHandleActiveForEventPos(event->buttonDownPos(Qt::LeftButton)))
		QGraphicsItem::mouseMoveEvent(event);

	else
		event->ignore();
}

/**
 * Handle mouse release events.
 */
void VuoRendererComment::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		resizeDragInProgress = false;

	if ((event->button() == Qt::LeftButton) && titleHandleActiveForEventPos(event->buttonDownPos(Qt::LeftButton)))
			QGraphicsItem::mouseReleaseEvent(event);
	else
		event->ignore();
}

/**
 * Schedules a redraw of this comment.
 */
void VuoRendererComment::updateGeometry(void)
{
	this->prepareGeometryChange();
}

/**
 * Sets the @c text content for this comment.
 */
void VuoRendererComment::setContent(string content)
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);
	updateGeometry();

	getBase()->setContent(content);
	updateFormattedCommentText();

	// Expand as necessary to accommodate the modified text. Normally we will at most need
	// to expand vertically, but some richtext content doesn't get word-wrapped
	// by the QGraphicsTextItem, so in these cases we might need to expand horizontally as well.
	QPointF minSize(max(getBase()->getWidth()*1.,
						this->textItem->boundingRect().toAlignedRect().width()+2*textMargin),
					max(getBase()->getHeight()*1.,
						this->textItem->boundingRect().toAlignedRect().height()+2*textMargin));

	getBase()->setWidth(minSize.toPoint().x());
	getBase()->setHeight(minSize.toPoint().y());

	updateFrameRect();
	setCacheMode(normalCacheMode);
}

/**
 * Sets the boolean indicating whether the comment should respond to
 * mouse events such as rubberband drag selection that occur within
 * the main body of the comment, outside of the comment's title handle.
 */
void VuoRendererComment::setBodySelectable(bool bodySelectable)
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);
	updateGeometry();

	this->bodySelectable = bodySelectable;

	setCacheMode(normalCacheMode);
}

/**
  * Retrieves the current text content from the base comment and updates the formatted
  * text displayed within this comment graphics item accordingly.
  */
void VuoRendererComment::updateFormattedCommentText()
{
	string textWithoutQuotes = "";

	// Unescape the JSON string.
	json_object *js = json_tokener_parse(getBase()->getContent().c_str());
	if (json_object_get_type(js) == json_type_string)
		textWithoutQuotes = json_object_get_string(js);
	json_object_put(js);

	QString textContent = QString::fromUtf8(textWithoutQuotes.c_str());

	// QPainter::drawText expects strings to be canonically composed,
	// else it renders diacritics next to (instead of superimposed upon) their base glyphs.
	textContent = textContent.normalized(QString::NormalizationForm_C);
	this->textItem->setHtml(generateTextStyleString()
							.append(VuoStringUtilities::generateHtmlFromMarkdown(textContent.toUtf8().constData()).c_str()));
}

/**
 * Calculates and updates the cached frame of a comment based on its current attributes.
 */
void VuoRendererComment::updateFrameRect(void)
{
	QRectF updatedFrameRect;
	int unalignedFrameWidth = floor(getBase()->getWidth());
	int unalignedFrameHeight = floor(getBase()->getHeight());

	// If in snap-to-grid mode, snap to the next-largest horizontal and vertical grid positions.
	int alignedFrameWidth = (getSnapToGrid()?
								 VuoRendererComposition::quantizeToNearestGridLine(QPointF(unalignedFrameWidth, 0), VuoRendererComposition::minorGridLineSpacing).x() :
								 unalignedFrameWidth);

	if (alignedFrameWidth < unalignedFrameWidth)
		alignedFrameWidth += VuoRendererComposition::minorGridLineSpacing;

	int alignedFrameHeight = unalignedFrameHeight;
	if (getSnapToGrid())
	{
		// Make it possible to enclose nodes inside comments with equal margins on the top and bottom by accounting for
		// the little bit of extra space that all nodes extend vertically beyond minor gridline increments.
		const int nodeHeightOverflow = floor(VuoRendererPort::portContainerMargin*2) + 1;

		alignedFrameHeight = VuoRendererComposition::quantizeToNearestGridLine(QPointF(0, unalignedFrameHeight-nodeHeightOverflow), VuoRendererComposition::minorGridLineSpacing).y()
							 + nodeHeightOverflow;

		if (alignedFrameHeight < unalignedFrameHeight)
			alignedFrameHeight += VuoRendererComposition::minorGridLineSpacing;
	}

	updatedFrameRect.setWidth(alignedFrameWidth);
	updatedFrameRect.setHeight(alignedFrameHeight);

	// Correct for the fact that VuoRendererComposition::drawBackground() corrects for the fact that
	// VuoRendererNode::paint() starts painting at (-1,0) rather than (0,0), to keep edges
	// aligned with grid lines.
	// @todo: Eliminate this correction after modifying VuoRendererNode::paint()
	// for https://b33p.net/kosada/node/10210 .
	const int xAlignmentCorrection = -1;
	updatedFrameRect.adjust(xAlignmentCorrection, 0, xAlignmentCorrection, 0);

	if (this->frameRect != updatedFrameRect)
	{
		this->frameRect = updatedFrameRect;
		this->commentFrame = getCommentFrame(this->frameRect);
		this->textItem->setTextWidth(frameRect.width() - 2*textMargin);
	}
}

/**
 * Regenerates the text and background to use the currently active light/dark style.
 */
void VuoRendererComment::updateColor()
{
	QGraphicsItem::CacheMode normalCacheMode = cacheMode();
	setCacheMode(QGraphicsItem::NoCache);
	prepareGeometryChange();

	VuoRendererColors::SelectionType selectionType = (isSelected()? VuoRendererColors::directSelection : VuoRendererColors::noSelection);
	VuoRendererColors *colors = new VuoRendererColors(getBase()->getTintColor(), selectionType);
	this->textItem->setDefaultTextColor(colors->commentText());
	updateFormattedCommentText();

	setCacheMode(normalCacheMode);
}

/**
 * Returns a boolean indicating whether the comment's title handle functionality
 * should be enabled for a mouse event at the provided position.
 */
bool VuoRendererComment::titleHandleActiveForEventPos(QPointF pos)
{
	return (titleHandleHoveredForEventPos(pos) || (textItem->boundingRect().contains(pos)) || isSelected());
}

/**
 * Returns a boolean indicating whether the comment's title handle is currently being hovered.
 */
bool VuoRendererComment::titleHandleHoveredForEventPos(QPointF pos)
{
	return (extendedTitleHandleBoundingRect().contains(pos) && !dragHandleHoveredForEventPos(pos));
}

/**
 * Returns a boolean indicating whether the comment's resize drag handle is currently being hovered.
 */
bool VuoRendererComment::dragHandleHoveredForEventPos(QPointF pos)
{
	return extendedDragHandleBoundingRect().contains(pos);
}

/**
 * Returns the stylesheet for comment text.
 */
QString VuoRendererComment::generateTextStyleString()
{
	VuoRendererFonts *f = VuoRendererFonts::getSharedFonts();
	return VUO_QSTRINGIFY(
					<style>
					* {
						%2;
					}
					a {
						color: %1;
					}
					h1,h2,h3,h4,h5,h6,b,strong,th {
						font-weight: bold;
					}
					pre,code {
						font-family: 'Monaco';
						font-size: 12px;
						background-color: %3;
						white-space: pre-wrap;
					}
					</style>)
				.arg(VuoRendererColors::isDark() ? "#88a2de" : "#74acec")
				.arg(f->getCSS(f->commentFont()))
				.arg(VuoRendererColors::isDark() ? "rgba(0,0,0,.25)" : "rgba(255,255,255,.6)");
}
