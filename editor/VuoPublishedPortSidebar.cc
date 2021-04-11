/**
 * @file
 * VuoPublishedPortSidebar implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoPublishedPortSidebar.hh"
#include "ui_VuoPublishedPortSidebar.h"

#include "VuoPublishedPortListItemDelegate.hh"

#include "VuoCompilerCable.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoComposition.hh"
#include "VuoDetailsEditorNumeric.hh"
#include "VuoDetailsEditorPoint.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoModuleManager.hh"
#include "VuoProtocol.hh"
#include "VuoPublishedPortNameEditor.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a published port list sidebar.
 */
VuoPublishedPortSidebar::VuoPublishedPortSidebar(QWidget *parent, VuoEditorComposition *composition, bool isInput, bool enableProtocolChanges) :
	QDockWidget(parent),
	ui(new Ui::VuoPublishedPortSidebar)
{
	ui->setupUi(this);
	this->composition = composition;
	this->isInput = isInput;
	this->portTypeMenusPopulated = false;
	this->menuSelectionInProgress = false;

	setWindowTitle(isInput ? tr("Inputs") : tr("Outputs"));
	updateActiveProtocol();

	setFocusPolicy(Qt::ClickFocus);
	setAcceptDrops(true);

	widget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
	widget()->setMinimumWidth(100);
	widget()->setMaximumWidth(100);

	ui->newPublishedPortButton->setIcon(QIcon(":/Icons/new.svg"));

	menuAddPort = new QMenu(this);
	menuAddPort->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuAddPort->setTitle(tr("New Port"));
	ui->newPublishedPortButton->setMenu(menuAddPort);
	connect(menuAddPort, &QMenu::aboutToShow, this, &VuoPublishedPortSidebar::populatePortTypeMenus);

#ifdef __APPLE__
	// Disable standard OS X focus 'glow', since it looks bad when the contents margins are so narrow.
	ui->publishedPortList->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

	contextMenuPortOptions = new QMenu(this);
	contextMenuPortOptions->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

	menuChangeProtocol = new QMenu(contextMenuPortOptions);
	menuChangeProtocol->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuChangeProtocol->setTitle(tr("Change Protocol"));
	if (enableProtocolChanges)
		contextMenuPortOptions->addMenu(menuChangeProtocol);

	contextMenuRemoveProtocol = new QMenu(this);
	contextMenuRemoveProtocol->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	contextMenuActionRemoveProtocol = new QAction(tr("Remove Protocol"), NULL);

	if (enableProtocolChanges)
		contextMenuRemoveProtocol->addAction(contextMenuActionRemoveProtocol);

	connect(ui->publishedPortList, &VuoPublishedPortList::publishedPortModified, this, &VuoPublishedPortSidebar::updatePortList);
	connect(ui->publishedPortList, &VuoPublishedPortList::publishedPortNameEditorRequested, this, &VuoPublishedPortSidebar::publishedPortNameEditorRequested);
	connect(ui->publishedPortList, &VuoPublishedPortList::publishedPortDetailsEditorRequested, this, &VuoPublishedPortSidebar::showPublishedPortDetailsEditor);
	connect(ui->publishedPortList, &VuoPublishedPortList::inputEditorRequested, this, &VuoPublishedPortSidebar::inputEditorRequested, Qt::QueuedConnection);
	connect(ui->publishedPortList, &VuoPublishedPortList::externalPortUnpublicationRequested, this, &VuoPublishedPortSidebar::externalPortUnpublicationRequested);
	connect(ui->publishedPortList, &VuoPublishedPortList::publishedPortPositionsUpdated, this, &VuoPublishedPortSidebar::publishedPortPositionsUpdated);
	connect(ui->publishedPortList, &VuoPublishedPortList::mouseMoveEventReceived, this, &VuoPublishedPortSidebar::updateHoverHighlighting);
	connect(ui->publishedPortList, &VuoPublishedPortList::publishedPortsReordered, this, &VuoPublishedPortSidebar::publishedPortsReordered);

	connect(this, &VuoPublishedPortSidebar::visibilityChanged, ui->publishedPortList, &VuoPublishedPortList::setVisible);

	ui->publishedPortList->setItemDelegate(new VuoPublishedPortListItemDelegate(composition, this));
	ui->publishedPortList->setInput(isInput);
	ui->publishedPortList->setComposition(composition);
	updatePortList();

	setMouseTracking(true);

	VuoEditor *editor = (VuoEditor *)qApp;
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoPublishedPortSidebar::updateColor);
	connect(editor, SIGNAL(darkInterfaceToggled(bool)), ui->publishedPortList, SLOT(repaint()));
	updateColor(editor->isInterfaceDark());
#ifdef VUO_PRO
	VuoPublishedPortSidebar_Pro();
#endif
}

/**
 *  Populates the published port list with an up-to-date list of published ports.
 */
void VuoPublishedPortSidebar::updatePortList()
{
	// Remember which ports were previously selected, so that we don't reset the selection whenever we update the port list.
	set<QString> previouslySelectedPorts;
	QList<QListWidgetItem *> previouslySelectedPortListItems = ui->publishedPortList->selectedItems();
	for (QList<QListWidgetItem *>::iterator portListItem = previouslySelectedPortListItems.begin(); portListItem != previouslySelectedPortListItems.end(); ++portListItem)
		previouslySelectedPorts.insert(((*portListItem)->data(Qt::DisplayRole).value<QString>()));

	ui->publishedPortList->clear();

	vector<VuoPublishedPort *> sortedPublishedPorts = composition->getBase()->getProtocolAwarePublishedPortOrder(composition->getActiveProtocol(),
																																ui->publishedPortList->getInput());
	foreach (VuoPublishedPort *publishedPort, sortedPublishedPorts)
	{
		bool portWasSelected = previouslySelectedPorts.find(publishedPort->getClass()->getName().c_str()) != previouslySelectedPorts.end();
		appendPublishedPortToList(publishedPort, portWasSelected);
	}

	// @todo: Fix this to re-enable vertical scrolling.
	ui->publishedPortList->setFixedHeight(ui->publishedPortList->sizeHint().height());
	ui->publishedPortList->update();
	ui->publishedPortList->updatePublishedPortLocs();

	update();
}

