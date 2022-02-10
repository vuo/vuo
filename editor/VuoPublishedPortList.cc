/**
 * @file
 * VuoPublishedPortList implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPublishedPortList.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoEditorComposition.hh"
#include "VuoInputEditor.hh"
#include "VuoInputEditorManager.hh"
#include "VuoRendererPublishedPort.hh"

Q_DECLARE_METATYPE(QListWidgetItem *)

/**
 * Class for customizing the appearance of the list insertion indicator.
 */
class InsertionIndicatorStyle : public QProxyStyle
{
public:
	/**
	 * Renders the insertion indicator.
	 */
	void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
	{
		if (element == QStyle::PE_IndicatorItemViewItemDrop)
		{
			QPen pen(QColor(160, 160, 160));
			pen.setWidth(2);
			painter->setPen(pen);
			if (option->rect.height() == 0)
				painter->drawLine(option->rect.topLeft(), option->rect.topRight());
			else
				painter->drawRect(option->rect);
		}
		else
			QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
};

const int VuoPublishedPortList::publishedPortPointerIndex = Qt::UserRole;
const qreal VuoPublishedPortList::componentCollisionRange = 6;

/**
 * Creates a published port list widget.
 */
VuoPublishedPortList::VuoPublishedPortList(QWidget *parent) :
	QListWidget(parent)
{
	composition = NULL;

	contextMenuSetPortConstant = new QAction(tr("Edit Value…"), NULL);
	contextMenuSetPortDetails = new QAction(tr("Edit Details…"), NULL);
	contextMenuRenamePublishedPort = new QAction(tr("Rename Port…"), NULL);
	contextMenuUnpublishPort = new QAction(tr("Delete Port"), NULL);

	connect(contextMenuSetPortConstant, &QAction::triggered, this, static_cast<void (VuoPublishedPortList::*)()>(&VuoPublishedPortList::editPublishedPortValue));
	connect(contextMenuSetPortDetails, &QAction::triggered, this, &VuoPublishedPortList::editPublishedPortDetails);
	connect(contextMenuRenamePublishedPort, &QAction::triggered, this, static_cast<void (VuoPublishedPortList::*)()>(&VuoPublishedPortList::renamePublishedPort));
	connect(contextMenuUnpublishPort, &QAction::triggered, this, &VuoPublishedPortList::unpublishExternalPort);

	setDragDropMode(QAbstractItemView::InternalMove);
	setDefaultDropAction(Qt::MoveAction);
	setAutoFillBackground(true);
	setStyle(new InsertionIndicatorStyle());
	setStyleSheet(QString("VuoPublishedPortList {"
													"border: 0px;"
												"}"
						)
				  );

	setMouseTracking(true);
	this->fillVerticalSpace = true;
	this->forwardingEventsToCanvas = false;
	menuSelectionInProgress = false;
}

/**
 * Returns a boolean indicating whether this published port list contains input ports
 * (as opposed to output ports).
 */
bool VuoPublishedPortList::getInput()
{
	return isInput;
}

/**
 * Sets the boolean indicating whether this published port list contains input ports
 * (as opposed to output ports).
 */
void VuoPublishedPortList::setInput(bool isInput)
{
	this->isInput = isInput;
}

/**
 * Sets the composition associated with this published port list.
 */
void VuoPublishedPortList::setComposition(VuoEditorComposition *composition)
{
	this->composition = composition;
}

/**
 * Displays a context menu tailored to the published port list item associated with the event
 * (i.e., the right-clicked item).
 */
void VuoPublishedPortList::contextMenuEvent(QContextMenuEvent *event)
{
	QListWidgetItem* activeListItem = itemAt(event->pos());
	if (! activeListItem)
	{
		// If the context menu event took place on empty space within the list, let the parent sidebar handle it.
		event->ignore();
		return;
	}

	QMenu contextMenu(this);
	contextMenu.setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

	VuoRendererPublishedPort *rendererPort = activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	VuoPublishedPort *port = static_cast<VuoPublishedPort *>(rendererPort->getBase());
	if (!port->isProtocolPort())
	{
		if (valueEditableForPublishedPort(port))
			contextMenu.addAction(contextMenuSetPortConstant);

		if (detailsEditableForPublishedPort(port))
			contextMenu.addAction(contextMenuSetPortDetails);

		if (!rendererPort->isPermanent())
		{
			contextMenu.addAction(contextMenuRenamePublishedPort);
			contextMenu.addAction(contextMenuUnpublishPort);
		}

		// Associate the right-clicked VuoRendererPublishedPort item with the context menu actions.
		contextMenuSetPortConstant->setData(activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex));
		contextMenuSetPortDetails->setData(activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex));
		contextMenuRenamePublishedPort->setData(QVariant::fromValue(activeListItem));
		contextMenuUnpublishPort->setData(activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex));
	}

	if (!contextMenu.actions().isEmpty())
	{
		menuSelectionInProgress = true;
		contextMenu.exec(event->globalPos());
		menuSelectionInProgress = false;
	}
}

