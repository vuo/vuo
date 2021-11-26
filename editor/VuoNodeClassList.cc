/**
 * @file
 * VuoNodeClassList implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeClassList.hh"
#include "VuoNodeClassListItemDelegate.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindow.hh"
#include "VuoFileUtilities.hh"
#include "VuoNodeClass.hh"
#include "VuoRendererComposition.hh"

/**
 * Creates a node class list widget.
 */
VuoNodeClassList::VuoNodeClassList(QWidget *parent)
	: QListWidget(parent)
{
	connect(this, &VuoNodeClassList::itemDoubleClicked, this, &VuoNodeClassList::addDoubleClickedNode);
	connect(this, &VuoNodeClassList::currentItemChanged, this, &VuoNodeClassList::displayPopoverForItem);
	setAutoScroll(false);

	// Disable the horizontal scrollbar.
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

#ifdef __APPLE__
	// Disable standard OS X focus 'glow'
	setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

	this->popoversEnabled = false;
}

VuoNodeClassList::~VuoNodeClassList()
{
}

/**
 * Initiates dragging of one or more node classes (onto, e.g., a composition editor canvas).
 */
void VuoNodeClassList::startDrag(Qt::DropActions /*supportedActions*/)
{
	// Don't display a popover in response to the beginning of a node drag --
	// extraction of the popover resources can cause the drag to fail.
	disablePopovers();

	QList<QListWidgetItem *> nodeClasses = selectedItems();
	QStringList nodeClassNames;
	for (QList<QListWidgetItem *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
	{
		QListWidgetItem *nodeClass = *i;
		QVariant variantItem = nodeClass->data(VuoNodeClassListItemDelegate::classNameIndex);
		QString nodeClassName = variantItem.toString();

		nodeClassNames.append(nodeClassName);
	}

	QString nodeClassNamesScsv = nodeClassNames.join(";");

	QMimeData *mimeData = new QMimeData();
	mimeData->setData("text/scsv", nodeClassNamesScsv.toUtf8());

	// Calling 'setText()' in addition to setData()' enables the node class name(s)
	// to be displayed during the drag, but is otherwise unnecessary.
	mimeData->setText(nodeClassNamesScsv.toUtf8());

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(Qt::CopyAction);

	// Now that the drag is complete, popovers may be re-enabled.
	enablePopoversAndDisplayCurrent();
}

/**
 * If a user selects a node class and presses 'Return', trigger node instantiation.
 */
void VuoNodeClassList::keyPressEvent(QKeyEvent *event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();

	switch (event->key()) {
		// If a user selects a node class and presses 'Return', trigger node instantiation.
		case Qt::Key_Return:
		{
			// Exception: Cmd+'Return' should trigger the application-wide "Show Node Library" command.
			if (modifiers & Qt::ControlModifier)
			{
				VuoEditor *e = (VuoEditor *)QCoreApplication::instance();
				e->showNodeLibrary();
				return;
			}

			VuoEditorWindow *targetWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
			if (!targetWindow)
				break;

			// Make sure the selection has updated to match the text filter.
			while (!selectionFinalized)
			{
				QApplication::processEvents();
				usleep(USEC_PER_SEC/100);
			}

			QPointF cursorScenePos = targetWindow->getCursorScenePos();
			QPointF startPos = targetWindow->getFittedScenePos(cursorScenePos-
															   QPointF(0,VuoRendererNode::nodeHeaderYOffset));
			int nextYPos = VuoRendererComposition::quantizeToNearestGridLine(startPos,
																				  VuoRendererComposition::minorGridLineSpacing).y();
			int snapDelta = nextYPos - startPos.y();

			QList<QListWidgetItem *> nodeClasses = selectedItems();
			QList<QGraphicsItem *> newNodes;
			int maxNewNodeWidth = 0;
			// int topNewNodeHeight = 0;
			bool topmostNode = true;
			for (QList<QListWidgetItem *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
			{
				VuoRendererNode *newNode = createSelectedNode(*i, "", startPos.x(), nextYPos - (VuoRendererItem::getSnapToGrid()? 0 : snapDelta));
				if (newNode)
				{
					int prevYPos = nextYPos;
					nextYPos = VuoRendererComposition::quantizeToNearestGridLine(QPointF(0,prevYPos+newNode->boundingRect().height()),
																				 VuoRendererComposition::minorGridLineSpacing).y();
					if (nextYPos <= prevYPos+newNode->boundingRect().height())
						nextYPos += VuoRendererComposition::minorGridLineSpacing;

					if (topmostNode)
					{
						// topNewNodeHeight = newNode->boundingRect().height();
						topmostNode = false;
					}

					if (newNode->boundingRect().width() > maxNewNodeWidth)
						maxNewNodeWidth = newNode->boundingRect().width();

					newNodes.append((QGraphicsItem *)newNode);
				}
			}

			// The following adjustments disable the "Add" action on the Undo stack, so skip them for now.
			// @todo Investigate for https://b33p.net/node/10270
			/*
			// If the cursor was beyond the right edge of the viewport, adjust the nodes' x-coordinates
			// so that they are fully visible within the viewport.
			if (startPos.x() < cursorScenePos.x())
			{
				double viewportRightEdge = targetWindow->getFittedScenePos(cursorScenePos, 0).x();
				foreach (QGraphicsItem *item, newNodes)
				{
					VuoRendererNode *rn = static_cast<VuoRendererNode *>(item);
					rn->setPos(viewportRightEdge-maxNewNodeWidth, rn->getBase()->getY());
				}
			}

			// If the cursor was beyond the bottom edge of the viewport, adjust the nodes' y-coordinates
			// so that the top node is fully visible within the viewport.
			if (startPos.y() < cursorScenePos.y())
			{
				double viewportBottomEdge = targetWindow->getFittedScenePos(cursorScenePos, 0).y();
				int yAdjustment = 0;
				bool topmostNode = true;
				foreach (QGraphicsItem *item, newNodes)
				{
					VuoRendererNode *rn = static_cast<VuoRendererNode *>(item);

					if (topmostNode)
					{
						yAdjustment = (viewportBottomEdge-topNewNodeHeight) - rn->getBase()->getY();
						topmostNode = false;
					}

					rn->setPos(rn->getBase()->getX(), rn->getBase()->getY() + yAdjustment);
				}
			}
			*/

			emit componentsAdded(newNodes, targetWindow->getComposition());

			break;
		}

		// Workaround to prevent node library from autoscrolling when it shouldn't, but still
		// allow it to autoscroll when the user navigates the node library using arrow keys.
		// See https://b33p.net/kosada/node/6555 .
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_Home:
		case Qt::Key_End:
		{
			setAutoScroll(true);
			QListWidget::keyPressEvent(event);
			setAutoScroll(false);
			break;
		}

		default:
		{
			QListWidget::keyPressEvent(event);
			break;
		}
	}
}

/**
 * Instantiates and adds a node of class @c nodeClass to the associated composition editor.
 */
void VuoNodeClassList::addDoubleClickedNode(QListWidgetItem *nodeClass)
{
	VuoEditorWindow *targetWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (!targetWindow)
		return;

	QPointF startPos = targetWindow->getFittedScenePos(targetWindow->getCursorScenePos()-
													   QPointF(0,VuoRendererNode::nodeHeaderYOffset));

	QList<QGraphicsItem *> newNodes;
	VuoRendererNode *newNode = createSelectedNode(nodeClass, "", startPos.x(), startPos.y());
	newNodes.append((QGraphicsItem *)newNode);

	emit componentsAdded(newNodes, VuoEditorWindow::getMostRecentActiveEditorWindow()->getComposition());
}

/**
 * Displays the popover for the @c targetNodeClass item.
 */
void VuoNodeClassList::displayPopoverForItem(QListWidgetItem *targetItem)
{
	if (!popoversEnabled)
		return;

	if (targetItem && (!isRowHidden(row(targetItem))))
	{
		VuoCompilerNodeClass *currentNodeClass = getNodeClassForItem(targetItem);
		if (currentNodeClass)
			emit nodePopoverRequestedForClass(currentNodeClass->getBase());
	}
}

/**
 * Initiates instantiation of a node of class @c nodeClass in the associated composition editor.
 */
VuoRendererNode * VuoNodeClassList::createSelectedNode(QListWidgetItem *nodeClass, string title, double x, double y)
{
	QVariant variantItem = nodeClass->data(VuoNodeClassListItemDelegate::classNameIndex);
	QString nodeClassName = variantItem.toString();
	VuoRendererNode *newNode = NULL;
	VuoEditorWindow *targetWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();

	if (targetWindow && targetWindow->getComposition())
		newNode = targetWindow->getComposition()->createNode(nodeClassName, title, x, y);

	return newNode;
}

/**
 * Sets the filter text used to search for node classes within the library.
 */
void VuoNodeClassList::setFilterText(QString filterText)
{
	((VuoNodeClassListItemDelegate *)(this->itemDelegate()))->setFilterText(filterText);
}

/**
 * Returns the node class for the provided @c item, or NULL if none.
 */
VuoCompilerNodeClass * VuoNodeClassList::getNodeClassForItem(QListWidgetItem *item)
{
	return item ? item->data(VuoNodeClassListItemDelegate::nodeClassPointerIndex).value<VuoCompilerNodeClass *>() : nullptr;
}

/**
 * Handle mouse press events.
 */
void VuoNodeClassList::mousePressEvent(QMouseEvent *event)
{
	disablePopovers();
	QListWidget::mousePressEvent(event);
}

/**
 * Handle mouse release events.
 */
void VuoNodeClassList::mouseReleaseEvent(QMouseEvent *event)
{
	QListWidget::mouseReleaseEvent(event);

	if (!getItemAtGlobalPos(event->globalPos()))
		setCurrentItem(NULL);

	enablePopoversAndDisplayCurrent();
}

/**
 * Enables display of popovers in response to user interaction.
 */
void VuoNodeClassList::enablePopovers()
{
	popoversEnabled = true;
}

/**
 * Enables display of popovers in response to user interaction
 * and displays the popover for the current item.
 */
void VuoNodeClassList::enablePopoversAndDisplayCurrent()
{
	enablePopovers();
	displayPopoverForItem(currentItem());
}

/**
 * Disables display of popovers in response to user interaction.
 */
void VuoNodeClassList::disablePopovers()
{
	popoversEnabled = false;
}

/**
 * Returns the list item at global position @c globalPos,
 * or NULL if no item is at that position.
 */
QListWidgetItem * VuoNodeClassList::getItemAtGlobalPos(QPoint globalPos)
{
	QPoint localPos = viewport()->parentWidget()->mapFromGlobal(globalPos);

	int numItems = count();
	for (int itemIndex = 0; itemIndex < numItems; ++itemIndex)
	{
		QListWidgetItem *currentItem = this->item(itemIndex);
		QRectF currentItemRect = visualItemRect(currentItem);

		if (currentItemRect.contains(localPos))
			return currentItem;
	}

	return NULL;
}

/**
 * Handle mouse wheel events.
 */
void VuoNodeClassList::wheelEvent(QWheelEvent * event)
{
	QWheelEvent *eventCopy = new QWheelEvent(event->posF(), event->globalPosF(), event->pixelDelta(),
		// Disable horizontal scrolling.
		QPoint(0, event->angleDelta().y()),
		event->delta(), event->orientation(), event->buttons(), event->modifiers());

	QListWidget::wheelEvent(eventCopy);

	delete eventCopy;
}

/**
 * Handle context menu events.
 */
void VuoNodeClassList::contextMenuEvent(QContextMenuEvent * event)
{
	QListWidgetItem* item = itemAt(event->pos());

	// @todo https://b33p.net/kosada/node/16535 Handle multiple-item selection.
	if (!item || (selectedItems().size() > 1))
		return;

	VuoCompilerNodeClass *nodeClass = getNodeClassForItem(item);
	if (!nodeClass)
		return;

	QMenu *contextMenu = new QMenu(this);
	contextMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	populateContextMenuForNodeClass(contextMenu, nodeClass);

	contextMenu->exec(event->globalPos());
}

/**
 * Populates the provided context menu with options corresponding to the provided node class.
 */
void VuoNodeClassList::populateContextMenuForNodeClass(QMenu *contextMenu, VuoCompilerNodeClass *nodeClass)
{
	if (!nodeClass)
		return;

	QString actionText, sourcePath;
	bool nodeClassIsEditable = VuoEditorUtilities::isNodeClassEditable(nodeClass->getBase(), actionText, sourcePath);
	bool nodeClassIs3rdParty = !nodeClass->isBuiltIn();

	if (nodeClassIsEditable)
	{
		QAction *contextMenuEdit = new QAction(NULL);
		contextMenuEdit->setText(actionText);
		contextMenuEdit->setData(QVariant::fromValue(sourcePath));
		connect(contextMenuEdit, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::openFileFromSenderData);
		contextMenu->addAction(contextMenuEdit);
	}

	if (nodeClassIsEditable || nodeClassIs3rdParty)
	{
		QString modulePath = (nodeClassIsEditable? sourcePath : nodeClass->getModulePath().c_str());
		if (!modulePath.isEmpty())
		{
			QString enclosingDirUrl = "file://" + QFileInfo(modulePath).dir().absolutePath();

			QAction *contextMenuOpenEnclosingFolder = new QAction(NULL);
			contextMenuOpenEnclosingFolder->setText(tr("Show in Finder"));
			contextMenuOpenEnclosingFolder->setData(QVariant::fromValue(enclosingDirUrl));
			connect(contextMenuOpenEnclosingFolder, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::openExternalUrlFromSenderData);
			contextMenu->addAction(contextMenuOpenEnclosingFolder);

			contextMenu->addSeparator();

			QAction *contextMenuRemove = new QAction(NULL);
			contextMenuRemove->setText(tr("Move to Trash"));
			contextMenuRemove->setData(QVariant::fromValue(modulePath));
			connect(contextMenuRemove, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::removeFileFromSenderData);
			contextMenu->addAction(contextMenuRemove);
		}
	}
}