/**
 * Adds the provided published @c port to the end of the sidebar's published port list.
 * If @c select is true, selects the item within the list.
 */
void VuoPublishedPortSidebar::appendPublishedPortToList(VuoPublishedPort *port, bool select)
{
	string publishedPortName = port->getClass()->getName();
	VuoType *type = static_cast<VuoCompilerPortClass *>(port->getClass()->getCompiler())->getDataVuoType();
	string publishedPortType = VuoStringUtilities::transcodeToGraphvizIdentifier(composition->formatTypeNameForDisplay(type).toUtf8().constData());
	QListWidgetItem *item = new QListWidgetItem(ui->publishedPortList);

	item->setData(Qt::DisplayRole, publishedPortName.c_str());	// used for sort order
	item->setData(VuoPublishedPortList::publishedPortPointerIndex, qVariantFromValue((void *)port->getRenderer()));

	QString formattedPublishedPortName = QString("<b><font size=+2>%1</font></b>").arg(publishedPortName.c_str());

	//: Appears in the tooltip on published input and output ports.
	QString formattedPublishedPortDataType = "<font size=+1 color=\"gray\">" + tr("Data type") + ": <b>" + QString::fromStdString(publishedPortType) + "</b></font>";

	QString description;
	{
		string protocol = composition->getActiveProtocol() ? composition->getActiveProtocol()->getId() : "";
		bool filter     = protocol == VuoProtocol::imageFilter;
		bool generator  = protocol == VuoProtocol::imageGenerator;
		bool transition = protocol == VuoProtocol::imageTransition;

		// Keep these descriptions in sync with VuoManual.txt's "Making compositions fit a mold with protocols" section.
		// Protocol published inputs:
		if (publishedPortName == "time")
		{
			if (filter || generator || transition)
				description = tr("A number that changes over time, used to control animations or other changing effects.");
			if (transition)
				description += "  " + tr("<code>time</code> is independent of <code>progress</code>.");
		}
		else if (publishedPortName == "image")
		{
			if (filter || generator || transition)
				description = tr("The image to be filtered.");
		}
		else if (publishedPortName == "width")
		{
			if (generator)
				description = tr("The requested width of the image, in pixels.");
		}
		else if (publishedPortName == "height")
		{
			if (generator)
				description = tr("The requested height of the image, in pixels.");
		}
		else if (publishedPortName == "startImage")
		{
			if (transition)
				description = tr("The image to transition from.");
		}
		else if (publishedPortName == "endImage")
		{
			if (transition)
				description = tr("The image to transition to.");
		}
		else if (publishedPortName == "progress")
		{
			if (transition)
				description = tr("A number from 0 to 1 for how far the transition has progressed. At 0, the transition is at the beginning, with only <code>startImage</code> showing. At 0.5, the transition is halfway through. At 1, the transition is complete, with only <code>endImage</code> showing. When previewing the composition in Vuo, the mouse position left to right controls <code>progress</code>.");
		}

		// Protocol published outputs:
		else if (publishedPortName == "outputImage")
		{
			if (filter)
				description = tr("The altered image.");
			else if (generator)
				description = tr("The created image. Its width and height should match the <code>width</code> and <code>height</code> published input ports.");
			else if (transition)
				description = tr("The resulting image.");
		}

		// Export-format-specific published inputs:
		else if (publishedPortName == "offlineRender")
		{
			if (generator)
				description = tr("For movie export: <code>true</code> if the composition is being exported to a movie and <code>false</code> otherwise.");
		}
		else if (publishedPortName == "motionBlur")
		{
			if (generator)
				description = tr("For movie export: The number of frames rendered per output frame. 1 means motion blur is disabled; 2, 4, 8, 16, 32, or 64 means motion blur is enabled.");
		}
		else if (publishedPortName == "duration")
		{
			if (filter)
				description = tr("For FxPlug: The length, in seconds, of the clip to be filtered.");
			else if (generator)
				description = tr("For movie export and FxPlug: The length, in seconds, of the movie/clip.");
			else if (transition)
				description = tr("For FxPlug: The length, in seconds, of the transition.");
		}
		else if (publishedPortName == "framerate")
		{
			if (filter || transition)
				description = tr("For FxPlug: The framerate of the project, in frames per second.");
			else if (generator)
				description = tr("For movie export and FxPlug: The framerate of the movie/project, in frames per second.");
		}
		else if (publishedPortName == "frameNumber")
		{
			if (filter)
				description = tr("For FxPlug: The number of frames since the beginning of the clip, starting at 0.");
			else if (generator)
				description = tr("For movie export and FxPlug: The number of frames since the beginning of the movie/clip, starting at 0.");
			else if (transition)
				description = tr("For FxPlug: The number of frames since the beginning of the transition, starting at 0.");
		}
		else if (publishedPortName == "quality")
		{
			if (filter || generator || transition)
				description = tr("For FxPlug: The rendering quality or level of detail.");
		}
		else if (publishedPortName == "screen")
		{
			if (generator)
				description = tr("For screen savers: Which display the screen saver is running on.  (macOS runs a separate instance of the composition on each display.)");
		}
		else if (publishedPortName == "preview")
		{
			if (generator)
				description = tr("For screen savers: <code>true</code> when the screen saver is running in the System Preferences preview thumbnail.");
		}

		if (!description.isEmpty())
			description = "<p><font size=+1>" + description + "</font></p>";
	}

	item->setToolTip(formattedPublishedPortName.append("<BR>")
					 .append(formattedPublishedPortDataType)
					 .append(description));

	// Disable interaction with published ports that are part of an active protocol.
	if (port->isProtocolPort())
		item->setFlags(item->flags() &~ Qt::ItemIsEnabled);

	ui->publishedPortList->addItem(item);
	item->setSelected(select);
}