/**
 * Returns a boolean indicating whether the provided published @c port has
 * a value that may be edited using an input editor.
 */
bool VuoPublishedPortList::valueEditableForPublishedPort(VuoPublishedPort *port)
{
	// Ports that are part of an active protocol cannot have their values edited.
	if (port->isProtocolPort())
		return false;

	// Neither output ports nor event-only ports can have their values edited.
	bool hasData = static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType();
	if (!(getInput() && hasData))
		return false;

	// Ports whose data types don't have available input editors cannot have their values edited.
	VuoInputEditorManager *inputEditorManager = composition->getInputEditorManager();
	VuoType *dataType = static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType();
	VuoInputEditor *inputEditorLoadedForPortDataType = (inputEditorManager? inputEditorManager->newInputEditor(dataType) : NULL);

	if (inputEditorLoadedForPortDataType)
		inputEditorLoadedForPortDataType->deleteLater();

	return inputEditorLoadedForPortDataType;
}

/**
 * Returns a boolean indicating whether the provided published @c port has
 * details that may be edited by the user.
 */
bool VuoPublishedPortList::detailsEditableForPublishedPort(VuoPublishedPort *port)
{
	// Ports that are part of an active protocol cannot have their details edited.
	if (port->isProtocolPort())
		return false;

	// Output ports cannot have their details edited.
	if (!getInput())
		return false;

	// For now, only ports with data types of VuoInteger, VuoReal, or VuoPoint*d can have their details edited.
	// Output ports cannot have their details edited.
	VuoType *dataType = static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType();

	return (dataType && ((dataType->getModuleKey() == "VuoReal")
						 || (dataType->getModuleKey() == "VuoInteger")
						 || (dataType->getModuleKey() == "VuoPoint2d")
						 || (dataType->getModuleKey() == "VuoPoint3d")
						 || (dataType->getModuleKey() == "VuoPoint4d")));
}

/**
 * Handle mouse double-click events.
 *
 * Double-clicking on an eligible port invokes the input editor for that port.
 */
void VuoPublishedPortList::mouseDoubleClickEvent(QMouseEvent *event)
{
	QListWidgetItem* activeListItem = itemAt(event->pos());

	if (activeListItem)
	{
		VuoRendererPublishedPort *port = activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		if (valueEditableForPublishedPort(dynamic_cast<VuoPublishedPort *>(port->getBase())))
			editPublishedPortValue(port);
	}

	else
		QListWidget::mouseDoubleClickEvent(event);
}

/**
 * Handle mouse press events.
 */
void VuoPublishedPortList::mousePressEvent(QMouseEvent *event)
{
	if (! composition->views().empty())
	{
		// If clicking directly on a port shape, let the composition handle the mouseclick to the port.
		VuoRendererPublishedPort *nearbyPort = getPublishedPortAtGlobalPos(event->globalPos(),
																		   VuoPublishedPortList::componentCollisionRange,
																		   true);
		if (nearbyPort)
		{
			QGraphicsSceneMouseEvent mouseEvent;
			mouseEvent.setButton(event->button());
			mouseEvent.setButtons(event->buttons());
			mouseEvent.setScenePos(composition->views()[0]->mapToScene(composition->views()[0]->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos())));
			composition->leftMousePressEventAtNearbyItem(static_cast<QGraphicsItem *>(nearbyPort), &mouseEvent);

			forwardingEventsToCanvas = true;
			event->accept();
			return;
		}
	}

	// Clicking on a disabled item (e.g., a protocol port) should clear the previous selection.
	QListWidgetItem* activeListItem = itemAt(event->pos());
	if (activeListItem && (!(activeListItem->flags() & Qt::ItemIsEnabled)))
		clearSelection();

	QListWidget::mousePressEvent(event);
}

/**
 * Handle mouse move events.
 */
void VuoPublishedPortList::mouseMoveEvent(QMouseEvent *event)
{
	if (forwardingEventsToCanvas)
	{
		QMouseEvent mouseEvent(QEvent::MouseMove,
							   composition->views()[0]->mapFromGlobal(event->globalPos()),
				event->screenPos(),
				event->button(),
				event->buttons(),
				event->modifiers());

		QApplication::sendEvent(composition->views()[0]->viewport(), &mouseEvent);

		event->accept();
		return;
	}

	emit mouseMoveEventReceived(event, VuoPublishedPortList::componentCollisionRange);

	QListWidget::mouseMoveEvent(event);
}

/**
 * Handle mouse release events.
 */
void VuoPublishedPortList::mouseReleaseEvent(QMouseEvent *event)
{
	if (forwardingEventsToCanvas)
	{
		QMouseEvent mouseEvent(QEvent::MouseButtonRelease,
							   composition->views()[0]->mapFromGlobal(event->globalPos()),
				event->screenPos(),
				event->button(),
				event->buttons(),
				event->modifiers());

		QApplication::sendEvent(composition->views()[0]->viewport(), &mouseEvent);

		forwardingEventsToCanvas = false;
		event->accept();
		return;
	}

	QListWidget::mouseReleaseEvent(event);
}

/**
 * Handle drag move events.
 */
void VuoPublishedPortList::dragMoveEvent(QDragMoveEvent *event)
{
	// Don't show the drop indicator for drops that would end among protocol ports.
	bool dropWouldFallWithinProtocolPorts = false;
	QListWidgetItem* activeListItem = (count()? itemAt(event->pos()+QPoint(0,0.5*sizeHintForRow(0))) : NULL);
	if (activeListItem)
	{
		VuoRendererPublishedPort *port = activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		if (port && dynamic_cast<VuoPublishedPort *>(port->getBase())->isProtocolPort())
			dropWouldFallWithinProtocolPorts = true;
	}

	setDropIndicatorShown(!dropWouldFallWithinProtocolPorts);
	QListWidget::dragMoveEvent(event);
}

/**
 * Calls the published port list's private dropEvent() function.
 * For use by the parent sidebar in forwarding drop events
 * whose drag events originated within this port list.
 */
void VuoPublishedPortList::adoptDropEvent(QDropEvent *event)
{
	dropEvent(event);
}

/**
 * Returns a boolean indicating whether selection from a context menu is currently in progress.
 */
bool VuoPublishedPortList::getMenuSelectionInProgress()
{
	return menuSelectionInProgress;
}

/**
 * Handle drop events.
 */
void VuoPublishedPortList::dropEvent(QDropEvent *event)
{
	if (!count())
		return;

	// Shift drops among protocol ports to just below the bottommost protocol port.
	QListWidgetItem* activeListItem = itemAt(event->pos()+QPoint(0,0.5*sizeHintForRow(0)));
	if (activeListItem)
	{
		VuoRendererPublishedPort *port = activeListItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		if (port && dynamic_cast<VuoPublishedPort *>(port->getBase())->isProtocolPort())
		{
			// Shift down one row at a time while we're still within the protocol port section.
			QDropEvent modifiedEvent(event->pos()+QPoint(0,sizeHintForRow(0)),
									 event->dropAction(),
									 event->mimeData(),
									 event->mouseButtons(),
									 event->keyboardModifiers());

			dropEvent(&modifiedEvent);
			return;
		}
	}

	QListWidget::dropEvent(event);

	// Respond to changes in port order via mouse drag.
	updatePortOrder();
}

/**
 * Handle keypress events.
 * Pressing 'Return' while the published port list has keyboard focus
 * and at least one published port within the sidebar is selected opens
 * the "Edit Value" input editor for the selected ports.
 */