/**
 * Highlights eligible drop locations within the published port sidebar for the provided @c internalFixedPort.
 * If @c eventOnlyConnection is true, operates as if the ports will be connected with a cable that is event-only
 * regardless of the data-carrying status of the ports.
 * Eligible drop locations may include:
 *   - Externally visible published ports of a type that would allow them to be associated with the @c internalFixedPort.
 *   - Published port dropboxes.
 */
void VuoPublishedPortSidebar::highlightEligibleDropLocations(VuoRendererPort *internalFixedPort, bool eventOnlyConnection)
{
	auto types = composition->getCompiler()->getTypes();

	int numPorts = ui->publishedPortList->count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *portItem = ui->publishedPortList->item(portIndex);
		VuoRendererPublishedPort *publishedPortToHighlight = static_cast<VuoRendererPublishedPort *>(portItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<void *>());

		QGraphicsItem::CacheMode normalCacheMode = publishedPortToHighlight->cacheMode();
		publishedPortToHighlight->setCacheMode(QGraphicsItem::NoCache);
		publishedPortToHighlight->updateGeometry();

		VuoRendererColors::HighlightType highlight = composition->getEligibilityHighlightingForPort(publishedPortToHighlight, internalFixedPort, eventOnlyConnection, types);
		publishedPortToHighlight->setEligibilityHighlight(highlight);

		publishedPortToHighlight->setCacheMode(normalCacheMode);
	}

	bool publishedPortWellShouldAcceptDrops = (internalFixedPort && canListPublishedPortAliasFor(internalFixedPort) && internalFixedPort->getPublishable());
	ui->publishedPortDropBox->setCurrentlyAcceptingDrops(publishedPortWellShouldAcceptDrops);
	ui->publishedPortDropBox->update();

	if (!isHidden())
	{
		ui->publishedPortList->setFillVerticalSpace(!publishedPortWellShouldAcceptDrops);
		ui->publishedPortList->viewport()->update();
	}
}

/**
* Clears the highlighting from eligible drop locations (sidebar published ports and drop box).
*/
void VuoPublishedPortSidebar::clearEligibleDropLocationHighlighting()
{
	int numPorts = ui->publishedPortList->count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *portItem = ui->publishedPortList->item(portIndex);
		VuoRendererPublishedPort *publishedPort = static_cast<VuoRendererPublishedPort *>(portItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<void *>());

		QGraphicsItem::CacheMode normalCacheMode = publishedPort->cacheMode();
		publishedPort->setCacheMode(QGraphicsItem::NoCache);
		publishedPort->updateGeometry();
		publishedPort->setEligibilityHighlight(VuoRendererColors::noHighlight);
		publishedPort->setCacheMode(normalCacheMode);
	}

	ui->publishedPortDropBox->setCurrentlyAcceptingDrops(false);
	ui->publishedPortDropBox->update();

	if (!isHidden())
	{
		ui->publishedPortList->setFillVerticalSpace(true);
		ui->publishedPortList->viewport()->update();
	}
}

/**
 * Updates the display of the active protocol.
 */
void VuoPublishedPortSidebar::updateActiveProtocol()
{
	// Display the name of the active protocol, if any.
	if (composition->getActiveProtocol())
	{
		ui->activeProtocolLabel->setStyleSheet(VUO_QSTRINGIFY(
												   QLabel {
													   background-color: %1;
													   color: rgba(255,255,255,224);
												   }
												   )
											   .arg(getActiveProtocolHeadingColor(0, !isInput).name())
											   );

		ui->activeProtocolLabel->setText(VuoEditor::tr(composition->getActiveProtocol()->getName().c_str()));
		ui->activeProtocolLabel->setHidden(false);
	}

	else
	{
		ui->activeProtocolLabel->setText("");
		ui->activeProtocolLabel->setHidden(true);
	}
}

/**
 * Returns the "Protocols" context menu associated with the sidebar.
 */
QMenu * VuoPublishedPortSidebar::getProtocolsContextMenu()
{
	return menuChangeProtocol;
}

/**
 * Returns the "Remove Protocol" action associated with the sidebar.
 */
QAction * VuoPublishedPortSidebar::getRemoveProtocolAction()
{
	return contextMenuActionRemoveProtocol;
}


/**
 * Begin and/or end hover-highlighting of components within the sidebar,
 * given the cursor position associated with the the input mouse @c event.
 * Components may include:
 *    - externally visible published ports; or
 *    - the published port dropbox.
 * @todo: Merge with VuoEditorComposition::updateHoverHighlighting(QGraphicsSceneMouseEvent *event).
 */
void VuoPublishedPortSidebar::updateHoverHighlighting(QMouseEvent *event, qreal tolerance)
{
	VuoCable *cableInProgress = composition->getCableInProgress();
	VuoRendererPublishedPort *publishedPortUnderCursor = getPublishedPortUnderCursorForEvent(event, tolerance, !cableInProgress);

	int numPorts = ui->publishedPortList->count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *portItem = ui->publishedPortList->item(portIndex);
		VuoRendererPublishedPort *publishedPort = static_cast<VuoRendererPublishedPort *>(portItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<void *>());

		if (publishedPort == publishedPortUnderCursor)
			publishedPort->extendedHoverMoveEvent(cableInProgress);
		else
			publishedPort->extendedHoverLeaveEvent();

		if (cableInProgress)
			cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(publishedPortUnderCursor && !publishedPortUnderCursor->getDataType());
	}

	ui->publishedPortDropBox->setHovered(isPublishedPortDropBoxUnderCursorForEvent(event));

	if (!isHidden())
		ui->publishedPortList->viewport()->update();
}