void VuoPublishedPortList::keyPressEvent(QKeyEvent *event)
{
	if (forwardingEventsToCanvas)
	{
		QKeyEvent keyEvent(QEvent::KeyPress, event->key(), event->modifiers());
		QApplication::sendEvent(composition->views()[0]->viewport(), &keyEvent);

		event->accept();
		return;
	}

	if (event->key() == Qt::Key_Return)
	{
		QList<QListWidgetItem *> selectedPorts = selectedItems();
		for (QList<QListWidgetItem *>::iterator portItem = selectedPorts.begin(); portItem != selectedPorts.end(); ++portItem)
		{
			VuoRendererPublishedPort *port = (*portItem)->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
			if (valueEditableForPublishedPort(dynamic_cast<VuoPublishedPort *>(port->getBase())))
				editPublishedPortValue(port);
		}
	}

	else
		QListWidget::keyPressEvent(event);
}

/**
 * Handle key release events.
 */
void VuoPublishedPortList::keyReleaseEvent(QKeyEvent *event)
{
	if (forwardingEventsToCanvas)
	{
		QKeyEvent keyEvent(QEvent::KeyRelease, event->key(), event->modifiers());
		QApplication::sendEvent(composition->views()[0]->viewport(), &keyEvent);

		event->accept();
		return;
	}

	QListWidget::keyReleaseEvent(event);
}

/**
 * Edits the constant value of the published port associated with the sender.
 */
void VuoPublishedPortList::editPublishedPortValue()
{
	QAction *sender = static_cast<QAction *>(QObject::sender());
	VuoRendererPublishedPort *port = sender->data().value<VuoRendererPublishedPort *>();

	editPublishedPortValue(port);
}

/**
 * Edits the constant value of the published port associated with the input @c listItem.
 */
void VuoPublishedPortList::editPublishedPortValue(VuoRendererPublishedPort *port)
{
	emit inputEditorRequested(port);
}

/**
 * Edits the details of the published port associated with the sender.
 */
void VuoPublishedPortList::editPublishedPortDetails()
{
	QAction *sender = static_cast<QAction *>(QObject::sender());
	VuoRendererPublishedPort *port = sender->data().value<VuoRendererPublishedPort *>();

	emit publishedPortDetailsEditorRequested(port);
}


/**
 * Renames the published port associated with the sender.
 */
void VuoPublishedPortList::renamePublishedPort()
{
	QAction *sender = (QAction *)QObject::sender();
	QListWidgetItem * portListItem = sender->data().value<QListWidgetItem *>();
	renamePublishedPort(portListItem);
}

/**
 * Renames the published port associated with the input @c listItem.
 */
void VuoPublishedPortList::renamePublishedPort(QListWidgetItem* portListItem)
{
	VuoRendererPublishedPort *port = portListItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	if (!dynamic_cast<VuoPublishedPort *>(port->getBase())->isProtocolPort())
		emit publishedPortNameEditorRequested(port, true);
}

/**
 * Updates the ordering of the stored published ports to match the currently displayed order (e.g.,
 * after the user has re-ordered them by dragging).
 */
void VuoPublishedPortList::updatePortOrder()
{
	vector<VuoPublishedPort *> publishedPorts;
	int numPorts = count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *currentPortItem = this->item(portIndex);
		VuoRendererPublishedPort *currentRenderedPublishedPort = currentPortItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		publishedPorts.push_back(dynamic_cast<VuoPublishedPort *>(currentRenderedPublishedPort->getBase()));
	}

	emit publishedPortsReordered(publishedPorts, isInput);
	emit publishedPortModified();
}

/**
 * Unpublishes the externally visible published port associated with the sender.
 */
void VuoPublishedPortList::unpublishExternalPort()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPublishedPort *port = sender->data().value<VuoRendererPublishedPort *>();

	emit externalPortUnpublicationRequested(port);
	emit publishedPortModified();
}

/**
 * @return The recommended size for the published port list.
 */