/**
 * End hover-highlighting of components within the sidebar.
 * Components may include:
 *    - externally visible published ports; or
 *    - the published port dropbox.
 * @todo: Merge with VuoEditorComposition::[update/clear]HoverHighlighting().
 */
void VuoPublishedPortSidebar::clearHoverHighlighting()
{
	int numPorts = ui->publishedPortList->count();
	for (int portIndex = 0; portIndex < numPorts; ++portIndex)
	{
		QListWidgetItem *portItem = ui->publishedPortList->item(portIndex);
		VuoRendererPublishedPort *publishedPort = static_cast<VuoRendererPublishedPort *>(portItem->data(VuoPublishedPortList::publishedPortPointerIndex).value<void *>());
		publishedPort->extendedHoverLeaveEvent();
	}

	ui->publishedPortDropBox->setHovered(false);

	if (!isHidden())
		ui->publishedPortList->viewport()->update();
}

/**
 * Conclude a cable drag in response to a mouse release @c event occurring directly over the sidebar.
 * This may mean:
 *    - completing the cable connection, if the mouse release occurred within
 * range of an eligible published port; or
 *    -  publishing the internal port connected to the cable, if the mouse release occured within
 * range of the published port dropbox.
 * @todo: Merge with VuoEditorComposition::concludeCableDrag(QGraphicsSceneMouseEvent *event).
 */
void VuoPublishedPortSidebar::concludePublishedCableDrag(QMouseEvent *event, VuoCable *cableInProgress, bool cableInProgressWasNew)
{
	VuoRendererPort *fixedPort = NULL;
	if (cableInProgress->getFromNode() && cableInProgress->getFromPort())
		fixedPort = cableInProgress->getFromPort()->getRenderer();
	else if (cableInProgress->getToNode() && cableInProgress->getToPort())
		fixedPort = cableInProgress->getToPort()->getRenderer();

	if (!fixedPort)
		return;

	// Potential side effects of a newly completed cable connection:
	// - A typecast may need to be automatically inserted.
	string typecastToInsert = "";

	// - A generic port involved in the new connection may need to be specialized.
	VuoRendererPort *portToSpecialize = NULL;
	string specializedTypeName = "";

	// Whether the cursor is hovered over the drop box must be determined before the cable drag is cancelled,
	// because cancelling the cable drag hides the drop box.
	bool isCursorOverPublishedPortDropBox = isPublishedPortDropBoxUnderCursorForEvent(event);
	bool forceEventOnlyPublication = !cableInProgress->getRenderer()->effectivelyCarriesData();

	if (isCursorOverPublishedPortDropBox && canListPublishedPortAliasFor(fixedPort))
	{
		emit undoStackMacroBeginRequested("Cable Connection");
		composition->cancelCableDrag();
		emit portPublicationRequestedViaDropBox(fixedPort->getBase(),
									  forceEventOnlyPublication,
									  false); // Avoid nested Undo stack macros.
		emit undoStackMacroEndRequested();
	}

	else
	{
		VuoRendererPublishedPort *publishedPort = getPublishedPortUnderCursorForEvent(event,  VuoEditorComposition::componentCollisionRange, false);
		if (publishedPort)
		{
			bool isPublishedInput = !publishedPort->getInput();
			bool cableInProgressExpectedToCarryData = (cableInProgress->getRenderer()->effectivelyCarriesData() &&
													  publishedPort->getDataType());

			// @todo: Don't assume that the cable is connected on the other end to an internal port (https://b33p.net/node/7756).
			if ((publishedPort->isCompatibleAliasWithSpecializationForInternalPort(fixedPort, forceEventOnlyPublication, &portToSpecialize, specializedTypeName))
					|| (isPublishedInput? composition->selectBridgingSolution(publishedPort, fixedPort, false, &portToSpecialize, specializedTypeName, typecastToInsert) :
						composition->selectBridgingSolution(fixedPort, publishedPort, true, &portToSpecialize, specializedTypeName, typecastToInsert)))
			{
				// If the port is a published output port and already had a connected data+event cable, displace it.
				VuoCable *dataCableToDisplace = NULL;
				if (!forceEventOnlyPublication && !ui->publishedPortList->getInput())
				{
					vector<VuoCable *> previousConnectedCables = publishedPort->getBase()->getConnectedCables(true);
					for (vector<VuoCable *>::iterator cable = previousConnectedCables.begin(); (! dataCableToDisplace) && (cable != previousConnectedCables.end()); ++cable)
						if ((*cable)->getRenderer()->effectivelyCarriesData())
							dataCableToDisplace = *cable;
				}

				// If replacing a preexisting cable with an identical cable, just cancel the operation
				// so that it doesn't go onto the Undo stack.
				VuoCable *cableToReplace = fixedPort->getCableConnectedTo(publishedPort, true);
				bool cableToReplaceHasMatchingDataCarryingStatus = (cableToReplace?
																		(cableToReplace->getRenderer()->effectivelyCarriesData() ==
																		 cableInProgressExpectedToCarryData) :
																		false);
				if (cableToReplace && cableToReplaceHasMatchingDataCarryingStatus)
				{
					composition->cancelCableDrag();
					return;
				}

				// If the user has simply disconnected a cable and reconnected it within the same mouse drag,
				// don't push the operation onto the Undo stack.
				bool recreatingSameConnection = ((cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort() ==
														 publishedPort->getBase()) &&
														(cableInProgress->getRenderer()->getPreviouslyAlwaysEventOnly() ==
														 cableInProgress->getCompiler()->getAlwaysEventOnly()));
				if (recreatingSameConnection)
				{
					composition->revertCableDrag();
					return;
				}

				emit undoStackMacroBeginRequested("Cable Connection");
				composition->cancelCableDrag();

				if (dataCableToDisplace)
				{
					QList<QGraphicsItem *> removedComponents;
					removedComponents.append(dataCableToDisplace->getRenderer());
					emit componentsRemoved(removedComponents, "Delete");
				}

				// If this source/target port combination already a cable connecting them, but of a different
				// data-carrying status, replace the old cable with the new one.
				if (cableToReplace && !cableToReplaceHasMatchingDataCarryingStatus)
				{
					QList<QGraphicsItem *> removedComponents;
					removedComponents.append(cableToReplace->getRenderer());
					emit componentsRemoved(removedComponents, "Delete");
				}

				emit portPublicationRequestedViaSidebarPort(fixedPort->getBase(),
											  dynamic_cast<VuoPublishedPort *>(publishedPort->getBase()),
											  forceEventOnlyPublication,
											  (portToSpecialize? portToSpecialize->getBase() : NULL),
											  specializedTypeName,
											  typecastToInsert,
											  false); // Avoid nested Undo stack macros.
				emit undoStackMacroEndRequested();
			}
		}
		else // Cable was dropped over empty space within the sidebar
			composition->cancelCableDrag();
	}
}

/**
 * Handle context menu events.
 */
void VuoPublishedPortSidebar::contextMenuEvent(QContextMenuEvent *event)
{
	menuSelectionInProgress = true;

	// @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	if (isActiveProtocolLabelUnderCursorForEvent(event))
	{
		contextMenuActionRemoveProtocol->setData(qVariantFromValue((void *)composition->getActiveProtocol()));
		contextMenuRemoveProtocol->exec(event->globalPos());
	}
	else
	{
		contextMenuPortOptions->addMenu(menuAddPort);
		contextMenuPortOptions->exec(event->globalPos());
		contextMenuPortOptions->removeAction(menuAddPort->menuAction());
	}

	menuSelectionInProgress = false;
}

/**
 * Handle mouse move events.
 */
void VuoPublishedPortSidebar::mouseMoveEvent(QMouseEvent *event)
{
	// Refrain from hover-highlighting published ports while the cursor is within the sidebar.
	clearHoverHighlighting();
}

/**
 * Handle mouse press events.
 */
void VuoPublishedPortSidebar::mousePressEvent(QMouseEvent *event)
{
	ui->publishedPortList->clearSelection();

	// Calling QDockWidget::mousePressEvent() here for some reason
	// increases the width of the sidebar, so don't call it.
}

/**
 * Handle notifications from external sources that an event affecting
 * this sidebar's position relative to the composition viewport has occurred.
 * When the widget receives this event, it is already at the new position.
 */
void VuoPublishedPortSidebar::externalMoveEvent()
{
	ui->publishedPortList->updatePublishedPortLocs();
}

/**
 * Handle drag enter events.
 */
void VuoPublishedPortSidebar::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->source() == ui->publishedPortList)
		event->accept();
	else
		event->ignore();
}

/**
 * Handle drop events.
 */
void VuoPublishedPortSidebar::dropEvent(QDropEvent *event)
{
	QDropEvent modifiedEvent(QPoint(10,10),
							 event->dropAction(),
							 event->mimeData(),
							 event->mouseButtons(),
							 event->keyboardModifiers());
	ui->publishedPortList->adoptDropEvent(&modifiedEvent);
	return;
}

/**
 * Handle close events.
 */
void VuoPublishedPortSidebar::closeEvent(QCloseEvent *event)
{
	emit closed();
	QDockWidget::closeEvent(event);
}

/**
 * Returns a boolean indicating whether the input internal @c port is eligible to be
 * listed in association with an externally visible published port alias within this sidebar.
 */
bool VuoPublishedPortSidebar::canListPublishedPortAliasFor(VuoRendererPort *port)
{
	return ((ui->publishedPortList->getInput() && port->getInput()) || ((! ui->publishedPortList->getInput()) && port->getOutput()));
}

/**
 * Returns a boolean indicating whether the published port dropbox was under the
 * cursor at the time of mouse event @c event.
 */
bool VuoPublishedPortSidebar::isPublishedPortDropBoxUnderCursorForEvent(QMouseEvent *event)
{
	QPoint cursorPosition = ((QMouseEvent *)(event))->globalPos();

	QRect publishedPortDropBoxRect = ui->publishedPortDropBox->geometry();
	publishedPortDropBoxRect.moveTopLeft(ui->publishedPortDropBox->parentWidget()->mapToGlobal(publishedPortDropBoxRect.topLeft()));

	bool dropBoxUnderCursor = (!ui->publishedPortDropBox->isHidden() && publishedPortDropBoxRect.contains(cursorPosition));
	return dropBoxUnderCursor;
}

/**
 * Returns a boolean indicating whether an active protocol label was under the
 * cursor at the time of context menu event @c event.
 */
bool VuoPublishedPortSidebar::isActiveProtocolLabelUnderCursorForEvent(QContextMenuEvent *event)
{
	QPoint cursorPosition = ((QContextMenuEvent *)(event))->globalPos();

	QRect activeProtocolLabelRect = ui->activeProtocolLabel->geometry();
	activeProtocolLabelRect.moveTopLeft(ui->activeProtocolLabel->parentWidget()->mapToGlobal(activeProtocolLabelRect.topLeft()));

	bool activeProtocolLabelUnderCursor = (!ui->activeProtocolLabel->isHidden() && activeProtocolLabelRect.contains(cursorPosition));
	return activeProtocolLabelUnderCursor;
}