QSize VuoPublishedPortList::sizeHint() const
{
	if (this->fillVerticalSpace)
		return QSize(QListWidget::sizeHint().width(), QWIDGETSIZE_MAX);

	else
	{
		int cumulativeListHeight = 0;
		int numRows = count();
		for (int rowIndex = 0; rowIndex < numRows; ++rowIndex)
			cumulativeListHeight += sizeHintForRow(rowIndex);

		return QSize(QListWidget::sizeHint().width(), cumulativeListHeight);
	}
}

/**
 * Updates the stored coordinates, relative to the composition viewport,
 * of each VuoRenderPublishedPort * @c port in the list.
 */
void VuoPublishedPortList::updatePublishedPortLocs()
{
	int numPorts = count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *currentPortItem = this->item(portIndex);
		VuoRendererPublishedPort *currentRenderedPublishedPort = currentPortItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		QPoint connectionPoint = QPoint(isInput? viewport()->geometry().right() :
												 viewport()->geometry().left(),
										viewport()->mapToParent(visualItemRect(currentPortItem).center()).y());

		QPoint globalPos = viewport()->parentWidget()->mapToGlobal(connectionPoint);

		if (composition && ! composition->views().empty())
		{
			QPoint compositionViewportPos = composition->views()[0]->mapFromGlobal(globalPos);
			currentRenderedPublishedPort->setCompositionViewportPos(compositionViewportPos);
		}

		currentRenderedPublishedPort->setVisible(isVisible());
	}

	emit publishedPortPositionsUpdated();
}

/**
 * Returns the published port at global position @c globalPos,
 * or NULL if no published port is at that position.
 *
 * If @c limitPortCollisionRange is false, a published port is considered to be at a given position
 * if the position is contained anywhere within the visual rectangle representing that port
 * within the list, extended by @c tolerance pixels to the left and right.
 *
 * If @c limitPortCollisionRange is true, a published port is considered to be at a given position
 * if the position is contained within the bounding rect of the port shape only,
 * extended by @c tolerance pixels to the left and right.
 */
VuoRendererPublishedPort * VuoPublishedPortList::getPublishedPortAtGlobalPos(QPoint globalPos, qreal xTolerance, bool limitPortCollisionRange)
{
	QPoint localPos = viewport()->parentWidget()->mapFromGlobal(globalPos);

	int numPorts = count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *currentPortItem = this->item(portIndex);
		QRectF currentPortRect = visualItemRect(currentPortItem);

		// Truncate item rect at list edge.
		currentPortRect.setWidth(fmin(currentPortRect.width(), width()));

		if (limitPortCollisionRange)
		{
			if (isInput)
				currentPortRect = QRectF(currentPortRect.right()-VuoRendererPort::portRadius,
										 currentPortRect.top(),
										 VuoRendererPort::portRadius,
										 currentPortRect.height());
			else
				currentPortRect = QRectF(currentPortRect.left(),
										 currentPortRect.top(),
										 VuoRendererPort::portRadius,
										 currentPortRect.height());
		}

		currentPortRect.adjust(-xTolerance, 0, xTolerance, 0);

		if (currentPortRect.contains(localPos))
			return currentPortItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
	}

	return NULL;
}

/**
 * Returns the global position of the center of the port circle.
 */
QPoint VuoPublishedPortList::getGlobalPosOfPublishedPort(VuoRendererPublishedPort *port)
{
	int numPorts = count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *currentPortItem = this->item(portIndex);
		VuoRendererPublishedPort *currentPort = currentPortItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<VuoRendererPublishedPort *>();
		if (currentPort == port)
		{
			QPoint connectionPoint = QPoint(isInput? viewport()->geometry().right() :
													 viewport()->geometry().left(),
											viewport()->mapToParent(visualItemRect(currentPortItem).center()).y());

			return viewport()->parentWidget()->mapToGlobal(connectionPoint);
		}
	}

	return QPoint(0,0);
}

/**
 * De-selects all items in the list.
 */
void VuoPublishedPortList::clearSelection()
{
	int numPorts = count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *currentPortItem = this->item(portIndex);
		currentPortItem->setSelected(false);
	}
}

/**
 * Sets the boolean indicating whether this list should expand to fill all available vertical space.
 */
void VuoPublishedPortList::setFillVerticalSpace(bool fill)
{
	this->fillVerticalSpace = fill;
	setFixedHeight(sizeHint().height());
}