/**
 * Returns the published port under the cursor at the time of mouse event @c event,
 * or NULL if no published port was under the cursor.
 */
VuoRendererPublishedPort * VuoPublishedPortSidebar::getPublishedPortUnderCursorForEvent(QMouseEvent *event, qreal tolerance, bool limitPortCollisionRange)
{
	QPoint cursorPosition = ((QMouseEvent *)(event))->globalPos();
	return ui->publishedPortList->getPublishedPortAtGlobalPos(cursorPosition, tolerance, limitPortCollisionRange);
}

/**
 * Returns the global position of the center of the port circle.
 */
QPoint VuoPublishedPortSidebar::getGlobalPosOfPublishedPort(VuoRendererPublishedPort *port)
{
	return ui->publishedPortList->getGlobalPosOfPublishedPort(port);
}

/**
 * Returns a boolean indicating whether selection from a context menu is currently in progress.
 */
bool VuoPublishedPortSidebar::getMenuSelectionInProgress()
{
	return menuSelectionInProgress || ui->publishedPortList->getMenuSelectionInProgress();
}

/**
 * Returns the background color to be used for the heading of an active protocol, given the
 * index of the protocol within the sidebar.
 */
QColor VuoPublishedPortSidebar::getActiveProtocolHeadingColor(int protocolIndex, bool isInput)
{
	VuoRendererColors activeProtocolColor(VuoRendererColors::getActiveProtocolTint(protocolIndex, isInput),
										  VuoRendererColors::noSelection,
										  false,
										  VuoRendererColors::noHighlight);
	return activeProtocolColor.nodeFrame();
}

/**
 * Returns the background color to be used for a published port belonging to an active protocol,
 * given the index of the protocol within the sidebar.
 */
QColor VuoPublishedPortSidebar::getActiveProtocolPortColor(int protocolIndex, bool isInput)
{
	VuoRendererColors activeProtocolColor(VuoRendererColors::getActiveProtocolTint(protocolIndex, isInput),
										  VuoRendererColors::noSelection,
										  false,
										  VuoRendererColors::noHighlight);
	return activeProtocolColor.nodeFill();
}

/**
 * Displays an input editor for a published port name.
 *
 * @param port The published port whose name is to be edited.
 */
string VuoPublishedPortSidebar::showPublishedPortNameEditor(VuoRendererPublishedPort *port)
{
	VuoTitleEditor *nameEditor = new VuoPublishedPortNameEditor();
	string originalValue = port->getBase()->getClass()->getName();

	// Position the input editor overtop the published port's list widget item.
	QPoint portLeftCenterGlobal = getGlobalPosOfPublishedPort(port);
	int yCenter = portLeftCenterGlobal.y();
	int xRight = parentWidget()->mapToGlobal( QPoint(x() + width(), 0) ).x();
	QPoint publishedPortRightCenterGlobal = QPoint(xRight, yCenter);

	// Adjust the input editor's position and width to keep its focus glow from extending beyond the sidebar boundaries.
	const int focusGlowWidth = 2;
	publishedPortRightCenterGlobal = publishedPortRightCenterGlobal - QPoint(focusGlowWidth, 0);
	nameEditor->setWidth(width() - 2*focusGlowWidth);

	json_object *details = json_object_new_object();
	json_object_object_add(details, "isDark", json_object_new_boolean(static_cast<VuoEditor *>(qApp)->isInterfaceDark()));

	json_object *originalValueAsJson = json_object_new_string(originalValue.c_str());
	json_object *newValueAsJson = nameEditor->show(publishedPortRightCenterGlobal, originalValueAsJson, details);
	string newValue = json_object_get_string(newValueAsJson);
	json_object_put(originalValueAsJson);
	json_object_put(newValueAsJson);

	return newValue;
}

/**
 * Displays a widget to edit a published input port's details.
 * Currently only supports numeric (VuoInteger and VuoReal) data types.
 */
void VuoPublishedPortSidebar::showPublishedPortDetailsEditor(VuoRendererPublishedPort *port)
{
	VuoPublishedPort *publishedPort = dynamic_cast<VuoPublishedPort *>(port->getBase());
	VuoType *type = port->getDataType();
	if (publishedPort && type &&
			((type->getModuleKey() == "VuoReal") ||
			 (type->getModuleKey() == "VuoInteger") ||
			 (type->getModuleKey() == "VuoPoint2d") ||
			 (type->getModuleKey() == "VuoPoint3d") ||
			 (type->getModuleKey() == "VuoPoint4d")))
	{
		QPoint portLeftCenterGlobal = getGlobalPosOfPublishedPort(port);

		bool isPublishedInput = !port->getInput();
		json_object *originalDetails = static_cast<VuoCompilerPublishedPort *>(publishedPort->getCompiler())->getDetails(isPublishedInput);

		composition->disableNondetachedPortPopovers();

		if (!originalDetails || json_object_get_type(originalDetails) != json_type_object)
			originalDetails = json_object_new_object();
		VuoEditor *editor = static_cast<VuoEditor *>(qApp);
		json_object_object_add(originalDetails, "isDark", json_object_new_boolean(editor->isInterfaceDark()));

		json_object *newDetails = NULL;
		if ((type->getModuleKey() == "VuoReal") || (type->getModuleKey() == "VuoInteger"))
		{
			VuoDetailsEditorNumeric *numericDetailsEditor = new VuoDetailsEditorNumeric(type);
			newDetails = numericDetailsEditor->show(portLeftCenterGlobal, originalDetails);
			numericDetailsEditor->deleteLater();
		}
		else if ((type->getModuleKey() == "VuoPoint2d") ||
				 (type->getModuleKey() == "VuoPoint3d") ||
				 (type->getModuleKey() == "VuoPoint4d"))
		{
			VuoDetailsEditorPoint *pointDetailsEditor = new VuoDetailsEditorPoint(type);
			newDetails = pointDetailsEditor->show(portLeftCenterGlobal, originalDetails);
			pointDetailsEditor->deleteLater();
		}

		// Check whether any of the editable values have been changed before creating an Undo stack command.
		string originalSuggestedMin, newSuggestedMin, originalSuggestedMax, newSuggestedMax, originalSuggestedStep, newSuggestedStep = "";

		// suggestedMin
		json_object *originalSuggestedMinValue = NULL;
		if (json_object_object_get_ex(originalDetails, "suggestedMin", &originalSuggestedMinValue))
			originalSuggestedMin = json_object_to_json_string(originalSuggestedMinValue);

		json_object *newSuggestedMinValue = NULL;
		if (json_object_object_get_ex(newDetails, "suggestedMin", &newSuggestedMinValue))
			newSuggestedMin = json_object_to_json_string(newSuggestedMinValue);
		else
			json_object_object_add(newDetails, "suggestedMin", NULL);

		// suggestedMax
		json_object *originalSuggestedMaxValue = NULL;
		if (json_object_object_get_ex(originalDetails, "suggestedMax", &originalSuggestedMaxValue))
			originalSuggestedMax = json_object_to_json_string(originalSuggestedMaxValue);

		json_object *newSuggestedMaxValue = NULL;
		if (json_object_object_get_ex(newDetails, "suggestedMax", &newSuggestedMaxValue))
			newSuggestedMax = json_object_to_json_string(newSuggestedMaxValue);
		else
			json_object_object_add(newDetails, "suggestedMax", NULL);

		// suggestedStep
		json_object *originalSuggestedStepValue = NULL;
		if (json_object_object_get_ex(originalDetails, "suggestedStep", &originalSuggestedStepValue))
			originalSuggestedStep = json_object_to_json_string(originalSuggestedStepValue);

		json_object *newSuggestedStepValue = NULL;
		if (json_object_object_get_ex(newDetails, "suggestedStep", &newSuggestedStepValue))
			newSuggestedStep = json_object_to_json_string(newSuggestedStepValue);
		else
			json_object_object_add(newDetails, "suggestedStep", NULL);

		if ((originalSuggestedMin != newSuggestedMin) ||
				(originalSuggestedMax != newSuggestedMax) ||
				(originalSuggestedStep != newSuggestedStep))
		{
			emit publishedPortDetailsChangeRequested(port, newDetails);
		}

		composition->disableNondetachedPortPopovers();
	}
}

/**
 * Handles user's selection of a type from the "New published port" tool-button's type menu.
 */
void VuoPublishedPortSidebar::newPublishedPortTypeSelected()
{
	QAction *sender = (QAction *)QObject::sender();
	QList<QVariant> actionData = sender->data().toList();
	QString typeName = actionData[1].toString();

	emit newPublishedPortRequested(typeName.toUtf8().constData(), isInput);
}

/**
 * Populates the published port type selection submenus with the available types,
 * if the menus have not already been populated.
 */
void VuoPublishedPortSidebar::populatePortTypeMenus()
{
	if (this->portTypeMenusPopulated)
		return;

	menuAddPort->clear();

	// Event-only option
	QAction *addEventOnlyPortAction = menuAddPort->addAction(tr("Event-Only"));
	string typeName = "";
	QList<QVariant> portAndSpecializedType;
	portAndSpecializedType.append(qVariantFromValue((void *)NULL));
	portAndSpecializedType.append(typeName.c_str());
	addEventOnlyPortAction->setData(QVariant(portAndSpecializedType));

	// Non-list type submenu
	QMenu *menuNonListType = new QMenu(menuAddPort);
	menuNonListType->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuNonListType->setTitle(tr("Single Value"));
	menuNonListType->setToolTipsVisible(true);
	menuAddPort->addMenu(menuNonListType);

	// List-type submenu
	QMenu *menuListType = new QMenu(menuAddPort);
	menuListType->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuListType->setTitle(tr("List"));
	menuListType->setToolTipsVisible(true);
	menuAddPort->addMenu(menuListType);

	// Non-list type options
	{
		vector<string> singleValueTypes = composition->getAllSpecializedTypeOptions(false);

		set<string> types;
		if (allowedPortTypes.empty())
			types.insert(singleValueTypes.begin(), singleValueTypes.end());
		else
		{
			vector<string> sortedAllowedPortTypes(allowedPortTypes.begin(), allowedPortTypes.end());
			std::sort(sortedAllowedPortTypes.begin(), sortedAllowedPortTypes.end());
			std::sort(singleValueTypes.begin(), singleValueTypes.end());

			std::set_intersection(singleValueTypes.begin(), singleValueTypes.end(),
								  sortedAllowedPortTypes.begin(), sortedAllowedPortTypes.end(),
								  std::inserter(types, types.begin()));
		}

		// First list compatible types that don't belong to any specific node set.
		QList<QAction *> actions = composition->getCompatibleTypesForMenu(NULL, types, types, true, "", menuNonListType);
		composition->addTypeActionsToMenu(actions, menuNonListType);

		// Now add a submenu for each node set that contains compatible types.
		composition->getModuleManager()->updateWithAlreadyLoadedModules();
		map<string, set<VuoCompilerType *> > loadedTypesForNodeSet = composition->getModuleManager()->getLoadedTypesForNodeSet();
		QList<QAction *> allNodeSetActionsToAdd;
		for (map<string, set<VuoCompilerType *> >::iterator i = loadedTypesForNodeSet.begin(); i != loadedTypesForNodeSet.end(); ++i)
		{
			string nodeSetName = i->first;
			if (!nodeSetName.empty())
				allNodeSetActionsToAdd += composition->getCompatibleTypesForMenu(NULL, types, types, true, nodeSetName, menuNonListType);
		}

		if ((menuNonListType->actions().size() > 0) && (allNodeSetActionsToAdd.size() > 0))
			menuNonListType->addSeparator();

		composition->addTypeActionsToMenu(allNodeSetActionsToAdd, menuNonListType);
	}

	// List type options
	{
		vector<string> listTypes = composition->getAllSpecializedTypeOptions(true);

		set<string> types;
		if (allowedPortTypes.empty())
			types.insert(listTypes.begin(), listTypes.end());
		else
		{
			vector<string> sortedAllowedPortTypes(allowedPortTypes.begin(), allowedPortTypes.end());
			std::sort(sortedAllowedPortTypes.begin(), sortedAllowedPortTypes.end());
			std::sort(listTypes.begin(), listTypes.end());

			std::set_intersection(listTypes.begin(), listTypes.end(),
								  sortedAllowedPortTypes.begin(), sortedAllowedPortTypes.end(),
								  std::inserter(types, types.begin()));
		}

		// First list compatible types that don't belong to any specific node set.
		QList<QAction *> actions = composition->getCompatibleTypesForMenu(NULL, types, types, true, "", menuListType);
		composition->addTypeActionsToMenu(actions, menuListType);

		// Now add a submenu for each node set that contains compatible types.
		composition->getModuleManager()->updateWithAlreadyLoadedModules();
		map<string, set<VuoCompilerType *> > loadedTypesForNodeSet = composition->getModuleManager()->getLoadedTypesForNodeSet();
		QList<QAction *> allNodeSetActionsToAdd;
		for (map<string, set<VuoCompilerType *> >::iterator i = loadedTypesForNodeSet.begin(); i != loadedTypesForNodeSet.end(); ++i)
		{
			string nodeSetName = i->first;
			if (!nodeSetName.empty())
				allNodeSetActionsToAdd += composition->getCompatibleTypesForMenu(NULL, types, types, true, nodeSetName, menuListType);
		}

		if ((menuListType->actions().size() > 0) && (allNodeSetActionsToAdd.size() > 0))
			menuListType->addSeparator();

		composition->addTypeActionsToMenu(allNodeSetActionsToAdd, menuListType);
	}

	// Set up context menu item connections.
	connect(addEventOnlyPortAction, &QAction::triggered, this, &VuoPublishedPortSidebar::newPublishedPortTypeSelected);
	addEventOnlyPortAction->setCheckable(false);

	foreach (QAction *setTypeAction, menuAddPort->actions())
	{
		QMenu *setTypeSubmenu = setTypeAction->menu();
		if (setTypeSubmenu)
		{
			foreach (QAction *setTypeSubaction, setTypeSubmenu->actions())
			{
				QMenu *setTypeSubmenu = setTypeSubaction->menu();
				if (setTypeSubmenu)
				{
					foreach (QAction *setTypeSubSubaction, setTypeSubmenu->actions())
					{
						connect(setTypeSubSubaction, &QAction::triggered, this, &VuoPublishedPortSidebar::newPublishedPortTypeSelected);
						setTypeSubSubaction->setCheckable(false);
					}
				}
				else
				{
					connect(setTypeSubaction, &QAction::triggered, this, &VuoPublishedPortSidebar::newPublishedPortTypeSelected);
					setTypeSubaction->setCheckable(false);
				}
			}
		}
	}

	this->portTypeMenusPopulated = true;
}

/**
 * Limits the menu options available for data type when creating a published port.
 *
 * Must be called before the first time the menu is shown.
 */
void VuoPublishedPortSidebar::limitAllowedPortTypes(const set<string> &allowedPortTypes)
{
	this->allowedPortTypes = allowedPortTypes;
}

/**
 * Makes the widget dark.
 */
void VuoPublishedPortSidebar::updateColor(bool isDark)
{
	QString titleTextColor            = isDark ? "#303030" : "#808080";
	QString titleBackgroundColor      = isDark ? "#919191" : "#efefef";
	QString dockwidgetBackgroundColor = isDark ? "#404040" : "#efefef";
	QString buttonTextColor           = isDark ? "#a0a0a0" : "#707070";

	setStyleSheet(VUO_QSTRINGIFY(
					  QDockWidget {
						  titlebar-close-icon: url(:/Icons/dockwidget-close-%3.png);
						  font-size: 11px;
						  border: none;
						  color: %1;
					  }
					  QDockWidget::title {
						  text-align: left;
						  padding-left: 6px;
						  background-color: %2;
					  }
				  )
				  .arg(titleTextColor)
				  .arg(titleBackgroundColor)
				  .arg(isDark ? "dark" : "light")
				  );

	ui->VuoPublishedPortSidebarContents->setStyleSheet(VUO_QSTRINGIFY(
														   QWidget#VuoPublishedPortSidebarContents {
															   background-color: %1;
														   }
														   )
													   .arg(dockwidgetBackgroundColor)
													   );

	ui->publishedPortList->setStyleSheet(
				ui->publishedPortList->styleSheet().append(
					VUO_QSTRINGIFY(
						VuoPublishedPortList {
							background-color: %1;
						}
						)
					.arg(dockwidgetBackgroundColor)
					)
				);

	ui->newPublishedPortButton->setStyleSheet(VUO_QSTRINGIFY(
		* {
			border-radius: 5px; // Rounded corners
			padding: 3px;
			margin: 5px 2px;
			color: %1;
		}
		QToolButton::menu-indicator {
			image: none;
		}
	).arg(buttonTextColor));
}
