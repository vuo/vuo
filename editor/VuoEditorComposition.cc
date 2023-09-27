/**
 * @file
 * VuoEditorComposition implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorComposition.hh"

#include "VuoCommandReplaceNode.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerIssue.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoEditorWindow.hh"
#include "VuoErrorDialog.hh"
#include "VuoException.hh"
#include "VuoFileType.h"
#include "VuoRendererComment.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererSignaler.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerCompoundType.hh"
#include "VuoCompilerDriver.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"
#include "VuoGenericType.hh"
#include "VuoRunningCompositionLibraries.hh"
#include "VuoComment.hh"
#include "VuoEditor.hh"
#include "VuoErrorMark.hh"
#include "VuoErrorPopover.hh"
#include "VuoModuleManager.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoPortPopover.hh"
#include "VuoProtocol.hh"
#include "VuoStringUtilities.hh"
#include "VuoInputEditorIcon.hh"
#include "VuoInputEditorManager.hh"
#include "VuoSubcompositionMessageRouter.hh"
#include "VuoEditorUtilities.hh"
#include "VuoNodeAndPortIdentifierCache.hh"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <objc/objc-runtime.h>
#endif

const qreal VuoEditorComposition::nodeMoveRate                   = 15;  // VuoRendererComposition::minorGridLineSpacing;
const qreal VuoEditorComposition::nodeMoveRateMultiplier         = 4;   // VuoRendererComposition::majorGridLineSpacing / VuoRendererComposition::minorGridLineSpacing
const qreal VuoEditorComposition::componentCollisionRange = 10;
const qreal VuoEditorComposition::showEventsModeUpdateInterval = 1000/20.; // interval, in ms, after which to update component transparency levels and animations while in 'Show Events' mode
const int VuoEditorComposition::initialChangeNodeSuggestionCount = 10; // The initial number of suggestions to list in the "Change (Node) To" context menu

/**
 * Creates an empty canvas upon which nodes and cables can be rendered.
 */
VuoEditorComposition::VuoEditorComposition(VuoMainWindow *window, VuoComposition *baseComposition) :
	VuoRendererComposition(baseComposition, false, true)
{
#if VUO_PRO
	VuoEditorComposition_Pro();
#endif

	this->window = window;
	compiler = NULL;
	inputEditorManager = NULL;
	activeProtocol = NULL;
	runner = NULL;
	runningComposition = NULL;
	runningCompositionActiveDriver = NULL;
	runningCompositionLibraries = NULL;
	stopRequested = false;
	duplicateOnNextMouseMove = false;
	duplicationDragInProgress = false;
	duplicationCancelled = false;
	cursorPosBeforeDuplicationDragMove = QPointF(0,0);
	cableInProgress = NULL;
	cableInProgressWasNew = false;
	cableInProgressShouldBeWireless = false;
	portWithDragInitiated = NULL;
	cableWithYankInitiated = NULL;
	menuSelectionInProgress = false;
	previousNearbyItem = NULL;
	dragStickinessDisabled = false;
	ignoreApplicationStateChangeEvents = false;
	popoverEventsEnabled = true;
	runCompositionQueue = dispatch_queue_create("org.vuo.editor.run", NULL);
	activePortPopoversQueue = dispatch_queue_create("org.vuo.editor.popovers", NULL);
	errorMark = NULL;
	errorMarkingUpdatesEnabled = true;
	triggerPortToRefire = "";

	contextMenuDeleteSelected = new QAction(NULL);
	contextMenuHideSelectedCables = new QAction(NULL);
	contextMenuRenameSelected = new QAction(NULL);
	contextMenuRefactorSelected = new QAction(NULL);
	contextMenuPublishPort = new QAction(NULL);
	contextMenuDeleteCables = new QAction(NULL);
	contextMenuHideCables = new QAction(NULL);
	contextMenuUnhideCables = new QAction(NULL);
	contextMenuFireEvent = new QAction(NULL);
	contextMenuAddInputPort = new QAction(NULL);
	contextMenuRemoveInputPort = new QAction(NULL);
	contextMenuSetPortConstant = new QAction(NULL);
	contextMenuEditSelectedComments = new QAction(NULL);

	contextMenuChangeNode = NULL;
	contextMenuSpecializeGenericType = NULL;

	contextMenuFireEvent->setText(tr("Fire Event"));
	contextMenuHideSelectedCables->setText(tr("Hide"));
	contextMenuRenameSelected->setText(tr("Rename…"));
	contextMenuRefactorSelected->setText(tr("Package as Subcomposition"));
	contextMenuAddInputPort->setText(tr("Add Input Port"));
	contextMenuRemoveInputPort->setText(tr("Remove Input Port"));
	contextMenuSetPortConstant->setText(tr("Edit Value…"));
	contextMenuEditSelectedComments->setText(tr("Edit…"));

	connect(contextMenuDeleteSelected, &QAction::triggered, this, static_cast<void (VuoEditorComposition::*)()>(&VuoEditorComposition::deleteSelectedCompositionComponents));
	connect(contextMenuHideSelectedCables, &QAction::triggered, this, &VuoEditorComposition::selectedInternalCablesHidden);
	connect(contextMenuRenameSelected, &QAction::triggered, this, &VuoEditorComposition::renameSelectedNodes);
	connect(contextMenuRefactorSelected, &QAction::triggered, this, &VuoEditorComposition::refactorRequested);
	connect(contextMenuPublishPort, &QAction::triggered, this, &VuoEditorComposition::togglePortPublicationStatus);
	connect(contextMenuDeleteCables, &QAction::triggered, this, &VuoEditorComposition::deleteConnectedCables);
	connect(contextMenuHideCables, &QAction::triggered, this, &VuoEditorComposition::hideConnectedCables);
	connect(contextMenuUnhideCables, &QAction::triggered, this, &VuoEditorComposition::unhideConnectedCables);
	connect(contextMenuFireEvent, &QAction::triggered, this, static_cast<void (VuoEditorComposition::*)()>(&VuoEditorComposition::fireTriggerPortEvent));
	connect(contextMenuAddInputPort, &QAction::triggered, this, &VuoEditorComposition::addInputPort);
	connect(contextMenuRemoveInputPort, &QAction::triggered, this, &VuoEditorComposition::removeInputPort);
	connect(contextMenuEditSelectedComments, &QAction::triggered, this, &VuoEditorComposition::editSelectedComments);

	// Use a queued connection to open input editors in order to avoid bug where invoking a
	// QColorDialog by context menu prevents subsequent interaction with the editor window
	// even after the color dialog has been closed.
	connect(contextMenuSetPortConstant, &QAction::triggered, this, &VuoEditorComposition::setPortConstant, Qt::QueuedConnection);

	{
		// Workaround for a bug in Qt 5.1.0-beta1 (https://b33p.net/kosada/node/5096).
		// For now, this sets up the actions for a menu, rather than setting up the menu itself.

		auto addThrottlingAction = [=](QString label, VuoPortClass::EventThrottling throttling) {
			QAction *action = new QAction(label, this);
			connect(action, &QAction::triggered, [=](){
				emit triggerThrottlingUpdated(action->data().value<VuoRendererPort *>()->getBase(), throttling);
			});
			contextMenuThrottlingActions.append(action);
		};
		addThrottlingAction(tr("Enqueue Events"), VuoPortClass::EventThrottling_Enqueue);
		addThrottlingAction(tr("Drop Events"),    VuoPortClass::EventThrottling_Drop);
	}

	{
		// Workaround for a bug in Qt 5.1.0-beta1 (https://b33p.net/kosada/node/5096).
		// For now, this sets up the actions for a menu, rather than setting up the menu itself.

		auto addTintAction = [=](QString label, VuoNode::TintColor tint) {
			QAction *action = new QAction(label, this);
			connect(action, &QAction::triggered, [=](){
				static_cast<VuoEditorWindow *>(window)->tintSelectedItems(tint);
			});

			// Add a color swatch to the menu item.
			{
				QColor fill(0,0,0,0);
				// For TintNone, draw a transparent icon, so that menu item's text indent is consistent with the other items.
				if (tint != VuoNode::TintNone)
				{
					VuoRendererColors colors(tint);
					fill = colors.nodeFill();
				}

				QIcon *icon = VuoInputEditorIcon::renderIcon(^(QPainter &p){
					p.setPen(Qt::NoPen);
					p.setBrush(fill);
					// Match distance between text baseline and ascender.
					p.drawEllipse(3, 3, 10, 10);
				});
				action->setIcon(*icon);
				delete icon;
			}

			contextMenuTintActions.append(action);
		};
		addTintAction(tr("Yellow"),    VuoNode::TintYellow);
		addTintAction(tr("Tangerine"), VuoNode::TintTangerine);
		addTintAction(tr("Orange"),    VuoNode::TintOrange);
		addTintAction(tr("Magenta"),   VuoNode::TintMagenta);
		addTintAction(tr("Violet"),    VuoNode::TintViolet);
		addTintAction(tr("Blue"),      VuoNode::TintBlue);
		addTintAction(tr("Cyan"),      VuoNode::TintCyan);
		addTintAction(tr("Green"),     VuoNode::TintGreen);
		addTintAction(tr("Lime"),      VuoNode::TintLime);
		addTintAction(tr("None"),      VuoNode::TintNone);
	}

	// 'Show Events' mode rendering setup
	this->refreshComponentAlphaLevelTimer = new QTimer(this);
	this->refreshComponentAlphaLevelTimer->setObjectName("VuoEditorComposition::refreshComponentAlphaLevelTimer");
	refreshComponentAlphaLevelTimer->setInterval(showEventsModeUpdateInterval);
	connect(refreshComponentAlphaLevelTimer, &QTimer::timeout, this, &VuoEditorComposition::updateGeometryForAllComponents);
	setShowEventsMode(false);

	connect(signaler, &VuoRendererSignaler::nodePopoverRequested, this, &VuoEditorComposition::enablePopoverForNode);
	connect(signaler, &VuoRendererSignaler::nodesMoved, this, &VuoEditorComposition::moveNodesBy);
	connect(signaler, &VuoRendererSignaler::commentsMoved, this, &VuoEditorComposition::moveCommentsBy);
	connect(signaler, &VuoRendererSignaler::commentResized, this, &VuoEditorComposition::resizeCommentBy);
	connect(signaler, &VuoRendererSignaler::inputEditorRequested, this, &VuoEditorComposition::inputEditorRequested);
	connect(signaler, &VuoRendererSignaler::nodeTitleEditorRequested, this, &VuoEditorComposition::nodeTitleEditorRequested);
	connect(signaler, &VuoRendererSignaler::commentEditorRequested, this, &VuoEditorComposition::commentEditorRequested);
	connect(signaler, &VuoRendererSignaler::commentZoomRequested, this, &VuoEditorComposition::commentZoomRequested);
	connect(signaler, &VuoRendererSignaler::nodeSourceEditorRequested, this, &VuoEditorComposition::nodeSourceEditorRequested);
	connect(signaler, &VuoRendererSignaler::inputPortCountAdjustmentRequested, this, &VuoEditorComposition::inputPortCountAdjustmentRequested);
	connect(signaler, &VuoRendererSignaler::dragStickinessDisableRequested, this, &VuoEditorComposition::setDisableDragStickiness);
	connect(signaler, &VuoRendererSignaler::openUrl, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::activeApplicationStateChanged, this, &VuoEditorComposition::updatePopoversForApplicationStateChange, Qt::QueuedConnection);
	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::focusChanged, this, &VuoEditorComposition::updatePopoversForActiveWindowChange, Qt::QueuedConnection);
	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::applicationWillHide, this, [=]{
		setPopoversHideOnDeactivate(true);
	});
	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::applicationDidUnhide, this, [=]{
		setPopoversHideOnDeactivate(false);
	});

	identifierCache = new VuoNodeAndPortIdentifierCache;
	identifierCache->addCompositionComponentsToCache(getBase());
}

/**
 * Specifies a compiler instance to be used by this composition (for, e.g., instantiating dropped nodes).
 */
void VuoEditorComposition::setCompiler(VuoCompiler *compiler)
{
	this->compiler = compiler;
}

/**
 * Returns the compiler instance being used by this composition.
 */
VuoCompiler *VuoEditorComposition::getCompiler()
{
	return compiler;
}

/**
 * Specifies the module manager used by this composition and its containing VuoEditorWindow.
 */
void VuoEditorComposition::setModuleManager(VuoModuleManager *moduleManager)
{
	this->moduleManager = moduleManager;
	moduleManager->setComposition(this);
}

/**
 * Returns the module manager used by this composition and its containing VuoEditorWindow.
 */
VuoModuleManager * VuoEditorComposition::getModuleManager(void)
{
	return moduleManager;
}

/**
 * Specifies an input editor manager instance to be used by this composition (for, e.g., determining
 * whether the port nearest a right-click has an available input editor and should therefore have a
 * "Set Value" item in its context menu).
 */
void VuoEditorComposition::setInputEditorManager(VuoInputEditorManager *inputEditorManager)
{
	this->inputEditorManager = inputEditorManager;
}

/**
 * Returns the input editor manager instance used by this composition (for, e.g., determining
 * whether the port nearest a right-click has an available input editor and should therefore have a
 * "Set Value" item in its context menu).
 */
VuoInputEditorManager * VuoEditorComposition::getInputEditorManager()
{
	return this->inputEditorManager;
}

/**
 * Instantiates a node of class name @c nodeClassName.
 */
VuoRendererNode * VuoEditorComposition::createNode(QString nodeClassName, string title, double x, double y)
{
	if (compiler)
	{
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toUtf8().constData());
		if (nodeClass)
		{
			VuoNode *node = createBaseNode(nodeClass, nullptr, title, x, y);
			if (node)
			{
				VuoRendererNode *rn = createRendererNode(node);
				setCustomConstantsForNewNode(rn);
				return rn;
			}
		}
	}
	return NULL;
}

/**
 * Creates an instance of @a nodeClass if it is legal to add to this composition. Otherwise, creates an instance
 * of a node class with the same name but no implementation.
 *
 * Copies attributes from @a modelNode if provided, otherwise @a title, @a x, and @a y.
 */
VuoNode * VuoEditorComposition::createBaseNode(VuoCompilerNodeClass *nodeClass, VuoNode *modelNode, string title, double x, double y)
{
	// If adding the node would create recursion (subcomposition contains itself), create a node without a compiler detail.
	__block bool isAllowed = true;
	if (nodeClass->isSubcomposition())
	{
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyIfInstalledAsSubcomposition(this, ^void (VuoEditorComposition *currComposition, string compositionPath) {
			string compositionModuleKey = VuoCompiler::getModuleKeyForPath(compositionPath);
			bool nodeIsThisComposition = (compositionModuleKey == nodeClass->getBase()->getClassName());

			set<string> dependencies = nodeClass->getDependencies();
			auto iter = std::find_if(dependencies.begin(), dependencies.end(), [=](const string &d){ return d == compositionModuleKey; });
			bool nodeContainsThisComposition = (iter != dependencies.end());

			isAllowed = ! (nodeIsThisComposition || nodeContainsThisComposition);
		});
	}

	VuoNode *node;
	if (isAllowed)
	{
		node = (modelNode ?
					compiler->createNode(nodeClass, modelNode) :
					compiler->createNode(nodeClass, title, x, y));
	}
	else
	{
		node = createNodeWithMissingImplementation(nodeClass->getBase(), modelNode, title, x, y);
		node->setForbidden(true);
	}
	return node;
}

/**
 * Creates an instance of an implementation-less node class.
 *
 * For the node class, copies attributes from @a modelNodeClass.
 * For the node, copies attributes from @a modelNode if provided, otherwise @a title, @a x, and @a y.
 */
VuoNode * VuoEditorComposition::createNodeWithMissingImplementation(VuoNodeClass *modelNodeClass, VuoNode *modelNode, string title, double x, double y)
{
	vector<string> inputPortClassNames;
	vector<string> outputPortClassNames;
	foreach (VuoPortClass *portClass, modelNodeClass->getInputPortClasses())
	{
		if (portClass == modelNodeClass->getRefreshPortClass())
			continue;
		inputPortClassNames.push_back(portClass->getName());
	}
	foreach (VuoPortClass *portClass, modelNodeClass->getOutputPortClasses())
		outputPortClassNames.push_back(portClass->getName());

	VuoNodeClass *dummyNodeClass = new VuoNodeClass(modelNodeClass->getClassName(), inputPortClassNames, outputPortClassNames);
	return (modelNode ?
				dummyNodeClass->newNode(modelNode) :
				dummyNodeClass->newNode(! title.empty() ? title : modelNodeClass->getDefaultTitle(), x, y));
}

/**
 * Sets certain custom constant values, overriding the defaults from the node class implementations,
 * for certain classes of nodes. For use, e.g., when instantiating nodes from the library
 * (but not when copying/pasting pre-existing nodes).
 */
void VuoEditorComposition::setCustomConstantsForNewNode(VuoRendererNode *newNode)
{
	// vuo.time.make: Set the 'year' input to the current year.
	if (newNode->getBase()->getNodeClass()->getClassName() == "vuo.time.make")
	{
		VuoPort *yearPort = newNode->getBase()->getInputPortWithName("year");
		if (yearPort)
		{
			QString currentYear = QString::number(QDateTime::currentDateTime().date().year());
			yearPort->getRenderer()->setConstant(VuoText_getString(currentYear.toUtf8().constData()));
		}
	}
}

/**
 * Adds a node to the canvas and registers the node and its ports in the composition's ID maps.
 */
void VuoEditorComposition::addNode(VuoNode *n, bool nodeShouldBeRendered, bool nodeShouldBeGivenUniqueIdentifier)
{
	VuoRendererComposition::addNode(n, nodeShouldBeRendered, nodeShouldBeGivenUniqueIdentifier);
	identifierCache->addNodeToCache(n);
}

/**
 * Removes a node from the canvas and disables all of its associated port popovers.
 * If @c resetState is true, all references to the node in the current editing
 * session (open popovers, trigger port marked for autofiring) are removed.
 */
void VuoEditorComposition::removeNode(VuoRendererNode *rn, bool resetState)
{
	if (resetState)
	{
		disablePortPopovers(rn);

		if (getTriggerPortToRefire() && (getTriggerPortToRefire()->getRenderer()->getUnderlyingParentNode() == rn))
			setTriggerPortToRefire(NULL);
	}

	VuoRendererComposition::removeNode(rn);
}

/**
 * Replaces @a oldNode with @a newNode in the composition in response to an updated
 * node class implementation.
 *
 * Transfers cable and published port connections from @a oldNode to @a newNode
 * where port names and data types correspond. Severs the rest.
 *
 * Transfers constant input port values from @a oldNode to @a newNode
 * where port names and data types correspond.
 *
 * Removes any drawers stranded by the replacement.
 * @todo https://b33p.net/kosada/node/16441 : Add any new drawers required after the replacement.
 */
void VuoEditorComposition::updateNodeImplementationInPlace(VuoRendererNode *oldNode, VuoNode *newNode)
{
	// Inventory the port constants and connected input cables associated with the old node, to be re-associated with the new node.
	map<VuoCable *, VuoPort *> cablesToTransferFromPort;
	map<VuoCable *, VuoPort *> cablesToTransferToPort;
	set<VuoCable *> cablesToRemove;
	getBase()->getCompiler()->getChangesToReplaceNode(oldNode->getBase(), newNode, cablesToTransferFromPort, cablesToTransferToPort, cablesToRemove);

	// Also inventory any typecasts collapsed onto the old node, to be attached to the new node instead.
	vector<VuoRendererInputDrawer *> attachedDrawers;
	vector<VuoRendererNode *> collapsedTypecasts;
	vector<VuoPort *> oldInputPorts = oldNode->getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator inputPort = oldInputPorts.begin(); inputPort != oldInputPorts.end(); ++inputPort)
	{
		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
		if (typecastPort)
		{
			VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
			collapsedTypecasts.push_back(typecastNode);
		}

		// If the original node is currently being rendered as collapsed typecast, uncollapse it.
		if (oldNode->getProxyCollapsedTypecast())
			uncollapseTypecastNode(oldNode);

		// Uncollapse typecasts attached to the original node.
		for (vector<VuoRendererNode *>::iterator i = collapsedTypecasts.begin(); i != collapsedTypecasts.end(); ++i)
			uncollapseTypecastNode(*i);
	}

	// Inventory any attachments to the old node, to make sure none are stranded in the replacement.
	for (vector<VuoPort *>::iterator inputPort = oldInputPorts.begin(); inputPort != oldInputPorts.end(); ++inputPort)
	{
		VuoRendererPort *inputPortRenderer = (*inputPort)->getRenderer();
		VuoRendererInputDrawer *attachedDrawer = inputPortRenderer->getAttachedInputDrawer();
		if (attachedDrawer)
			attachedDrawers.push_back(attachedDrawer);
	}

	// Perform the node replacement.
	replaceNode(oldNode, newNode);

	// Restore connected cables.
	for (map<VuoCable *, VuoPort *>::iterator i = cablesToTransferFromPort.begin(); i != cablesToTransferFromPort.end(); ++i)
		i->first->getRenderer()->setFrom(newNode, i->second);
	for (map<VuoCable *, VuoPort *>::iterator i = cablesToTransferToPort.begin(); i != cablesToTransferToPort.end(); ++i)
		i->first->getRenderer()->setTo(newNode, i->second);
	foreach (VuoCable *cable, cablesToRemove)
		removeCable(cable->getRenderer());

	// Restore constant values.
	for (VuoPort *oldInputPort : oldNode->getBase()->getInputPorts())
	{
		VuoPort *newInputPort = newNode->getInputPortWithName(oldInputPort->getClass()->getName());
		if (! newInputPort)
			continue;

		if (! oldInputPort->getRenderer()->carriesData() || ! newInputPort->getRenderer()->carriesData())
			continue;

		if (oldNode->getBase()->hasCompiler() && newNode->hasCompiler())
		{
			VuoType *oldDataType = static_cast<VuoCompilerPort *>(oldInputPort->getCompiler())->getDataVuoType();
			VuoType *newDataType = static_cast<VuoCompilerPort *>(newInputPort->getCompiler())->getDataVuoType();
			if (! (oldDataType == newDataType && oldDataType && ! dynamic_cast<VuoGenericType *>(oldDataType)) )
				continue;
		}

		string oldConstantValue;
		if (oldNode->getBase()->hasCompiler())
			oldConstantValue = oldInputPort->getRenderer()->getConstantAsString();
		else
			oldConstantValue = oldInputPort->getRawInitialValue();

		if (newNode->hasCompiler())
			updatePortConstant(static_cast<VuoCompilerPort *>(newInputPort->getCompiler()), oldConstantValue, false);
		else
			newInputPort->setRawInitialValue(oldConstantValue);
	}

	// Remove any stranded drawers and their incoming cables.
	// @todo https://b33p.net/kosada/node/16441 and https://b33p.net/kosada/node/16441 :
	// Decide how to handle stranded attachment deletion and insertion properly, including
	// updates to the running composition and all types of incoming connections to the attachments.
	// For now just make sure not to leave behind a stranded drawer or any of its incoming cables.
	foreach (VuoRendererInputDrawer *drawer, attachedDrawers)
	{
		if (!drawer->getRenderedHostPort())
		{
			foreach (VuoRendererPort *drawerPort, drawer->getDrawerPorts())
			{
				VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(drawerPort->getBase()->getRenderer());
				if (typecastPort)
					uncollapseTypecastNode(typecastPort);

				foreach (VuoCable *cable, drawerPort->getBase()->getConnectedCables())
					removeCable(cable->getRenderer());
			}

			removeNode(drawer);
		}
	}

	// Restore connected typecasts.
	for (vector<VuoRendererNode *>::iterator i = collapsedTypecasts.begin(); i != collapsedTypecasts.end(); ++i)
		collapseTypecastNode(*i);

	// Re-collapse the updated node, if applicable.
	collapseTypecastNode(newNode->getRenderer());
}

/**
 * Replaces @a oldNode with @a newNode in the composition, performing the same cleanup/setup of editor state
 * as @ref VuoEditorComposition::removeNode and @ref VuoEditorComposition::addNode.
 */
void VuoEditorComposition::replaceNode(VuoRendererNode *oldNode, VuoNode *newNode)
{
	disablePortPopovers(oldNode);

	if (getTriggerPortToRefire() && (getTriggerPortToRefire()->getRenderer()->getUnderlyingParentNode() == oldNode))
		setTriggerPortToRefire(NULL);

	newNode->setRawGraphvizDeclaration(oldNode->getBase()->getRawGraphvizDeclaration());
	if (newNode->hasCompiler())
	{
		string graphvizIdentifier = (oldNode->getBase()->hasCompiler() ?
										 oldNode->getBase()->getCompiler()->getGraphvizIdentifier() :
										 oldNode->getBase()->getRawGraphvizIdentifier());
		newNode->getCompiler()->setGraphvizIdentifier(graphvizIdentifier);
	}

	removeNode(oldNode);
	addNode(newNode, true, false);

	identifierCache->addNodeToCache(newNode);
}

/**
 * Removes a cable from the canvas.
 */
void VuoEditorComposition::removeCable(VuoRendererCable *rc, bool emitHiddenCableNotification)
{
	bool cableHidden = rc->getBase()->getCompiler()->getHidden();
	VuoRendererComposition::removeCable(rc);

	if (cableHidden && emitHiddenCableNotification)
		emit changeInHiddenCables();
}

/**
 * Adds a cable to the canvas.
 */
void VuoEditorComposition::addCable(VuoCable *cable, bool emitHiddenCableNotification)
{
	bool cableHidden = cable->getCompiler()->getHidden();
	VuoRendererComposition::addCable(cable);

	if (cableHidden && emitHiddenCableNotification)
		emit changeInHiddenCables();
}

/**
 * Creates a "Make List" node, and creates a cable from the "Make List" node to the given input port.
 *
 * @param toNode The node that contains @c toPort.
 * @param toPort The input port. Assumed to be a data-and-event input port carrying list data.
 * @param[out] rendererCable The created cable.
 * @return The created "Make List" node.
 */
VuoRendererNode * VuoEditorComposition::createAndConnectMakeListNode(VuoNode *toNode, VuoPort *toPort, VuoRendererCable *&rendererCable)
{
	return VuoRendererComposition::createAndConnectMakeListNode(toNode, toPort, compiler, rendererCable);
}

/**
 * Creates the nodes and connecting cables that the input @c node will need to provide
 * it with an input dictionary of keys and values, to be attached to the node's "values" input port.
 *
 * @param node The node that needs the dictionary attachments created.
 * @param[out] createdNodes The created nodes.
 * @param[out] createdCables The created cables.
 */
void VuoEditorComposition::createAndConnectDictionaryAttachmentsForNode(VuoNode *node,
																		  set<VuoRendererNode *> &createdNodes,
																		  set<VuoRendererCable *> &createdCables)
{
	return VuoRendererComposition::createAndConnectDictionaryAttachmentsForNode(node, compiler, createdNodes, createdCables);
}

/**
 * Creates and connects the appropriate input attachments to the provided @c node.
 *
 * @param node The node that needs the input attachments created.
 * @param createButDoNotAdd A boolean specifying whether to create the attachments without adding them to the composition.
 */
QList<QGraphicsItem *>  VuoEditorComposition::createAndConnectInputAttachments(VuoRendererNode *node, bool createButDoNotAdd)
{
	QList<QGraphicsItem *> addedComponents = VuoRendererComposition::createAndConnectInputAttachments(node, compiler, createButDoNotAdd);
	foreach (QGraphicsItem *component, addedComponents)
	{
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(component);
		if (rn && !createButDoNotAdd)
			identifierCache->addNodeToCache(rn->getBase());
	}

	return addedComponents;
}

/**
 * Returns the set of connected attachments upstream of the provided node, meaning that if
 * the node is deleted, the attachments should be as well.
 *
 * @param rn The renderer node whose attachments should be returned.
 * @param includeCoattachments If the node is itself an attachment, include sibling attachments.
 */
set<QGraphicsItem *> VuoEditorComposition::getDependentAttachmentsForNode(VuoRendererNode *rn, bool includeCoattachments)
{
	set<QGraphicsItem *> dependentAttachments;

	// Get upstream attachments.
	vector<VuoPort *> inputPorts = rn->getBase()->getInputPorts();
	for(unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		set<VuoRendererInputAttachment *> portUpstreamAttachments = inputPorts[i]->getRenderer()->getAllUnderlyingUpstreamInputAttachments();
		dependentAttachments.insert(portUpstreamAttachments.begin(), portUpstreamAttachments.end());
	}

	// Get co-attachments.
	VuoRendererInputAttachment *nodeAsAttachment = dynamic_cast<VuoRendererInputAttachment *>(rn);
	if (nodeAsAttachment && includeCoattachments)
	{
		foreach (VuoNode *coattachment, nodeAsAttachment->getCoattachments())
			dependentAttachments.insert(coattachment->getRenderer());
	}

	return dependentAttachments;
}


/**
 * Wraps the call to @a modify in code that saves and restores the state of the composition,
 * such as selected components.
 */
void VuoEditorComposition::modifyComponents(void (^modify)(void))
{
	identifierCache->clearCache();

	// Record the IDs of the currently selected components so that the selection status
	// of the corresponding items may be restored after the composition is reset.
	set<string> selectedNodeIDs;
	foreach (VuoRendererNode *rn, getSelectedNodes())
	{
		if (rn->getBase()->hasCompiler())
			selectedNodeIDs.insert(rn->getBase()->getCompiler()->getGraphvizIdentifier());
	}

	set<string> selectedCommentIDs;
	foreach (VuoRendererComment *rc, getSelectedComments())
	{
		if (rc->getBase()->hasCompiler())
			selectedCommentIDs.insert(rc->getBase()->getCompiler()->getGraphvizIdentifier());
	}

	set<string> selectedCableIDs;
	foreach (VuoRendererCable *rc, getSelectedCables(true))
	{
		if (rc->getBase()->hasCompiler())
			selectedCableIDs.insert(rc->getBase()->getCompiler()->getGraphvizDeclaration());
	}

	modify();

	// Restore the selection status of pre-existing components.
	foreach (QGraphicsItem *item, items())
	{
		if (dynamic_cast<VuoRendererNode *>(item))
		{
			string currentNodeID = (dynamic_cast<VuoRendererNode *>(item)->getBase()->hasCompiler()?
										dynamic_cast<VuoRendererNode *>(item)->getBase()->getCompiler()->getGraphvizIdentifier() :
										"");
			if (!currentNodeID.empty() && (selectedNodeIDs.find(currentNodeID) != selectedNodeIDs.end()))
				item->setSelected(true);
		}

		if (dynamic_cast<VuoRendererComment *>(item))
		{
			string currentCommentID = (dynamic_cast<VuoRendererComment *>(item)->getBase()->hasCompiler()?
										dynamic_cast<VuoRendererComment *>(item)->getBase()->getCompiler()->getGraphvizIdentifier() :
										"");
			if (!currentCommentID.empty() && (selectedCommentIDs.find(currentCommentID) != selectedCommentIDs.end()))
				item->setSelected(true);
		}

		else if (dynamic_cast<VuoRendererCable *>(item))
		{
			string currentCableID = (dynamic_cast<VuoRendererCable *>(item)->getBase()->hasCompiler()?
										 dynamic_cast<VuoRendererCable *>(item)->getBase()->getCompiler()->getGraphvizDeclaration() :
										 "");
			if (!currentCableID.empty() && (selectedCableIDs.find(currentCableID) != selectedCableIDs.end()))
				item->setSelected(true);
		}
	}

	// Re-establish mappings between the stored composition components and the running
	// composition components, if applicable.
	identifierCache->addCompositionComponentsToCache(getBase());

	// Close popovers for ports no longer present in the composition.
	disableStrandedPortPopovers();
}

/**
 * Returns a boolean indicating whether a change in value at the provided port will
 * trigger structural changes to the composition.
 */
bool VuoEditorComposition::requiresStructuralChangesAfterValueChangeAtPort(VuoRendererPort *port)
{
	string portName = port->getBase()->getClass()->getName();
	VuoRendererNode *parentNode = port->getRenderedParentNode();

	// A changed math expression input to a "Calculate" node will require changes to the node's
	// upstream input lists of variable names and values.
	if ((portName == "expression") &&
			VuoStringUtilities::beginsWith(parentNode->getBase()->getNodeClass()->getClassName(), "vuo.math.calculate"))
		return true;

	return false;
}

/**
 * Replaces nodes, if needed, to accommodate the new port value.
 */
void VuoEditorComposition::performStructuralChangesAfterValueChangeAtPort(VuoEditorWindow *editorWindow, QUndoStack *undoStack, VuoRendererPort *port, string originalEditingSessionValue, string finalEditingSessionValue)
{
	if (!requiresStructuralChangesAfterValueChangeAtPort(port))
		return;

	VuoRendererNode *parentNode = port->getRenderedParentNode();

	// Only current possibility: modifications to "Calculate" node's 'expression' input
	string nodeClassName = parentNode->getBase()->getNodeClass()->getClassName();
	vector<string> inputVariablesBeforeEditing = extractInputVariableListFromExpressionsConstant(originalEditingSessionValue, nodeClassName);
	vector<string> inputVariablesAfterEditing = extractInputVariableListFromExpressionsConstant(finalEditingSessionValue, nodeClassName);

	// Don't make any structural changes if the variables in the input expression remain
	// the same, even if the expression itself has changed.
	if (inputVariablesBeforeEditing != inputVariablesAfterEditing)
	{
		VuoPort *valuesPort = port->getRenderedParentNode()->getBase()->getInputPortWithName("values");

		set<VuoRendererInputAttachment *> attachments = valuesPort->getRenderer()->getAllUnderlyingUpstreamInputAttachments();

		QList<QGraphicsItem *> attachmentsToRemove;

		VuoRendererReadOnlyDictionary *oldDictionary = nullptr;
		VuoRendererValueListForReadOnlyDictionary *oldValueList = nullptr;
		VuoRendererKeyListForReadOnlyDictionary *oldKeyList = nullptr;
		foreach (VuoRendererInputAttachment *attachment, attachments)
		{
			attachmentsToRemove.append(attachment);

			if (dynamic_cast<VuoRendererReadOnlyDictionary *>(attachment))
				oldDictionary = dynamic_cast<VuoRendererReadOnlyDictionary *>(attachment);

			else if (dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(attachment))
				oldValueList = dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(attachment);

			else if (dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(attachment))
				oldKeyList = dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(attachment);
		}

		if (oldValueList && oldDictionary && oldKeyList)
		{
			set<VuoRendererNode *> nodesToAdd;
			set<VuoRendererCable *> cablesToAdd;
			createAndConnectDictionaryAttachmentsForNode(parentNode->getBase(), nodesToAdd, cablesToAdd);

			VuoRendererReadOnlyDictionary *newDictionary = nullptr;
			VuoRendererValueListForReadOnlyDictionary *newValueList = nullptr;
			VuoRendererKeyListForReadOnlyDictionary *newKeyList = nullptr;
			foreach (VuoRendererNode *node, nodesToAdd)
			{
				if (dynamic_cast<VuoRendererReadOnlyDictionary *>(node))
					newDictionary = dynamic_cast<VuoRendererReadOnlyDictionary *>(node);

				else if (dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(node))
					newValueList = dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(node);

				else if (dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(node))
					newKeyList = dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(node);
			}

			undoStack->push(new VuoCommandReplaceNode(oldValueList, newValueList, editorWindow, "Set Port Constant", false, false));
			undoStack->push(new VuoCommandReplaceNode(oldKeyList, newKeyList, editorWindow, "Set Port Constant", false, true));
			undoStack->push(new VuoCommandReplaceNode(oldDictionary, newDictionary, editorWindow, "Set Port Constant", false, true));

			foreach (VuoRendererCable *cable, cablesToAdd)
			{
				cable->setFrom(nullptr, nullptr);
				cable->setTo(nullptr, nullptr);
			}
		}
	}
}

/**
 * Deletes currently selected nodes, comments, and cables.
 */
void VuoEditorComposition::deleteSelectedCompositionComponents()
{
	deleteSelectedCompositionComponents("");
}

/**
 * Deletes currently selected nodes, comments, and cables.
 */
void VuoEditorComposition::deleteSelectedCompositionComponents(string commandDescription)
{
	if (commandDescription.empty())
	{
		if (getContextMenuDeleteSelectedAction()->text().contains("Reset"))
			commandDescription = "Reset";
		else
			commandDescription = "Delete";
	}

	QList<QGraphicsItem *> selectedCompositionComponents = selectedItems();
	emit componentsRemoved(selectedCompositionComponents, commandDescription);
}

/**
 * Deletes currently selected nodes.
 */
void VuoEditorComposition::deleteSelectedNodes(string commandDescription)
{
	if (commandDescription.empty())
		commandDescription = "Delete";

	QList<QGraphicsItem *> selectedNodes;
	foreach (VuoRendererNode *node, getSelectedNodes())
		selectedNodes.append(node);

	emit componentsRemoved(selectedNodes, commandDescription);
}

/**
 * Deletes all composition components.
 */
void VuoEditorComposition::clear()
{
	identifierCache->clearCache();

	uncollapseTypecastNodes();

	foreach (VuoCable *cable, getBase()->getCables())
		removeCable(cable->getRenderer(), false);

	// @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	removeActiveProtocolWithoutModifyingPorts(getActiveProtocol());

	foreach (VuoPublishedPort *publishedPort, getBase()->getPublishedOutputPorts())
		removePublishedPort(publishedPort, false);

	foreach (VuoPublishedPort *publishedPort, getBase()->getPublishedInputPorts())
		removePublishedPort(publishedPort, true);

	foreach (VuoNode *node, getBase()->getNodes())
		removeNode(node->getRenderer(), false);

	getBase()->getCompiler()->clearGraphvizNodeIdentifierHistory();

	foreach (VuoComment *comment, getBase()->getComments())
		removeComment(comment->getRenderer());

	getBase()->getCompiler()->clearGraphvizCommentIdentifierHistory();
}

/**
 * Inserts a node at the scene position indicated in the sender data.
 */
void VuoEditorComposition::insertNode()
{
	QAction *sender = (QAction *)QObject::sender();
	QPair<QPointF, QString> pair = sender->data().value<QPair<QPointF, QString> >();

	QList<QGraphicsItem *> newNodes;
	VuoRendererNode *newNode = createNode(pair.second, "",
										  pair.first.x(),
										  pair.first.y());

	if (newNode)
	{
		newNodes.append(newNode);
		emit componentsAdded(newNodes, this);
	}
}

/**
 * Inserts a comment at the scene position indicated in the sender data.
 */
void VuoEditorComposition::insertComment()
{
	QAction *sender = (QAction *)QObject::sender();
	QPointF scenePos = sender->data().value<QPointF>();

	emit commentInsertionRequested(scenePos);
}

/**
 * Inserts a subcomposition node at the scene position indicated in the sender data.
 */
void VuoEditorComposition::insertSubcomposition()
{
	QAction *sender = (QAction *)QObject::sender();
	QPointF scenePos = sender->data().value<QPointF>();

	emit subcompositionInsertionRequested(scenePos);
}

/**
 * Publishes or unpublishes the port associated with the signal
 * that activated this slot, as appropriate.
 */
void VuoEditorComposition::togglePortPublicationStatus()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();

	if (isPortPublished(port))
		emit portUnpublicationRequested(port->getBase());
	else
		emit portPublicationRequested(port->getBase(), false);
}

/**
 * Deletes the visible cables connected directly to the port
 * associated with the signal that activated this slot; if the port
 * has a collapsed typecast, delete any visible cables connected
 * to the typecast's child port (along with the typecast itself, if appropriate).
 */
void VuoEditorComposition::deleteConnectedCables()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();
	vector<VuoCable *> connectedCables = port->getBase()->getConnectedCables(true);
	QList<QGraphicsItem *> cablesToRemove;

	// Delete visible directly connected cables.
	foreach (VuoCable *cable, port->getBase()->getConnectedCables(true))
	{
		if (!cable->getRenderer()->paintingDisabled())
			cablesToRemove.append(cable->getRenderer());
	}

	// Delete visible cables connected to the typecast's child port.
	VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(port);
	if (typecastPort)
	{
		VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
		VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
		foreach(VuoCable *cable, typecastInPort->getConnectedCables(true))
		{
			if (!cable->getRenderer()->paintingDisabled())
				cablesToRemove.append(cable->getRenderer());
		}
	}

	emit componentsRemoved(QList<QGraphicsItem *>(cablesToRemove));
}

/**
 * Hides the visible cables connected directly to the port
 * associated with the signal that activated this slot; if the port
 * has a collapsed typecast, hide any visible cables connected
 * to the typecast's child port.
 */
void VuoEditorComposition::hideConnectedCables()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();
	set<VuoRendererCable *> cablesToHide;

	// Hide visible directly connected cables.
	foreach (VuoCable *cable, port->getBase()->getConnectedCables(false))
	{
		if (!cable->getRenderer()->paintingDisabled() && !cable->getCompiler()->getHidden())
			cablesToHide.insert(cable->getRenderer());
	}

	// Hide visible cables connected to the typecast's child port.
	VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(port);
	if (typecastPort)
	{
		VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
		VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
		foreach (VuoCable *cable, typecastInPort->getConnectedCables(false))
		{
			if (!cable->getRenderer()->paintingDisabled() && !cable->getCompiler()->getHidden())
				cablesToHide.insert(cable->getRenderer());
		}
	}

	emit cablesHidden(cablesToHide);
}

/**
 * Unhides the hidden cables connected directly to the port
 * associated with the signal that activated this slot; if the port
 * has a collapsed typecast, unhide any hidden cables connected
 * to the typecast's child port.
 */
void VuoEditorComposition::unhideConnectedCables()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();
	set<VuoRendererCable *> cablesToUnhide;

	// Unhide visible directly connected cables.
	foreach (VuoCable *cable, port->getBase()->getConnectedCables(true))
	{
		if (cable->getRenderer()->getEffectivelyWireless())
			cablesToUnhide.insert(cable->getRenderer());
	}

	// Unhide visible cables connected to the typecast's child port.
	VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(port);
	if (typecastPort)
	{
		VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
		VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
		foreach (VuoCable *cable, typecastInPort->getConnectedCables(true))
		{
			if (cable->getRenderer()->getEffectivelyWireless())
				cablesToUnhide.insert(cable->getRenderer());
		}
	}

	emit cablesUnhidden(cablesToUnhide);
}

/**
 * Fires an event from the trigger port associated with the signal
 * that activated this slot, if the composition is running.
 */
void VuoEditorComposition::fireTriggerPortEvent()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();
	fireTriggerPortEvent(port->getBase());
}

/**
 * Fires another event from the trigger port that was most recently manually fired.
 */
void VuoEditorComposition::refireTriggerPortEvent()
{
	fireTriggerPortEvent(getTriggerPortToRefire());
}

/**
 * Returns the trigger port to be fired when the user selects the "re-fire" option.
 */
VuoPort * VuoEditorComposition::getTriggerPortToRefire()
{
	if (triggerPortToRefire.empty())
		return nullptr;

	VuoPort *triggerPort = nullptr;
	identifierCache->doForPortWithIdentifier(triggerPortToRefire, [&triggerPort](VuoPort *port) {
		triggerPort = port;
	});
	return triggerPort;
}

/**
 * Sets the trigger port to be fired when the user selects the "re-fire" option.
 */
void VuoEditorComposition::setTriggerPortToRefire(VuoPort *port)
{
	string portID = getIdentifierForStaticPort(port);
	if (portID != this->triggerPortToRefire)
	{
		this->triggerPortToRefire = portID;
		emit refirePortChanged();
	}
}

/**
 * Invokes the input editor for the input port associated with the signal
 * that activated this slot.
 */
void VuoEditorComposition::setPortConstant()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();

	if (port->isConstant())
		emit inputEditorRequested(port);
}

/**
 * Emits a signal to request that the constant value for the provided @c port
 * be set to the provided @c value.
 */
void VuoEditorComposition::setPortConstantToValue(VuoRendererPort *port, string value)
{
	if (port->isConstant())
		emit portConstantChangeRequested(port, value);
}

/**
 * Specializes a generic port as indicated by the user's selection from
 * the port's context menu.
 */
void VuoEditorComposition::specializeGenericPortType()
{
	QAction *sender = (QAction *)QObject::sender();
	QList<QVariant> portAndSpecializedType= sender->data().toList();
	VuoRendererPort *port = portAndSpecializedType[0].value<VuoRendererPort *>();
	QString specializedTypeName = portAndSpecializedType[1].toString();

	// If the port is already specialized to the target type, do nothing.
	if (port && (port->getDataType()->getModuleKey() == specializedTypeName.toUtf8().constData()))
		return;

	// If the port is already specialized to a different type, re-specialize it.
	if (port && !dynamic_cast<VuoGenericType *>(port->getDataType()))
		emit respecializePort(port, specializedTypeName.toUtf8().constData());

	// Otherwise, specialize the port from generic.
	else
		emit specializePort(port, specializedTypeName.toUtf8().constData());
}

/**
 * Reverts a specialized port to its generic origins in response to the
 * user's directive from the port's context menu.
 */
void VuoEditorComposition::unspecializePortType()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();

	// If the port is already generic, do nothing.
	if (port && dynamic_cast<VuoGenericType *>(port->getDataType()))
		return;

	emit unspecializePort(port);
}

/**
 * Outputs the composition components that would need to be modified in order to unspecialize the given port.
 *
 * @param portToUnspecialize The port to unspecialize.
 * @param shouldOutputNodesToReplace If true, `nodesToReplace` will be populated.
 * @param nodesToReplace The nodes that would need to be replaced, and the names of the less-specialized node classes
 *			that would replace them.
 * @param cablesToDelete The cables (including published) that would need to be removed because they would become invalid,
 *			having a generic port on one end and a non-generic port on the other end.
 */
void VuoEditorComposition::createReplacementsToUnspecializePort(VuoPort *portToUnspecialize, bool shouldOutputNodesToReplace, map<VuoNode *, string> &nodesToReplace, set<VuoCable *> &cablesToDelete)
{
	// Find the ports that will share the same generic type as portToUnspecialize, and organize them by node.
	set<pair<VuoNode *, VuoPort *>> connectedPotentiallyGenericPorts = getBase()->getCompiler()->getCorrelatedGenericPorts(portToUnspecialize->getRenderer()->getUnderlyingParentNode()->getBase(),
																														   portToUnspecialize, true);
	map<VuoNode *, set<VuoPort *> > portsToUnspecializeForNode;
	for (pair<VuoNode *, VuoPort *> i : connectedPotentiallyGenericPorts)
	{
		VuoNode *node = i.first;
		VuoPort *connectedPort = i.second;

		// @todo: Don't just exclude ports that aren't currently revertible, also exclude ports that are only
		// within the current network by way of ports that aren't currently revertible.
		if (isPortCurrentlyRevertible(connectedPort->getRenderer()))
			portsToUnspecializeForNode[node].insert(connectedPort);
	}

	for (map<VuoNode *, set<VuoPort *> >::iterator i = portsToUnspecializeForNode.begin(); i != portsToUnspecializeForNode.end(); ++i)
	{
		VuoNode *node = i->first;
		set<VuoPort *> ports = i->second;

		if (shouldOutputNodesToReplace)
		{
			// Create the unspecialized node class name for each node to unspecialize.
			set<VuoPortClass *> portClasses;
			for (set<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
				portClasses.insert((*j)->getClass());
			VuoCompilerSpecializedNodeClass *nodeClass = static_cast<VuoCompilerSpecializedNodeClass *>(node->getNodeClass()->getCompiler());
			string unspecializedNodeClassName = nodeClass->createUnspecializedNodeClassName(portClasses);
			nodesToReplace[node] = unspecializedNodeClassName;
		}

		// Identify the cables that will become invalid (data-carrying cable with generic port at one end, non-generic port at the other end)
		// when the node is unspecialized.
		for (set<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoPort *port = *j;
			for (VuoCable *cable : port->getConnectedCables(true))
			{
				bool areEndsCompatible = false;

				if (!cable->getRenderer()->effectivelyCarriesData())
					areEndsCompatible = true;

				else if (! cable->isPublished())
				{
					VuoPort *portOnOtherEnd = (cable->getFromPort() == port ? cable->getToPort() : cable->getFromPort());
					if (portOnOtherEnd && isPortCurrentlyRevertible(portOnOtherEnd->getRenderer()))
					{
						VuoNode *nodeOnOtherEnd = portOnOtherEnd->getRenderer()->getUnderlyingParentNode()->getBase();
						VuoCompilerNodeClass *nodeClassOnOtherEnd = nodeOnOtherEnd->getNodeClass()->getCompiler();
						VuoCompilerSpecializedNodeClass *specializedNodeClassOnOtherEnd = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClassOnOtherEnd);
						if (specializedNodeClassOnOtherEnd)
						{
							VuoType *typeOnOtherEnd = specializedNodeClassOnOtherEnd->getOriginalPortType( portOnOtherEnd->getClass() );
							if (! typeOnOtherEnd || dynamic_cast<VuoGenericType *>(typeOnOtherEnd))
								areEndsCompatible = true;
						}
					}
				}

				if (! areEndsCompatible && (cable != cableInProgress))
					cablesToDelete.insert(cable);
			}
		}
	}
}

/**
 * Fires an event from @a port if it's a trigger port or into @a port if it's an input port, if the composition is running.
 */
void VuoEditorComposition::fireTriggerPortEvent(VuoPort *port)
{
	if (! (port && port->hasCompiler()) )
		return;

	VuoPort *oldManuallyFirableInputPort = getBase()->getCompiler()->getManuallyFirableInputPort();
	string oldSnapshot = takeSnapshot();

	string runningTriggerPortIdentifier = "";
	bool isTriggerPort = false;
	if (dynamic_cast<VuoCompilerTriggerPort *>(port->getCompiler()))
	{
		// Trigger port — The event will be fired from the port.

		getBase()->getCompiler()->setManuallyFirableInputPort(nullptr, nullptr);

		runningTriggerPortIdentifier = identifierCache->getIdentifierForPort(port);
		isTriggerPort = true;
	}
	else if (port->hasRenderer() && port->getRenderer()->getInput())
	{
		// Input port — The event will be fired from the composition's manually firable trigger into the port.

		getBase()->getCompiler()->setManuallyFirableInputPort(port->getRenderer()->getUnderlyingParentNode()->getBase(), port);

		VuoCompilerGraph *graph = getBase()->getCompiler()->getCachedGraph();
		VuoCompilerTriggerPort *triggerPort = graph->getManuallyFirableTrigger();
		VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(triggerPort);
		runningTriggerPortIdentifier = getIdentifierForStaticPort(triggerPort->getBase(), triggerNode->getBase());
	}
	else
		return;

	VUserLog("%s:      Fire %s",
		window->getWindowTitleWithoutPlaceholder().toUtf8().data(),
		runningTriggerPortIdentifier.c_str());

	VuoPort *newManuallyFirableInputPort = getBase()->getCompiler()->getManuallyFirableInputPort();
	bool manuallyFirableInputPortChanged = (oldManuallyFirableInputPort != newManuallyFirableInputPort);
	string newSnapshot = takeSnapshot();

	auto fireIfRunning = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		dispatch_async(topLevelComposition->runCompositionQueue, ^{
			if (topLevelComposition->isRunningThreadUnsafe())
			{
				topLevelComposition->runner->fireTriggerPortEvent(thisCompositionIdentifier, runningTriggerPortIdentifier);

				// Display the trigger port animation when the user manually fires an event
				// even if not in "Show Events" mode.  (If in "Show Events" mode, this will
				// be handled for trigger ports by VuoEditorComposition::receivedTelemetryOutputPortUpdated(...).)
				if (! (this->showEventsMode && isTriggerPort) )
					this->animatePort(port->getRenderer());
			}
		});
	};

	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier) {
		if (this == topLevelComposition || ! manuallyFirableInputPortChanged)
		{
			// Top-level composition or unmodified subcomposition — Fire the trigger immediately.

			if (! newSnapshot.empty() && manuallyFirableInputPortChanged)
				updateRunningComposition(oldSnapshot, newSnapshot);

			fireIfRunning(topLevelComposition, thisCompositionIdentifier);
		}
		else
		{
			// Modified subcomposition — Fire the trigger after the subcomposition has been reloaded.

			static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyIfInstalledAsSubcomposition(this, ^void (VuoEditorComposition *currComposition, string compositionPath) {
				string nodeClassName = VuoCompiler::getModuleKeyForPath(compositionPath);
				moduleManager->doNextTimeNodeClassIsLoaded(nodeClassName, ^{
					static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier) {
						fireIfRunning(topLevelComposition, thisCompositionIdentifier);
					});
				});

				if (! newSnapshot.empty())
					updateRunningComposition(oldSnapshot, newSnapshot);
			});
		}
	});

	setTriggerPortToRefire(port);
}

/**
 * Adds an input port to the node associated with the signal
 * that activated this slot.
 */
void VuoEditorComposition::addInputPort()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererNode *node = sender->data().value<VuoRendererNode *>();
	emit inputPortCountAdjustmentRequested(node, 1, false);
}

/**
 * Removes an input port from the node associated with the signal
 * that activated this slot.
 */
void VuoEditorComposition::removeInputPort()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererNode *node = sender->data().value<VuoRendererNode *>();
	emit inputPortCountAdjustmentRequested(node, -1, false);
}

/**
 * Replaces the node associated with the signal that activated this slot
 * with a new node of the node class specified.
 */
void VuoEditorComposition::swapNode()
{
	QAction *sender = (QAction *)QObject::sender();
	QList<QVariant> nodeAndReplacementType= sender->data().toList();
	VuoRendererNode *node = nodeAndReplacementType[0].value<VuoRendererNode *>();
	QString newNodeClassName = nodeAndReplacementType[1].toString();
	emit nodeSwapRequested(node, newNodeClassName.toUtf8().constData());
}

/**
 * Triggers interactive re-naming of selected non-drawer nodes.
 */
void VuoEditorComposition::renameSelectedNodes()
{
	// Open a title editor for each selected non-attachment node.
	set<VuoRendererNode *> selectedNodes = getSelectedNodes();
	for (set<VuoRendererNode *>::iterator i = selectedNodes.begin(); i != selectedNodes.end(); ++i)
	{
		if (!dynamic_cast<VuoRendererInputAttachment *>(*i))
			emit nodeTitleEditorRequested(*i);
	}
}

/**
 * Triggers interactive content editing of selected comments.
 */
void VuoEditorComposition::editSelectedComments()
{
	// Open a text editor for each selected comment.
	set<VuoRendererComment *> selectedComments = getSelectedComments();
	for (set<VuoRendererComment *>::iterator i = selectedComments.begin(); i != selectedComments.end(); ++i)
		emit commentEditorRequested(*i);
}

/**
 * Returns the set of cables creating internal connections within the set of input @c subcompositionComponents.
 */
set<VuoRendererCable *> VuoEditorComposition::getCablesInternalToSubcomposition(QList<QGraphicsItem *> subcompositionComponents)
{
	set<VuoRendererCable *> internalCables;

	for (QList<QGraphicsItem *>::iterator i = subcompositionComponents.begin(); i != subcompositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
		if (rn)
		{
			set<VuoCable *> connectedCables = rn->getConnectedCables(false);
			for (set<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
			{
				VuoNode *fromNode = (*cable)->getFromNode();
				VuoNode *toNode = (*cable)->getToNode();

				if (fromNode && toNode && subcompositionComponents.contains(fromNode->getRenderer()) && subcompositionComponents.contains(toNode->getRenderer()))
					internalCables.insert((*cable)->getRenderer());
			}
		}
	}

	return internalCables;
}

/**
 * Returns the cable currently being dragged, or NULL if no cable is currently being dragged.
 */
VuoCable * VuoEditorComposition::getCableInProgress()
{
	return cableInProgress;
}

/**
 * Returns a boolean indicating whether the cable currently being dragged was only just created
 * at the initiation of the drag.
 */
bool VuoEditorComposition::getCableInProgressWasNew()
{
	return cableInProgressWasNew;
}

/**
 * Returns a boolean indicating whether selection from a context menu is currently in progress.
 */
bool VuoEditorComposition::getMenuSelectionInProgress()
{
	return menuSelectionInProgress;
}

/**
 * Selects all nodes, cables, and comments on the canvas.
 */
void VuoEditorComposition::selectAllCompositionComponents()
{
	QList<QGraphicsItem *> compositionComponents = items();
	for (QList<QGraphicsItem *>::iterator i = compositionComponents.begin(); i != compositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;

		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(compositionComponent);
		if (!rc || !rc->paintingDisabled())
			compositionComponent->setSelected(true);
	}
}

/**
 * Selects all comments on the canvas.
 */
void VuoEditorComposition::selectAllComments()
{
	QList<QGraphicsItem *> compositionComponents = items();
	for (QList<QGraphicsItem *>::iterator i = compositionComponents.begin(); i != compositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(compositionComponent);
		if (rcomment)
			rcomment->setSelected(true);
	}
}

/**
 * Deselects all nodes, cables, and comments on the canvas.
 */
void VuoEditorComposition::deselectAllCompositionComponents()
{
	QList<QGraphicsItem *> compositionComponents = items();
	for (QList<QGraphicsItem *>::iterator i = compositionComponents.begin(); i != compositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		compositionComponent->setSelected(false);
	}
}

/**
 * Opens any selected node that is editable.
 */
void VuoEditorComposition::openSelectedEditableNodes()
{
	foreach (VuoRendererNode *node, getSelectedNodes())
	{
		VuoNodeClass *nodeClass = node->getBase()->getNodeClass();
		QString actionText, sourcePath;
		if (VuoEditorUtilities::isNodeClassEditable(nodeClass, actionText, sourcePath))
			emit nodeSourceEditorRequested(node);
	}
}

/**
 * Moves currently selected nodes and comments @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveSelectedItemsBy(qreal dx, qreal dy)
{
	set<VuoRendererNode *> selectedNodes = getSelectedNodes();
	set<VuoRendererComment *> selectedComments = getSelectedComments();
	moveItemsBy(selectedNodes, selectedComments, dx, dy, false);
}

/**
 * Moves the indicated @c nodes @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveNodesBy(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging)
{
	set<VuoRendererComment *> comments;
	moveItemsBy(nodes, comments, dx, dy, movedByDragging);
}

/**
 * Moves the indicated @c comments @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveCommentsBy(set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging)
{
	set<VuoRendererNode *> nodes;
	moveItemsBy(nodes, comments, dx, dy, movedByDragging);
}

/**
 * Moves the indicated @c nodes and @c comments @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveItemsBy(set<VuoRendererNode *> nodes, set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging)
{
	emit itemsMoved(nodes, comments, dx, dy, movedByDragging);
	repaintFeedbackErrorMarks();
	for (set<VuoRendererNode *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
		(*it)->updateConnectedCableGeometry();
}

/**
 * Resizes the indicated @c comment @c by +dx points horizontally and @c +dy points vertically.
 */
void VuoEditorComposition::resizeCommentBy(VuoRendererComment *comment, qreal dx, qreal dy)
{
	emit commentResized(comment, dx, dy);
}

/**
 * Returns the set of currently selected nodes.
 */
set<VuoRendererNode *> VuoEditorComposition::getSelectedNodes()
{
	QList<QGraphicsItem *> selectedCompositionComponents = selectedItems();
	set<VuoRendererNode *> selectedNodes;
	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin(); i != selectedCompositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
		if (rn)
			selectedNodes.insert(rn);
	}

	return selectedNodes;
}

/**
 * Returns the set of currently selected comments.
 */
set<VuoRendererComment *> VuoEditorComposition::getSelectedComments()
{
	QList<QGraphicsItem *> selectedCompositionComponents = selectedItems();
	set<VuoRendererComment *> selectedComments;
	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin(); i != selectedCompositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		VuoRendererComment *rc = dynamic_cast<VuoRendererComment *>(compositionComponent);
		if (rc)
			selectedComments.insert(rc);
	}

	return selectedComments;
}

/**
 * Returns the set of currently selected cables.
 */
set<VuoRendererCable *> VuoEditorComposition::getSelectedCables(bool includePublishedCables)
{
	QList<QGraphicsItem *> selectedCompositionComponents = selectedItems();
	set<VuoRendererCable *> selectedCables;
	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin(); i != selectedCompositionComponents.end(); ++i)
	{
		QGraphicsItem *compositionComponent = *i;
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(compositionComponent);
		if (rc && (includePublishedCables || !rc->getBase()->isPublished()))
			selectedCables.insert(rc);
	}

	return selectedCables;
}

/**
 * Decide whether we can accept the dragged data.
 */
void VuoEditorComposition::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	bool disablePortHoverHighlighting = true;

	// Accept drags of files.
	if (mimeData->hasFormat("text/uri-list"))
	{
		QList<QUrl> urls = mimeData->urls();

		QGraphicsItem *nearbyItem = findNearbyComponent(event->scenePos());
		VuoRendererPort *portAtDropLocation = dynamic_cast<VuoRendererPort *>(nearbyItem);
		if (portAtDropLocation)
		{
			if ((urls.size() == 1) && portAtDropLocation->isConstant() &&
					(portAtDropLocation->getDataType()->getModuleKey() == "VuoText"))
				disablePortHoverHighlighting = false;
			else
				event->setDropAction(Qt::IgnoreAction);
		}
		else // if (!portAtDropLocation)
		{
			bool dragIncludesDroppableFile = false;
			foreach (QUrl url, urls)
			{
				char *urlZ = strdup(url.path().toUtf8().constData());
				bool isSupportedDragNDropFile =
					VuoFileType_isFileOfType(urlZ, VuoFileType_Image)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Movie)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Scene)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Audio)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Feed)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_JSON)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_XML)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Table)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Mesh)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_Data)
				 || VuoFileType_isFileOfType(urlZ, VuoFileType_App)
				 || isDirectory(urlZ);
				free(urlZ);
				if (isSupportedDragNDropFile)
				{
					dragIncludesDroppableFile = true;
					break;
				}
			}

			if (!dragIncludesDroppableFile)
				event->setDropAction(Qt::IgnoreAction);
		}

		event->accept();
	}

	// Accept drags of single or multiple nodes from the node library.
	else if (mimeData->hasFormat("text/plain") || mimeData->hasFormat("text/scsv"))
		event->acceptProposedAction();

	else
	{
		event->setDropAction(Qt::IgnoreAction);
		event->accept();
	}

	updateHoverHighlighting(event->scenePos(), disablePortHoverHighlighting);
}

/**
 * Decide what to do with the dragged data as it leaves.
 */
void VuoEditorComposition::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
	event->acceptProposedAction();
}

/**
 * Decide what to do with the moved drag-drop data.
 */
void VuoEditorComposition::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
	dragEnterEvent(event);
}

/**
 * Handle the dropped data.
 */
void VuoEditorComposition::dropEvent(QGraphicsSceneDragDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();

	// Accept drops of certain types of files.
	if (mimeData->hasFormat("text/uri-list"))
	{
		// Retrieve the composition directory so that file paths may be specified relative to it.
		// Note: Providing the directory's canonical path as the argument to the
		// QDir constructor is necessary in order for QDir::relativeFilePath() to
		// work correctly when the non-canonical path contains symbolic links (e.g.,
		// '/tmp' -> '/private/tmp' for example compositions).
		string topCompositionPath = compiler->getCompositionLocalPath();
		if (topCompositionPath.empty())
			topCompositionPath = getBase()->getDirectory();
		QDir compositionDir(QDir(topCompositionPath.c_str()).canonicalPath());

		// Use the absolute file path if the "Option" key was pressed.
		bool useAbsoluteFilePaths = VuoEditorUtilities::optionKeyPressedForEvent(event);

		QList<QGraphicsItem *> newNodes;
		QList<QUrl> urls = mimeData->urls();

		QGraphicsItem *nearbyItem = findNearbyComponent(event->scenePos());
		VuoRendererPort *portAtDropLocation = dynamic_cast<VuoRendererPort *>(nearbyItem);
		if (portAtDropLocation)
		{
			if ((urls.size() == 1) && portAtDropLocation->isConstant() &&
					(portAtDropLocation->getDataType()->getModuleKey() == "VuoText"))
			{
				QString filePath = (useAbsoluteFilePaths? urls[0].path() : compositionDir.relativeFilePath(urls[0].path()));
				string constantValue = "\"" + string(filePath.toUtf8().constData()) + "\"";
				event->accept();

				emit portConstantChangeRequested(portAtDropLocation, constantValue);
			}
			else
				event->ignore();
		}

		else // if (!portAtDropLocation)
		{
			const int ySpacingForNewNodes = 11;
			int yOffsetForPreviousNewNode = -1 * ySpacingForNewNodes;

			foreach (QUrl url, urls)
			{
				QStringList targetNodeClassNames;
				char *urlZ = strdup(url.path().toUtf8().constData());

				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Image))
					targetNodeClassNames += "vuo.image.fetch";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Movie))
				{
					if (!getActiveProtocol())
						targetNodeClassNames += "vuo.video.play";

					targetNodeClassNames += "vuo.video.decodeImage";
				}
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Scene))
					targetNodeClassNames += "vuo.scene.fetch";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Audio))
					targetNodeClassNames += "vuo.audio.file.play";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Mesh))
					targetNodeClassNames += "vuo.image.project.dome";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Feed))
					targetNodeClassNames += "vuo.rss.fetch";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_JSON))
					targetNodeClassNames += "vuo.tree.fetch.json";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_XML))
					targetNodeClassNames += "vuo.tree.fetch.xml";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Table))
					targetNodeClassNames += "vuo.table.fetch";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_Data))
					targetNodeClassNames += "vuo.data.fetch";
				if (VuoFileType_isFileOfType(urlZ, VuoFileType_App))
					targetNodeClassNames += "vuo.app.launch";
				if (isDirectory(urlZ))
					targetNodeClassNames += "vuo.file.list";

				free(urlZ);

				QString selectedNodeClassName = "";
				if (targetNodeClassNames.size() == 1)
					selectedNodeClassName = targetNodeClassNames[0];
				else if (targetNodeClassNames.size() > 1)
				{
					QMenu nodeMenu(views()[0]->viewport());
					nodeMenu.setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

					foreach (QString nodeClassName, targetNodeClassNames)
					{
						VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toUtf8().constData());
						string nodeTitle = (nodeClass? nodeClass->getBase()->getDefaultTitle() : nodeClassName.toUtf8().constData());
						//: Appears in a popup menu when dragging files from Finder onto the composition canvas.
						QAction *nodeAction = nodeMenu.addAction(tr("Insert \"%1\" Node").arg(nodeTitle.c_str()));
						nodeAction->setData(nodeClassName);
					}

					menuSelectionInProgress = true;
					QAction *selectedNode = nodeMenu.exec(QCursor::pos());
					menuSelectionInProgress = false;

					selectedNodeClassName = (selectedNode? selectedNode->data().toString().toUtf8().constData() : "");
				}

				if (!selectedNodeClassName.isEmpty())
				{
					VuoRendererNode *newNode = createNode(selectedNodeClassName, "",
														  event->scenePos().x(),
														  event->scenePos().y() + yOffsetForPreviousNewNode + ySpacingForNewNodes);

					if (newNode)
					{
						VuoPort *urlPort = newNode->getBase()->getInputPortWithName(selectedNodeClassName == "vuo.file.list"?
																						"folder" :
																						"url");
						if (urlPort)
						{
							QString filePath = (useAbsoluteFilePaths? url.path() : compositionDir.relativeFilePath(url.path()));
							urlPort->getRenderer()->setConstant(VuoText_getString(filePath.toUtf8().constData()));

							newNodes.append(newNode);

							yOffsetForPreviousNewNode += newNode->boundingRect().height();
							yOffsetForPreviousNewNode += ySpacingForNewNodes;
						}
					}
				}
			}

			if (newNodes.size() > 0)
			{
				event->accept();

				emit componentsAdded(newNodes, this);
			}
			else
				event->ignore();
		}
	}

	// Accept drops of one or more nodes from the node library class list.
	else if (mimeData->hasFormat("text/scsv"))
	{
		event->setDropAction(Qt::CopyAction);
		event->accept();

		QByteArray scsvData = event->mimeData()->data("text/scsv");
		QString scsvText = QString::fromUtf8(scsvData);
		QStringList nodeClassNames = scsvText.split(';');

		QPointF startPos = event->scenePos()-QPointF(0,VuoRendererNode::nodeHeaderYOffset);
		int nextYPos = VuoRendererComposition::quantizeToNearestGridLine(startPos,
																		 VuoRendererComposition::minorGridLineSpacing).y();
		int snapDelta = nextYPos - startPos.y();

		QList<QGraphicsItem *> newNodes;
		for (QStringList::iterator i = nodeClassNames.begin(); i != nodeClassNames.end(); ++i)
		{
			VuoRendererNode *newNode = createNode(*i, "",
						startPos.x(),
						nextYPos - (VuoRendererItem::getSnapToGrid()? 0 : snapDelta));

			if (newNode)
			{
				int prevYPos = nextYPos;
				nextYPos = VuoRendererComposition::quantizeToNearestGridLine(QPointF(0,prevYPos+newNode->boundingRect().height()),
																			 VuoRendererComposition::minorGridLineSpacing).y();
				if (nextYPos <= prevYPos+newNode->boundingRect().height())
					nextYPos += VuoRendererComposition::minorGridLineSpacing;

				newNodes.append((QGraphicsItem *)newNode);
			}
		}

		emit componentsAdded(newNodes, this);
	}

	// Accept drops of single nodes from the node library documentation panel.
	else if (mimeData->hasFormat("text/plain"))
	{
		event->setDropAction(Qt::CopyAction);
		event->accept();

		QList<QGraphicsItem *> newNodes;
		QStringList dropItems = event->mimeData()->text().split('\n');
		QString nodeClassName = dropItems[0];

		// Account for the offset between cursor and dragged item pixmaps
		// to drop node in-place.
		QPoint hotSpot = (dropItems.size() >= 3? QPoint(dropItems[1].toInt(), dropItems[2].toInt()) : QPoint(0,0));
		VuoRendererNode *newNode = createNode(nodeClassName, "",
						event->scenePos().x()-hotSpot.x()+1,
						event->scenePos().y()-VuoRendererNode::nodeHeaderYOffset-hotSpot.y()+1);

		newNodes.append(newNode);
		emit componentsAdded(newNodes, this);
	}
	else
	{
		event->ignore();
	}
}

/**
 * Handle mouse double-click events.
 */
void VuoEditorComposition::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem *nearbyItem = findNearbyComponent(event->scenePos());
	if (dynamic_cast<VuoRendererPort *>(nearbyItem))
	{
		QGraphicsScene::sendEvent(nearbyItem, event);
		event->accept();
	}

	else
		QGraphicsScene::mouseDoubleClickEvent(event);
}

/**
 * Handle mouse press events.
 */
void VuoEditorComposition::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF scenePos = event->scenePos();

	// Handle left-button presses.
	if (event->button() == Qt::LeftButton)
	{
		// Correct for erroneous tracking behavior that occurs following the first left-click
		// directly on a node, cable, or comment after the cancellation of a component duplication operation.
		// See https://b33p.net/kosada/node/3339
		if (duplicationCancelled)
		{
			QGraphicsItem *itemClickedDirectly = itemAt(scenePos, views()[0]->transform());
			if (dynamic_cast<VuoRendererNode *>(itemClickedDirectly) ||
					dynamic_cast<VuoRendererCable *>(itemClickedDirectly) ||
					dynamic_cast<VuoRendererComment *>(itemClickedDirectly)
				)
			{
				duplicationCancelled = false;
				correctForCancelledDuplication(event);
			}
		}

		// Determine whether the cursor is in range of any operable composition components.
		QGraphicsItem *nearbyItem = findNearbyComponent(scenePos);
		leftMousePressEventAtNearbyItem(nearbyItem, event);
	}

	// Handle non-left-button presses.
	else // if (event->button() != Qt::LeftButton)
		mousePressEventNonLeftButton(event);
}

/**
 * Handle a left mouse press event already determined to be within operable range
 * of the provided @c nearbyItem.
 */
void VuoEditorComposition::leftMousePressEventAtNearbyItem(QGraphicsItem *nearbyItem, QGraphicsSceneMouseEvent *event)
{
	bool eventHandled = false;

	// If click did not occur within range of a port, check whether
	// the click occured on a cable within its yank zone.
	VuoRendererCable *cableYankedDirectly = NULL;
	VuoRendererPort *currentPort = dynamic_cast<VuoRendererPort *>(nearbyItem);
	if (! currentPort)
	{
		VuoRendererCable *currentCable = dynamic_cast<VuoRendererCable *>(nearbyItem);
		if (currentCable &&
				currentCable->yankZoneIncludes(event->scenePos()) &&
				currentCable->getBase()->getToPort())
		{
			currentPort = currentCable->getBase()->getToPort()->getRenderer();
			cableYankedDirectly = currentCable;
		}
	}

	if (currentPort)
	{
		// Case: Firing an event by Command+click
		bool isTriggerPort = (currentPort->getBase()->hasCompiler() && dynamic_cast<VuoCompilerTriggerPort *>(currentPort->getBase()->getCompiler()));
		if ((event->modifiers() & Qt::ControlModifier) && (currentPort->getInput() || isTriggerPort))
			fireTriggerPortEvent(currentPort->getBase());

		else
		{
			portWithDragInitiated = currentPort;
			cableWithYankInitiated = cableYankedDirectly;
			event->accept();
		}

		eventHandled = true;
	}

	// Case: Initiating duplication of selected components with Option/Alt+drag
	else if (nearbyItem && VuoEditorUtilities::optionKeyPressedForEvent(event) && !duplicationDragInProgress)
	{
		// Duplicate the selected components.
		duplicateOnNextMouseMove = true;
		duplicationDragInProgress = true;
		cursorPosBeforeDuplicationDragMove = (VuoRendererItem::getSnapToGrid()?
												  VuoRendererComposition::quantizeToNearestGridLine(event->scenePos(),
																									VuoRendererComposition::minorGridLineSpacing) :
												  event->scenePos());

		QGraphicsScene::mousePressEvent(event);
		eventHandled = true;
	}

	// Case: Left mouse-click made near enough to a cable to trigger cable selection
	if (dynamic_cast<VuoRendererCable *>(nearbyItem))
	{
		if (event->modifiers() & Qt::ControlModifier)
			nearbyItem->setSelected(! nearbyItem->isSelected());

		else
		{
			deselectAllCompositionComponents();
			nearbyItem->setSelected(true);
		}

		event->accept();
		eventHandled = true;
	}

	// Case: Left mouse-click made for some other reason, not handled here
	if (!eventHandled)
		QGraphicsScene::mousePressEvent(event);
}

/**
 * Handle mouse press events involving any button but the left one.
 * Helper function for VuoEditorComposition::mousePressEvent(QGraphicsSceneMouseEvent *event).
 */
void VuoEditorComposition::mousePressEventNonLeftButton(QGraphicsSceneMouseEvent *event)
{
	cancelCableDrag();

	// If a right-click occurred, generate the context menu event ourselves
	// rather than leaving it to QGraphicsScene.  This prevents existing selected
	// components from being erroneously de-selected before
	// VuoEditorComposition::contextMenuEvent(...) is called if the right-click
	// occurred within range of, but not directly upon, a composition component.
	if (event->button() == Qt::RightButton)
	{
		QGraphicsSceneContextMenuEvent contextMenuEvent(QEvent::GraphicsSceneContextMenu);
		contextMenuEvent.setScreenPos(event->screenPos());
		contextMenuEvent.setScenePos(event->scenePos());
		contextMenuEvent.setReason(QGraphicsSceneContextMenuEvent::Mouse);
		QApplication::sendEvent(this, &contextMenuEvent);
		event->accept();
	}

	else
		QGraphicsScene::mousePressEvent(event);

	return;
}

/**
 * Correct for erroneous mouse tracking behavior that occurs following the first left-click
 * on a node or cable following the cancellation of a component duplication operation.
 * See https://b33p.net/kosada/node/3339
 * Helper function for VuoEditorComposition::mousePressEvent(QGraphicsSceneMouseEvent *event).
 */
void VuoEditorComposition::correctForCancelledDuplication(QGraphicsSceneMouseEvent *event)
{
	// Simulate an extra mouse click for now to force the cursor
	// to track correctly with the next set of dragged components.
	QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
	pressEvent.setScenePos(event->scenePos());
	pressEvent.setButton(Qt::LeftButton);
	QApplication::sendEvent(this, &pressEvent);

	QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
	releaseEvent.setScenePos(event->scenePos());
	releaseEvent.setButton(Qt::LeftButton);
	QApplication::sendEvent(this, &releaseEvent);
}

/**
 * Initiate a cable drag from port @c currentPort in response to a mouse press @c event.
 * This may involve the creation of a new cable from the port, or the disconnection of
 * an existing cable (@c cableYankedDirectly if non-NULL, or the cable most recently connected
 * to the port otherwise).
 * Helper function for VuoEditorComposition::mousePressEvent(QGraphicsSceneMouseEvent *event).
 */
void VuoEditorComposition::initiateCableDrag(VuoRendererPort *currentPort, VuoRendererCable *cableYankedDirectly, QGraphicsSceneMouseEvent *event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();
	bool optionKeyPressed = (modifiers & Qt::AltModifier);
	bool shiftKeyPressed = (modifiers & Qt::ShiftModifier);

	// For now, a left mouse press on an input port with a constant value, attached typecast, or attached "Make List" node does nothing.
	// Eventually, a mouse drag will detach the constant value, typecast, or "Make List" node.
	if (! (cableYankedDirectly || currentPort->getOutput() || currentPort->supportsDisconnectionByDragging()))
	{
		return;
	}

	// Determine based on the keypress modifiers and the attributes of the port whether to
	// create a new cable, disconnect an existing cable, or duplicate an existing cable.
	bool creatingNewCable = false;
	bool disconnectingExistingCable = false;
	bool duplicatingExistingCable = false;

	VuoPort *fixedPort = currentPort->getBase();
	VuoNode *fixedNode = getUnderlyingParentNodeForPort(fixedPort, this);

	VuoNode *fromNode = NULL;
	VuoPort *fromPort = NULL;
	VuoNode *toNode = NULL;
	VuoPort *toPort = NULL;

	// Case: Dragging from an output port
	if (currentPort->getOutput())
	{
		// Prepare for the "forward" creation of a new cable.
		fromPort = fixedPort;
		fromNode = fixedNode;
		creatingNewCable = true;
	}

	// Case: Dragging from an input port
	else if (! currentPort->getFunctionPort())
	{
		// If the input port has no connected cables to disconnect, prepare for the
		// "backward" creation of a new cable.
		if (currentPort->getBase()->getConnectedCables(true).empty())
		{
			toPort = fixedPort;
			toNode = fixedNode;
			creatingNewCable = true;
		}

		// If the input port does have connected cables, prepare for the
		// disconnection or duplication of one of these cables.
		else
		{
			if (optionKeyPressed)
			{
				duplicatingExistingCable = true;
			}

			else
				disconnectingExistingCable = true;
		}
	}

	// @todo: Case: Dragging from a function port


	// Perform the actual cable creation, if applicable.
	if (creatingNewCable)
	{
		// Create the cable first and set its endpoints later in case either endpoint is published,
		// since published nodes don't have compilers.
		cableInProgress = (new VuoCompilerCable(NULL,
												NULL,
												NULL,
												NULL))->getBase();

		// If the 'Option' key was pressed at the time of the mouse click, make the cable event-only
		// regardless of its connected ports.
		cableInProgress->getCompiler()->setAlwaysEventOnly(shiftKeyPressed);

		cableInProgressWasNew = true;
		cableInProgressShouldBeWireless = false;

		addCable(cableInProgress);
		cableInProgress->getRenderer()->setFrom(fromNode, fromPort);
		cableInProgress->getRenderer()->setTo(toNode, toPort);
		cableInProgress->getRenderer()->setFloatingEndpointLoc(event->scenePos());
		cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(false);
		highlightEligibleEndpointsForCable(cableInProgress);
		fixedPort->getRenderer()->updateGeometry();
	}

	// Perform the actual cable disconnection, if applicable.
	else if (disconnectingExistingCable)
	{
		// If a cable was yanked by clicking on it directly (rather than on the port), disconnect that cable.
		if (cableYankedDirectly)
			cableInProgress = cableYankedDirectly->getBase();

		// Otherwise, disconnect the cable that was connected to the port most recently.
		else
			cableInProgress = currentPort->getBase()->getConnectedCables(true).back();

		cableInProgressWasNew = false;
		cableInProgressShouldBeWireless = cableInProgress->hasCompiler() && cableInProgress->getCompiler()->getHidden();

		currentPort->updateGeometry();
		cableInProgress->getRenderer()->updateGeometry();
		cableInProgress->getRenderer()->setHovered(false);
		cableInProgress->getRenderer()->setFloatingEndpointLoc(event->scenePos());
		cableInProgress->getRenderer()->setFloatingEndpointPreviousToPort(cableInProgress->getToPort());
		cableInProgress->getRenderer()->setPreviouslyAlwaysEventOnly(cableInProgress->getCompiler()->getAlwaysEventOnly());
		cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(false);
		cableInProgress->getRenderer()->setTo(NULL, NULL);
		highlightEligibleEndpointsForCable(cableInProgress);
		deselectAllCompositionComponents();
		cableInProgress->getRenderer()->setSelected(true);
	}
	// Perform the actual cable duplication, if applicable.
	else if (duplicatingExistingCable)
	{
		VuoCable *cableToDuplicate = NULL;
		// If a cable was yanked by clicking on it directly (rather than on the port), disconnect that cable.
		if (cableYankedDirectly)
			cableToDuplicate = cableYankedDirectly->getBase();

		// Otherwise, disconnect the cable that was connected to the port most recently.
		else
			cableToDuplicate = currentPort->getBase()->getConnectedCables(true).back();

		// Create the cable first and set its endpoints later in case either endpoint is published,
		// since published nodes don't have compilers.
		cableInProgress = (new VuoCompilerCable(NULL,
												NULL,
												NULL,
												NULL))->getBase();

		// If the 'Option' key was pressed at the time of the mouse click, make the cable event-only
		// regardless of its connected ports.
		cableInProgress->getCompiler()->setAlwaysEventOnly(shiftKeyPressed);

		cableInProgressWasNew = true;
		cableInProgressShouldBeWireless = cableToDuplicate->hasCompiler() &&
													  cableToDuplicate->getCompiler()->getHidden();
		addCable(cableInProgress);
		cableInProgress->getRenderer()->setFrom(getUnderlyingParentNodeForPort(cableToDuplicate->getFromPort(), this),
												cableToDuplicate->getFromPort());
		cableInProgress->getRenderer()->setTo(NULL, NULL);
		cableInProgress->getRenderer()->setFloatingEndpointLoc(event->scenePos());
		cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(false);
		highlightEligibleEndpointsForCable(cableInProgress);
		deselectAllCompositionComponents();
		cableInProgress->getRenderer()->setSelected(true);
		fixedPort->getRenderer()->updateGeometry();
	}

	// The cable will need to be re-painted as its endpoint is dragged.
	cableInProgress->getRenderer()->setCacheMode(QGraphicsItem::NoCache);

	emit cableDragInitiated();

	event->accept();
}

/**
 * Handle mouse release events.
 */
void VuoEditorComposition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	// Handle left-button releases.
	if (event->button() == Qt::LeftButton)
	{
		portWithDragInitiated = NULL;
		cableWithYankInitiated = NULL;
		duplicateOnNextMouseMove = false;
		duplicationDragInProgress = false;
		dragStickinessDisabled = false;
		emit leftMouseButtonReleased();
		clearCableEndpointEligibilityHighlighting();

		// If there was a cable drag in progress at the time of the left-mouse-button
		// release, conclude the drag -- either by connecting the cable to the eligible port
		// at which it was dropped, or, if not dropped at an eligible port, by deleting the cable.
		bool cableDragEnding = cableInProgress;
		if (cableDragEnding)
			concludeCableDrag(event);

		QGraphicsItem *item = findNearbyComponent(event->scenePos());
		VuoRendererPort *port = dynamic_cast<VuoRendererPort *>(item);
		VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(item);
		if (port)
		{
			// Display the port popover, as long as the mouse release did not have any keyboard modifiers
			// and did not mark the end of a drag.
			// Do so even if a cable drag was technically in progress, because all it takes to initiate
			// a cable drag is a mouse press. We could trigger cable drags upon mouse move events instead of
			// mouse press events to avoid this.
			if ((event->modifiers() == Qt::NoModifier) &&
					(QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()))
			{
				VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(port);
				if (typecastPort)
				{
					// Since the rendering of the typecast body includes its host port, display popovers for both.
					VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
					enablePopoverForNode(typecastNode);
					enableInactivePopoverForPort(typecastPort->getReplacedPort());
				}

				else
					enableInactivePopoverForPort(port);
			}
		}

		else // if (!port)
		{
			if (!cableDragEnding && !node)
				emit emptyCanvasLocationLeftClicked();
		}

		if (!cableDragEnding)
			QGraphicsScene::mouseReleaseEvent(event);
	}

	// Handle non-left-button releases.
	else // if (event->button() != Qt::LeftButton)
	{
		QGraphicsScene::mouseReleaseEvent(event);
	}
}

/**
 * Conclude a cable drag in response to a mouse release @c event.
 * This may mean completing the cable connection, if the mouse release occurred within
 * range of an eligible port, or deleting the cable otherwise.
 * Helper function for VuoEditorComposition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event).
 */
void VuoEditorComposition::concludeCableDrag(QGraphicsSceneMouseEvent *event)
{
	cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(false);

	// Conclude drags of published cables as a special case.
	// @todo: Integrate this more naturally.
	if (cableInProgress->isPublished())
	{
		concludePublishedCableDrag(event);
		return;
	}

	if (hasFeedbackErrors())
	{
		cancelCableDrag();
		updateFeedbackErrors();
		return;
	}

	cableInProgress->getRenderer()->setWireless(cableInProgressShouldBeWireless);

	// Input or output port that the cable is being dragged from (i.e., the fixed endpoint).
	VuoRendererPort *fixedPort = NULL;
	if (cableInProgress->getFromNode() && cableInProgress->getFromPort())
		fixedPort = cableInProgress->getFromPort()->getRenderer();
	else if (cableInProgress->getToNode() && cableInProgress->getToPort())
		fixedPort = cableInProgress->getToPort()->getRenderer();

	// We will determine based on the presence or absence of an eligible port near the
	// location of the mouse release whether to connect or delete the dragged cable.
	bool completedCableConnection = false;

	// Potential side effects of a newly completed cable connection:
	VuoCable *dataCableToDisplace = NULL; // A previously existing incoming data+event cable may need to be displaced.
	VuoCable *cableToReplace = NULL; // A previously existing cable connecting the same two ports may need to be replaced.
	VuoRendererNode *typecastNodeToDelete = NULL; // A previously attached typecast may need to be deleted.
	VuoRendererPort *portToUnpublish = NULL; // A previously published port may need to be unpublished.
	string typecastToInsert = ""; // A typecast may need to be automatically inserted.

	// A generic port involved in the new connection may need to be specialized.
	VuoRendererPort *portToSpecialize = NULL;
	string specializedTypeName = "";

	// Input or output port that the cable is being dropped onto, if any.
	VuoRendererPort *targetPort = (VuoRendererPort *)findNearbyPort(event->scenePos(), false);

	// Node header area that the cable is being dropped onto, if any.
	// (If over both a port drop zone and a node header, the node header gets precedence.)
	{
		VuoRendererNode *targetNode = findNearbyNodeHeader(event->scenePos());
		if (targetNode)
		{
			targetPort = findTargetPortForCableDroppedOnNodeHeader(targetNode);
			if (targetPort)
				cableInProgress->getCompiler()->setAlwaysEventOnly(true);
		}
	}

	bool draggingPreviouslyPublishedCable = (!cableInProgressWasNew &&
											 (dynamic_cast<VuoRendererPublishedPort *>(fixedPort) ||
											 ((cableInProgress->getToPort() == NULL) &&
											  dynamic_cast<VuoRendererPublishedPort *>(cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort()->getRenderer()))));

	if (fixedPort && targetPort)
	{
		// If the user has simply disconnected a cable and reconnected it within the same mouse drag,
		// don't push the operation onto the Undo stack.
		bool recreatingSameConnection = ((cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort() ==
									 targetPort->getBase()) &&
									(cableInProgress->getRenderer()->getPreviouslyAlwaysEventOnly() ==
									 cableInProgress->getCompiler()->getAlwaysEventOnly()));
		if (recreatingSameConnection)
		{
			revertCableDrag();
			return;
		}

		bool cableInProgressExpectedToCarryData = cableInProgress->getRenderer()->effectivelyCarriesData() &&
												  targetPort->getDataType();

		VuoCable *preexistingCable = fixedPort->getCableConnectedTo(targetPort, false);
		bool preexistingCableWithMatchingDataCarryingStatus = (preexistingCable?
																   (preexistingCable->getRenderer()->effectivelyCarriesData() ==
																	cableInProgressExpectedToCarryData) :
																   false);

		// Case: Replacing a pre-existing cable that connected the same two ports
		// but with a different data-carrying status, and whose "To" port is
		// the child port of a collapsed typecast.
		if (preexistingCable && !preexistingCableWithMatchingDataCarryingStatus &&
				preexistingCable->getToPort()->hasRenderer() &&
				preexistingCable->getToPort()->getRenderer()->getTypecastParentPort())
		{
			// @todo Implement for https://b33p.net/kosada/node/14153
			// For now, don't attempt it.
			revertCableDrag();
			return;
		}

		// Case: Completing a "forward" cable connection from an output port to an input port
		if (!preexistingCableWithMatchingDataCarryingStatus &&
				(fixedPort->canConnectDirectlyWithoutSpecializationTo(targetPort, !cableInProgress->getRenderer()->effectivelyCarriesData()) ||
				 selectBridgingSolution(fixedPort, targetPort, true, &portToSpecialize, specializedTypeName, typecastToInsert)))
		{
			// If input port had a connected collapsed typecast, uncollapse it.
			VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(targetPort);
			if (typecastPort)
			{
				VuoRendererPort *adjustedTargetPort = typecastPort->getReplacedPort();
				VuoRendererPort *childPort = typecastPort->getChildPort();
				VuoRendererNode *uncollapsedTypecast = uncollapseTypecastNode(typecastPort);
				VuoPort *typecastOutPort = uncollapsedTypecast->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

				if (portToSpecialize == targetPort)
					portToSpecialize = adjustedTargetPort;

				targetPort = adjustedTargetPort;

				// If the typecast did not have multiple incoming cables, and the new cable will
				// be replacing it as a data source, delete the typecast.
				if (cableInProgressExpectedToCarryData &&
						childPort->getBase()->getConnectedCables(true).size() < 2 &&
						(*typecastOutPort->getConnectedCables(true).begin())->getRenderer()->effectivelyCarriesData())
					typecastNodeToDelete = uncollapsedTypecast;
			}

			// If connecting two ports that already had a cable (of different data-carrying status) connecting them,
			// replace the pre-existing cable.
			if (preexistingCable)
				cableToReplace = preexistingCable;

			// If the cable carries data, determine what other sources of input data need to be
			// removed before completing this connection.
			if (cableInProgressExpectedToCarryData)
			{
				// If input port already had a connected data cable, delete that cable.
				vector<VuoCable *> previousConnectedCables = targetPort->getBase()->getConnectedCables(false);
				for (vector<VuoCable *>::iterator cable = previousConnectedCables.begin(); (! dataCableToDisplace) && (cable != previousConnectedCables.end()); ++cable)
					if ((((*cable)->getRenderer()->effectivelyCarriesData()) && (*cable) != cableToReplace))
						dataCableToDisplace = *cable;

				// If the input port was published as a data+event port, unpublish it.
				// @todo: Let published cable disconnection handle port unpublication.
				if (isPortPublished(targetPort))
				{
					vector<VuoRendererPublishedPort *> publishedDataConnections = targetPort->getPublishedPortsConnectedByDataCarryingCables();
					if (publishedDataConnections.size() > 0)
						portToUnpublish = targetPort;
				}
			}

			completedCableConnection = true;
		}

		// Case: Completing a "backward" cable connection from an input port to an output port
		else if (!preexistingCableWithMatchingDataCarryingStatus &&
				 (targetPort->canConnectDirectlyWithoutSpecializationTo(fixedPort, !cableInProgress->getRenderer()->effectivelyCarriesData()) ||
				  selectBridgingSolution(targetPort, fixedPort, false, &portToSpecialize, specializedTypeName, typecastToInsert)))

		{
			completedCableConnection = true;
		}
	}

	// Complete the actual cable connection, if applicable.
	if (completedCableConnection)
	{
		emit undoStackMacroBeginRequested("Cable Connection");

		if (draggingPreviouslyPublishedCable)
		{
			// Record some information about the cable in progress, to create a replica.
			VuoNode *cableInProgressFromNode = cableInProgress->getFromNode();
			VuoNode *cableInProgressToNode = cableInProgress->getToNode();
			VuoPort *cableInProgressFromPort = cableInProgress->getFromPort();
			VuoPort *cableInProgressToPort = cableInProgress->getToPort();
			QPointF cableInProgressFloatingEndpointLoc = cableInProgress->getRenderer()->getFloatingEndpointLoc();
			bool cableInProgressAlwaysEventOnly = cableInProgress->getCompiler()->getAlwaysEventOnly();

			// Remove the original cable, unpublishing the port in the process.
			cancelCableDrag();

			// Restore a copy of the cable to participate in the new connection.
			VuoCable *cableInProgressCopy = (new VuoCompilerCable(NULL,
													NULL,
													NULL,
													NULL))->getBase();

			cableInProgressCopy->setFrom(cableInProgressFromNode, cableInProgressFromPort);
			cableInProgressCopy->setTo(cableInProgressToNode, cableInProgressToPort);

			addCable(cableInProgressCopy);

			cableInProgressCopy->getCompiler()->setAlwaysEventOnly(cableInProgressAlwaysEventOnly);
			cableInProgressCopy->getRenderer()->setFloatingEndpointLoc(cableInProgressFloatingEndpointLoc);

			cableInProgress = cableInProgressCopy;
			cableInProgressWasNew = true;
		}

		// Workaround to avoid Qt's parameter count limit for signals/slots.
		pair<VuoRendererCable *, VuoRendererCable *> cableArgs = std::make_pair((dataCableToDisplace? dataCableToDisplace->getRenderer() : NULL),
															(cableToReplace? cableToReplace->getRenderer() : NULL));
		pair<string, string> typeArgs = std::make_pair(typecastToInsert, specializedTypeName);
		pair<VuoRendererPort *, VuoRendererPort *> portArgs = std::make_pair(portToUnpublish, portToSpecialize);

		emit connectionCompletedByDragging(cableInProgress->getRenderer(),
										   targetPort,
										   cableArgs,
										   typecastNodeToDelete,
										   typeArgs,
										   portArgs);

		emit undoStackMacroEndRequested();

		// Resume caching for dragged cable.
		if (cableInProgress)
		{
			cableInProgress->getRenderer()->updateGeometry();
			cableInProgress->getRenderer()->setCacheMode(getCurrentDefaultCacheMode());
			cableInProgress = NULL;
		}
	}

	// Otherwise, delete the cable.
	else // if (! completedCableConnection)
		cancelCableDrag();

	emit cableDragEnded();
}

/**
 * Conclude the drag of a published cable in response to a mouse release @c event.
 */
void VuoEditorComposition::concludePublishedCableDrag(QGraphicsSceneMouseEvent *event)
{
	VuoRendererPort *fixedPort;
	if (cableInProgress->getFromNode() && cableInProgress->getFromPort())
		fixedPort = cableInProgress->getFromPort()->getRenderer();
	else // if (cableInProgress->getToNode() && cableInProgress->getToPort())
		fixedPort = cableInProgress->getToPort()->getRenderer();

	// Potential side effects of a newly completed cable connection:
	// - A typecast may need to be automatically inserted.
	string typecastToInsert = "";

	// - A generic port involved in the new connection may need to be specialized.
	VuoRendererPort *portToSpecialize = NULL;
	string specializedTypeName = "";

	bool forceEventOnlyPublication = !cableInProgress->getRenderer()->effectivelyCarriesData();

	// Case: Initiating a cable drag from a published input port.
	if (cableInProgress && cableInProgress->isPublishedInputCable())
	{
		VuoRendererPort *internalInputPort = static_cast<VuoRendererPort *>(findNearbyPort(event->scenePos(), false));
		VuoRendererPublishedPort *publishedInputPort = dynamic_cast<VuoRendererPublishedPort *>(cableInProgress->getFromPort()->getRenderer());

		// If the cable was dropped onto a node header area, connect an event-only cable to the node's first input port.
		// (If over both a port drop zone and a node header, the node header gets precedence.)
		{
			VuoRendererNode *targetNode = findNearbyNodeHeader(event->scenePos());
			if (targetNode)
			{
				internalInputPort = findTargetPortForCableDroppedOnNodeHeader(targetNode);
				if (internalInputPort)
				{
					cableInProgress->getCompiler()->setAlwaysEventOnly(true);
					forceEventOnlyPublication = true;
				}
			}
		}

		// Case: Cable was dropped onto an internal input port
		if (internalInputPort &&
				publishedInputPort)
		{
			// If the user has simply disconnected a cable and reconnected it within the same mouse drag,
			// don't push the operation onto the Undo stack.
			bool recreatingSameConnection = ((cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort() ==
										 internalInputPort->getBase()) &&
										(cableInProgress->getRenderer()->getPreviouslyAlwaysEventOnly() ==
										 cableInProgress->getCompiler()->getAlwaysEventOnly()));
			if (recreatingSameConnection)
			{
				revertCableDrag();
				return;
			}

			// Case: Ports are compatible
			if ((publishedInputPort->isCompatibleAliasWithSpecializationForInternalPort(internalInputPort, forceEventOnlyPublication, &portToSpecialize, specializedTypeName))
					|| selectBridgingSolution(publishedInputPort, internalInputPort, true, &portToSpecialize, specializedTypeName, typecastToInsert))
			{
				bool cableInProgressExpectedToCarryData = (cableInProgress->getRenderer()->effectivelyCarriesData() &&
														  internalInputPort->getDataType());
				VuoCable *cableToReplace = publishedInputPort->getCableConnectedTo(internalInputPort, true);
				bool cableToReplaceHasMatchingDataCarryingStatus = (cableToReplace?
																		(cableToReplace->getRenderer()->effectivelyCarriesData() ==
																		 cableInProgressExpectedToCarryData) :
																		false);

				// If replacing a preexisting cable with an identical cable, just cancel the operation
				// so that it doesn't go onto the Undo stack.
				if (cableToReplace && cableToReplaceHasMatchingDataCarryingStatus)
					cancelCableDrag();

				// Case: Replacing a pre-existing cable that connected the same two ports
				// but with a different data-carrying status, and whose "To" port is
				// the child port of a collapsed typecast.
				else if (cableToReplace && !cableToReplaceHasMatchingDataCarryingStatus &&
						 cableToReplace->getToPort()->hasRenderer() &&
						 cableToReplace->getToPort()->getRenderer()->getTypecastParentPort())
				{
					// @todo Implement for https://b33p.net/kosada/node/14153
					// For now, don't attempt it.
					cancelCableDrag();
				}

				else
				{
					emit undoStackMacroBeginRequested("Cable Connection");
					cancelCableDrag();

					// If this source/target port combination already a cable connecting them, but of a different
					// data-carrying status, replace the old cable with the new one.
					if (cableToReplace && !cableToReplaceHasMatchingDataCarryingStatus)
					{
						QList<QGraphicsItem *> removedComponents;
						removedComponents.append(cableToReplace->getRenderer());
						emit componentsRemoved(removedComponents, "Delete");
					}

					// If the target port had a connected collapsed typecast, uncollapse and delete it.
					// But first check whether the collapsed typecast is still present on canvas -- it's possible it was
					// already removed during the cancelCableDrag() call, if the cable being dragged was the
					// typecast's incoming data+event cable.
					VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(internalInputPort);
					if (typecastPort && typecastPort->scene())
					{
						VuoRendererPort *childPort = typecastPort->getChildPort();
						VuoRendererNode *uncollapsedTypecast = uncollapseTypecastNode(typecastPort);
						VuoPort *typecastOutPort = uncollapsedTypecast->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

						// If the typecast did not have multiple incoming cables, and the new cable will
						// be replacing it as a data source, delete the typecast.
						if (cableInProgressExpectedToCarryData &&
								childPort->getBase()->getConnectedCables(true).size() < 2 &&
								(*typecastOutPort->getConnectedCables(true).begin())->getRenderer()->effectivelyCarriesData())
						{
							QList<QGraphicsItem *> removedComponents;
							removedComponents.append(uncollapsedTypecast);
							emit componentsRemoved(removedComponents, "Delete");
						}
					}

					emit portPublicationRequested(internalInputPort->getBase(),
												  dynamic_cast<VuoPublishedPort *>(publishedInputPort->getBase()),
												  forceEventOnlyPublication,
												  (portToSpecialize? portToSpecialize->getBase() : NULL),
												  specializedTypeName,
												  typecastToInsert,
												  false); // Avoid nested Undo stack macros.
					emit undoStackMacroEndRequested();
				}
			}
			else // Source port and target port aren't compatible
				cancelCableDrag();
		}
		else // Cable was dropped over empty canvas space
			cancelCableDrag();
	}

	// Case: Initiating a cable drag from a published output port.
	else if (cableInProgress && cableInProgress->isPublishedOutputCable())
	{
		VuoRendererPort *internalOutputPort = static_cast<VuoRendererPort *>(findNearbyPort(event->scenePos(), false));
		VuoRendererPublishedPort *publishedOutputPort = dynamic_cast<VuoRendererPublishedPort *>(cableInProgress->getToPort()->getRenderer());

		// If the cable was dropped onto a node header area, connect an event-only cable to the node's first output port.
		// (If over both a port drop zone and a node header, the node header gets precedence.)
		{
			VuoRendererNode *targetNode = findNearbyNodeHeader(event->scenePos());
			if (targetNode)
			{
				internalOutputPort = findTargetPortForCableDroppedOnNodeHeader(targetNode);
				if (internalOutputPort)
				{
					cableInProgress->getCompiler()->setAlwaysEventOnly(true);
					forceEventOnlyPublication = true;
				}
			}
		}

		// Case: Cable was dropped onto an internal output port
		if (internalOutputPort &&
				publishedOutputPort)
		{

			// Case: Ports are compatible
			if ((publishedOutputPort->isCompatibleAliasWithSpecializationForInternalPort(internalOutputPort, forceEventOnlyPublication, &portToSpecialize, specializedTypeName) &&
				  publishedOutputPort->canAccommodateInternalPort(internalOutputPort, forceEventOnlyPublication))
					|| selectBridgingSolution(internalOutputPort, publishedOutputPort, false, &portToSpecialize, specializedTypeName, typecastToInsert))
			{
				emit portPublicationRequested(internalOutputPort->getBase(),
											  dynamic_cast<VuoPublishedPort *>(publishedOutputPort->getBase()),
											  forceEventOnlyPublication,
											  portToSpecialize? portToSpecialize->getBase() : NULL,
											  specializedTypeName,
											  typecastToInsert,
											  true);
			}
		}

		cancelCableDrag();
	}

	emit cableDragEnded();
}

/**
 * Cancel the current cable drag operation, if applicable.
 */
void VuoEditorComposition::cancelCableDrag()
{
	clearCableEndpointEligibilityHighlighting();

	if (! cableInProgress)
		return;

	if (cableInProgressWasNew)
		removeCable(cableInProgress->getRenderer());
	else
	{
		QList<QGraphicsItem *> removedComponents;
		removedComponents.append(cableInProgress->getRenderer());
		emit componentsRemoved(removedComponents, "Delete");
	}

	cableInProgress = NULL;
}

/**
 * Revert the current cable drag operation, if applicable, without pushing
 * the operation onto the Undo stack. To be used when the user disconnects
 * and reconnects a cable (without changing its data-carrying status) to the
 * same port within a single mouse drag, or cancels an in-progress cable drag.
 */
void VuoEditorComposition::revertCableDrag()
{
	if (cableInProgress && cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort())
	{
		cableInProgress->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
		cableInProgress->getRenderer()->updateGeometry();
		cableInProgress->setTo(getUnderlyingParentNodeForPort(cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort(), this),
							   cableInProgress->getRenderer()->getFloatingEndpointPreviousToPort());
		cableInProgress->getRenderer()->setCacheMode(getCurrentDefaultCacheMode());
		cableInProgress = NULL;
	}
	else if (cableInProgress)
	{
		cancelCableDrag();
	}

	clearCableEndpointEligibilityHighlighting();
	updateFeedbackErrors();
	emit cableDragEnded();
}

/**
 * Handle mouse move events.
 */
void VuoEditorComposition::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	bool leftMouseButtonPressed = (event->buttons() & Qt::LeftButton);

	// Disable Mission Control workaround, since it sometimes misdetects whether the left mouse
	// button is pressed and overzealously cancels cable drags (specifically, those originating from
	// sidebar published output ports).  The workaround no longer appears to be effective as of Qt 5.2.1 anyway.
	/*
	// In the unlikely situation that we have a cable drag in progress but the left mouse
	// button is not pressed (e.g., if "Mission Control" was activated during a cable drag and
	// deactivated by releasing the mouse button), cancel the cable drag.
	// See https://b33p.net/kosada/node/3305
	if (cableInProgress && !menuSelectionInProgress && !leftMouseButtonPressed)
		cancelCableDrag();
	*/

	// If in the process of a cable drag or mouse-over operation,
	// locate the nearest port or cable eligible for hover highlighting.
	if (cableInProgress || (! leftMouseButtonPressed))
		updateHoverHighlighting(event->scenePos());


	// Case: Left mouse button pressed
	if (leftMouseButtonPressed)
	{
		// Eliminate mouse jitter noise.
		if ((! dragStickinessDisabled) && (QLineF(event->screenPos(), event->buttonDownScreenPos(Qt::LeftButton)).length() < QApplication::startDragDistance()))
			return;

		else
			dragStickinessDisabled = true;

		// If a port or cable has been clicked for dragging and this is the first mouse move event
		// since the click, initiate the cable drag.
		if (portWithDragInitiated || cableWithYankInitiated)
		{
			initiateCableDrag(portWithDragInitiated, cableWithYankInitiated, event);
			portWithDragInitiated = NULL;
			cableWithYankInitiated = NULL;
		}

		// If there is a cable drag in progress, update the location of the cable's
		// floating endpoint to match that of the cursor.
		if (cableInProgress)
		{
			VuoRendererCable *rc = cableInProgress->getRenderer();

			rc->updateGeometry();
			rc->setFloatingEndpointLoc(event->scenePos());

			repaintFeedbackErrorMarks();
		}

		else if (duplicationDragInProgress)
		{
			if (duplicateOnNextMouseMove)
			{
				emit selectedComponentsDuplicated();
				duplicateOnNextMouseMove = false;
			}

			QPointF newPos = (VuoRendererItem::getSnapToGrid()?
								  VuoRendererComposition::quantizeToNearestGridLine(event->scenePos(),
																					VuoRendererComposition::minorGridLineSpacing) :
								  event->scenePos());
			QPointF delta = newPos - cursorPosBeforeDuplicationDragMove;
			moveItemsBy(getSelectedNodes(), getSelectedComments(), delta.x(), delta.y(), false);
			cursorPosBeforeDuplicationDragMove = newPos;
		}

		else
			QGraphicsScene::mouseMoveEvent(event);
	}

	// Case: Left mouse button not pressed
	else
		QGraphicsScene::mouseMoveEvent(event);
}

/**
 * Begin and/or end hover-highlighting of appropriate nearby composition components,
 * given the cursor position associated with the the input mouse @c event.
 * Helper function for VuoEditorComposition::mouseMoveEvent(QGraphicsSceneMouseEvent *event).
 */
void VuoEditorComposition::updateHoverHighlighting(QPointF scenePos, bool disablePortHoverHighlighting)
{
	// Detect cable and port hover events ourselves, since we need to account
	// for their extended hover ranges.
	QGraphicsItem *item = cableInProgress? findNearbyPort(scenePos, false) : findNearbyComponent(scenePos);
	VuoRendererCable *cable = dynamic_cast<VuoRendererCable *>(item);
	VuoRendererPort *port = dynamic_cast<VuoRendererPort *>(item);
	VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(item);
	VuoRendererInputListDrawer *makeListDrawer = dynamic_cast<VuoRendererInputListDrawer *>(item);

	// If hovering over a node header area while dragging a cable, treat it like hovering over the first input or output port.
	// (If over both a port drop zone and a node header, the node header gets precedence.)
	bool hoveringOverNodeHeader = false;
	if (cableInProgress)
	{
		VuoRendererNode *targetNode = findNearbyNodeHeader(scenePos);
		if (targetNode)
		{
			port = findTargetPortForCableDroppedOnNodeHeader(targetNode);
			if (port)
			{
				item = targetNode;
				hoveringOverNodeHeader = true;
			}
		}
	}

	VuoRendererNode *node = NULL;
	if (! hoveringOverNodeHeader)
	{
		// Do not handle hover events for (non-drawer) nodes here.
		if (dynamic_cast<VuoRendererNode *>(item) && !makeListDrawer)
			item = NULL;

		node = dynamic_cast<VuoRendererNode *>(item);
	}

	// Case: The item (if any) that requires hover-highlighting given the current position of the cursor
	// is not the same as the item (if any) that was hover-highlighted previously.
	if (item != previousNearbyItem)
	{
		VuoRendererPort *previousPort = dynamic_cast<VuoRendererPort *>(previousNearbyItem);
		VuoRendererNode *previousNode = dynamic_cast<VuoRendererNode *>(previousNearbyItem);
		if (previousNode)
			previousPort = findTargetPortForCableDroppedOnNodeHeader(previousNode);

		// End hover-highlighting of the previous item, if any.
		if (previousNearbyItem)
			clearHoverHighlighting();

		// Begin hover-highlighting of the current item, if any.
		if (cable)
			cable->extendedHoverEnterEvent();
		else if (node)
		{
			if (! cableInProgress)
			{
				if (makeListDrawer)
					makeListDrawer->extendedHoverEnterEvent(scenePos);
			}
		}
		else if (typecastPort && !disablePortHoverHighlighting)
			port->extendedHoverEnterEvent((bool)cableInProgress);
		else if (port && !disablePortHoverHighlighting)
		{
			port->extendedHoverEnterEvent((bool)cableInProgress);
			if (!cableInProgress)
			{
				foreach (VuoRendererPort *connectedAntennaPort, port->getPortsConnectedWirelessly(false))
					connectedAntennaPort->extendedHoverEnterEvent((bool)cableInProgress, true);
			}
		}

		VuoRendererPort *previousTargetPort = (previousPort && previousPort->isEligibleForConnection() ? previousPort : NULL);

		// If the current item or previous item is a port and it got this status because the mouse hovered
		// over a node header, update the eligibility-highlighting of the port.
		if (cableInProgress)
		{
			VuoRendererPort *fixedPort = (cableInProgress->getFromPort() ?
											  cableInProgress->getFromPort()->getRenderer() :
											  cableInProgress->getToPort()->getRenderer());

			QList< QPair<VuoRendererPort *, bool> > updatedPorts;
			if (hoveringOverNodeHeader)
				updatedPorts.append( QPair<VuoRendererPort *, bool>(port, true) );
			if (previousNode && previousPort)
				updatedPorts.append( QPair<VuoRendererPort *, bool>(previousPort, !cableInProgress->getRenderer()->effectivelyCarriesData()) );

			QPair<VuoRendererPort *, bool> p;
			foreach (p, updatedPorts)
			{
				VuoRendererPort *updatedPort = p.first;

				VuoRendererPort *typecastParentPort = updatedPort->getTypecastParentPort();
				if (typecastParentPort)
					updatedPort = typecastParentPort;

				updateEligibilityHighlightingForPort(updatedPort, fixedPort, p.second);

				VuoRendererNode *potentialDrawer = updatedPort->getUnderlyingParentNode();
				updateEligibilityHighlightingForNode(potentialDrawer);
			}
		}

		VuoRendererPort *targetPort = (port && port->isEligibleForConnection() ? port : NULL);

		// Update error dialogs and cable's data-carrying status.
		if (targetPort || previousTargetPort)
		{
			if (cableInProgress)
				cableInProgress->getRenderer()->setFloatingEndpointAboveEventPort(targetPort && (!targetPort->getDataType() || hoveringOverNodeHeader));

			updateFeedbackErrors(targetPort);
		}

		previousNearbyItem = item;
	}

	// Case: The previously hover-highlighted item should remain hover-highlighted.
	else if (item)
	{
		if (cable)
			cable->extendedHoverMoveEvent();
		else if (port && !disablePortHoverHighlighting)
		{
			port->extendedHoverMoveEvent((bool)cableInProgress);
			if (!cableInProgress)
			{
				foreach (VuoRendererPort *connectedAntennaPort, port->getPortsConnectedWirelessly(false))
					connectedAntennaPort->extendedHoverMoveEvent((bool)cableInProgress, true);
			}
		}
		else if (makeListDrawer)
			makeListDrawer->extendedHoverMoveEvent(scenePos);
	}
}

/**
 * Clear hover-highlighting of the previously highlighted composition component.
 */
void VuoEditorComposition::clearHoverHighlighting()
{
	if (previousNearbyItem)
	{
		VuoRendererCable *cable = dynamic_cast<VuoRendererCable *>(previousNearbyItem);
		VuoRendererPort *port = dynamic_cast<VuoRendererPort *>(previousNearbyItem);
		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(previousNearbyItem);
		VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(previousNearbyItem);

		if (node)
		{
			VuoRendererInputListDrawer *drawer = dynamic_cast<VuoRendererInputListDrawer *>(previousNearbyItem);
			if (drawer)
				drawer->extendedHoverLeaveEvent();
			else
				port = findTargetPortForCableDroppedOnNodeHeader(node);
		}

		if (cable)
			cable->extendedHoverLeaveEvent();
		else if (typecastPort)
			port->extendedHoverLeaveEvent();
		else if (port)
		{
			port->extendedHoverLeaveEvent();
			foreach (VuoRendererPort *connectedAntennaPort, port->getPortsConnectedWirelessly(false))
				connectedAntennaPort->extendedHoverLeaveEvent();
		}

		previousNearbyItem = NULL;
	}
}

/**
 * Handle keypress events.
 */
void VuoEditorComposition::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() != Qt::Key_Alt) && (event->key() != Qt::Key_Shift) && (event->key() != Qt::Key_Escape))
		cancelCableDrag();

	Qt::KeyboardModifiers modifiers = event->modifiers();
	qreal adjustedNodeMoveRate = nodeMoveRate;
	if (modifiers & Qt::ShiftModifier)
	{
		adjustedNodeMoveRate *= nodeMoveRateMultiplier;
	}

	switch (event->key()) {
		case Qt::Key_Backspace:
		{
			deleteSelectedCompositionComponents();
			break;
		}
		case Qt::Key_Delete:
		{
			deleteSelectedCompositionComponents();
			break;
		}
		case Qt::Key_Up:
		{
			moveSelectedItemsBy(0, -1*adjustedNodeMoveRate);
			break;
		}
		case Qt::Key_Down:
		{
			if (modifiers & Qt::ControlModifier)
				openSelectedEditableNodes();
			else
				moveSelectedItemsBy(0, adjustedNodeMoveRate);
			break;
		}
		case Qt::Key_Left:
		{
			moveSelectedItemsBy(-1*adjustedNodeMoveRate, 0);
			break;
		}
		case Qt::Key_Right:
		{
			moveSelectedItemsBy(adjustedNodeMoveRate, 0);
			break;
		}
		case Qt::Key_Shift:
		{
			if (cableInProgress)
			{
				cableInProgress->getRenderer()->updateGeometry();
				cableInProgress->getCompiler()->setAlwaysEventOnly(true);
				highlightEligibleEndpointsForCable(cableInProgress);

				VuoRendererPort *hoveredPort = dynamic_cast<VuoRendererPort *>(previousNearbyItem);
				if (hoveredPort)
					updateFeedbackErrors(hoveredPort);
			}

			break;
		}
		case Qt::Key_Enter:
		case Qt::Key_Return:
		{
			// Make sure the event is forwarded to any composition component within hover range.
			QGraphicsScene::keyPressEvent(event);

			if (!event->isAccepted())
			{
				// Otherwise, if there are any (and only) nodes currently selected, open a title editor for each one;
				// if there are any (and only) comments currently selected, open a comment text editor for each one.
				set<VuoRendererNode *> selectedNodes = getSelectedNodes();
				set<VuoRendererComment *> selectedComments = getSelectedComments();

				if (selectedComments.empty() && !selectedNodes.empty())
					renameSelectedNodes();
			}

			break;
		}

		case Qt::Key_Escape:
		{
		   if (duplicateOnNextMouseMove || duplicationDragInProgress)
		   {
			  emit duplicationOperationCancelled();

			  duplicateOnNextMouseMove = false;
			  duplicationDragInProgress = false;
			  duplicationCancelled = true;
		   }
		   else if (cableInProgress)
		   {
			   revertCableDrag();
		   }
		   break;
		}

		default:
		{
			QGraphicsScene::keyPressEvent(event);
			break;
		}
	}
}

/**
 * Handle key release events.
 */
void VuoEditorComposition::keyReleaseEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_Shift:
		{
			if (cableInProgress)
			{
				cableInProgress->getRenderer()->updateGeometry();
				cableInProgress->getCompiler()->setAlwaysEventOnly(false);
				highlightEligibleEndpointsForCable(cableInProgress);

				VuoRendererPort *hoveredPort = dynamic_cast<VuoRendererPort *>(previousNearbyItem);
				if (hoveredPort)
					updateFeedbackErrors(hoveredPort);
			}

			break;
		}

		default:
		{
			QGraphicsScene::keyReleaseEvent(event);
			break;
		}
	}
}

/**
 * Display the context menu for the canvas or for a node, port, cable, or comment.
 */
void VuoEditorComposition::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
	// Determine whether the cursor is in range of any operable composition components.
	QGraphicsItem *item = findNearbyComponent(event->scenePos());

	QMenu contextMenu(VuoEditorWindow::getMostRecentActiveEditorWindow());
	contextMenu.setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

	// Customize context menu for the canvas.
	if (! item)
	{
		QAction *insertNodeSection = new QAction(tr("Insert Node"), NULL);
		insertNodeSection->setEnabled(false);
		contextMenu.addAction(insertNodeSection);

		QString spacer("    ");

		{
			// "Share" nodes
			QMenu *shareMenu = new QMenu(&contextMenu);
			shareMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			shareMenu->setTitle(spacer + tr("Share"));
			contextMenu.addMenu(shareMenu);

			{
				QAction *action = new QAction(tr("Share Value"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.data.share"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				shareMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Share List"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.data.share.list"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				shareMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Share Event"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.event.share"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				shareMenu->addAction(action);
			}
		}

		{
			// "Hold" nodes
			QMenu *holdMenu = new QMenu(&contextMenu);
			holdMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			holdMenu->setTitle(spacer + tr("Hold"));
			contextMenu.addMenu(holdMenu);

			{
				QAction *action = new QAction(tr("Hold Value"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.data.hold2"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				holdMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Hold List"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.data.hold.list2"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				holdMenu->addAction(action);
			}
		}

		{
			// "Allow" nodes
			QMenu *allowMenu = new QMenu(&contextMenu);
			allowMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			allowMenu->setTitle(spacer + tr("Allow"));
			contextMenu.addMenu(allowMenu);

			{
				QAction *action = new QAction(tr("Allow First Event"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.event.allowFirst"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				allowMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Allow First Value"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.event.allowFirstValue"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				allowMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Allow Periodic Events"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.time.allowPeriodic"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				allowMenu->addAction(action);
			}

			{
				QAction *action = new QAction(tr("Allow Changes"), NULL);
				action->setData(QVariant::fromValue(qMakePair(event->scenePos(), QStringLiteral("vuo.event.allowChanges2"))));
				connect(action, &QAction::triggered, this, &VuoEditorComposition::insertNode);
				allowMenu->addAction(action);
			}
		}

		contextMenu.addSeparator();

		QAction *contextMenuInsertComment = new QAction(NULL);
		contextMenuInsertComment->setText(tr("Insert Comment"));
		contextMenuInsertComment->setData(event->scenePos());
		connect(contextMenuInsertComment, &QAction::triggered, this, &VuoEditorComposition::insertComment);
		contextMenu.addAction(contextMenuInsertComment);

		QAction *contextMenuInsertSubcomposition = new QAction(NULL);
		contextMenuInsertSubcomposition->setText(tr("Insert Subcomposition"));
		contextMenuInsertSubcomposition->setData(event->scenePos());
		connect(contextMenuInsertSubcomposition, &QAction::triggered, this, &VuoEditorComposition::insertSubcomposition);
		contextMenu.addAction(contextMenuInsertSubcomposition);
	}

	// Prepare to take a snapshot of the contextMenuDeleteSelection QAction's current values, so that
	// they do not change while the context menu is displayed.
	QAction *contextMenuDeleteSelectedSnapshot = new QAction(NULL);

	// Customize context menu for ports.
	if (dynamic_cast<VuoRendererPort *>(item))
	{
		VuoRendererPort *port = (VuoRendererPort *)item;

		if (port->isConstant() && inputEditorManager)
		{
			VuoType *dataType = static_cast<VuoCompilerInputEventPort *>(port->getBase()->getCompiler())->getDataVuoType();
			VuoInputEditor *inputEditorLoadedForPortDataType = inputEditorManager->newInputEditor(dataType);
			if (inputEditorLoadedForPortDataType)
			{
				contextMenuSetPortConstant->setData(QVariant::fromValue(port));
				contextMenu.addAction(contextMenuSetPortConstant);

				inputEditorLoadedForPortDataType->deleteLater();
			}
		}

		if (!isPortPublished(port) && port->getPublishable())
		{
			contextMenuPublishPort->setText(tr("Publish Port"));
			contextMenuPublishPort->setData(QVariant::fromValue(port));
			contextMenu.addAction(contextMenuPublishPort);
		}

		else if (isPortPublished(port))
		{
			vector<VuoRendererPublishedPort *> externalPublishedPorts = port->getPublishedPorts();
			bool hasExternalPublishedPortWithMultipleInternalPorts = false;
			bool hasExternalPublishedPortBelongingToActiveProtocol = false;
			foreach (VuoRendererPublishedPort *externalPort, externalPublishedPorts)
			{
				if (externalPort->getBase()->getConnectedCables(true).size() > 1)
					hasExternalPublishedPortWithMultipleInternalPorts = true;

				if (dynamic_cast<VuoPublishedPort *>(externalPort->getBase())->isProtocolPort())
					hasExternalPublishedPortBelongingToActiveProtocol = true;
			}

			// Omit the "Delete Published Port" context menu item if the internal port is connected to
			// an external published port that cannot be deleted, either because:
			// - It is part of an active protocol;
			// - It has more than one connected cable.
			if (!hasExternalPublishedPortWithMultipleInternalPorts &&!hasExternalPublishedPortBelongingToActiveProtocol)
			{
				contextMenuPublishPort->setText(tr("Delete Published Port"));
				contextMenuPublishPort->setData(QVariant::fromValue(port));
				contextMenu.addAction(contextMenuPublishPort);
			}
		}

		bool isTriggerPort = dynamic_cast<VuoCompilerTriggerPort *>(port->getBase()->getCompiler());
		if (isTriggerPort || port->getInput())
		{
			__block bool isTopLevelCompositionRunning = false;
			static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
			{
				isTopLevelCompositionRunning = topLevelComposition->isRunning();
			});

			if (isTopLevelCompositionRunning)
			{
				contextMenuFireEvent->setData(QVariant::fromValue(port));
				contextMenu.addAction(contextMenuFireEvent);
			}
		}

		if (isTriggerPort)
		{
			QMenu *contextMenuThrottling = new QMenu(&contextMenu);
			contextMenuThrottling->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			contextMenuThrottling->setTitle(tr("Set Event Throttling"));
			int i = VuoPortClass::EventThrottling_Enqueue;
			foreach (QAction *action, contextMenuThrottlingActions)
			{
				contextMenuThrottling->addAction(action);
				action->setData(QVariant::fromValue(port));
				action->setCheckable(true);
				action->setChecked( i++ == port->getBase()->getEventThrottling() );
			}
			contextMenu.addMenu(contextMenuThrottling);
		}

		// Allow the user to specialize, respecialize, or unspecialize generic data types.
		if (dynamic_cast<VuoGenericType *>(port->getDataType()) || isPortCurrentlyRevertible(port))
		{
			if (contextMenuSpecializeGenericType)
				contextMenuSpecializeGenericType->deleteLater();

			contextMenuSpecializeGenericType = new QMenu(VuoEditorWindow::getMostRecentActiveEditorWindow());
			contextMenuSpecializeGenericType->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			contextMenuSpecializeGenericType->setTitle(tr("Set Data Type"));
			contextMenuSpecializeGenericType->setToolTipsVisible(true);

			populateSpecializePortMenu(contextMenuSpecializeGenericType, port, true);
			contextMenu.addMenu(contextMenuSpecializeGenericType);
		}

		// Allow the user to hide, unhide, or delete cables connected directly to the port or, if the
		// port has a collapsed typecast, to the typecast's child port.
		int numVisibleDirectlyConnectedCables = 0;
		int numHidableDirectlyConnectedCables = 0;
		int numUnhidableDirectlyConnectedCables = 0;

		foreach (VuoCable *cable, port->getBase()->getConnectedCables(true))
		{
			if (!cable->getRenderer()->paintingDisabled())
			{
				numVisibleDirectlyConnectedCables++;
				if (!cable->isPublished())
					numHidableDirectlyConnectedCables++;
			}

			else if (cable->getRenderer()->getEffectivelyWireless())
				numUnhidableDirectlyConnectedCables++;
		}

		int numVisibleChildPortConnectedCables = 0;
		int numHidableChildPortConnectedCables = 0;
		int numUnhidableChildPortConnectedCables = 0;
		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(port);
		if (typecastPort)
		{
			VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
			VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
			foreach(VuoCable *cable, typecastInPort->getConnectedCables(true))
			{
				if (!cable->getRenderer()->paintingDisabled())
				{
					numVisibleChildPortConnectedCables++;

					if (!cable->isPublished())
						numHidableChildPortConnectedCables++;
				}
				else if (cable->getRenderer()->getEffectivelyWireless())
					numUnhidableChildPortConnectedCables++;
			}
		}

		// Count the number of directly or indirectly connected cables that are currently visible.
		int numVisibleConnectedCables = numVisibleDirectlyConnectedCables + numVisibleChildPortConnectedCables;

		// Count the number of directly or indirectly connected cables that are currently hidable.
		int numHidableConnectedCables = numHidableDirectlyConnectedCables + numHidableChildPortConnectedCables;

		// Count the number of directly or indirectly connected cables that are currently hidden.
		int numUnhidableConnectedCables = numUnhidableDirectlyConnectedCables + numUnhidableChildPortConnectedCables;

		if ((!renderHiddenCables && ((numHidableConnectedCables >= 1) || (numUnhidableConnectedCables >= 1)))
				|| (numVisibleConnectedCables >= 1))
		{
			if (!contextMenu.actions().empty() && !contextMenu.actions().last()->isSeparator())
				contextMenu.addSeparator();
		}

		if (!renderHiddenCables)
		{
			if (numHidableConnectedCables >= 1)
			{
				// Use numApparentlyHidableConnectedCables instead of numHidableConnectedCables to determine pluralization
				// of menu item text to avoid the appearance of a bug, even though the "Hide" operation will not
				// impact visible published cables.
				int numApparentlyHidableConnectedCables = numVisibleConnectedCables + numUnhidableConnectedCables;
				contextMenuHideCables->setText(numApparentlyHidableConnectedCables > 1? "Hide Cables" : "Hide Cable");
				contextMenuHideCables->setData(QVariant::fromValue(port));
				contextMenu.addAction(contextMenuHideCables);
			}

			if (numUnhidableConnectedCables >= 1)
			{
				int numApparentlyUnhidableConnectedCables = numVisibleConnectedCables + numUnhidableConnectedCables;
				contextMenuUnhideCables->setText(numApparentlyUnhidableConnectedCables > 1? "Unhide Cables" : "Unhide Cable");
				contextMenuUnhideCables->setData(QVariant::fromValue(port));
				contextMenu.addAction(contextMenuUnhideCables);
			}
		}

		if (numVisibleConnectedCables >= 1)
		{
			contextMenuDeleteCables->setText(numVisibleConnectedCables > 1? "Delete Cables" : "Delete Cable");
			contextMenuDeleteCables->setData(QVariant::fromValue(port));
			contextMenu.addAction(contextMenuDeleteCables);
		}
	}

	// Customize context menu for nodes, cables, and/or comments.
	else if (item)
	{
		if (! item->isSelected())
		{
			clearSelection();
			item->setSelected(true);
		}

		QList<QGraphicsItem *> selectedComponents = selectedItems();
		bool onlyCommentsSelected = true;
		bool onlyCablesSelected = true;
		bool selectionContainsMissingNode = false;
		foreach (QGraphicsItem *item, selectedComponents)
		{
			if (!dynamic_cast<VuoRendererComment *>(item))
				onlyCommentsSelected = false;

			if (!dynamic_cast<VuoRendererCable *>(item))
				onlyCablesSelected = false;

			if (dynamic_cast<VuoRendererNode *>(item) && !dynamic_cast<VuoRendererNode *>(item)->getBase()->hasCompiler())
				selectionContainsMissingNode = true;
		}

		contextMenuDeleteSelectedSnapshot->setText(contextMenuDeleteSelected->text());
		connect(contextMenuDeleteSelectedSnapshot, &QAction::triggered, this, static_cast<void (VuoEditorComposition::*)()>(&VuoEditorComposition::deleteSelectedCompositionComponents));

		// Comments
		VuoRendererComment *comment = dynamic_cast<VuoRendererComment *>(item);
		if (comment)
		{
			// Option: Edit selected comments
			if (onlyCommentsSelected)
				contextMenu.addAction(contextMenuEditSelectedComments);

			// Option: Tint selected components(s)
			contextMenu.addMenu(getContextMenuTints(&contextMenu));

			contextMenu.addSeparator();

			// Option: Refactor selected component(s)
			contextMenu.addAction(contextMenuRefactorSelected);

			contextMenu.addSeparator();

			// Option: Delete selected components(s)
			contextMenu.addAction(contextMenuDeleteSelectedSnapshot);
		}

		// Cables
		VuoRendererCable *cable = dynamic_cast<VuoRendererCable *>(item);
		if (cable)
		{
			if (!renderHiddenCables)
			{
				if (onlyCablesSelected && !cable->getBase()->isPublished())
					contextMenu.addAction(contextMenuHideSelectedCables);
			}

			contextMenu.addAction(contextMenuDeleteSelectedSnapshot);
		}

		// Nodes
		else if (dynamic_cast<VuoRendererNode *>(item))
		{
			VuoRendererNode *node = (VuoRendererNode *)item;

			// Input drawers
			if (dynamic_cast<VuoRendererInputDrawer *>(node) &&
					node->getBase()->getNodeClass()->hasCompiler())
			{
				// Offer "Add/Remove Input Port" options for resizable list input drawers.
				if (dynamic_cast<VuoRendererInputListDrawer *>(node))
				{
					contextMenuAddInputPort->setData(QVariant::fromValue(node));
					contextMenuRemoveInputPort->setData(QVariant::fromValue(node));

					int listItemCount = ((VuoCompilerMakeListNodeClass *)(node->getBase()->getNodeClass()->getCompiler()))->getItemCount();
					contextMenuRemoveInputPort->setEnabled(listItemCount >= 1);

					contextMenu.addAction(contextMenuAddInputPort);
					contextMenu.addAction(contextMenuRemoveInputPort);

					contextMenu.addSeparator();
				}

				// Offer "Reset" option for all input drawers.
				contextMenu.addAction(contextMenuDeleteSelectedSnapshot);
			}

			// Non-input-drawer nodes
			else
			{
				VuoNodeClass *nodeClass = node->getBase()->getNodeClass();
				if (nodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()))
				{
					string originalGenericNodeClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler())->getOriginalGenericNodeClassName();
					VuoCompilerNodeClass *originalGenericNodeClass = compiler->getNodeClass(originalGenericNodeClassName);
					if (originalGenericNodeClass)
						nodeClass = originalGenericNodeClass->getBase();
				}

				QString actionText, sourcePath;
				bool nodeClassIsEditable = VuoEditorUtilities::isNodeClassEditable(nodeClass, actionText, sourcePath);
				bool nodeClassIs3rdParty = nodeClass->hasCompiler() && !nodeClass->getCompiler()->isBuiltIn();

				int numSelectedNonAttachmentNodes = 0;
				QList<QGraphicsItem *> selectedComponents = selectedItems();
				foreach (QGraphicsItem *item, selectedComponents)
				{
					if (dynamic_cast<VuoRendererNode *>(item) && !dynamic_cast<VuoRendererInputAttachment *>(item))
						numSelectedNonAttachmentNodes++;
				}

				if ((numSelectedNonAttachmentNodes == 1) && nodeClassIsEditable)
				{
					// Option: Edit an installed subcomposition, shader, or text-code node.
					QAction *editAction = new QAction(NULL);
					editAction->setText(actionText);
					editAction->setData(QVariant::fromValue(node));
					connect(editAction, &QAction::triggered, this, &VuoEditorComposition::emitNodeSourceEditorRequested);

					contextMenu.addAction(editAction);
					contextMenu.addSeparator();
				}

				// Option: Rename selected node(s)
				if (node->getBase()->hasCompiler())
					contextMenu.addAction(contextMenuRenameSelected);

				// Option: Tint selected component(s)
				contextMenu.addMenu(getContextMenuTints(&contextMenu));

				contextMenu.addSeparator();

				// Option: Replace selected node with a similar node
				if (numSelectedNonAttachmentNodes == 1)
				{
					if (contextMenuChangeNode)
						contextMenuChangeNode->deleteLater();

					contextMenuChangeNode = new QMenu(VuoEditorWindow::getMostRecentActiveEditorWindow());
					contextMenuChangeNode->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
					contextMenuChangeNode->setTitle(tr("Change To"));

					populateChangeNodeMenu(contextMenuChangeNode, node, initialChangeNodeSuggestionCount);
					if (!contextMenuChangeNode->actions().isEmpty())
						contextMenu.addMenu(contextMenuChangeNode);
				} // End single selected node

				// Option: Refactor selected component(s)
				if (!selectionContainsMissingNode)
					contextMenu.addAction(contextMenuRefactorSelected);

				if ((numSelectedNonAttachmentNodes == 1) && (nodeClassIsEditable || nodeClassIs3rdParty))
				{
					// Option: Open the enclosing folder for an editable or other 3rd party node class.
					QString modulePath = nodeClassIsEditable? sourcePath : nodeClass->getCompiler()->getModulePath().c_str();
					if (!modulePath.isEmpty())
					{
						QString enclosingDirUrl = "file://" + QFileInfo(modulePath).dir().absolutePath();
						QAction *openEnclosingFolderAction = new QAction(NULL);
						openEnclosingFolderAction->setText("Show in Finder");
						openEnclosingFolderAction->setData(enclosingDirUrl);
						connect(openEnclosingFolderAction, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::openExternalUrlFromSenderData);
						contextMenu.addAction(openEnclosingFolderAction);
					}
				}

				if (!contextMenu.actions().empty() && !contextMenu.actions().last()->isSeparator())
					contextMenu.addSeparator();

				// Option: Delete selected components(s)
				contextMenu.addAction(contextMenuDeleteSelectedSnapshot);

			} // End non-input-drawer nodes
		} // End nodes
	} // End nodes or cables

	if (!contextMenu.actions().isEmpty())
	{
		// Disable non-detached port popovers so that they don't obscure the view of the context menu.
		disableNondetachedPortPopovers();

		menuSelectionInProgress = true;
		contextMenu.exec(event->screenPos());
		menuSelectionInProgress = false;
	}

	delete contextMenuDeleteSelectedSnapshot;
}

/**
 * Workaround so that the "Edit {Composition/Shader/Node}" context menu item can emit a signal with argument when triggered.
 */
void VuoEditorComposition::emitNodeSourceEditorRequested()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererNode *node = sender->data().value<VuoRendererNode *>();
	emit nodeSourceEditorRequested(node);
}

/**
 * Expands the port's "Set Data Type" menu to include additional specialization options, and redisplays the menu.
 */
void VuoEditorComposition::expandSpecializePortMenu()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererPort *port = sender->data().value<VuoRendererPort *>();

	populateSpecializePortMenu(contextMenuSpecializeGenericType, port, false);
	contextMenuSpecializeGenericType->exec();
}

/**
 * Populates the "Set Data Type" @c menu for the provided @c port.
 *
 * If @c limitInitialOptions is true, displays only the specialization options "above the fold,"
 * (i.e., core types, or all compatible types if there are only a small number),
 * and adds a "More…" option at the end to display the full menu.
 */
void VuoEditorComposition::populateSpecializePortMenu(QMenu *menu, VuoRendererPort *port, bool limitInitialOptions)
{
	menu->clear();

	if (!port)
		return;

	VuoGenericType *genericDataType = dynamic_cast<VuoGenericType *>(port->getDataType());
	QAction *unspecializeAction = menu->addAction(tr("Generic"));

	// It is safe to assume that there will be types listed after the "Generic" option
	// since any network of connected generic/specialized ports has at least one compatible
	// data type in common, so the separator can be added here without forward-checking.
	menu->addSeparator();

	unspecializeAction->setData(QVariant::fromValue(port));
	unspecializeAction->setCheckable(true);
	unspecializeAction->setChecked(genericDataType);

	VuoGenericType *genericTypeFromPortClass = NULL; // Original generic type of the port
	set<string> compatibleTypes; // Compatible types in the context of the connected generic port network
	set<string> compatibleTypesInIsolation; // Compatible types for the port in isolation

	// Allow the user to specialize generic ports.
	if (genericDataType)
	{
		VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(port->getBase()->getClass()->getCompiler());
		genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());

		// Determine compatible types in the context of the connected generic port network.
		VuoGenericType::Compatibility compatibility;
		vector<string> compatibleTypesVector = genericDataType->getCompatibleSpecializedTypes(compatibility);
		compatibleTypes = set<string>(compatibleTypesVector.begin(), compatibleTypesVector.end());

		// If all types or all list types are compatible, add them to (currently empty) compatibleTypes.
		if (compatibility == VuoGenericType::anyType || compatibility == VuoGenericType::anyListType)
		{
			vector<string> compatibleTypeNames = getAllSpecializedTypeOptions(compatibility == VuoGenericType::anyListType);
			compatibleTypes.insert(compatibleTypeNames.begin(), compatibleTypeNames.end());
		}
	}

	// Allow the user to re-specialize already specialized ports.
	else if (isPortCurrentlyRevertible(port))
	{
		map<VuoNode *, string> nodesToReplace;
		set<VuoCable *> cablesToDelete;
		createReplacementsToUnspecializePort(port->getBase(), false, nodesToReplace, cablesToDelete);
		if (cablesToDelete.size() >= 1)
		{
			// @todo https://b33p.net/kosada/node/8895 : Note that this specialization will break connections.
		}

		VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(
																	port->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler());
		genericTypeFromPortClass = dynamic_cast<VuoGenericType *>( specializedNodeClass->getOriginalPortType(port->getBase()->getClass()) );
		compatibleTypes = getRespecializationOptionsForPortInNetwork(port);
	}

	// Determine compatible types in isolation.
	if (genericTypeFromPortClass)
	{
		// "Make List" drawer child ports require special handling, since technically they are compatible with all types
		// but practically they are limited to types compatible with their host ports.
		VuoRendererInputListDrawer *drawer = dynamic_cast<VuoRendererInputListDrawer *>(port->getUnderlyingParentNode());
		if (drawer)
		{
			VuoPort *hostPort = drawer->getUnderlyingHostPort();
			if (hostPort && hostPort->hasRenderer())
			{
				VuoCompilerPortClass *portClass = static_cast<VuoCompilerPortClass *>(hostPort->getClass()->getCompiler());
				VuoGenericType *genericHostPortDataType = dynamic_cast<VuoGenericType *>(hostPort->getRenderer()->getDataType());
				if (genericHostPortDataType)
					genericTypeFromPortClass = static_cast<VuoGenericType *>(portClass->getDataVuoType());
				else if (isPortCurrentlyRevertible(hostPort->getRenderer()))
				{
					VuoCompilerNodeClass *nodeClass = hostPort->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
					VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass);
					genericTypeFromPortClass = dynamic_cast<VuoGenericType *>( specializedNodeClass->getOriginalPortType(portClass->getBase()) );
				}
			}
		}

		VuoGenericType::Compatibility compatibilityInIsolation = VuoGenericType::whitelistedTypes;
		vector<string> compatibleTypesInIsolationVector;
		if (genericTypeFromPortClass)
			compatibleTypesInIsolationVector = genericTypeFromPortClass->getCompatibleSpecializedTypes(compatibilityInIsolation);

		// If all types or all list types are compatible, add them to (currently empty) compatibleTypesInIsolationVector.
		if (compatibilityInIsolation == VuoGenericType::anyType || compatibilityInIsolation == VuoGenericType::anyListType)
			compatibleTypesInIsolationVector = getAllSpecializedTypeOptions(compatibilityInIsolation == VuoGenericType::anyListType);

		foreach (string type, compatibleTypesInIsolationVector)
			compatibleTypesInIsolation.insert(drawer? VuoType::extractInnermostTypeName(type) : type);
	}

	// List the compatible types in the menu.
	{
		// If there are only a handful of eligible types, display them all in a flat menu.
		const int maxTypeCountForFlatMenuDisplay = 10;
		if (compatibleTypesInIsolation.size() <= maxTypeCountForFlatMenuDisplay)
		{
			QList<QAction *> actions = getCompatibleTypesForMenu(port, compatibleTypesInIsolation, compatibleTypes, false, "", menu);
			addTypeActionsToMenu(actions, menu);
		}

		// Otherwise, organize them by node set.
		else
		{
			// First list compatible types that don't belong to any specific node set.
			QList<QAction *> actions = getCompatibleTypesForMenu(port, compatibleTypesInIsolation, compatibleTypes, true, "", menu);
			addTypeActionsToMenu(actions, menu);

			// Now add a submenu for each node set that contains compatible types.
			QList<QAction *> allNodeSetActionsToAdd;
			for (const string &nodeSet : moduleManager->getKnownNodeSets())
				allNodeSetActionsToAdd += getCompatibleTypesForMenu(port, compatibleTypesInIsolation, compatibleTypes, true, nodeSet, menu);

			bool usingExpansionMenu = false;
			if ((menu->actions().size() > 0) && (allNodeSetActionsToAdd.size() > 0))
			{
				menu->addSeparator();

				if (limitInitialOptions)
				{
					//: Appears at the bottom of the "Set Data Type" menu when there are additional options to display.
					QAction *showMoreAction = menu->addAction(tr("More…"));
					showMoreAction->setData(QVariant::fromValue(port));
					connect(showMoreAction, &QAction::triggered, this, &VuoEditorComposition::expandSpecializePortMenu);
					usingExpansionMenu = true;
				}
			}

			if (!usingExpansionMenu)
				addTypeActionsToMenu(allNodeSetActionsToAdd, menu);
		}

		foreach (QAction *action, menu->actions())
		{
			QMenu *specializeSubmenu = action->menu();
			if (specializeSubmenu)
			{
				foreach (QAction *specializeSubaction, specializeSubmenu->actions())
					connect(specializeSubaction, &QAction::triggered, this, &VuoEditorComposition::specializeGenericPortType);
			}
			else if (action == unspecializeAction)
				connect(action, &QAction::triggered, this, &VuoEditorComposition::unspecializePortType);
			else
				connect(action, &QAction::triggered, this, &VuoEditorComposition::specializeGenericPortType);
		}
	}
}

/**
 * Returns the full list of type names that should be listed as specialization options
 * for a completely generic port. If @c lists is true, returns the list versions of those types;
 * otherwise, returns the individual-element versions of the types.
 */
vector<string> VuoEditorComposition::getAllSpecializedTypeOptions(bool lists)
{
	if (lists)
	{
		set<string> types = moduleManager->getKnownListTypeNames(true);
		return vector<string>(types.begin(), types.end());
	}
	else
	{
		vector<string> typeOptions;
		for (auto i : moduleManager->getLoadedSingletonTypes(true))
			typeOptions.push_back(i.first);

		return typeOptions;
	}
}

/**
 * Returns a list of types to which the provided @c port may be respecialized
 * without breaking any connections within its current network of connected
 * specialized ports.
 */
set<string> VuoEditorComposition::getRespecializationOptionsForPortInNetwork(VuoRendererPort *port)
{
	if (!port)
		return {};

	VuoGenericType::Compatibility compatibility;
	vector<string> compatibleTypes = getBase()->getCompiler()->getCompatibleSpecializedTypesForPort(port->getUnderlyingParentNode()->getBase(),
																									port->getBase(), compatibility);

	if (compatibility == VuoGenericType::anyType || compatibility == VuoGenericType::anyListType)
	{
		vector<string> allTypes = getAllSpecializedTypeOptions(compatibility == VuoGenericType::anyListType);
		return set<string>(allTypes.begin(), allTypes.end());
	}
	else
		return set<string>(compatibleTypes.begin(), compatibleTypes.end());
}

/**
 * Constructs a list of actions, each of which represents a compatible type
 * for the provided `genericPort` with the set of `compatibleTypesInContext`
 * to which the port may be specialized within its current generic port network
 * (or `compatibleTypesInIsolation` when the port is in isolation), organizing the
 * compatible types by node set and associating each action with the data and slot
 * necessary to accomplish the "Specialize" operation.
 *
 * If `limitToNodeSet` is `true`, includes only types specific to the provided `nodeSetName`
 * in a submenu named for that type.
 * - If `limitToNodeSet` is `true` but the provided `nodeSetName` is empty, the compatible "Core" types are
 * included in the top-level list.
 * - If `limitToNodeSet` is `false`, all compatible types (regardless of node set) are included in the top-level list.
 *
 * If a menu is provided, any submenus or actions created will be initialized with the provided menu as their parent
 * (although not added to the menu).
 *
 * Returns the generated list of actions, which may be used to populate a menu.
 */
QList<QAction *> VuoEditorComposition::getCompatibleTypesForMenu(VuoRendererPort *genericPort,
																 set<string> compatibleTypesInIsolation,
																 set<string> compatibleTypesInContext,
																 bool limitToNodeSet,
																 string nodeSetName,
																 QMenu *menu)
{
	QList<QAction *> actionsToAddToMenu;

	vector<string> compatibleTypesForNodeSetDisplay;
	for (const string &typeName : compatibleTypesInIsolation)
	{
		string innermostTypeName = VuoType::extractInnermostTypeName(typeName);
		if (! VuoGenericType::isGenericTypeName(innermostTypeName) &&
				// @todo: Re-enable listing of VuoUrl type for https://b33p.net/kosada/node/9204
				innermostTypeName != "VuoUrl" &&
				// @todo: Re-enable listing of interaction type for https://b33p.net/kosada/node/11631
				innermostTypeName != "VuoInteraction" &&
				innermostTypeName != "VuoInteractionType" &&
				innermostTypeName != "VuoUuid" &&
				// Hide deprecated types.
				innermostTypeName != "VuoIconPosition" &&
				innermostTypeName != "VuoMesh" &&
				innermostTypeName != "VuoWindowProperty" &&
				innermostTypeName != "VuoWindowReference" &&
				(! limitToNodeSet || moduleManager->getNodeSetForType(innermostTypeName) == nodeSetName))
			compatibleTypesForNodeSetDisplay.push_back(typeName);
	}

	if (!compatibleTypesForNodeSetDisplay.empty())
	{
		QMenu *contextMenuNodeSetTypes = NULL;
		QList<QAction *> actionsToAddToNodeSetSubmenu;
		bool enabledContentAdded = false;

		// Populate the "Specialize Type" submenu for the target node set.
		if (!nodeSetName.empty())
		{
			contextMenuNodeSetTypes = new QMenu(menu);
			contextMenuNodeSetTypes->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			contextMenuNodeSetTypes->setTitle(formatNodeSetNameForDisplay(nodeSetName.c_str()));
			contextMenuNodeSetTypes->setToolTipsVisible(true);
			actionsToAddToMenu.append(contextMenuNodeSetTypes->menuAction());
		}

		for (const string &typeName : compatibleTypesForNodeSetDisplay)
		{
			QList<QVariant> portAndSpecializedType;
			portAndSpecializedType.append(QVariant::fromValue(genericPort));
			portAndSpecializedType.append(typeName.c_str());

			QAction *specializeAction;
			QString typeTitle = formatTypeNameForDisplay(typeName);

			// Case: Adding to the node set submenu
			if (!nodeSetName.empty())
			{
				specializeAction = new QAction(typeTitle, contextMenuNodeSetTypes);
				actionsToAddToNodeSetSubmenu.append(specializeAction);
			}

			// Case: Adding to the top-level action list
			else
			{
				specializeAction = new QAction(typeTitle, menu);
				actionsToAddToMenu.append(specializeAction);
			}

			specializeAction->setData(QVariant(portAndSpecializedType));
			specializeAction->setCheckable(true);
			specializeAction->setChecked(genericPort && genericPort->getDataType() && (genericPort->getDataType()->getModuleKey() == typeName));

			if (compatibleTypesInContext.find(typeName) == compatibleTypesInContext.end())
			{
				specializeAction->setEnabled(false);
				//: Appears in a tooltip when hovering over a menu item for a type specialization that's prevented by a cable connection.
				specializeAction->setToolTip(tr("To change to this type, disconnect the cable from this port and from this node's other ports of the same type."));
			}
			else
				enabledContentAdded = true;
		}

		if (contextMenuNodeSetTypes)
		{
			addTypeActionsToMenu(actionsToAddToNodeSetSubmenu, contextMenuNodeSetTypes);

			if (!enabledContentAdded)
				contextMenuNodeSetTypes->setEnabled(false);
		}
	}

	QList<QAction *> actionListWithPromotions = promoteSingletonsFromSubmenus(actionsToAddToMenu);
	return actionListWithPromotions;
}

/**
 * Iterates through a list of actions that may include submenus; for any submenu that contains
 * exactly one item, replaces the submenu with the item itself and returns the resulting list.
 */
QList<QAction *> VuoEditorComposition::promoteSingletonsFromSubmenus(QList<QAction *> actionList)
{
	QList<QAction *> modifiedActionList;
	foreach (QAction *action, actionList)
	{
		if (action->menu() && (action->menu()->actions().size() == 1))
		{
			QAction *singleSubaction = action->menu()->actions().first();
			action->menu()->removeAction(singleSubaction);
			modifiedActionList.append(singleSubaction);
		}
		else
			modifiedActionList.append(action);
	}

	return modifiedActionList;
}

/**
 * Sorts and adds the provided list of actions representing Vuo types to the specified menu.
 */
void VuoEditorComposition::addTypeActionsToMenu(QList<QAction *> actionList, QMenu *menu)
{
	std::sort(actionList.begin(), actionList.end(),
			  [](QAction *action1, QAction *action2) { return action1->text().compare(action2->text(), Qt::CaseInsensitive) < 0; });

	foreach (QAction *action, actionList)
		menu->addAction(action);
}

/**
 * Shows or hides port popovers in response to changes in the active window.
 */
void VuoEditorComposition::updatePopoversForActiveWindowChange(QWidget *old, QWidget *now)
{
	if (!now)
		return;

	VuoEditorWindow *activeWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (activeWindow)
		emit compositionOnTop(activeWindow->getComposition() == this);
}

/**
 * Shows or hides popovers in response to changes in the active application.
 */
void VuoEditorComposition::updatePopoversForApplicationStateChange(bool active)
{
	if (ignoreApplicationStateChangeEvents)
		return;

	VuoEditorWindow *activeWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (activeWindow && (activeWindow->getComposition() == this) && (!activeWindow->isMinimized()))
		emit applicationActive(active);
}

/**
 * Returns a pointer to the port that should be operated upon
 * if the user were to click at position @c scenePos,
 * or NULL if no port is within collision range.
 * If @c limitPortCollisionRange is true, only the port circle
 * and constant value flag, not the port's boundingRect as a whole,
 * are taken into account for purposes of collision detection.
 */
QGraphicsItem * VuoEditorComposition::findNearbyPort(QPointF scenePos, bool limitPortCollisionRange)
{
	return findNearbyComponent(scenePos, VuoEditorComposition::targetTypePort, limitPortCollisionRange);
}

/**
 * Returns the node whose header area @a scenePos falls within, or NULL if none.
 */
VuoRendererNode * VuoEditorComposition::findNearbyNodeHeader(QPointF scenePos)
{
	QGraphicsItem *item = findNearbyComponent(scenePos, VuoEditorComposition::targetTypeNodeHeader);
	return dynamic_cast<VuoRendererNode *>(item);
}

/**
 * Returns a pointer to the component (port, cable, node, or comment)
 * that should be operated upon if the user were to click at position @c scenePos,
 * or NULL if no component is within collision range.
 * The input @c targetType specifies what type of component to search for.  Options are:
 *    VuoEditorComposition::targetTypePort (to include only ports),
 *    VuoEditorComposition::targetTypeNodeHeader (to include only node headers), or
 *    VuoEditorComposition::targetTypeAny (to include ports, cables, nodes, and comments).
 * If @c limitPortCollisionRange is true, only the port circle
 * and constant value flag, not the port's boundingRect as a whole,
 * are taken into account for purposes of collision detection.
 */
QGraphicsItem * VuoEditorComposition::findNearbyComponent(QPointF scenePos,
														  targetComponentType targetType,
														  bool limitPortCollisionRange)
{
	// Determine which types of components to filter out of search results.
	bool ignoreCables;
	bool ignoreNodes;
	bool ignorePorts;
	bool ignoreComments;

	switch(targetType)
	{
		case VuoEditorComposition::targetTypePort:
		{
			ignoreCables = true;
			ignoreNodes = true;
			ignorePorts = false;
			ignoreComments = true;
			break;
		}
		case VuoEditorComposition::targetTypeNodeHeader:
		{
			ignoreCables = true;
			ignoreNodes = true;
			ignorePorts = true;
			ignoreComments = true;
			break;
		}
		default:
		{
			ignoreCables = false;
			ignoreNodes = false;
			ignorePorts = false;
			ignoreComments = false;
			break;
		}
	}

	// The topmost item under the cursor is not necessarily the one we will return
	// (e.g., if the cursor is positioned directly over a node but also within range
	// of one of that node's ports), but it will factor in to the decision.
	QGraphicsItem *topmostItemUnderCursor = itemAt(scenePos, views()[0]->transform());
	if (topmostItemUnderCursor && (!topmostItemUnderCursor->isEnabled()))
		topmostItemUnderCursor = NULL;

	for (int rectLength = componentCollisionRange/2; rectLength <= componentCollisionRange; rectLength += componentCollisionRange/2)
	{
		QRectF searchRect(scenePos.x()-0.5*rectLength, scenePos.y()-0.5*rectLength, rectLength, rectLength);
		QList<QGraphicsItem *> itemsInRange = items(searchRect);
		for (QList<QGraphicsItem *>::iterator i = itemsInRange.begin(); i != itemsInRange.end(); ++i)
		{
			if (!(*i)->isEnabled())
				continue;

			// Check whether we have located an unobscured "Make List" drawer drag handle.
			if (! ignoreNodes)
			{
				VuoRendererInputListDrawer *makeListDrawer = dynamic_cast<VuoRendererInputListDrawer *>(*i);
				bool makeListDragHandle =
						(makeListDrawer &&
						 makeListDrawer->getExtendedDragHandleRect().translated(makeListDrawer->scenePos()).contains(scenePos));
				if (makeListDragHandle &&
						((! topmostItemUnderCursor) ||
						 (topmostItemUnderCursor == makeListDrawer) ||
						 (topmostItemUnderCursor->zValue() < makeListDrawer->zValue())))
				{
					return makeListDrawer;
				}
			}

			// Check whether we have located an unobscured port.
			// Hovering within range of a port takes precedence over hovering
			// directly over that port's parent node.
			if (! ignorePorts)
			{
				VuoRendererPort *port = dynamic_cast<VuoRendererPort *>(*i);
				if (port &&
						((! topmostItemUnderCursor) ||
						 (topmostItemUnderCursor == port) ||
						 (topmostItemUnderCursor == port->getRenderedParentNode()) ||
						 (topmostItemUnderCursor->zValue() < port->zValue()))
						&&
						((! limitPortCollisionRange) ||
						 port->getPortRect().united(port->getPortConstantTextRect()).translated(port->scenePos()).intersects(searchRect))
						&&
						! port->getFunctionPort())  /// @todo Remove after function ports are added back in (https://b33p.net/kosada/node/3927)
				{
					return port;
				}
			}

			// Check whether we have located an unobscured cable.
			if (! ignoreCables)
			{
				VuoRendererCable *cable = dynamic_cast<VuoRendererCable *>(*i);
				if (cable &&
							((! topmostItemUnderCursor) ||
							(topmostItemUnderCursor == cable) ||
							(topmostItemUnderCursor->zValue() < cable->zValue())))
				{
					return cable;
				}
			}

			// Check whether we have located an unobscured comment.
			if (! ignoreComments)
			{
				VuoRendererComment *comment = dynamic_cast<VuoRendererComment *>(*i);
				if (!comment && dynamic_cast<VuoRendererComment *>((*i)->parentItem()))
					comment = dynamic_cast<VuoRendererComment *>((*i)->parentItem());

				if (comment &&
							((! topmostItemUnderCursor) ||
							(topmostItemUnderCursor == (*i)) ||
							(topmostItemUnderCursor->zValue() < (*i)->zValue())))
				{
					return comment;
				}
			}

			// Check whether we have located an unobscured node header.
			if (targetType == VuoEditorComposition::targetTypeNodeHeader)
			{
				VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(*i);
				if (node && ! dynamic_cast<VuoRendererInputDrawer *>(node))
				{
					QRectF headerRect = node->getOuterNodeFrameBoundingRect();
					headerRect = node->mapToScene(headerRect).boundingRect();
					if (headerRect.intersects(searchRect) && scenePos.y() <= headerRect.bottom())
						return node;
				}
			}
		}
	}

	// Having failed to locate any other relevant components within range, return the node
	// directly under the cursor, if applicable.
	if (! ignoreNodes)
	{
		if (dynamic_cast<VuoRendererNode *>(topmostItemUnderCursor))
			return topmostItemUnderCursor;

		// It is possible that the item under the cursor is a port, but that it didn't meet
		// the more stringent limitPortCollisionRange requirement.  In this case, return its parent node.
		if (dynamic_cast<VuoRendererPort *>(topmostItemUnderCursor))
			return ((VuoRendererPort *)(topmostItemUnderCursor))->getRenderedParentNode();
	}

	return NULL;
}

/**
 * Returns the port to which an event-only cable would be connected if the endpoint of `cableInProgress`
 * were dropped onto the header of @a node.
 *
 * This is generally the node's first input or output port (depending on the direction of `cableInProgress`),
 * but there's special handling for walled input ports and input ports with attached drawers or type-converters.
 *
 * If the node has no input/output port or `cableInProgress` is NULL, this function returns NULL.
 */
VuoRendererPort * VuoEditorComposition::findTargetPortForCableDroppedOnNodeHeader(VuoRendererNode *node)
{
	if (! cableInProgress)
		return NULL;

	VuoPort *fromPort = cableInProgress->getFromPort();
	VuoPort *toPort = cableInProgress->getToPort();
	VuoPort *fixedPort = (fromPort? fromPort: toPort);

	if (dynamic_cast<VuoRendererPort *>(fixedPort->getRenderer())->getUnderlyingParentNode() == node)
		return NULL;

	return findDefaultPortForEventOnlyConnection(node, (fixedPort == fromPort));
}

/**
 * Returns the port to which an event-only cable should be connected by default.
 * If @a input is true, selects from the node's input ports; otherwise, selects from its output ports.
 */
VuoRendererPort * VuoEditorComposition::findDefaultPortForEventOnlyConnection(VuoRendererNode *node, bool inputPort)
{
	// Start with the first input or output port (other than the refresh port).
	vector<VuoRendererPort *> portList;
	int firstPortIndex;

	if (inputPort)
	{
		portList = node->getInputPorts();
		firstPortIndex = VuoNodeClass::unreservedInputPortStartIndex;
	}
	else
	{
		portList = node->getOutputPorts();
		firstPortIndex = VuoNodeClass::unreservedOutputPortStartIndex;
	}

	VuoRendererPort *targetPort = NULL;
	if (portList.size() > firstPortIndex)
	{
		targetPort = portList[firstPortIndex];

		// If the first input port has a wall,
		// keep moving down until we find one without a wall.
		// (Unless they all have walls, in which case stick with the first.)
		VuoRendererPort *firstPortWithoutWall = nullptr;
		for (int i = firstPortIndex; i < portList.size(); ++i)
		{
			if (portList[i]->getBase()->getClass()->getEventBlocking() != VuoPortClass::EventBlocking_Wall)
			{
				firstPortWithoutWall = portList[i];
				break;
			}
		}
		if (firstPortWithoutWall)
			targetPort = firstPortWithoutWall;

		// If the first input port has a drawer attached to it,
		// instead select the first input port of the drawer.
		// (Unless the drawer has no input ports, in which case don't select any port.)
		VuoRendererInputDrawer *drawer = targetPort->getAttachedInputDrawer();
		if (drawer)
		{
			portList = drawer->getInputPorts();
			if (portList.size() > firstPortIndex)
				targetPort = portList[firstPortIndex];
			else
				targetPort = NULL;
		}

		// If the selected input port has a collapsed type-converter node attached to it,
		// instead select the type-converter node's input port.
		VuoRendererTypecastPort *typecast = dynamic_cast<VuoRendererTypecastPort *>(targetPort);
		if (typecast)
			targetPort = typecast->getChildPort();
	}

	return targetPort;
}

/**
 * Calculates and returns the bounding rect of all internal composition components on the scene.
 * This excludes published cables.
 */
QRectF VuoEditorComposition::internalItemsBoundingRect() const
{
	QRectF boundingRect;
	foreach (QGraphicsItem *item, items())
	{
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(item);
		if (! (rc && rc->getBase()->isPublished()))
			boundingRect |= item->sceneBoundingRect();
	}

	return boundingRect;
}

/**
 * Calculates and returns the bounding rect of all currenty selected composition nodes and cables.
 * This excludes constant flags and published cables.
 */
QRectF VuoEditorComposition::internalSelectedItemsBoundingRect() const
{
	QRectF boundingRect;

	foreach (QGraphicsItem *item, selectedItems())
	{
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(item);
		if (! (rc && rc->getBase()->isPublished()))
			boundingRect |= item->sceneBoundingRect();
	}

	return boundingRect;
}

/**
 * Calculates and returns the bounding rect of all currenty selected composition nodes, ports, and cables.
 * This includes port constant flags and attached typecasts and drawers; it excludes published cables.
 */
QRectF VuoEditorComposition::internalSelectedItemsChildrenBoundingRect() const
{
	QRectF boundingRect;

	foreach (QGraphicsItem *item, selectedItems())
	{
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(item);
		if (! (rc && rc->getBase()->isPublished()))
			boundingRect |= item->mapToScene(item->childrenBoundingRect()).boundingRect();

		// Include attached drawers.
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(item);
		if (rn)
		{
			foreach (VuoPort *port, rn->getBase()->getInputPorts())
			{
				VuoRendererInputDrawer *drawer = (port->hasRenderer()? port->getRenderer()->getAttachedInputDrawer() : NULL);
				if (drawer)
					boundingRect |= drawer->mapToScene(drawer->childrenBoundingRect()).boundingRect();
			}
		}
	}

	return boundingRect;
}

/**
 * Updates the constant value of the internal input port with the provided @c portID
 * to the @c newValue, in both the stored and running copies of the composition (if applicable).
 */
void VuoEditorComposition::updatePublishedPortConstant(string portName, string newValue, bool updateInRunningComposition)
{
	VuoPublishedPort *port = getBase()->getPublishedInputPortWithName(portName);
	if (!port)
		return;

	updatePortConstant(dynamic_cast<VuoCompilerPublishedPort *>(port->getCompiler()), newValue, updateInRunningComposition);
}

/**
 * Updates the provided @c port constant with the @c newValue in both the stored
 * and running copies of the composition (if applicable).
 */
void VuoEditorComposition::updatePortConstant(VuoCompilerPort *port, string newValue, bool updateInRunningComposition)
{
	// Internal ports
	if (dynamic_cast<VuoCompilerInputEventPort *>(port))
	{
		port->getBase()->getRenderer()->setConstant(newValue);

		if (updateInRunningComposition)
			updateInternalPortConstantInRunningComposition(dynamic_cast<VuoCompilerInputEventPort *>(port), newValue);
	}

	// Published ports
	else if (dynamic_cast<VuoCompilerPublishedPort *>(port))
	{
		dynamic_cast<VuoCompilerPublishedPort *>(port)->setInitialValue(newValue);

		if (updateInRunningComposition)
			updatePublishedInputPortConstantInRunningComposition(dynamic_cast<VuoPublishedPort *>(port->getBase()), newValue);
	}
}

/**
 * Gives each group/network of connected generic ports a unique generic type;
 * updates port popovers accordingly.
 */
void VuoEditorComposition::updateGenericPortTypes(void)
{
	getBase()->getCompiler()->updateGenericPortTypes();
	updatePortPopovers();
}

/**
 * Checks the composition, along with the cable being dragged (if any), for invalid feedback loops.
 * Updates the rendered compositions's error markings.
 *
 * @param targetPort If there is a cable drag in progress, the port (if any) that the
 *		cable would connect to if dropped at its current position. Otherwise null.
 */
void VuoEditorComposition::updateFeedbackErrors(VuoRendererPort *targetPort)
{

	// Prevent recursive updates of feedback errors (resulting, e.g., from show()ing popovers).
	if (!errorMarkingUpdatesEnabled)
		return;

	errorMarkingUpdatesEnabled = false;

	// Remove any error annotations from the previous call to this function.
	if (errorMark)
	{
		errorMark->removeFromScene();
		errorMark = NULL;
	}

	disableErrorPopovers();

	// Check for errors.

	VuoCompilerIssues *issues = new VuoCompilerIssues();
	VuoCompilerCable *potentialCable = NULL;
	try
	{
		set<VuoCompilerCable *> potentialCables;

		if (targetPort && cableInProgress)
		{
			VuoNode *fromNode;
			VuoPort *fromPort;
			VuoNode *toNode;
			VuoPort *toPort;
			if (cableInProgress->getFromNode())
			{
				fromNode = cableInProgress->getFromNode();
				fromPort = cableInProgress->getFromPort();
				toNode = targetPort->getUnderlyingParentNode()->getBase();
				toPort = targetPort->getBase();
			}
			else
			{
				fromNode = targetPort->getUnderlyingParentNode()->getBase();
				fromPort = targetPort->getBase();
				toNode = cableInProgress->getToNode();
				toPort = cableInProgress->getToPort();
			}
			potentialCable = new VuoCompilerCable(NULL, NULL, NULL, NULL);
			potentialCable->getBase()->setFrom(fromNode, fromPort);
			potentialCable->getBase()->setTo(toNode, toPort);
			potentialCable->setAlwaysEventOnly(! cableInProgress->getRenderer()->effectivelyCarriesData() ||
											   cableInProgress->getRenderer()->isFloatingEndpointAboveEventPort());

			fromPort->removeConnectedCable(potentialCable->getBase());
			toPort->removeConnectedCable(potentialCable->getBase());
			potentialCables.insert(potentialCable);
		}

		getBase()->getCompiler()->checkForEventFlowIssues(potentialCables, issues);
	}
	catch (const VuoCompilerException &e)
	{
	}

	if (! issues->isEmpty())
	{
		VUserLog("%s:      Showing error popover: %s",
			window->getWindowTitleWithoutPlaceholder().toUtf8().data(),
			issues->getShortDescription(false).c_str());

		this->errorMark = new VuoErrorMark();

		foreach (VuoCompilerIssue issue, issues->getList())
		{
			set<VuoRendererNode *> nodesToMark;
			set<VuoRendererCable *> cablesToMark;

			set<VuoNode *> problemNodes = issue.getNodes();
			foreach (VuoNode *node, problemNodes)
				if (node->hasRenderer())
					nodesToMark.insert(node->getRenderer());

			set<VuoCable *> problemCables = issue.getCables();
			foreach (VuoCable *cable, problemCables)
			{
				VuoCable *cableToMark = (cable->getCompiler() == potentialCable ? cableInProgress : cable);
				if (cableToMark->hasRenderer())
					cablesToMark.insert(cableToMark->getRenderer());
			}

			/// @todo https://b33p.net/kosada/node/13572 Check if issue.getIssueType() is error or warning.
			errorMark->addMarkedComponents(nodesToMark, cablesToMark);

			VuoErrorPopover *errorPopover = new VuoErrorPopover(issue, NULL);
			errorPopovers.insert(errorPopover);
			connect(this, &VuoEditorComposition::compositionOnTop, errorPopover, &VuoErrorPopover::setWindowLevelAndVisibility);
			connect(this, &VuoEditorComposition::applicationActive, errorPopover, &VuoErrorPopover::setWindowLevel);

			QRectF viewportRect = views()[0]->mapToScene(views()[0]->viewport()->rect()).boundingRect();

			// Place the popover near an appropriate nearby node involved in the feedback loop.
			VuoRendererNode *nearbyNode = NULL;
			if (targetPort && cableInProgress && nodesToMark.find(targetPort->getRenderedParentNode()) != nodesToMark.end())
			{
				nearbyNode = targetPort->getRenderedParentNode();
			}
			else if (! nodesToMark.empty())
			{
				VuoRendererNode *topmostVisibleNode = NULL;
				qreal topY = viewportRect.bottom();
				foreach (VuoRendererNode *node, nodesToMark)
				{
					if (node->getProxyNode())
						node = node->getProxyNode();

					QPointF scenePos = node->scenePos();
					if (viewportRect.contains(scenePos) && (scenePos.y() < topY))
					{
						topmostVisibleNode = node;
						topY = scenePos.y();
					}
				}

				if (topmostVisibleNode)
					nearbyNode = topmostVisibleNode;
				else
				{
					VuoRendererNode *firstMarkedNode = *nodesToMark.begin();
					nearbyNode = (firstMarkedNode->getProxyNode()? firstMarkedNode->getProxyNode(): firstMarkedNode);
				}
			}
			else
			{
				VUserLog("Warning: no nearby node (no marked nodes).");
			}

			// If no nodes are known to be involved in the feedback loop, display the popover
			// in the center of the viewport.
			const QPoint offsetFromNode(0,10);
			QPoint popoverTopLeftInScene = (nearbyNode?
												(nearbyNode->scenePos().toPoint() +
												nearbyNode->getOuterNodeFrameBoundingRect().bottomLeft().toPoint() +
												offsetFromNode) :
												QPoint(viewportRect.center().x() - 0.5*errorPopover->sizeHint().width(),
													   viewportRect.center().y() - 0.5*errorPopover->sizeHint().height()));

			// If all nodes involved in the feedback loop are offscreen, display the popover at the edge
			// of the viewport closest to the feedback loop.
			const int margin = 5;
			popoverTopLeftInScene = (QPoint(fmin(popoverTopLeftInScene.x(), viewportRect.bottomRight().x() - errorPopover->sizeHint().width() - margin),
											fmin(popoverTopLeftInScene.y(), viewportRect.bottomRight().y() - errorPopover->sizeHint().height() - margin)));

			popoverTopLeftInScene = (QPoint(fmax(popoverTopLeftInScene.x(), viewportRect.topLeft().x() + margin),
											fmax(popoverTopLeftInScene.y(), viewportRect.topLeft().y() + margin)));

			QPoint popoverTopLeftInView = views()[0]->mapFromScene(popoverTopLeftInScene);
			QPoint popoverTopLeftGlobal = views()[0]->mapToGlobal(popoverTopLeftInView);

			errorPopover->move(popoverTopLeftGlobal);
			errorPopover->show();
			emit popoverDetached();
		}

		// Add error annotations to the composition.
		addItem(errorMark);
	}
	delete issues;
	delete potentialCable;

	errorMarkingUpdatesEnabled = true;
}

/**
 * Returns true if the most recent call to updateFeedbackErrors() found any errors.
 */
bool VuoEditorComposition::hasFeedbackErrors(void)
{
	return this->errorMark;
}

/**
 * Updates the renderings of any currently existing feedback error marks.
 */
void VuoEditorComposition::repaintFeedbackErrorMarks()
{
	if (hasFeedbackErrors())
		this->errorMark->updateErrorMarkPath();
}

/**
 * Creates @c runningComposition from the snapshot. Compiles and links @c runningComposition.
 *
 * @threadQueue{runCompositionQueue}
 */
void VuoEditorComposition::buildComposition(string compositionSnapshot, const set<string> &dependenciesUninstalled)
{
	try
	{
		emit buildStarted();

		if (! dependenciesUninstalled.empty())
		{
			vector<string> dependenciesRemovedVec(dependenciesUninstalled.begin(), dependenciesUninstalled.end());
			string dependenciesStr = VuoStringUtilities::join(dependenciesRemovedVec, " ");
			throw VuoException("Some modules that the composition needs were uninstalled: " + dependenciesStr);
		}

		delete runningComposition;
		runningComposition = NULL;
		runningComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionSnapshot, compiler);

		runningCompositionActiveDriver = getDriverForActiveProtocol();
		if (runningCompositionActiveDriver)
			runningCompositionActiveDriver->applyToComposition(runningComposition, compiler);

		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(this->getBase()->getMetadata()->getName(), "bc");
		string dir, file, ext;
		VuoFileUtilities::splitPath(compiledCompositionPath, dir, file, ext);
		linkedCompositionPath = dir + file + ".dylib";

		compiler->setShouldPotentiallyShowSplashWindow(false);

		VuoCompilerIssues *issues = new VuoCompilerIssues();
		compiler->compileComposition(runningComposition, compiledCompositionPath, true, issues);
		compiler->linkCompositionToCreateDynamicLibraries(compiledCompositionPath, linkedCompositionPath, runningCompositionLibraries.get());
		delete issues;

		remove(compiledCompositionPath.c_str());

		emit buildFinished("");
	}
	catch (VuoException &e)
	{
		delete runningComposition;
		runningComposition = NULL;

		emit buildFinished(e.what());
		throw;
	}
}

/**
 * Returns true if the composition is running.
 *
 * @threadQueue{runCompositionQueue}
 */
bool VuoEditorComposition::isRunningThreadUnsafe(void)
{
	return runner != NULL && ! stopRequested && ! runner->isStopped();
}

/**
 * Returns true if the composition is running.
 *
 * @threadNoQueue{runCompositionQueue}
 */
bool VuoEditorComposition::isRunning(void)
{
	__block bool running;
	dispatch_sync(runCompositionQueue, ^{
					  running = isRunningThreadUnsafe();
				  });
	return running;
}

/**
 * Asynchronously compiles, links, and runs a composition created from the snapshot.
 *
 * Assumes the composition is not already running.
 */
void VuoEditorComposition::run(string compositionSnapshot)
{
	VuoSubcompositionMessageRouter *subcompositionRouter = static_cast<VuoEditor *>(qApp)->getSubcompositionRouter();

	// If this is a subcomposition that was opened from a parent composition, now treat it as its own top-level composition.
	subcompositionRouter->unlinkSubcompositionFromNodeInSupercomposition(this);

	// If this is a subcomposition, tell the compiler to reload it as a node class and notify other compositions that depend on it.
	subcompositionRouter->applyToAllOtherCompositionsInstalledAsSubcompositions(this, ^void (VuoEditorComposition *subcomposition, string subcompositionPath)
	{
		compiler->overrideInstalledNodeClass(subcompositionPath, subcomposition->takeSnapshot());
	});

	// Tell this composition and all subcompositions opened from it to start displaying live debug info — part 1.
	subcompositionRouter->applyToAllLinkedCompositions(this, ^void (VuoEditorComposition *matchingComposition, string matchingCompositionIdentifier)
	{
		if (matchingComposition->showEventsMode)
			matchingComposition->beginDisplayingActivity();
	});

	stopRequested = false;
	dispatch_async(runCompositionQueue, ^{
		try
		{
		   runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();

		   buildComposition(compositionSnapshot);

		   string compositionLoaderPath = compiler->getCompositionLoaderPath();
		   string compositionSourceDir = getBase()->getDirectory();

		   runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compositionLoaderPath, linkedCompositionPath, runningCompositionLibraries, compositionSourceDir, true, true);
		   runner->setDelegate(this);
		   runner->setRuntimeChecking(VuoIsDebugEnabled());
		   runner->startPaused();
		   pid_t pid = runner->getCompositionPid();

		   // Tell this composition and all subcompositions opened from it to start displaying live debug info — part 2.
		   subcompositionRouter->applyToAllLinkedCompositions(this, ^void (VuoEditorComposition *matchingComposition, string matchingCompositionIdentifier)
		   {
			   if (matchingComposition->showEventsMode)
				   this->runner->subscribeToEventTelemetry(matchingCompositionIdentifier);

			   dispatch_sync(activePortPopoversQueue, ^{
				   for (auto i : matchingComposition->activePortPopovers)
				   {
					   string portID = i.first;
					   updateDataInPortPopoverFromRunningTopLevelComposition(matchingComposition, matchingCompositionIdentifier, portID);
				   }
			   });
		   });

		   runner->unpause();

		   // Focus the composition's windows (if any).
		   VuoFileUtilities::focusProcess(pid, true);
		}
		catch (...) { }
	});
}

/**
 * Asynchronously stops the running composition.
 *
 * Assumes the composition is running.
 */
void VuoEditorComposition::stop(void)
{
	stopRequested = true;
	dispatch_async(runCompositionQueue, ^{
					   if (runner && ! runner->isStopped())
					   {
						   runner->stop();
						   runner->waitUntilStopped();
					   }
					   delete runner;
					   runner = NULL;

					   linkedCompositionPath = "";

					   runningCompositionLibraries = nullptr;  // release shared_ptr

					   delete runningComposition;
					   runningComposition = NULL;

					   emit stopFinished();
				   });

	VuoSubcompositionMessageRouter *subcompositionRouter = static_cast<VuoEditor *>(qApp)->getSubcompositionRouter();

	// Tell this composition and all subcompositions opened from it to stop display live debug info.
	subcompositionRouter->applyToAllLinkedCompositions(this, ^void (VuoEditorComposition *matchingComposition, string matchingCompositionIdentifier)
	{
		if (matchingComposition->showEventsMode)
			matchingComposition->stopDisplayingActivity();

		dispatch_sync(activePortPopoversQueue, ^{
			for (auto i : matchingComposition->activePortPopovers)
			{
				VuoPortPopover *popover = i.second;
				popover->setCompositionRunning(false);
			}
		});
	});
}

/**
 * Asynchronously replaces the running composition (if any) with a composition created from @c newCompositionSnapshot.
 *
 * @param oldCompositionSnapshot A snapshot of the currently running composition (which may include minor changes made
 *     since the composition was last run/updated, for which the running composition did not need to be updated).
 * @param newCompositionSnapshot A snapshot of the composition to run.
 * @param diffInfo Mappings from the old composition to the new composition. This function becomes responsible for destroying it.
 * @param dependenciesUninstalled Module keys of dependencies of the composition that have been uninstalled. This may include dependencies
 *     of dependencies, which would not be detected by @ref VuoCompilerComposition::check. If this is not empty, the composition
 *     will stop with an error.
 */
void VuoEditorComposition::updateRunningComposition(string oldCompositionSnapshot, string newCompositionSnapshot,
													VuoCompilerCompositionDiff *diffInfo, set<string> dependenciesUninstalled)
{
	if (! diffInfo)
		diffInfo = new VuoCompilerCompositionDiff();

	dispatch_async(runCompositionQueue, ^{
					   if (isRunningThreadUnsafe())
					   {
						   try
						   {
							   foreach (string moduleKey, diffInfo->getModuleKeysReplaced())
							   {
								   runningCompositionLibraries->enqueueLibraryContainingDependencyToUnload(moduleKey);
							   }

							   string oldBuiltCompositionSnapshot = oldCompositionSnapshot;
							   VuoCompilerDriver *previouslyActiveDriver = runningCompositionActiveDriver;
							   if (previouslyActiveDriver)
							   {
								   VuoCompilerComposition *oldBuiltComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(oldCompositionSnapshot, compiler);
								   previouslyActiveDriver->applyToComposition(oldBuiltComposition, compiler);
								   oldBuiltCompositionSnapshot = oldBuiltComposition->getGraphvizDeclaration(getActiveProtocol());
							   }

							   buildComposition(newCompositionSnapshot, dependenciesUninstalled);

							   string compositionDiff = diffInfo->diff(oldBuiltCompositionSnapshot, runningComposition, compiler);
							   runner->replaceComposition(linkedCompositionPath, compositionDiff);
						   }
						   catch (exception &e)
						   {
							   VUserLog("Composition stopped itself: %s", e.what());
							   emit compositionStoppedItself();
						   }
						   catch (...)
						   {
							   VUserLog("Composition stopped itself.");
							   emit compositionStoppedItself();
						   }
					   }
					   else
					   {
						   dispatch_async(dispatch_get_main_queue(), ^{
							   updateCompositionsThatContainThisSubcomposition(newCompositionSnapshot);
						   });
					   }

					   delete diffInfo;
				   });
}

/**
 * If this is a subcomposition, reloads it so that the node library and all compositions containing instances
 * of it see the current (not-yet-saved) state of the subcomposition.
 */
void VuoEditorComposition::updateCompositionsThatContainThisSubcomposition(string newCompositionSnapshot)
{
	void (^reloadSubcompositionIfUnsaved)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *currComposition, string compositionPath)
	{
		compiler->overrideInstalledNodeClass(compositionPath, newCompositionSnapshot);
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyIfInstalledAsSubcomposition(this, reloadSubcompositionIfUnsaved);
}

/**
 * If the composition is running, tells the running composition to synchronize the constant value for the
 * internal port with the provided ID to that of the associated port in the stored composition.
 */
void VuoEditorComposition::syncInternalPortConstantInRunningComposition(string runningPortID)
{
	string constant;
	identifierCache->doForPortWithIdentifier(runningPortID, [&constant](VuoPort *port) {
		if (port->hasCompiler() && port->hasRenderer())
			constant = port->getRenderer()->getConstantAsString();
	});

	// Live-update the top-level composition, which may be either the composition itself or a supercomposition.
	void (^updateRunningComposition)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		if (this == topLevelComposition)
		{
			dispatch_async(runCompositionQueue, ^{
				if (isRunningThreadUnsafe())
				{
					json_object *constantObject = json_tokener_parse(constant.c_str());
					runner->setInputPortValue(thisCompositionIdentifier, runningPortID, constantObject);
				}
			});
		}
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, updateRunningComposition);

	// If this is a subcomposition, live-update all other top-level compositions that contain it.
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyIfInstalledAsSubcomposition(this, ^(VuoEditorComposition *currComposition, string subcompositionPath)
	{
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToAllOtherTopLevelCompositions(this, ^(VuoEditorComposition *topLevelComposition)
		{
			topLevelComposition->updateInternalPortConstantInSubcompositionInstances(subcompositionPath, runningPortID, constant);
		});
	});
}

/**
 * If the composition is running, tells the running composition to synchronize the constant value for the
 * published port of the provided name to that of the associated port in the stored composition.
 */
void VuoEditorComposition::syncPublishedPortConstantInRunningComposition(string portName)
{
	VuoPublishedPort *port = getBase()->getPublishedInputPortWithName(portName);
	if (!(port && port->hasCompiler()))
		return;

	string constant = dynamic_cast<VuoCompilerPublishedPort *>(port->getCompiler())->getInitialValue();
	updatePublishedInputPortConstantInRunningComposition(port, constant);
}

/**
 * If the composition is running, tells the composition to set a new constant value on the port.
 */
void VuoEditorComposition::updateInternalPortConstantInRunningComposition(VuoCompilerInputEventPort *port, string constant)
{
	string runningPortIdentifier = identifierCache->getIdentifierForPort(port->getBase());
	if (runningPortIdentifier.empty())
		return;

	// Live-update the top-level composition, which may be either the composition itself or a supercomposition.
	void (^updateRunningComposition)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		if (this == topLevelComposition)
		{
			dispatch_async(runCompositionQueue, ^{
				if (isRunningThreadUnsafe())
				{
					json_object *constantObject = json_tokener_parse(constant.c_str());
					runner->setInputPortValue(thisCompositionIdentifier, runningPortIdentifier, constantObject);
				}
			});
		}
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, updateRunningComposition);

	// If this is a subcomposition, live-update all other top-level compositions that contain it.
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyIfInstalledAsSubcomposition(this, ^(VuoEditorComposition *currComposition, string subcompositionPath)
	{
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToAllOtherTopLevelCompositions(this, ^(VuoEditorComposition *topLevelComposition)
		{
			topLevelComposition->updateInternalPortConstantInSubcompositionInstances(subcompositionPath, runningPortIdentifier, constant);
		});
	});
}

/**
 * If the composition is running and contains one or more instances of the subcomposition whose source code is at @a subcompositionPath,
 * tells the composition to set a new constant value on the port in each instance of the subcomposition.
 */
void VuoEditorComposition::updateInternalPortConstantInSubcompositionInstances(string subcompositionPath, string portIdentifier, string constant)
{
	dispatch_async(runCompositionQueue, ^{
					   if (isRunningThreadUnsafe())
					   {
						   json_object *constantObject = json_tokener_parse(constant.c_str());
						   set<string> subcompositionIdentifiers = moduleManager->findInstancesOfNodeClass(subcompositionPath);
						   foreach (string subcompositionIdentifier, subcompositionIdentifiers)
						   {
							   runner->setInputPortValue(subcompositionIdentifier, portIdentifier, constantObject);
						   }
					   }
				   });
}

/**
 * If the composition is running, tells the composition to set a new constant value on the published input port.
 */
void VuoEditorComposition::updatePublishedInputPortConstantInRunningComposition(VuoPublishedPort *port, string constant)
{
	dispatch_async(runCompositionQueue, ^{
					   if (isRunningThreadUnsafe())
					   {
						   VuoRunner::Port *publishedPort = runner->getPublishedInputPortWithName(port->getClass()->getName());
						   if (publishedPort)
						   {
							   json_object *constantObject = json_tokener_parse(constant.c_str());
							   map<VuoRunner::Port *, json_object *> m;
							   m[publishedPort] = constantObject;
							   runner->setPublishedInputPortValues(m);
							   json_object_put(constantObject);
						   }
					   }
				   });
}


/**
 * Returns the context menu's action that deletes a component.
 */
QAction * VuoEditorComposition::getContextMenuDeleteSelectedAction()
{
	return contextMenuDeleteSelected;
}

/**
 * Returns a "Tint" submenu for the provided parent menu.
 */
QMenu * VuoEditorComposition::getContextMenuTints(QMenu *parent)
{
	// Workaround for a bug in Qt 5.1.0-beta1 (https://b33p.net/kosada/node/5096).
	// For now, this recreates the context menu rather than accessing a data member.
	QMenu *contextMenuTints = new QMenu(parent);
	contextMenuTints->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	contextMenuTints->setTitle(tr("Tint"));
	foreach (QAction *tintAction, contextMenuTintActions)
		contextMenuTints->addAction(tintAction);
	contextMenuTints->insertSeparator(contextMenuTintActions.last());

	return contextMenuTints;
}

/**
 * Expands the "Change (Node) To" menu to include additional matches, and redisplays the menu.
 */
void VuoEditorComposition::expandChangeNodeMenu()
{
	QAction *sender = (QAction *)QObject::sender();
	VuoRendererNode *node = sender->data().value<VuoRendererNode *>();

	// If the menu hasn't been expanded previously, expand it enough now to fill
	// the available vertical screenspace.
	int currentMatchesListed = contextMenuChangeNode->actions().size()-1; // -1 to account for the "More…" row
	if (currentMatchesListed <= initialChangeNodeSuggestionCount)
	{
		int availableVerticalSpace = QApplication::desktop()->availableGeometry(VuoEditorWindow::getMostRecentActiveEditorWindow()).height();
		int verticalSpacePerItem = 21; // menu row height in pixels
		// Estimate the number of matches that will fit within the screen without scrolling:
		// -1 to account for a possible extra "More…" row;
		// -1 wiggle room to match observations
		int targetMatches = availableVerticalSpace/verticalSpacePerItem - 2;

		populateChangeNodeMenu(contextMenuChangeNode, node, targetMatches);
	}

	// If the menu has already been expanded once, don't impose any cap on listed matches this time.
	else
		populateChangeNodeMenu(contextMenuChangeNode, node, 0);

	contextMenuChangeNode->exec();
}

/**
 * Populates the "Change To" menu for the provided @c node with the requested number of matches
 * if available, or however many non-zero-scoring matches are available if fewer.
 * If a @c matchLimit less than 1 is specified, returns all available matches.
 */
void VuoEditorComposition::populateChangeNodeMenu(QMenu *menu, VuoRendererNode *node, int matchLimit)
{
	menu->clear();

	if (!node)
		return;

	map<string, VuoCompilerNodeClass *> nodeClassesMap = compiler->getNodeClasses();
	vector<VuoCompilerNodeClass *> loadedNodeClasses;
	for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClassesMap.begin(); i != nodeClassesMap.end(); ++i)
		loadedNodeClasses.push_back(i->second);
	VuoNodeLibrary::cullHiddenNodeClasses(loadedNodeClasses);

	vector<string> bestMatches;
	map<string, double> matchScores;
	matchScores[""] = 0;

	int targetMatchCount = (matchLimit > 0? matchLimit : loadedNodeClasses.size());
	for (int i = 0; i < targetMatchCount; ++i)
		bestMatches.push_back("");

	// Maintain a priority queue with the @c targetMatchCount best matches.
	bool overflow = false;

	VuoNodeClass *nodeClass = node->getBase()->getNodeClass();
	string originalGenericNodeClassName;
	if (nodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler()))
		originalGenericNodeClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass->getCompiler())->getOriginalGenericNodeClassName();
	else
		originalGenericNodeClassName = nodeClass->getClassName();

	foreach (VuoCompilerNodeClass *loadedNodeClass, loadedNodeClasses)
	{
		string loadedNodeClassName = loadedNodeClass->getBase()->getClassName();
		if (loadedNodeClassName == originalGenericNodeClassName)
			continue;

		bool canSwapNondestructively = canSwapWithoutBreakingCables(node, loadedNodeClass->getBase());
		double matchScore = (canSwapNondestructively? calculateNodeSimilarity(nodeClass, loadedNodeClass->getBase()) : 0);
		int highestIndexWithCompetitiveScore = -1;
		for (int i = targetMatchCount-1; (i >= 0) && (highestIndexWithCompetitiveScore == -1); --i)
			if (matchScore <= matchScores[bestMatches[i] ])
				highestIndexWithCompetitiveScore = i;

		if (highestIndexWithCompetitiveScore < targetMatchCount-1)
		{
			if (matchScores[bestMatches[targetMatchCount-1] ] > 0)
				overflow = true;

			for (int j = targetMatchCount-2; j > highestIndexWithCompetitiveScore; --j)
				bestMatches[j+1] = bestMatches[j];

			bestMatches[highestIndexWithCompetitiveScore+1] = loadedNodeClassName;
			matchScores[loadedNodeClassName] = matchScore;
		}
	}

	for (int i = 0; i < targetMatchCount; ++i)
	{
		if (matchScores[bestMatches[i] ] > 0)
		{
			// Disambiguate between identical node titles using node class names.
			QString matchDisplayText = compiler->getNodeClass(bestMatches[i])->getBase()->getDefaultTitle().c_str();
			if (matchDisplayText == nodeClass->getDefaultTitle().c_str())
				matchDisplayText += QString(" (%1)").arg(bestMatches[i].c_str());

			QAction *changeAction = menu->addAction(matchDisplayText);

			QList<QVariant> currentNodeAndNewClass;
			currentNodeAndNewClass.append(QVariant::fromValue(node));
			currentNodeAndNewClass.append(bestMatches[i].c_str());
			changeAction->setData(QVariant(currentNodeAndNewClass));
			connect(changeAction, &QAction::triggered, this, &VuoEditorComposition::swapNode);
		}
	}

	if (overflow)
	{
		//: Appears at the bottom of the "Change Node" menu when there are more options than can fit onscreen.
		QAction *showMoreAction = menu->addAction(tr("More…"));
		showMoreAction->setData(QVariant::fromValue(node));
		connect(showMoreAction, &QAction::triggered, this, &VuoEditorComposition::expandChangeNodeMenu);
	}
}

/**
  * Returns a boolean indicating whether the provided node within the composition can be replaced
  * with a new node of class @c newNodeClass without breaking any cable connections.
  */
bool VuoEditorComposition::canSwapWithoutBreakingCables(VuoRendererNode *origNode, VuoNodeClass *newNodeClass)
{
	// Inventory required input port types (connected data inputs) in the node to be replaced.
	map<string, int> requiredInputs;
	bool inputEventSourceRequired = false;
	foreach (VuoRendererPort *inputPort, origNode->getInputPorts())
	{
		bool hasDrawerWithNoIncomingCables = false;
		bool hasDrawerWithNoIncomingDataCables = false;

		if (inputPort->getDataType() && inputPort->effectivelyHasConnectedDataCable(true))
		{
			VuoRendererInputDrawer *inputDrawer = inputPort->getAttachedInputDrawer();
			if (inputDrawer)
			{
				hasDrawerWithNoIncomingCables = true;
				hasDrawerWithNoIncomingDataCables = true;
				vector<VuoRendererPort *> childPorts = inputDrawer->getDrawerPorts();
				foreach (VuoRendererPort *childPort, childPorts)
				{
					if (childPort->getBase()->getConnectedCables(true).size() > 0)
						hasDrawerWithNoIncomingCables = false;
					if (childPort->effectivelyHasConnectedDataCable(true))
						hasDrawerWithNoIncomingDataCables = false;
				}
			}

			string typeKey = inputPort->getDataType()->getModuleKey();
			if (!hasDrawerWithNoIncomingDataCables)
			{
				// @todo https://b33p.net/kosada/node/16966, https://b33p.net/kosada/node/16967 :
				// Accommodate generic types.
				if (VuoGenericType::isGenericTypeName(typeKey))
					return false;

				requiredInputs[typeKey] = ((requiredInputs.find(typeKey) == requiredInputs.end())? 1 : requiredInputs[typeKey]+1);
			}
		}

		bool hasIncomingCables = (inputPort->getBase()->getConnectedCables(true).size() > 0);
		if (hasIncomingCables && !hasDrawerWithNoIncomingCables)
			inputEventSourceRequired = true;
	}

	// Inventory required output port types (connected data outputs) in the node to be replaced.
	map<string, int> requiredOutputs;
	bool outputEventSourceRequired = false;
	foreach (VuoRendererPort *outputPort, origNode->getOutputPorts())
	{
		if (outputPort->getDataType() && outputPort->effectivelyHasConnectedDataCable(true))
		{
			string typeKey = outputPort->getDataType()->getModuleKey();

			// @todo https://b33p.net/kosada/node/16966, https://b33p.net/kosada/node/16967 :
			// Accommodate generic types.
			if (VuoGenericType::isGenericTypeName(typeKey))
				return false;

			requiredOutputs[typeKey] = ((requiredOutputs.find(typeKey) == requiredOutputs.end())? 1 : requiredOutputs[typeKey]+1);
		}

		if (outputPort->getBase()->getConnectedCables(true).size() > 0)
			outputEventSourceRequired = true;
	}

	// Inventory available input port types in the candidate replacement node.
	bool inputEventSourceAvailable = false;
	map<string, int> availableInputs;
	foreach (VuoPortClass *inputPortClass, newNodeClass->getInputPortClasses())
	{
		VuoType *dataType = (inputPortClass->hasCompiler()?
								 static_cast<VuoCompilerPortClass *>(inputPortClass->getCompiler())->getDataVuoType() : NULL);
		if (dataType)
		{
			string typeKey = dataType->getModuleKey();
			availableInputs[typeKey] = ((availableInputs.find(typeKey) == availableInputs.end())? 1 : availableInputs[typeKey]+1);
		}
	}

	if (newNodeClass->getInputPortClasses().size() > VuoNodeClass::unreservedInputPortStartIndex)
		inputEventSourceAvailable = true;

	// Inventory available output port types in the candidate replacement node.
	bool outputEventSourceAvailable = false;
	map<string, int> availableOutputs;
	foreach (VuoPortClass *outputPortClass, newNodeClass->getOutputPortClasses())
	{
		VuoType *dataType = (outputPortClass->hasCompiler()?
								 static_cast<VuoCompilerPortClass *>(outputPortClass->getCompiler())->getDataVuoType() : NULL);
		if (dataType)
		{
			string typeKey = dataType->getModuleKey();
			availableOutputs[typeKey] = ((availableOutputs.find(typeKey) == availableOutputs.end())? 1 : availableOutputs[typeKey]+1);
		}
	}

	if (newNodeClass->getOutputPortClasses().size() > VuoNodeClass::unreservedOutputPortStartIndex)
		outputEventSourceAvailable = true;

	// Check whether the candidate replacement node meets input data requirements.
	for (std::map<string,int>::iterator it=requiredInputs.begin(); it!=requiredInputs.end(); ++it)
	{
		string typeKey = it->first;
		int typeRequiredCount = it->second;
		if (availableInputs[typeKey] < typeRequiredCount)
			return false;
	}

	// Check whether the candidate replacement node meets output data requirements.
	for (std::map<string,int>::iterator it=requiredOutputs.begin(); it!=requiredOutputs.end(); ++it)
	{
		string typeKey = it->first;
		int typeRequiredCount = it->second;
		if (availableOutputs[typeKey] < typeRequiredCount)
			return false;
	}

	if (inputEventSourceRequired && !inputEventSourceAvailable)
		return false;

	if (outputEventSourceRequired && !outputEventSourceAvailable)
		return false;

	return true;
}

/**
 * Returns true if the provided @c port is a specialized port that may, in its
 * current configuration within the composition, be reverted by the user to generic form.
 */
bool VuoEditorComposition::isPortCurrentlyRevertible(VuoRendererPort *port)
{
	// If this port is not a specialization of a formerly generic port, then it cannot be reverted.
	VuoCompilerNodeClass *nodeClass = port->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
	VuoCompilerSpecializedNodeClass *specializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(nodeClass);

	if (!specializedNodeClass)
		return false;

	VuoPortClass *portClass = port->getBase()->getClass();
	VuoGenericType *originalGenericType = dynamic_cast<VuoGenericType *>(specializedNodeClass->getOriginalPortType(portClass));
	if (!originalGenericType)
		return false;

	// If this port belongs to an attachment connected to a port that is not revertible, then
	// this port cannot be reverted, either.
	VuoRendererInputAttachment *attachment = dynamic_cast<VuoRendererInputAttachment *>(port->getUnderlyingParentNode());
	if (attachment)
	{
		VuoPort *hostPort = attachment->getUnderlyingHostPort();
		if (hostPort && (!isPortCurrentlyRevertible(hostPort->getRenderer())))
			return false;
	}

	return true;
}

/**
 * Publishes this composition's internal @c port under the provided @c name, if possible;
 * returns a pointer to the VuoRendererPublishedPort aliased to the internal port.
 * If the requested @c name is already taken by an existing external published port and
 * @c shouldAttemptMerge is true, the existing external port will accommodate the new internal port
 * provided that their types are compatible and the new port would not displace any previously
 * connected port. Otherwise, the newly published internal port will be published under a unique name
 * derived from the requested name, and with the @c type provided.
 *
 * @param port The internal port to be published.
 * @param forceEventOnlyPublication Forces an event-only cable to the published port.
 * @param name The name under which the port is to be published, if possible.
 * @param type The desired data type of the external published port to be created.
 * @param attemptMerge A boolean indicating whether the port should be published in association
 * with a pre-existing (rather than a newly created) external published port of the given name, if possible.
 * @param[out] mergePerformed A boolean indicating whether the port was published in association
 * with a pre-existing (rather than a newly created) external published port.
 */
VuoRendererPublishedPort * VuoEditorComposition::publishInternalPort(VuoPort *port, bool forceEventOnlyPublication, string name, VuoType *type, bool attemptMerge, bool *mergePerformed)
{
	string publishedPortName = ((! name.empty())?
									name :
									VuoRendererPort::sanitizePortName(port->getRenderer()->getPortNameToRenderWhenDisplayed().c_str()).toUtf8().constData());
	bool isPublishedInput = port->getRenderer()->getInput();
	VuoType *portType = port->getRenderer()->getDataType();
	VuoPublishedPort *publishedPort = NULL;

	// If merging is enabled:
	// Check whether this composition has a pre-existing externally visible published port
	// that has the requested name and type and that can accommodate the newly published internal port.
	// If so, add this internal port as a connected port for the existing alias.
	bool performedMerge = false;
	if (attemptMerge)
	{
		publishedPort = (isPublishedInput ?
							 getBase()->getPublishedInputPortWithName(publishedPortName) :
							 getBase()->getPublishedOutputPortWithName(publishedPortName));

		if (publishedPort && dynamic_cast<VuoRendererPublishedPort *>(publishedPort->getRenderer())->canAccommodateInternalPort(port->getRenderer(), forceEventOnlyPublication))
		{
			if (isPublishedInput && portType && type && !forceEventOnlyPublication)
			{
				VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( publishedPort->getCompiler() );
				updatePortConstant(static_cast<VuoCompilerInputEventPort *>(port->getCompiler()),
								   publishedInputPort->getInitialValue(),
								   false);
			}

			performedMerge = true;
		}
	}


	// Otherwise, create a new externally visible published port with a unique name derived from
	// the specified name, containing the current port as its lone connected internal port.
	if (! performedMerge)
	{
		publishedPort = static_cast<VuoPublishedPort *>(VuoCompilerPublishedPort::newPort(getUniquePublishedPortName(publishedPortName), type)->getBase());
		if (isPublishedInput && type)
		{
			VuoCompilerPublishedPort *publishedInputPort = static_cast<VuoCompilerPublishedPort *>( publishedPort->getCompiler() );
			publishedInputPort->setInitialValue(port->getRenderer()->getConstantAsString());
		}
	}

	addPublishedPort(publishedPort, isPublishedInput);

	VuoRendererPublishedPort *rendererPublishedPort = (publishedPort->hasRenderer()?
															  dynamic_cast<VuoRendererPublishedPort *>(publishedPort->getRenderer()) :
															  createRendererForPublishedPortInComposition(publishedPort, isPublishedInput));

	VuoCable *existingPublishedCable = port->getCableConnecting(publishedPort);

	if (! existingPublishedCable)
	{
		VuoCable *publishedCable = createPublishedCable(publishedPort, port, forceEventOnlyPublication);
		addCable(publishedCable);
	}

	if (mergePerformed != NULL)
		*mergePerformed = performedMerge;

	return rendererPublishedPort;
}

/**
 * Creates a published cable connecting published @c externalPort with
 * internal port @c internalPort (in whichever direction appropriate).
 */
VuoCable * VuoEditorComposition::createPublishedCable(VuoPort *externalPort, VuoPort *internalPort, bool forceEventOnlyPublication)
{
	VuoCable *publishedCable = NULL;
	bool creatingPublishedInputCable = internalPort->getRenderer()->getInput();

	if (creatingPublishedInputCable)
	{
		// If creating a published input cable, it will need to have an associated VuoCompilerCable.
		VuoPort *fromPort = externalPort;
		VuoNode *fromNode = this->publishedInputNode;

		VuoPort *toPort = internalPort;
		VuoNode *toNode = internalPort->getRenderer()->getUnderlyingParentNode()->getBase();

		publishedCable = (new VuoCompilerCable(NULL,
											  NULL,
											  toNode->getCompiler(),
											  (VuoCompilerPort *)(toPort->getCompiler())))->getBase();
		publishedCable->setFrom(fromNode, fromPort);
	}

	else
	{
		// If creating a published output cable, it will need to have an associated VuoCompilerCable
		// even though we don't currently construct a VuoCompilerNode for the published output node.
		VuoPort *fromPort = internalPort;
		VuoNode *fromNode = internalPort->getRenderer()->getUnderlyingParentNode()->getBase();

		VuoPort *toPort = externalPort;
		VuoNode *toNode = this->publishedOutputNode;

		publishedCable = (new VuoCompilerCable(fromNode->getCompiler(),
											  (VuoCompilerPort *)(fromPort->getCompiler()),
											  NULL,
											   NULL))->getBase();
		publishedCable->setTo(toNode, toPort);
	}

	if (forceEventOnlyPublication)
		publishedCable->getCompiler()->setAlwaysEventOnly(true);

	return publishedCable;
}

/**
 * Adds an active protocol for this composition, removing any previous active protocol.
 * Updates the `isProtocolPort` attribute for each published port affected by the change in protocol.
 *
 * If `useUndoStack` is `true`, pushes published port modifications onto the Undo stack. It is
 * expected (and enforced) that `useUndoStack` will be `true` if any published ports are
 * to be removed from the composition. A `useUndoStack` value of `false` would be expected
 * during the original creation of a protocol composition, but never in response to user changes.
 *
 * @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
 */
void VuoEditorComposition::addActiveProtocol(VuoProtocol *protocol, bool useUndoStack)
{
	vector<VuoPublishedPort *> publishedPortsToAdd;
	map<VuoPublishedPort *, string> publishedPortsToRename;

	// Remove the previously active protocol.
	VuoProtocol *previousActiveProtocol = this->activeProtocol;
	bool removingPreviousProtocol = previousActiveProtocol && (previousActiveProtocol != protocol);

	bool portChangesMadeDuringProtocolRemoval = false;
	if (removingPreviousProtocol)
		 portChangesMadeDuringProtocolRemoval = removeActiveProtocol(previousActiveProtocol, protocol);

	if (portChangesMadeDuringProtocolRemoval && !useUndoStack)
	{
		VUserLog("Warning: Unexpected combination: Removing protocol ports, but useUndoStack=false");
		useUndoStack = true;
	}

	// Add the newly active protocol.
	this->activeProtocol = protocol;
	if (!protocol)
		return;

	vector<pair<string, string> > protocolInputs = protocol->getInputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolInputs.begin(); i != protocolInputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		bool compositionHadCompatiblePort = false;
		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedInputPortWithName(portName);
		if (preexistingPublishedPort)
		{
			VuoType *preexistingType = static_cast<VuoCompilerPortClass *>(preexistingPublishedPort->getClass()->getCompiler())->getDataVuoType();

			bool portTypesMatch = ((preexistingType && (preexistingType->getModuleKey() == portType)) ||
							   (!preexistingType && (portType == "")));
			if (portTypesMatch)
			{
				compositionHadCompatiblePort = true;
				preexistingPublishedPort->setProtocolPort(true);
			}
			else
			{
				compositionHadCompatiblePort = false;
				publishedPortsToRename[preexistingPublishedPort] = getUniquePublishedPortName(getNonProtocolVariantForPortName(portName));
			}
		}

		if (!compositionHadCompatiblePort)
		{
			VuoCompilerType *ctype = compiler->getType(portType);
			VuoType *type = (ctype? ctype->getBase() : NULL);
			VuoPublishedPort *publishedPort = static_cast<VuoPublishedPort *>(VuoCompilerPublishedPort::newPort(getUniquePublishedPortName(portName), type)->getBase());
			publishedPort->setProtocolPort(true);

			if (!useUndoStack)
				addPublishedPort(publishedPort, true);
			else
				publishedPortsToAdd.push_back(publishedPort);

			createRendererForPublishedPortInComposition(publishedPort, true);
		}
	}

	vector<pair<string, string> > protocolOutputs = protocol->getOutputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolOutputs.begin(); i != protocolOutputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		bool compositionHadCompatiblePort = false;
		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedOutputPortWithName(portName);
		if (preexistingPublishedPort)
		{
			VuoType *preexistingType = static_cast<VuoCompilerPortClass *>(preexistingPublishedPort->getClass()->getCompiler())->getDataVuoType();
			bool portTypesMatch = ((preexistingType && (preexistingType->getModuleKey() == portType)) ||
							   (!preexistingType && (portType == "")));
			if (portTypesMatch)
			{
				compositionHadCompatiblePort = true;
				preexistingPublishedPort->setProtocolPort(true);
			}
			else
			{
				compositionHadCompatiblePort = false;
				publishedPortsToRename[preexistingPublishedPort] = getUniquePublishedPortName(getNonProtocolVariantForPortName(portName));
			}
		}

		if (!compositionHadCompatiblePort)
		{
			VuoCompilerType *ctype = compiler->getType(portType);
			VuoType *type = (ctype? ctype->getBase() : NULL);
			VuoPublishedPort *publishedPort = static_cast<VuoPublishedPort *>(VuoCompilerPublishedPort::newPort(getUniquePublishedPortName(portName), type)->getBase());
			publishedPort->setProtocolPort(true);

			if (!useUndoStack)
				addPublishedPort(publishedPort, false);
			else
				publishedPortsToAdd.push_back(publishedPort);

			createRendererForPublishedPortInComposition(publishedPort, false);
		}
	}

	if (useUndoStack)
	{
		bool undoStackMacroBegunAlready = (removingPreviousProtocol && portChangesMadeDuringProtocolRemoval);
		if (!publishedPortsToRename.empty() || !publishedPortsToAdd.empty() || undoStackMacroBegunAlready)
		{
			set<VuoPublishedPort *> publishedPortsToRemove;
			bool beginUndoStackMacro = !undoStackMacroBegunAlready;
			bool endUndoStackMacro = true;
			emit protocolPortChangesRequested(publishedPortsToRename, publishedPortsToRemove, publishedPortsToAdd, beginUndoStackMacro, endUndoStackMacro);
		}
	}

	emit activeProtocolChanged();
}

/**
 * Returns a suggested new name for a published port that complies with a protocol that is
 * now being deactivated. Re-naming the port will prevent the composition from being deemed
 * to comply with that protocol any longer.
 *
 * The returned name is not guaranteed to be unique within the composition.
 */
string VuoEditorComposition::getNonProtocolVariantForPortName(string portName)
{
	string modifiedPortName = portName;
	if (modifiedPortName.length() > 0)
		modifiedPortName[0] = toupper(modifiedPortName[0]);
	modifiedPortName = "some" + modifiedPortName;

	return modifiedPortName;
}

/**
 * Unsets the active `protocol` for this composition, pushing any necessary changes to
 * published ports onto the Undo stack.
 *
 * Also makes the necessary changes to the composition's set of published ports to ensure that
 * the composition will no longer be deemed compliant with the removed protocol:
 * - Protocol ports with no internally connected ports will be removed.
 * - If no protocol ports are eligible to be removed, all protocol ports will instead be renamed.
 *
 * If the removed protocol is to be replaced with a `replacementProtocol`, protocol
 * ports common to both protocols are left unmodified. This function does not actually activate
 * the new protocol.
 *
 * Returns a boolean indicating whether any changes to the composition's published ports were in fact made.
 *
 * @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
 */
bool VuoEditorComposition::removeActiveProtocol(VuoProtocol *protocol, VuoProtocol *replacementProtocol)
{
	removeActiveProtocolWithoutModifyingPorts(protocol);

	set<VuoPublishedPort *> publishedPortsToRemove;
	map<VuoPublishedPort *, string> publishedPortsToRename;

	vector<pair<string, string> > protocolInputs = protocol->getInputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolInputs.begin(); i != protocolInputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedInputPortWithName(portName);
		if (preexistingPublishedPort)
		{
			bool portCompatibleAcrossProtocolTransition = false;
			if (replacementProtocol)
			{
				vector<pair<string, string> > protocolInputs = replacementProtocol->getInputPortNamesAndTypes();
				for (vector<pair<string, string> >::iterator i = protocolInputs.begin(); i != protocolInputs.end() && !portCompatibleAcrossProtocolTransition; ++i)
				{
					string replacementPortName = i->first;
					string replacementPortType = i->second;

					if ((portName == replacementPortName) && (portType == replacementPortType))
						portCompatibleAcrossProtocolTransition = true;
				}
			}

			if (preexistingPublishedPort->getConnectedCables(true).empty())
				publishedPortsToRemove.insert(preexistingPublishedPort);
			else if (!portCompatibleAcrossProtocolTransition)
				publishedPortsToRename[preexistingPublishedPort] = getUniquePublishedPortName(getNonProtocolVariantForPortName(portName));
		}
	}

	vector<pair<string, string> > protocolOutputs = protocol->getOutputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolOutputs.begin(); i != protocolOutputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedOutputPortWithName(portName);
		if (preexistingPublishedPort)
		{
			bool portCompatibleAcrossProtocolTransition = false;
			if (replacementProtocol)
			{
				vector<pair<string, string> > protocolOutputs = replacementProtocol->getOutputPortNamesAndTypes();
				for (vector<pair<string, string> >::iterator i = protocolOutputs.begin(); i != protocolOutputs.end() && !portCompatibleAcrossProtocolTransition; ++i)
				{
					string replacementPortName = i->first;
					string replacementPortType = i->second;

					if ((portName == replacementPortName) && (portType == replacementPortType))
						portCompatibleAcrossProtocolTransition = true;
				}
			}

			if (preexistingPublishedPort->getConnectedCables(true).empty())
				publishedPortsToRemove.insert(preexistingPublishedPort);
			else if (!portCompatibleAcrossProtocolTransition)
				publishedPortsToRename[preexistingPublishedPort] = getUniquePublishedPortName(getNonProtocolVariantForPortName(portName));
		}
	}

	// If we are removing any ports, the composition will no longer be deemed to adhere to the
	// removed protocol when it is re-opened, so there is no need to re-name any other ports.
	if (!publishedPortsToRemove.empty())
		publishedPortsToRename.clear();

	bool portChangesRequired = (!publishedPortsToRename.empty() || !publishedPortsToRemove.empty());
	if (portChangesRequired)
	{
		vector<VuoPublishedPort *> publishedPortsToAdd;
		bool beginUndoStackMacro = true;
		bool endUndoStackMacro = !replacementProtocol;
		emit protocolPortChangesRequested(publishedPortsToRename, publishedPortsToRemove, publishedPortsToAdd, beginUndoStackMacro, endUndoStackMacro);
	}

	emit activeProtocolChanged();

	return portChangesRequired;
}

/**
 * Deactivates the provided protocol for this composition, updating the `isProtocolPort` attribute
 * of each affected port.
 *
 * @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
 */
void VuoEditorComposition::removeActiveProtocolWithoutModifyingPorts(VuoProtocol *protocol)
{
	if ((activeProtocol != protocol) || !activeProtocol)
		return;

	activeProtocol = NULL;

	vector<pair<string, string> > protocolInputs = protocol->getInputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolInputs.begin(); i != protocolInputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedInputPortWithName(portName);
		if (preexistingPublishedPort)
			preexistingPublishedPort->setProtocolPort(false);
	}

	vector<pair<string, string> > protocolOutputs = protocol->getOutputPortNamesAndTypes();
	for (vector<pair<string, string> >::iterator i = protocolOutputs.begin(); i != protocolOutputs.end(); ++i)
	{
		string portName = i->first;
		string portType = i->second;

		VuoPublishedPort *preexistingPublishedPort = getBase()->getPublishedOutputPortWithName(portName);
		if (preexistingPublishedPort)
			preexistingPublishedPort->setProtocolPort(false);
	}

	emit activeProtocolChanged();
}

/**
 * Returns the active protocol for this composition.
 * @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
 */
VuoProtocol * VuoEditorComposition::getActiveProtocol()
{
	return activeProtocol;
}

/**
 * Returns the driver for the currently active protocol, or NULL if there is no
 * active protocol or there is no driver available for the active protocol.
 */
VuoCompilerDriver * VuoEditorComposition::getDriverForActiveProtocol()
{
	if (!activeProtocol)
		return NULL;

	return static_cast<VuoEditor *>(qApp)->getBuiltInDriverForProtocol(activeProtocol);
}

/**
 * Adds an existing VuoPublishedPort as one of this composition's published ports.
 */
void VuoEditorComposition::addPublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, bool shouldUpdateUi)
{
	VuoRendererComposition::addPublishedPort(publishedPort, isPublishedInput, compiler);

	identifierCache->addPublishedPortToCache(publishedPort);

	if (shouldUpdateUi)
		emit publishedPortModified();
}

/**
 * Removes a published input or output VuoRendererPublishedPort from the list
 * of published ports associated with this composition.
 *
 * @return The index within the list of published input port output ports at which the port was located, or -1 if not located.
 */
int VuoEditorComposition::removePublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, bool shouldUpdateUi)
{
	// @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	if (shouldUpdateUi && publishedPort->isProtocolPort())
		removeActiveProtocolWithoutModifyingPorts(getActiveProtocol());

	int removalResult = VuoRendererComposition::removePublishedPort(publishedPort, isPublishedInput, compiler);

	if (shouldUpdateUi)
		emit publishedPortModified();

	return removalResult;
}

/**
 * Sets the name of the provided @c publishedPort to @c name; updates the composition's
 * published pseudo-node and connected published cables accordingly.
 */
void VuoEditorComposition::setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name)
{
	// @todo: Allow multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	if (dynamic_cast<VuoPublishedPort *>(publishedPort->getBase())->isProtocolPort())
		removeActiveProtocolWithoutModifyingPorts(getActiveProtocol());

	VuoRendererComposition::setPublishedPortName(publishedPort, name, compiler);

	identifierCache->addPublishedPortToCache( static_cast<VuoPublishedPort *>(publishedPort->getBase()) );

	emit publishedPortModified();
}

/**
 * Highlights all ports and dropboxes eligible to be connected to the unattached end of the input
 * @c cable, with or without a typeconverter.
 * Eligible endpoints may include:
 *   Internal ports (belonging to nodes rendered on the canvas).
 *   Published port dropboxes within the "Published Port" sidebars.
 */
void VuoEditorComposition::highlightEligibleEndpointsForCable(VuoCable *cable)
{
	bool eventOnlyConnection = cable->hasRenderer() && !cable->getRenderer()->effectivelyCarriesData();
	VuoRendererPort *fixedPort = NULL;

	if ((cable->getFromNode()) && (cable->getFromPort()) && (! (cable->getToNode())) & (! (cable->getToPort())))
		fixedPort = cable->getFromPort()->getRenderer();

	else if ((! (cable->getFromNode())) && (! (cable->getFromPort())) && (cable->getToNode()) && (cable->getToPort()))
		fixedPort = cable->getToPort()->getRenderer();

	if (fixedPort)
	{
		highlightInternalPortsConnectableToPort(fixedPort, cable->getRenderer());
		emit highlightPublishedSidebarDropLocationsRequested(fixedPort, eventOnlyConnection);
	}
}

/**
 * Highlights all ports on the canvas that are eligible to be connected by @c cable
 * to or from the input @c port, with or without a typeconverter.
 * Helper function for VuoEditorComposition::highlightEligibleEndpointsForCable(VuoCable *cable).
 */
void VuoEditorComposition::highlightInternalPortsConnectableToPort(VuoRendererPort *port, VuoRendererCable *cable)
{
	QList<QGraphicsItem *> compositionComponents = items();

	// Cache the (fairly time-consuming) computation of eligibility highlighting for each port so we can reuse the result.
	map<VuoRendererPort *, VuoRendererColors::HighlightType> highlightForPort;

	for (QGraphicsItem *compositionComponent : compositionComponents)
	{
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
		if (rn)
		{
			// Check for eligible internal input ports.
			vector<VuoPort *> inputPorts = rn->getBase()->getInputPorts();
			for (VuoPort *inputPort : inputPorts)
				highlightForPort[inputPort->getRenderer()] =
						updateEligibilityHighlightingForPort(inputPort->getRenderer(), port, !cable->effectivelyCarriesData());

			// Check for eligible internal output ports.
			vector<VuoPort *> outputPorts = rn->getBase()->getOutputPorts();
			for (VuoPort *outputPort : outputPorts)
				highlightForPort[outputPort->getRenderer()] =
						updateEligibilityHighlightingForPort(outputPort->getRenderer(), port, !cable->effectivelyCarriesData());
		}
	}

	for (QGraphicsItem *compositionComponent : compositionComponents)
	{
		// Fade out cables that aren't relevant to the current cable drag.
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(compositionComponent);
		if (rc && rc != cable)
		{
			QGraphicsItem::CacheMode normalCacheMode = rc->cacheMode();
			rc->setCacheMode(QGraphicsItem::NoCache);
			rc->updateGeometry();

			VuoPort *otherCablePort = port->getInput()
				? rc->getBase()->getFromPort()
				: rc->getBase()->getToPort();

			VuoRendererColors::HighlightType highlight = otherCablePort ?
															 highlightForPort[otherCablePort->getRenderer()] :
															 VuoRendererColors::noHighlight;
\
			// Don't apply extra highlighting to compatible, already-connected cables.
			if (highlight == VuoRendererColors::standardHighlight)
				highlight = VuoRendererColors::noHighlight;

			rc->setEligibilityHighlight(highlight);

			rc->setCacheMode(normalCacheMode);
		}
	}

	// Now that the ports and cables have been highlighted, also highlight the nodes based on those results.
	for (QGraphicsItem *compositionComponent : compositionComponents)
		updateEligibilityHighlightingForNode(dynamic_cast<VuoRendererNode *>(compositionComponent));
}

/**
 * Updates the eligibility highlighting of the @a portToHighlight given that a cable with event-only status specified by
 * @a eventOnlyConnection is being drawn from @a fixedPort.
 */
VuoRendererColors::HighlightType VuoEditorComposition::updateEligibilityHighlightingForPort(VuoRendererPort *portToHighlight,
																							VuoRendererPort *fixedPort,
																							bool eventOnlyConnection)
{
	QGraphicsItem::CacheMode normalCacheMode = portToHighlight->cacheMode();
	portToHighlight->setCacheMode(QGraphicsItem::NoCache);

	portToHighlight->updateGeometry();

	VuoRendererColors::HighlightType highlight = getEligibilityHighlightingForPort(portToHighlight, fixedPort, eventOnlyConnection);

	portToHighlight->setEligibilityHighlight(highlight);
	VuoRendererTypecastPort *typecastPortToHighlight = dynamic_cast<VuoRendererTypecastPort *>(portToHighlight);
	if (typecastPortToHighlight)
		typecastPortToHighlight->getReplacedPort()->setEligibilityHighlight(highlight);

	portToHighlight->setCacheMode(normalCacheMode);

	if (typecastPortToHighlight)
		updateEligibilityHighlightingForPort(typecastPortToHighlight->getChildPort(), fixedPort, eventOnlyConnection);

	return highlight;
}

/**
 * Returns the appropriate eligibility highlighting for the specified @c portToHighlight given that
 * the @c fixedPort will be at the other end of the cable connection.
 *
 * @param portToHighlight The port to have connection eligibility highlighting applied.
 * @param fixedPort The port already selected for connection, and against which eligibility is to be checked.
 * @param eventOnlyConnection If true, determines eligibility as if the ports will be connected with a cable
 * that is event-only regardless of the data-carrying status of the ports.
 */
VuoRendererColors::HighlightType VuoEditorComposition::getEligibilityHighlightingForPort(VuoRendererPort *portToHighlight, VuoRendererPort *fixedPort, bool eventOnlyConnection)
{
	// Determine whether the port endpoints are internal canvas ports or external published sidebar ports.
	VuoRendererPublishedPort *fixedExternalPublishedPort = dynamic_cast<VuoRendererPublishedPort *>(fixedPort);
	VuoRendererPublishedPort *externalPublishedPortToHighlight = dynamic_cast<VuoRendererPublishedPort *>(portToHighlight);

	VuoRendererPort *fromPort;
	VuoRendererPort *toPort;
	bool forwardConnection;
	if (fixedPort->getOutput())
	{
		fromPort = fixedPort;
		toPort = portToHighlight;
		forwardConnection = true;
	}
	else
	{
		fromPort = portToHighlight;
		toPort = fixedPort;
		forwardConnection = false;
	}

	bool directConnectionPossible;

	// Temporarily disallow direct cable connections between published inputs and published outputs.
	// @todo: Allow for https://b33p.net/kosada/node/7756 .
	if (fixedExternalPublishedPort && externalPublishedPortToHighlight) // both ports are external published sidebar ports
		directConnectionPossible = false;
	else if (fixedExternalPublishedPort && !externalPublishedPortToHighlight) // only the fixed port is an external published sidebar port
		directConnectionPossible = fixedExternalPublishedPort->isCompatibleAliasWithSpecializationForInternalPort(portToHighlight, eventOnlyConnection);
	else if (!fixedExternalPublishedPort && externalPublishedPortToHighlight) // only the port to highlight is an external published sidebar port
		directConnectionPossible = externalPublishedPortToHighlight->isCompatibleAliasWithSpecializationForInternalPort(fixedPort, eventOnlyConnection);
	else // both ports are internal canvas ports
		directConnectionPossible = fromPort->canConnectDirectlyWithSpecializationTo(toPort, eventOnlyConnection);

	VuoRendererColors::HighlightType highlight;
	if (directConnectionPossible)
		highlight = VuoRendererColors::standardHighlight;
	else if (!findBridgingSolutions(fromPort, toPort, forwardConnection).empty())
		highlight = VuoRendererColors::subtleHighlight;
	else if (fixedPort == portToHighlight)
		highlight = VuoRendererColors::noHighlight;
	else
		highlight = VuoRendererColors::ineligibleHighlight;

	return highlight;
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from this port to @c toPort, taking into account
 * the respective port types and the possibility that one
 * specialized port may be respecialized in preparation for the connection.
 *
 * Returns false if the respecialization will break connections between
 * the specialized port's generic network and connected static ports or if it
 * will break connections within the generic network.
 *
 * Convenience function for VuoEditorComposition::canConnectDirectlyWithRespecializationNondestructively(
 * VuoRendererPort *fromPort, VuoRendererPort *toPort, bool eventOnlyConnection, bool forwardConnection,
 * VuoRendererPort **portToRespecialize, string &respecializedTypeName), for use when only the returned boolean
 * and none of the other output parameter values are needed.
 */
bool VuoEditorComposition::canConnectDirectlyWithRespecializationNondestructively(VuoRendererPort *fromPort,
																				  VuoRendererPort *toPort,
																				  bool eventOnlyConnection,
																				  bool forwardConnection)
{
	VuoRendererPort *portToRespecialize = NULL;
	string respecializedTypeName = "";

	return canConnectDirectlyWithRespecializationNondestructively(fromPort,
																  toPort,
																  eventOnlyConnection,
																  forwardConnection,
																  &portToRespecialize,
																  respecializedTypeName);
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from this port to @c toPort, taking into account
 * the respective port types and the possibility that one
 * specialized port may be respecialized in preparation for the connection.
 *
 * Returns false if the respecialization will break connections between
 * the specialized port's generic network and connected static ports or if it
 * will break connections within the generic network.
 */
bool VuoEditorComposition::canConnectDirectlyWithRespecializationNondestructively(VuoRendererPort *fromPort,
																				  VuoRendererPort *toPort,
																				  bool eventOnlyConnection,
																				  bool forwardConnection,
																				  VuoRendererPort **portToRespecialize,
																				  string &respecializedTypeName)
{
	*portToRespecialize = NULL;
	respecializedTypeName = "";

	bool canConnectWithRespecialization = canConnectDirectlyWithRespecialization(fromPort,
																				 toPort,
																				 eventOnlyConnection,
																				 forwardConnection,
																				 portToRespecialize,
																				 respecializedTypeName);
	if (!canConnectWithRespecialization)
		return false;

	if (canConnectWithRespecialization && !portToRespecialize)
		return true;

	bool nondestructive = portCanBeUnspecializedNondestructively((*portToRespecialize)->getBase());
	if (!nondestructive)
	{
		*portToRespecialize = NULL;
		respecializedTypeName = "";
	}
	return nondestructive;
}

/**
 * Returns false if unspecialization of the provided port will break connections between
 * the port's generic network and connected static ports or if it
 * will break connections within the generic network.
 */
bool VuoEditorComposition::portCanBeUnspecializedNondestructively(VuoPort *portToUnspecialize)
{
	map<VuoNode *, string> nodesToReplace;
	set<VuoCable *> cablesToDelete;
	createReplacementsToUnspecializePort(portToUnspecialize, false, nodesToReplace, cablesToDelete);

	// Check whether unspecialization would disconnect any existing cables
	// (other than the cable that would normally be displaced by the new cable connection).
	if (cablesToDelete.empty())
		return true;

	else if ((cablesToDelete.size() == 1) && ((*(cablesToDelete.begin()))->getToPort() == portToUnspecialize))
		return true;

	return false;
}

/**
 * Returns a boolean indicating whether there may be a cable
 * attached directly from @c fromPort to @c toPort, taking into account
 * the respective port types (input vs. output; event-only vs.
 * event+data; respective data types), and the possibility that one
 * specialized port may be respecialized in preparation for the connection.
 *
 * Returns true even if the respecialization will break connections between
 * the specialized port's generic network and connected static ports, but not
 * if it will break connections within the generic network.
 *
 * @param[in] toPort The port to consider connecting to.
 * @param[in] eventOnlyConnection A boolean indicating whether the connection under consideration would be always-event-only.
 * @param[in] forwardConnection A boolean indicating whether the connection under consideration would be the result of a "forward" cable drag
 * (starting at an output port and ending at an input port).
 * @param[out] portToRespecialize The port, either @c fromPort or @c toPort, that will require respecialization in order for the connection to be completed.
 *                              Does not account for potential cascade effects. May be NULL, if the connection may be completed without respecialization.
 * @param[out] respecializedTypeName The name of the specialized port type with which the generic port type is to be replaced.
 */
bool VuoEditorComposition::canConnectDirectlyWithRespecialization(VuoRendererPort *fromPort,
																  VuoRendererPort *toPort,
																  bool eventOnlyConnection,
																  bool forwardConnection,
																  VuoRendererPort **portToRespecialize,
																  string &respecializedTypeName)
{
	// @todo https://b33p.net/kosada/node/10481 Still need eventOnlyConnection?

	*portToRespecialize = NULL;
	respecializedTypeName = "";

	// // @todo https://b33p.net/kosada/node/10481 Necessary?
	if (fromPort->canConnectDirectlyWithoutSpecializationTo(toPort, eventOnlyConnection))
		return true;

	// // @todo https://b33p.net/kosada/node/10481 Necessary?
	if (fromPort->canConnectDirectlyWithSpecializationTo(toPort, eventOnlyConnection, portToRespecialize, respecializedTypeName))
		return true;

	bool fromPortIsEnabledOutput = (fromPort && fromPort->getOutput() && fromPort->isEnabled());
	bool toPortIsEnabledInput = (toPort && toPort->getInput() && toPort->isEnabled());

	if (!(fromPortIsEnabledOutput && toPortIsEnabledInput))
		return false;

	VuoType *currentFromDataType = fromPort->getDataType();
	VuoType *currentToDataType = toPort->getDataType();

	if (!(currentFromDataType && currentToDataType))
		return false;

	/// @todo (https://b33p.net/kosada/node/7032)
	if (VuoType::isListTypeName(currentFromDataType->getModuleKey()) != VuoType::isListTypeName(currentToDataType->getModuleKey()))
		return false;

	VuoGenericType *currentFromGenericType = dynamic_cast<VuoGenericType *>(currentFromDataType);
	VuoGenericType *currentToGenericType = dynamic_cast<VuoGenericType *>(currentToDataType);

	VuoGenericType *originalFromGenericType = NULL;
	if (fromPort->getBase()->getRenderer()->getUnderlyingParentNode())
	{
		VuoCompilerNodeClass *fromNodeClass = fromPort->getBase()->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
		VuoCompilerSpecializedNodeClass *fromSpecializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(fromNodeClass);
		if (fromSpecializedNodeClass)
		{
			VuoPortClass *portClass = fromPort->getBase()->getClass();
			originalFromGenericType = dynamic_cast<VuoGenericType *>( fromSpecializedNodeClass->getOriginalPortType(portClass) );
		}
	}

	VuoGenericType *originalToGenericType = NULL;
	if (toPort->getBase()->getRenderer()->getUnderlyingParentNode())
	{
		VuoCompilerNodeClass *toNodeClass = toPort->getBase()->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
		VuoCompilerSpecializedNodeClass *toSpecializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(toNodeClass);
		if (toSpecializedNodeClass)
		{
			VuoPortClass *portClass = toPort->getBase()->getClass();
			originalToGenericType = dynamic_cast<VuoGenericType *>( toSpecializedNodeClass->getOriginalPortType(portClass) );
		}
	}

	// Determine whether the port at each endpoint is 1) generic, or
	// 2) specialized and currently revertible, or 3) effectively static.
	bool fromPortIsGeneric = currentFromGenericType;
	bool fromPortIsSpecialized = originalFromGenericType && !currentFromGenericType && isPortCurrentlyRevertible(fromPort);
	bool fromPortIsStatic = (!fromPortIsGeneric && !fromPortIsSpecialized);

	bool toPortIsGeneric = currentToGenericType;
	bool toPortIsSpecialized = originalToGenericType && !currentToGenericType && isPortCurrentlyRevertible(toPort);
	bool toPortIsStatic = (!toPortIsGeneric && !toPortIsSpecialized);

	// Figure out which port to try to respecialize, and to what type.
	set<string> compatibleTypes;
	string specializedType = "";
	VuoRendererPort *portToTryToRespecialize = NULL;

	// Case: One port static, one port specialized.
	if ((fromPortIsStatic && toPortIsSpecialized) || (fromPortIsSpecialized && toPortIsStatic))
	{
		VuoRendererPort *staticPort = (fromPortIsStatic? fromPort : toPort);
		specializedType = staticPort->getDataType()->getModuleKey();
		portToTryToRespecialize = (fromPortIsSpecialized? fromPort : toPort);
	}

	// Case: One port specialized, other port generic or specialized.
	else if ((fromPortIsSpecialized || toPortIsSpecialized) && !fromPortIsStatic && !toPortIsStatic)
	{
		VuoRendererPort *dragSource = (forwardConnection? fromPort : toPort);
		bool dragSourceIsGeneric = (forwardConnection? fromPortIsGeneric : toPortIsGeneric);

		VuoRendererPort *dragDestination = (forwardConnection? toPort : fromPort);
		bool dragDestinationIsGeneric = (forwardConnection? toPortIsGeneric : fromPortIsGeneric);

		// @todo https://b33p.net/kosada/node/10481 : Currently handled in VuoEditorComposition::canConnectDirectlyWithSpecialization(); merge?
		/*
		if (dragSourceIsGeneric && !dragDestinationIsGeneric)
		{
			specializedType = dragDestination->getDataType()->getModuleKey();
			portToTryToRespecialize = dragSource;
		}
		else if (dragDestinationIsGeneric && !dragSourceIsGeneric)
		{
			specializedType = dragSource->getDataType()->getModuleKey();
			portToTryToRespecialize = dragDestination;
		}
		else
		*/

		if (!dragSourceIsGeneric && !dragDestinationIsGeneric)
		{
			specializedType = dragSource->getDataType()->getModuleKey();
			portToTryToRespecialize = dragDestination;
		}
	}

	// @todo https://b33p.net/kosada/node/10481 Other cases.
	else
		return false;

	if (portToTryToRespecialize)
		compatibleTypes = getRespecializationOptionsForPortInNetwork(portToTryToRespecialize);

	bool portsAreCompatible = (compatibleTypes.find(specializedType) != compatibleTypes.end());

	if (portsAreCompatible)
	{
		*portToRespecialize = portToTryToRespecialize;
		respecializedTypeName = specializedType;
	}

	return portsAreCompatible;
}

/**
 * If @a node is a drawer, updates its eligibility highlighting given the eligibility highlighting
 * of its ports.
 *
 * If @a node is any other kind of node, does nothing.
 */
void VuoEditorComposition::updateEligibilityHighlightingForNode(VuoRendererNode *node)
{
	VuoRendererInputDrawer *drawer = dynamic_cast<VuoRendererInputDrawer *>(node);
	if (drawer)
	{
		VuoRendererColors::HighlightType bestEligibility = VuoRendererColors::ineligibleHighlight;
		foreach (VuoRendererPort *drawerPort, drawer->getDrawerPorts())
			if (drawerPort->eligibilityHighlight() == VuoRendererColors::standardHighlight)
				bestEligibility = VuoRendererColors::standardHighlight;
			else if (drawerPort->eligibilityHighlight() == VuoRendererColors::subtleHighlight
					 && bestEligibility != VuoRendererColors::standardHighlight)
				bestEligibility = VuoRendererColors::subtleHighlight;

		// If this drawer has no eligible ports, fade it out.
		{
			QGraphicsItem::CacheMode normalCacheMode = drawer->cacheMode();
			drawer->setCacheMode(QGraphicsItem::NoCache);
			drawer->updateGeometry();

			drawer->setEligibilityHighlight(bestEligibility);

			drawer->setCacheMode(normalCacheMode);
		}

		// Make sure the host port is repainted to take into account the eligibility of its drawer ports.
		if (drawer->getRenderedHostPort()
		 && drawer->getRenderedHostPort()->getRenderer())
		{
			VuoRendererPort *hostPort = drawer->getRenderedHostPort()->getRenderer();

			QGraphicsItem::CacheMode normalCacheMode = hostPort->cacheMode();
			hostPort->setCacheMode(QGraphicsItem::NoCache);
			hostPort->updateGeometry();
			hostPort->setCacheMode(normalCacheMode);
		}
	}
}

/**
 * Clears highlighting of eligible cable connection endpoints.
 */
void VuoEditorComposition::clearCableEndpointEligibilityHighlighting()
{
	clearInternalPortEligibilityHighlighting();
	emit clearPublishedSidebarDropLocationHighlightingRequested();
}

/**
 * Prompts the user to select a bridging (typeconversion and/or (re-)specialization) solution
 * to connect the provided ports.
 *
 * @param fromPort The output port that will provide the source data for typeconversion.
 * @param toPort The input port that will accept the typeconverted data.
 * @param toPortIsDragDestination Whether the cable connection is being completed at the toPort
 *                              (as in a forward cable drag) as opposed to the fromPort
 *                              (as in a backward cable drag), which may be the tie-breaking factor
 *                              in deciding which port to attempt to specialize.
 * @param[out] portToSpecialize The port, either @c fromPort or @c toPort, that will require specialization in order for the connection to be completed.
 *                              Does not account for potential cascade effects. NULL if the connection can be completed without specialization.
 * @param[out] specializedTypeName The name of the specialized port type with which the generic port type is to be replaced. Empty string if
 *                              the connection can be completed without specialization.
 * @param[out] typecastToInsert The class name of the typecast to be inserted in order to complete the connection. Empty string if typeconversion is unnecessary.
 *
 * Returns true if a solution was selected, and false otherwise.
 */
bool VuoEditorComposition::selectBridgingSolution(VuoRendererPort *fromPort,
												  VuoRendererPort *toPort,
												  bool toPortIsDragDestination,
												  VuoRendererPort **portToSpecialize,
												  string &specializedTypeName,
												  string &typecastToInsert)
{
	*portToSpecialize = NULL;
	specializedTypeName = "";

	map<string, VuoRendererPort *> portToSpecializeForTypecast;
	map<string, string> specializedTypeNameForTypecast;

	vector<string> candidateTypecasts = findBridgingSolutions(fromPort, toPort, toPortIsDragDestination, portToSpecializeForTypecast, specializedTypeNameForTypecast);
	bool solutionSelected = selectBridgingSolutionFromOptions(candidateTypecasts, portToSpecializeForTypecast, specializedTypeNameForTypecast, typecastToInsert);

	if (!solutionSelected)
		return false;

	if (portToSpecializeForTypecast.find(typecastToInsert) != portToSpecializeForTypecast.end())
		*portToSpecialize = portToSpecializeForTypecast[typecastToInsert];
	if (specializedTypeNameForTypecast.find(typecastToInsert) != specializedTypeNameForTypecast.end())
		specializedTypeName = specializedTypeNameForTypecast[typecastToInsert];

	return true;
}

/**
 * If the provided list of bridging options contains exactly one candidate, returns
 * the class name of that candidate; if the list contains more than one candidate, presents
 * the options in a menu and returns the option selected by the user.  If no candidates
 * exist or the user dismissed the menu without a selection, returns the empty string.
 *
 * Helper function for selectBridgingSolution().
 *
 * The @c portToSpecializeForTypecast and @c specializedTypeNameForTypecast input maps contain information
 * about the port that would need to be specialized and the type that it would need to be specialized
 * to in conjunction with a given typecast selection.
 *
 * @param[out] selectedTypecast The class name of the typecast to be inserted in order to complete the connection,
 * or the empty string if typeconversion is unnecessary.
 *
 * Returns true if a bridging solution was selected, and false otherwise.
 *
 */
bool VuoEditorComposition::selectBridgingSolutionFromOptions(vector<string> suitableTypecasts,
															 map<string, VuoRendererPort *> portToSpecializeForTypecast,
															 map<string, string> specializedTypeNameForTypecast,
															 string &selectedTypecast)
{
	if (suitableTypecasts.empty())
	{
		selectedTypecast = "";
		return false;
	}

	else if (suitableTypecasts.size() == 1)
	{
		selectedTypecast = suitableTypecasts[0];
		return true;
	}

	else
		return promptForBridgingSelectionFromOptions(suitableTypecasts, portToSpecializeForTypecast, specializedTypeNameForTypecast, selectedTypecast);
}

/**
 * Returns true if the provided ports pass a basic eligibility test to be connected
 * in the direction provided, where @c fromPort is the candidate output port
 * and @c toPort is the candidate input port.
 */
bool VuoEditorComposition::portsPassSanityCheckToBridge(VuoRendererPort *fromPort, VuoRendererPort *toPort)
{
	bool fromPortIsEnabledOutput = (fromPort && fromPort->getOutput() && fromPort->isEnabled());
	bool toPortIsEnabledInput = (toPort && toPort->getInput() && toPort->isEnabled());

	return (fromPortIsEnabledOutput && toPortIsEnabledInput &&
			fromPort->getBase()->getClass()->hasCompiler() &&
			toPort->getBase()->getClass()->hasCompiler());
}

/**
 * Returns true if the provided ports pass a basic eligibility test to be connected by typeconverter.
 * If candidate types are provided, performs the check as if the ports were to be specialized to
 * those types; otherwise, uses the ports' current types.
 */
bool VuoEditorComposition::portsPassSanityCheckToTypeconvert(VuoRendererPort *fromPort, VuoRendererPort *toPort,
															 const string &candidateFromTypeName, const string &candidateToTypeName)
{
	if (!portsPassSanityCheckToBridge(fromPort, toPort))
		return false;

	VuoType *fromPortType = static_cast<VuoCompilerPortClass *>(fromPort->getBase()->getClass()->getCompiler())->getDataVuoType();
	VuoType *toPortType = static_cast<VuoCompilerPortClass *>(toPort->getBase()->getClass()->getCompiler())->getDataVuoType();

	string fromTypeName = ! candidateFromTypeName.empty() ?
							  candidateFromTypeName :
							  (fromPortType ? fromPortType->getModuleKey() : "");
	string toTypeName = ! candidateToTypeName.empty() ?
							candidateToTypeName :
							(toPortType ? toPortType->getModuleKey() : "");

	// To reduce confusion, don't offer Boolean -> Integer as a type conversion option for nodes that use 1-based indices.
	if (fromTypeName == "VuoBoolean" && toTypeName == "VuoInteger")
	{
		bool toNodeUsesIndex = toPort->getUnderlyingParentNode() &&
							   (VuoStringUtilities::beginsWith(toPort->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.text.") ||
								VuoStringUtilities::beginsWith(toPort->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.select.") ||
								(VuoStringUtilities::beginsWith(toPort->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.list.") &&
								 !VuoStringUtilities::beginsWith(toPort->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.list.make."))
								);

		if (toNodeUsesIndex)
			return false;
	}

	return true;
}

/**
 * Convenience function for the other version of VuoEditorComposition::findBridgingSolutions(),
 * when all that matters is whether a bridging solution between two ports exists, not what the solution is.
 *
 * @param fromPort The output port that will provide the source data for typeconversion.
 * @param toPort The input port that will accept the typeconverted data.
 * @param toPortIsDragDestination Whether the cable connection is being completed at the toPort
 *                              (as in a forward cable drag) as opposed to the fromPort
 *                              (as in a backward cable drag), which may be the tie-breaking factor
 *                              in deciding which port to attempt to specialize.
 *
 * Returns a vector containing the names of all loaded typecast classes that, in combination
 * with potential respecialization of the @c fromPort or @c toPort, are capable of bridging
 * the connection between the two ports. An empty string in place of a typecast name means that
 * the ports can be bridged without typeconversion (but still might require specialization).
 * An empty vector means that no bridging solutions were found.
 */
vector<string> VuoEditorComposition::findBridgingSolutions(VuoRendererPort *fromPort,
														   VuoRendererPort *toPort,
														   bool toPortIsDragDestination)
{
	map<string, VuoRendererPort *> portToSpecializeForTypecast;
	map<string, string> specializedTypeNameForTypecast;
	return findBridgingSolutions(fromPort, toPort, toPortIsDragDestination, portToSpecializeForTypecast, specializedTypeNameForTypecast);
}

/**
 * Returns a list of ways that a valid cable connection could be made between two ports of different data types,
 * by inserting a type-converter node, changing the data type of a generic port, or both.
 *
 * - For inserting a type-converter node, the list item is the node class name.
 * - For changing the data type of a generic port, the list item is an empty string, and entries are added to
 *   @a portToSpecializeForTypecast and @a specializedTypeNameForTypecast with an empty string as the key.
 * - For doing both, the list item is the type-converter node class name, and entries are added to
 *   @a portToSpecializeForTypecast and @a specializedTypeNameForTypecast with the node class name as the key.
 *
 * If a connection could be made by changing the data type of either @a fromPort or @a toPort, and the two ports
 * are equally good candidates, then the drag destination (as indicated by @a toPortIsDragDestination) will be
 * chosen as the one to change.
 */
vector<string> VuoEditorComposition::findBridgingSolutions(VuoRendererPort *fromPort,
														   VuoRendererPort *toPort,
														   bool toPortIsDragDestination,
														   map<string, VuoRendererPort *> &portToSpecializeForTypecast,
														   map<string, string> &specializedTypeNameForTypecast)
{
	portToSpecializeForTypecast.clear();
	specializedTypeNameForTypecast.clear();

	if (!portsPassSanityCheckToBridge(fromPort, toPort))
		return {};

	// Temporarily disallow direct cable connections between published inputs and published outputs.
	// @todo: Allow for https://b33p.net/kosada/node/7756 .
	if (dynamic_cast<VuoRendererPublishedPort *>(fromPort) && dynamic_cast<VuoRendererPublishedPort *>(toPort))
		return {};

	// Case: We have an unspecialized (generic) port. See whether we can specialize it to complete the connection without typeconversion.
	{
		VuoRendererPort *portToSpecialize = NULL;
		string specializedTypeName = "";
		if (fromPort->canConnectDirectlyWithSpecializationTo(toPort, !cableInProgress->getRenderer()->effectivelyCarriesData(), &portToSpecialize, specializedTypeName))
		{
			portToSpecializeForTypecast[""] = portToSpecialize;
			specializedTypeNameForTypecast[""] = specializedTypeName;
			return {""};
		}
	}

	VuoType *currentFromDataType = fromPort->getDataType();
	VuoType *currentToDataType = toPort->getDataType();

	if (!(currentFromDataType && currentToDataType))
		return {};

	VuoGenericType *currentFromGenericType = dynamic_cast<VuoGenericType *>(currentFromDataType);
	VuoGenericType *currentToGenericType = dynamic_cast<VuoGenericType *>(currentToDataType);

	VuoGenericType *originalFromGenericType = NULL;
	if (fromPort->getBase()->getRenderer()->getUnderlyingParentNode())
	{
		VuoCompilerNodeClass *fromNodeClass = fromPort->getBase()->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
		VuoCompilerSpecializedNodeClass *fromSpecializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(fromNodeClass);
		if (fromSpecializedNodeClass)
		{
			VuoPortClass *portClass = fromPort->getBase()->getClass();
			originalFromGenericType = dynamic_cast<VuoGenericType *>( fromSpecializedNodeClass->getOriginalPortType(portClass) );
		}
	}

	VuoGenericType *originalToGenericType = NULL;
	if (toPort->getBase()->getRenderer()->getUnderlyingParentNode())
	{
		VuoCompilerNodeClass *toNodeClass = toPort->getBase()->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getCompiler();
		VuoCompilerSpecializedNodeClass *toSpecializedNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(toNodeClass);
		if (toSpecializedNodeClass)
		{
			VuoPortClass *portClass = toPort->getBase()->getClass();
			originalToGenericType = dynamic_cast<VuoGenericType *>( toSpecializedNodeClass->getOriginalPortType(portClass) );
		}
	}

	// Determine whether the port at each endpoint is:
	// 1) generic (unspecialized), or
	// 2) specialized and currently revertible, or
	// 3) effectively static.
	bool fromPortIsGeneric = currentFromGenericType;
	bool fromPortIsSpecialized = originalFromGenericType && !currentFromGenericType && isPortCurrentlyRevertible(fromPort);
	bool fromPortIsStatic = (!fromPortIsGeneric && !fromPortIsSpecialized);

	bool toPortIsGeneric = currentToGenericType;
	bool toPortIsSpecialized = originalToGenericType && !currentToGenericType && isPortCurrentlyRevertible(toPort);
	bool toPortIsStatic = (!toPortIsGeneric && !toPortIsSpecialized);

	// No typeconversion or specialization options between two unspecialized generic ports.
	if (fromPortIsGeneric && toPortIsGeneric)
		return {};

	// Typeconversion options but no specialization options between two static ports.
	else if (fromPortIsStatic && toPortIsStatic)
	{
		if (! portsPassSanityCheckToTypeconvert(fromPort, toPort))
			return {};

		return moduleManager->getCompatibleTypecastClasses(currentFromDataType->getModuleKey(), currentFromDataType,
														   currentToDataType->getModuleKey(), currentToDataType);
	}

	// Remaining combinations might require (re-)specializing one port or the other.
	// Figure out which port to consider (re-)specializing.
	bool specializeToPort = true;
	if (toPortIsGeneric)
		specializeToPort = true;
	else if (fromPortIsGeneric)
		specializeToPort = false;
	else if (fromPortIsSpecialized && toPortIsStatic)
		specializeToPort = false;
	else if (fromPortIsStatic && toPortIsSpecialized)
		specializeToPort = true;
	else if (fromPortIsSpecialized && toPortIsSpecialized)
		specializeToPort = toPortIsDragDestination;

	// Now that ports have been categorized, figure out what combinations of (re-)specialization
	// and/or typeconversion we can use to bridge them.
	set<string> compatibleTypes;
	if (specializeToPort && (toPortIsGeneric || (toPortIsSpecialized && portCanBeUnspecializedNondestructively(toPort->getBase()))))
		compatibleTypes = getRespecializationOptionsForPortInNetwork(toPort);
	else if (!specializeToPort && (fromPortIsGeneric || (fromPortIsSpecialized && portCanBeUnspecializedNondestructively(fromPort->getBase()))))
		compatibleTypes = getRespecializationOptionsForPortInNetwork(fromPort);

	// Typeconversion without re-specialization may be possible. In this case, don't require that the port be
	// non-destructively unspecializable, since it already has the appropriate specialization.
	compatibleTypes.insert(specializeToPort? currentToDataType->getModuleKey() : currentFromDataType->getModuleKey());

	// If there's at least one bridging solution that involves only type-conversion or only specialization, then return that.
	{
		vector<string> limitedSuitableTypecasts;

		// Check for bridging solutions that involve typeconversion without specialization.
		if (portsPassSanityCheckToTypeconvert(fromPort, toPort))
		{
			limitedSuitableTypecasts = moduleManager->getCompatibleTypecastClasses(currentFromDataType->getModuleKey(), currentFromDataType,
																				   currentToDataType->getModuleKey(), currentToDataType);
			foreach (string typecastName, limitedSuitableTypecasts)
			{
				portToSpecializeForTypecast[typecastName] = specializeToPort? toPort : fromPort;
				specializedTypeNameForTypecast[typecastName] = specializeToPort? currentToDataType->getModuleKey() :
																				 currentFromDataType->getModuleKey();
			}
		}

		// Check for bridging solutions that involve specialization without typeconversion.
		string fixedDataType = specializeToPort? currentFromDataType->getModuleKey() : currentToDataType->getModuleKey();
		if (compatibleTypes.find(fixedDataType) != compatibleTypes.end())
		{
			limitedSuitableTypecasts.push_back("");
			portToSpecializeForTypecast[""] = specializeToPort? toPort : fromPort;
			specializedTypeNameForTypecast[""] = fixedDataType;
		}

		if (limitedSuitableTypecasts.size() >= 1)
			return limitedSuitableTypecasts;
	}

	// Search for bridging solutions that involve both type-conversion and specialization.
	vector<string> suitableTypecasts;
	for (const string &compatibleTypeName : compatibleTypes)
	{
		// Don't look up the VuoCompilerType for compatibleTypeName since we don't actually need it
		// and loading it (if not already loaded) would slow things down.
		string candidateFromTypeName;
		string candidateToTypeName;
		VuoType *candidateFromType;
		VuoType *candidateToType;
		if (specializeToPort)
		{
			candidateFromTypeName = currentFromDataType->getModuleKey();
			candidateFromType = currentFromDataType;
			candidateToTypeName = compatibleTypeName;
			candidateToType = nullptr;
		}
		else
		{
			candidateFromTypeName = compatibleTypeName;
			candidateFromType = nullptr;
			candidateToTypeName = currentToDataType->getModuleKey();
			candidateToType = currentToDataType;
		}

		// Re-specialization without typeconversion may be possible.
		if (candidateFromTypeName == candidateToTypeName)
		{
			suitableTypecasts.push_back("");
			portToSpecializeForTypecast[""] = specializeToPort? toPort : fromPort;
			specializedTypeNameForTypecast[""] = compatibleTypeName;
		}

		if (portsPassSanityCheckToTypeconvert(fromPort, toPort, candidateFromTypeName, candidateToTypeName))
		{
			vector<string> suitableTypecastsForCurrentTypes = moduleManager->getCompatibleTypecastClasses(candidateFromTypeName, candidateFromType,
																										  candidateToTypeName, candidateToType);
			foreach (string typecast, suitableTypecastsForCurrentTypes)
			{
				suitableTypecasts.push_back(typecast);
				portToSpecializeForTypecast[typecast] = specializeToPort? toPort : fromPort;
				specializedTypeNameForTypecast[typecast] = compatibleTypeName;
			}
		}
	}

	return suitableTypecasts;
}

/**
 * Presents the user with a menu displaying the input @c suitableTypecasts list.
 * This list is expected to contain class names of typecast nodes; the menu will display
 * the same text that would appear in the collapsed typeconverters (e.g., "real -> roundedDownInteger").
 *
 * The @c portToSpecializeForTypecast and @c specializedTypeNameForTypecast input maps contain information
 * about the port that would need to be specialized and the type that it would need to be specialized
 * to in conjunction with a given typecast selection.
 *
 * Returns a boolean indicating whether the user selected an option from the menu, and
 * sets @c selectedType to the user-selected typecast class name (or the empty string if none).
 */
bool VuoEditorComposition::promptForBridgingSelectionFromOptions(vector<string> suitableTypecasts,
															 map<string, VuoRendererPort *> portToSpecializeForTypecast,
															 map<string, string> specializedTypeNameForTypecast,
															 string &selectedTypecast)
{
	QMenu typecastMenu(views()[0]->viewport());
	typecastMenu.setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	QString spacer("    ");

	// Inventory specialization options
	set <pair<VuoRendererPort *, string> > specializationDetails;
	vector<string> typeconversionOptionsRequiringNoSpecialization;
	foreach (string typecastClassName, suitableTypecasts)
	{
		VuoRendererPort *portToSpecialize = portToSpecializeForTypecast[typecastClassName];
		string specializedTypeName = specializedTypeNameForTypecast[typecastClassName];
		specializationDetails.insert(std::make_pair(portToSpecialize,
													specializedTypeName));

		bool portAlreadyHasTargetType = (!portToSpecialize || (portToSpecialize->getDataType() && (portToSpecialize->getDataType()->getModuleKey() == specializedTypeName)));
		if (portAlreadyHasTargetType)
			typeconversionOptionsRequiringNoSpecialization.push_back(typecastClassName);
	}

	// If there is a bridging option that requires no typeconversion, it doesn't need the usual
	// specialization heading under which multiple typeconversion options may be listed.
	// Selecting this item itself will invoke the specialization.
	if ((std::find(suitableTypecasts.begin(), suitableTypecasts.end(), "") != suitableTypecasts.end()))
	{
		QString menuText = getDisplayTextForSpecializationOption(portToSpecializeForTypecast[""], specializedTypeNameForTypecast[""]);
		QAction *typecastAction = typecastMenu.addAction(menuText);
		typecastAction->setData(QVariant(""));
	}

	bool foundSpecializationOptionsRequiringNoTypeconversion = typecastMenu.actions().size() >= 1;
	bool foundTypeconversionOptionsRequiringNoSpecialization = typeconversionOptionsRequiringNoSpecialization.size() >= 1;

	// If there are bridging options that require no specialization, list them next.
	bool includingTypeconvertWithNoSpecializationHeader = foundSpecializationOptionsRequiringNoTypeconversion;
	if (foundTypeconversionOptionsRequiringNoSpecialization)
	{
		if (foundSpecializationOptionsRequiringNoTypeconversion)
			typecastMenu.addSeparator();

		VuoRendererPort *portToSpecialize = portToSpecializeForTypecast[typeconversionOptionsRequiringNoSpecialization[0] ];
		string specializedTypeName = specializedTypeNameForTypecast[typeconversionOptionsRequiringNoSpecialization[0] ];

		if (portToSpecialize && !specializedTypeName.empty())
		{
			QString menuText = getDisplayTextForSpecializationOption(portToSpecialize, specializedTypeName);
			QAction *typecastAction = typecastMenu.addAction(menuText);
			typecastAction->setEnabled(false);
			includingTypeconvertWithNoSpecializationHeader = true;
		}
	}

	foreach (string typecastClassName, typeconversionOptionsRequiringNoSpecialization)
	{
		VuoCompilerNodeClass *typecastClass = compiler->getNodeClass(typecastClassName);
		if (typecastClass)
		{
			QAction *typecastAction = typecastMenu.addAction((includingTypeconvertWithNoSpecializationHeader? spacer : "") + VuoRendererTypecastPort::getTypecastTitleForNodeClass(typecastClass->getBase(), true));
			typecastAction->setData(QVariant(typecastClassName.c_str()));
		}
	}

	// Now list the remaining bridging options.
	for (set<pair<VuoRendererPort *, string> >::iterator i = specializationDetails.begin(); i != specializationDetails.end(); ++i)
	{
		VuoRendererPort *portToSpecialize = i->first;
		string specializedTypeName = i->second;

		// We've already listed the no-typeconversion bridging option, so skip it here.
		if ((std::find(suitableTypecasts.begin(), suitableTypecasts.end(), "") != suitableTypecasts.end()) &&
				(portToSpecializeForTypecast[""] == portToSpecialize) &&
				(specializedTypeNameForTypecast[""] == specializedTypeName))
		{
			continue;
		}

		// We've already listed the no-specialization bridging option, so skip it here.
		bool portAlreadyHasTargetType = (!portToSpecialize || (portToSpecialize->getDataType() && (portToSpecialize->getDataType()->getModuleKey() == specializedTypeName)));
		if (portAlreadyHasTargetType)
		{
			continue;
		}

		if (typecastMenu.actions().size() >= 1)
			typecastMenu.addSeparator();

		QString menuText = getDisplayTextForSpecializationOption(portToSpecialize, specializedTypeName);
		QAction *typecastAction = typecastMenu.addAction(menuText);
		typecastAction->setEnabled(false);

		// Inventory typeconversion options associated with this specialization option.
		foreach (string typecastClassName, suitableTypecasts)
		{
			if ((portToSpecializeForTypecast[typecastClassName] == portToSpecialize) &&
					(specializedTypeNameForTypecast[typecastClassName] == specializedTypeName))
			{
				VuoCompilerNodeClass *typecastClass = compiler->getNodeClass(typecastClassName);
				if (typecastClass)
				{
					QAction *typecastAction = typecastMenu.addAction(spacer + VuoRendererTypecastPort::getTypecastTitleForNodeClass(typecastClass->getBase(), true));
					typecastAction->setData(QVariant(typecastClassName.c_str()));
				}
			}
		}
	}

	menuSelectionInProgress = true;
	QAction *selectedTypecastAction = typecastMenu.exec(QCursor::pos());
	menuSelectionInProgress = false;

	selectedTypecast = (selectedTypecastAction? selectedTypecastAction->data().toString().toUtf8().constData() : "");
	return selectedTypecastAction;
}

/**
 * Returns menu display text for the provided port specialization option.
 */
QString VuoEditorComposition::getDisplayTextForSpecializationOption(VuoRendererPort *portToSpecialize, string specializedTypeName)
{
	if (!portToSpecialize || specializedTypeName.empty())
		return "";

	bool isInput = portToSpecialize && portToSpecialize->getInput();
	QString typeDisplayName = formatTypeNameForDisplay(specializedTypeName);

	bool portAlreadyHasTargetType = (!portToSpecialize || (portToSpecialize->getDataType() && (portToSpecialize->getDataType()->getModuleKey() == specializedTypeName)));

	QString displayText;
	if (portAlreadyHasTargetType)
	{
		if (isInput)
		{
			//: Appears as a section label in the popup menu when connecting a cable to an input port that has multiple specialization/type-conversion options.
			displayText = tr("Keep Input Port as %1");
		}
		else
		{
			//: Appears as a section label in the popup menu when connecting a cable to an output port that has multiple specialization/type-conversion options.
			displayText = tr("Keep Output Port as %1");
		}
	}
	else
	{
		if (isInput)
		{
			//: Appears as an item in the popup menu when connecting a cable to an input port that has multiple specialization/type-conversion options.
			displayText = tr("Change Input Port to %1");
		}
		else
		{
			//: Appears as an item in the popup menu when connecting a cable to an output port that has multiple specialization/type-conversion options.
			displayText = tr("Change Output Port to %1");
		}
	}

	return displayText.arg(typeDisplayName);
}

/**
 * If the composition is running (or it's a subcomposition and the top-level composition is running),
 * returns the current data value associated with the port. Otherwise returns NULL.
 */
json_object * VuoEditorComposition::getPortValueInRunningComposition(VuoPort *port)
{
	__block json_object *portValue = NULL;

	if (! port->getRenderer()->getDataType())
		return portValue;

	string runningPortIdentifier = identifierCache->getIdentifierForPort(port);
	bool isInput = port->getRenderer()->getInput();

	void (^getPortValue)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		dispatch_sync(topLevelComposition->runCompositionQueue, ^{
			if (topLevelComposition->isRunningThreadUnsafe())
			{
				portValue = isInput ?
								topLevelComposition->runner->getInputPortValue(thisCompositionIdentifier, runningPortIdentifier) :
								topLevelComposition->runner->getOutputPortValue(thisCompositionIdentifier, runningPortIdentifier);
			}
		});
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, getPortValue);

	return portValue;
}

/**
 * Returns the identifier of the provided @c runningPort in the running composition.
 */
string VuoEditorComposition::getIdentifierForRunningPort(VuoPort *runningPort)
{
	return static_cast<VuoCompilerPort *>(runningPort->getCompiler())->getIdentifier();
}

/**
 * If `staticPort` is a published port, returns its name (without the node identifier + colon).
 *
 * If `staticPort` is an internal port, returns its identifier (including the node identifier + colon) in the stored composition.
 * If a @c parentNode is provided, uses that node identifier to help derive the
 * port identifier. Otherwise, attempts to determine the parent node via renderer items.
 */
string VuoEditorComposition::getIdentifierForStaticPort(VuoPort *staticPort, VuoNode *parentNode)
{
	if (!staticPort)
		return "";

	// Published ports
	if (dynamic_cast<VuoPublishedPort *>(staticPort))
		return dynamic_cast<VuoPublishedPort *>(staticPort)->getClass()->getName();

	// Internal ports
	// We might as well use the same naming scheme here as is used in the running composition,
	// but the VuoCompilerPort::getIdentifier() call will fail unless its parent
	// node identifier has been explicitly set.
	string nodeIdentifier = "";
	if (parentNode && parentNode->hasCompiler())
		nodeIdentifier = parentNode->getCompiler()->getIdentifier();
	else if (staticPort->hasRenderer() &&
			staticPort->getRenderer()->getUnderlyingParentNode() &&
			staticPort->getRenderer()->getUnderlyingParentNode()->getBase()->hasCompiler())
	{
		nodeIdentifier = staticPort->getRenderer()->getUnderlyingParentNode()->getBase()->getCompiler()->getIdentifier();
	}

	if (staticPort->hasCompiler() && !nodeIdentifier.empty())
	{
		dynamic_cast<VuoCompilerPort *>(staticPort->getCompiler())->setNodeIdentifier(nodeIdentifier);
		return static_cast<VuoCompilerPort *>(staticPort->getCompiler())->getIdentifier();
	}
	else
		return "";
}

/**
 * Returns the port registered to the composition under the provided @c portID.
 */
VuoPort * VuoEditorComposition::getPortWithStaticIdentifier(string portID)
{
	VuoPort *port = nullptr;
	identifierCache->doForPortWithIdentifier(portID, [&port](VuoPort *p) {
		port = p;
	});
	return port;
}

/**
 * Returns the underlying parent node of the provided @c port within the provided @c composition.
 *
 * The provided port and composition need not have allocations or renderers.
 */
VuoNode * VuoEditorComposition::getUnderlyingParentNodeForPort(VuoPort *port, VuoEditorComposition *composition)
{
	if (port->hasRenderer())
	{
		if (dynamic_cast<VuoRendererPublishedPort *>(port->getRenderer()))
		{
			bool isPublishedInput = !port->getRenderer()->getInput();
			return (isPublishedInput? composition->getPublishedInputNode() :
									  composition->getPublishedOutputNode());
		}

		else
			return port->getRenderer()->getUnderlyingParentNode()->getBase();
	}

	foreach (VuoNode *n, composition->getBase()->getNodes())
	{
		VuoPort *candidateInputPort = n->getInputPortWithName(port->getClass()->getName());
		if (candidateInputPort == port)
			return n;

		VuoPort *candidateOutputPort = n->getOutputPortWithName(port->getClass()->getName());
		if (candidateOutputPort == port)
			return n;
	}

	return NULL;
}

/**
 * Returns a pointer to the active popover for the port with the provided @c portID,
 * if such a popover exists.
 * Otherwise returns NULL.
 *
 * @threadQueue{activePortPopoversQueue}
 */
VuoPortPopover * VuoEditorComposition::getActivePopoverForPort(string portID)
{
	map<string, VuoPortPopover *>::iterator popover = activePortPopovers.find(portID);
	if (popover != activePortPopovers.end())
		return popover->second;

	else
		return NULL;
}

/**
 * Enables the popover for the provided @c port if and only if
 * it is not already displayed and was not only just hidden at the most recent
 * recorded event, like the mouse press preceding the mouse release
 * that triggered this function call.
 * Exception: If the popover is detached, leaves it alone.
 */
void VuoEditorComposition::enableInactivePopoverForPort(VuoRendererPort *rp)
{
	string portID = identifierCache->getIdentifierForPort(rp->getBase());
	bool popoverJustClosedAtLastEvent = portsWithPopoversClosedAtLastEvent.find(portID) != portsWithPopoversClosedAtLastEvent.end();
	if (!popoverJustClosedAtLastEvent)
		enablePopoverForPort(rp);
}

/**
 * Displays a new popover for the provided @c port.
 */
void VuoEditorComposition::enablePopoverForPort(VuoRendererPort *rp)
{
	if (!popoverEventsEnabled)
		return;

	VuoPort *port = rp->getBase();
	string portID = identifierCache->getIdentifierForPort(port);

	VUserLog("%s:      Open   popover for %s",
		window->getWindowTitleWithoutPlaceholder().toUtf8().data(),
		portID.c_str());

	dispatch_sync(runCompositionQueue, ^{  // Don't add any new popovers while the composition is starting. https://b33p.net/kosada/node/15572

		dispatch_sync(activePortPopoversQueue, ^{

			if (activePortPopovers.find(portID) == activePortPopovers.end())
			{
				// Assigning the popover a parent widget allows us to give it rounded corners
				// and a background fill that respects its rounded boundaries.
				VuoPortPopover *popover = new VuoPortPopover(port, this, views()[0]->viewport());

				connect(popover, &VuoPortPopover::popoverClosedForPort, this, &VuoEditorComposition::disablePopoverForPortThreadSafe);
				connect(popover, &VuoPortPopover::popoverDetachedFromPort, [=]{
					VUserLog("%s:      Detach popover for %s",
						window->getWindowTitleWithoutPlaceholder().toUtf8().data(),
						portID.c_str());
					popoverDetached();
				});
				connect(popover, &VuoPortPopover::popoverResized, this, &VuoEditorComposition::repositionPopover);
				connect(this, &VuoEditorComposition::compositionOnTop, popover, &VuoPortPopover::setWindowLevelAndVisibility);
				connect(this, &VuoEditorComposition::applicationActive, popover, &VuoPortPopover::setWindowLevel);

				// Line up the top left of the dialog with the port.
				QPoint portLeftInScene = port->getRenderer()->scenePos().toPoint() - QPoint(port->getRenderer()->getPortRect().width()/2., 0);

				// Don't let popovers get cut off at the right or bottom edges of the canvas.
				const int cutoffMargin = 16;
				QRectF viewportRect = views()[0]->mapToScene(views()[0]->viewport()->rect()).boundingRect();
				if (portLeftInScene.x() + popover->size().width() + cutoffMargin > viewportRect.right())
					portLeftInScene = QPoint(viewportRect.right() - popover->size().width() - cutoffMargin, portLeftInScene.y());
				if (portLeftInScene.y() + popover->size().height() + cutoffMargin > viewportRect.bottom())
					portLeftInScene = QPoint(portLeftInScene.x(), viewportRect.bottom() - popover->size().height() - cutoffMargin);

				QPoint popoverLeftInView = views()[0]->mapFromScene(portLeftInScene);

				const QPoint offset = QPoint(12, 6);

				QPoint popoverTopLeft = popoverLeftInView + offset;
				popover->move(popoverTopLeft);
				popover->show();

				activePortPopovers[portID] = popover;

				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{  // Get off of runCompositionQueue. https://b33p.net/kosada/node/14612
					updateDataInPortPopover(portID);
				});
			}
		});
	});
}

/**
 * Displays a new popover for the provided @c node, provided the node
 * is not currently rendered in drawer form.
 */
void VuoEditorComposition::enablePopoverForNode(VuoRendererNode *rn)
{
	if (popoverEventsEnabled && !dynamic_cast<VuoRendererInputDrawer *>(rn))
		emit nodePopoverRequestedForClass(rn->getBase()->getNodeClass());
}

/**
 * Closes the popover for the port with the provided @c portID.
 *
 * The caller should be on the main thread and schedule this function synchronously on `activePortPopoversQueue`.
 *
 * @threadQueue{activePortPopoversQueue}
 */
void VuoEditorComposition::disablePopoverForPort(string portID)
{
	VUserLog("%s:      Close  popover for %s",
		window->getWindowTitleWithoutPlaceholder().toUtf8().data(),
		portID.c_str());

	VuoPortPopover *popover = NULL;
	map<string, VuoPortPopover *>::iterator i = activePortPopovers.find(portID);
	if (i != activePortPopovers.end())
	{
		popover = i->second;
		activePortPopovers.erase(i);
	}

	if (popover)
	{
		popover->hide();
		popover->deleteLater();
	}

	bool isInput = false;
	bool foundPort = identifierCache->doForPortWithIdentifier(portID, [&isInput](VuoPort *port) {
		isInput = port->getRenderer()->getInput();
	});

	if (! foundPort)
		return;

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{  // Get off of activePortPopoversQueue.
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
		{
			dispatch_async(topLevelComposition->runCompositionQueue, ^{
				if (topLevelComposition->isRunningThreadUnsafe())
				{
					(isInput ?
					  topLevelComposition->runner->unsubscribeFromInputPortTelemetry(thisCompositionIdentifier, portID) :
					  topLevelComposition->runner->unsubscribeFromOutputPortTelemetry(thisCompositionIdentifier, portID));
				}
			});
		});
	});
}

/**
 * Closes the popover for the port with the provided @c portID.
 */
void VuoEditorComposition::disablePopoverForPortThreadSafe(string portID)
{
	dispatch_sync(activePortPopoversQueue, ^{
		disablePopoverForPort(portID);
	});
}

/**
 * Disables all popovers associated with this composition, whether they are detached or not.
 */
void VuoEditorComposition::disablePopovers()
{
	disablePortPopovers();
	disableErrorPopovers();
}

/**
 * Disables all error popovers associated with this composition.
 */
void VuoEditorComposition::disableErrorPopovers()
{
	foreach (VuoErrorPopover *errorPopover, errorPopovers)
	{
		errorPopover->hide();
		errorPopover->deleteLater();
	}

	errorPopovers.clear();
}

/**
 * Disables all port popovers associated with this composition, whether they are detached or not.
 * If an input @c node is provided, only popovers whose ports belong to that node are disabled.
 */
void VuoEditorComposition::disablePortPopovers(VuoRendererNode *node)
{
	dispatch_sync(activePortPopoversQueue, ^{
		map<string, VuoPortPopover *> popoversToDisable = activePortPopovers;
		for (map<string, VuoPortPopover *>::iterator i = popoversToDisable.begin(); i != popoversToDisable.end(); ++i)
		{
			string portID = i->first;
			bool shouldDisable = false;

			if (! node)
				shouldDisable = true;
			else
			{
				identifierCache->doForPortWithIdentifier(portID, [&shouldDisable, node](VuoPort *port) {
					shouldDisable = port->hasRenderer() && (port->getRenderer()->getUnderlyingParentNode() == node);
				});
			}

			if (shouldDisable)
				disablePopoverForPort(portID);
		}
	});
}

/**
 * Disables any port popover that does not currently have an associated port in the composition.
 */
void VuoEditorComposition::disableStrandedPortPopovers()
{
	dispatch_sync(activePortPopoversQueue, ^{
		map<string, VuoPortPopover *> popoversToDisable = activePortPopovers;
		for (map<string, VuoPortPopover *>::iterator i = popoversToDisable.begin(); i != popoversToDisable.end(); ++i)
		{
			string portID = i->first;

			bool foundPort = identifierCache->doForPortWithIdentifier(portID, [](VuoPort *port) {});
			if (! foundPort)
				disablePopoverForPort(portID);
		}
	});
}

/**
 * Disables non-detached port popovers associated with this composition.
 * If an input @c node is provided, only non-detached popovers whose ports belong to that node are disabled.
 */
void VuoEditorComposition::disableNondetachedPortPopovers(VuoRendererNode *node, bool recordWhichPopoversClosed)
{
	if (recordWhichPopoversClosed)
		portsWithPopoversClosedAtLastEvent.clear();

	dispatch_sync(activePortPopoversQueue, ^{
		map<string, VuoPortPopover *> popoversToDisable = activePortPopovers;
		for (map<string, VuoPortPopover *>::iterator i = popoversToDisable.begin(); i != popoversToDisable.end(); ++i)
		{
			string portID = i->first;
			bool shouldDisable = false;

			if (! node)
				shouldDisable = true;
			else
			{
				identifierCache->doForPortWithIdentifier(portID, [&shouldDisable, node](VuoPort *port) {
					shouldDisable = port->hasRenderer() && (port->getRenderer()->getUnderlyingParentNode() == node);
				});
			}

			if (shouldDisable)
			{
				VuoPortPopover *popover = getActivePopoverForPort(portID);
				if (! (popover && popover->getDetached()))
				{
					disablePopoverForPort(portID);
					portsWithPopoversClosedAtLastEvent.insert(portID);
				}
			}
		}
	});
}

/**
 * Moves all popovers associated with this composition @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::movePopoversBy(int dx, int dy)
{
	moveDetachedPortPopoversBy(dx, dy);
	moveErrorPopoversBy(dx, dy);
}

/**
 * Moves all error popovers associated with this composition @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveErrorPopoversBy(int dx, int dy)
{
	foreach(VuoErrorPopover *errorPopover, errorPopovers)
		errorPopover->move(errorPopover->pos().x()+dx, errorPopover->pos().y()+dy);
}

/**
 * Moves all detached port popovers associated with this composition @c dx points horizontally and @c dy points vertically.
 */
void VuoEditorComposition::moveDetachedPortPopoversBy(int dx, int dy)
{
	dispatch_sync(activePortPopoversQueue, ^{
		map<string, VuoPortPopover *> portPopovers = activePortPopovers;
		for (map<string, VuoPortPopover *>::iterator i = portPopovers.begin(); i != portPopovers.end(); ++i)
		{
			VuoPortPopover *popover = i->second;
			if (popover && popover->getDetached())
				popover->move(popover->pos().x()+dx, popover->pos().y()+dy);
		}
	});
}

/**
 * Tells detached popovers whether they should hide themselves when the user switches to another app.
 */
void VuoEditorComposition::setPopoversHideOnDeactivate(bool shouldHide)
{
	dispatch_sync(activePortPopoversQueue, ^{
		auto portPopovers = activePortPopovers;
		for (auto i : portPopovers)
		{
			VuoPortPopover *popover = i.second;
			if (popover && popover->getDetached())
			{
				id nsWindow = (id)VuoPopover::getWindowForPopover(popover);
				((void (*)(id, SEL, BOOL))objc_msgSend)(nsWindow, sel_getUid("setHidesOnDeactivate:"), shouldHide);
			}
		}
	});
}

/**
 * Updates the text of all active popovers associated with this composition.
 * If an input @c node is provided, only popovers whose ports belong to that node are disabled.
 */
void VuoEditorComposition::updatePortPopovers(VuoRendererNode *node)
{
	dispatch_sync(activePortPopoversQueue, ^{
		for (map<string, VuoPortPopover *>::iterator i = activePortPopovers.begin(); i != activePortPopovers.end(); ++i)
		{
			string portID = i->first;
			VuoPortPopover *popover = i->second;
			bool shouldUpdate = false;

			if (! node)
				shouldUpdate = true;
			else
			{
				identifierCache->doForPortWithIdentifier(portID, [&shouldUpdate, node](VuoPort *port) {
					shouldUpdate = port->hasRenderer() && (port->getRenderer()->getUnderlyingParentNode() == node);
				});
			}

			if (shouldUpdate)
				QMetaObject::invokeMethod(popover, "updateTextAndResize", Qt::QueuedConnection);
		}
	});
}

/**
 * Updates the data value displayed in the port's popover, and subscribes to further updates for the port.
 *
 * Call this function only when `this` is a top-level composition and only when it's running.
 *
 * @param popoverComposition The (sub)composition containing the port.
 * @param popoverCompositionIdentifier The identifier of the (sub)composition containing the port.
 * @param portID The port whose popover is to be updated.
 *
 * @threadQueue{runCompositionQueue}
 */
void VuoEditorComposition::updateDataInPortPopoverFromRunningTopLevelComposition(VuoEditorComposition *popoverComposition,
																				 string popoverCompositionIdentifier,
																				 string portID)
{
	bool isInput;
	bool foundPort = popoverComposition->identifierCache->doForPortWithIdentifier(portID, [&isInput](VuoPort *port) {
		isInput = port->getRenderer()->getInput();
	});

	if (! foundPort)
		return;

	string portSummary = (isInput ?
							  runner->subscribeToInputPortTelemetry(popoverCompositionIdentifier, portID) :
							  runner->subscribeToOutputPortTelemetry(popoverCompositionIdentifier, portID));

	dispatch_async(popoverComposition->activePortPopoversQueue, ^{
		VuoPortPopover *popover = popoverComposition->getActivePopoverForPort(portID);
		if (popover)
		{
			QMetaObject::invokeMethod(popover, "updateDataValueImmediately", Qt::QueuedConnection, Q_ARG(QString, portSummary.c_str()));
			QMetaObject::invokeMethod(popover, "setCompositionRunning", Qt::QueuedConnection, Q_ARG(bool, true), Q_ARG(bool, false));
		}
	});
}

/**
 * Updates the data value displayed in the port's popover, and subscribes to further updates for the port.
 *
 * @param portID A port in the current composition (`this`).
 *
 * @threadNoQueue{runCompositionQueue}
 */
void VuoEditorComposition::updateDataInPortPopover(string portID)
{
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		dispatch_sync(topLevelComposition->runCompositionQueue, ^{
			if (topLevelComposition->isRunningThreadUnsafe())
				topLevelComposition->updateDataInPortPopoverFromRunningTopLevelComposition(this, thisCompositionIdentifier, portID);
		});
	});
}

/**
 * This delegate method is invoked every time any input port receives an event or data.
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier,
															 bool receivedEvent, bool receivedData, string dataSummary)
{
	void (^updatePortDisplay)(VuoEditorComposition *) = ^void (VuoEditorComposition *matchingComposition)
	{
		dispatch_sync(matchingComposition->activePortPopoversQueue, ^{
			VuoPortPopover *popover = matchingComposition->getActivePopoverForPort(portIdentifier);
			if (popover)
			   QMetaObject::invokeMethod(popover, "updateLastEventTimeAndDataValue", Qt::QueuedConnection,
										 Q_ARG(bool, receivedEvent),
										 Q_ARG(bool, receivedData),
										 Q_ARG(QString, dataSummary.c_str()));
		});
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedCompositionWithIdentifier(this, compositionIdentifier, updatePortDisplay);
}

/**
 * This delegate method is invoked every time any output port transmits or fires an event.
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier,
															  bool sentEvent, bool sentData, string dataSummary)
{
	void (^updatePortDisplay)(VuoEditorComposition *) = ^void (VuoEditorComposition *matchingComposition)
	{
		dispatch_sync(matchingComposition->activePortPopoversQueue, ^{
			VuoPortPopover *popover = matchingComposition->getActivePopoverForPort(portIdentifier);
			if (popover)
				QMetaObject::invokeMethod(popover, "updateLastEventTimeAndDataValue", Qt::QueuedConnection,
										  Q_ARG(bool, sentEvent),
										  Q_ARG(bool, sentData),
										  Q_ARG(QString, dataSummary.c_str()));
		});

		if (matchingComposition->showEventsMode && sentEvent)
		{
			matchingComposition->identifierCache->doForPortWithIdentifier(portIdentifier, [matchingComposition](VuoPort *port) {
				if (dynamic_cast<VuoCompilerTriggerPort *>(port->getCompiler()) && port->hasRenderer())
				{
					port->getRenderer()->setFiredEvent();
					matchingComposition->animatePort(port->getRenderer());
				}
			});
		}
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedCompositionWithIdentifier(this, compositionIdentifier, updatePortDisplay);
}

/**
 * This delegate method is invoked every time any trigger port drops an event.
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::receivedTelemetryEventDropped(string compositionIdentifier, string portIdentifier)
{
	void (^updatePortDisplay)(VuoEditorComposition *) = ^void (VuoEditorComposition *matchingComposition)
	{
		dispatch_async(matchingComposition->runCompositionQueue, ^{
			if (matchingComposition->isRunningThreadUnsafe())
			{
				dispatch_sync(matchingComposition->activePortPopoversQueue, ^{
					VuoPortPopover *popover = matchingComposition->getActivePopoverForPort(portIdentifier);
					if (popover)
						QMetaObject::invokeMethod(popover, "incrementDroppedEventCount", Qt::QueuedConnection);
				});
			}
		});
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedCompositionWithIdentifier(this, compositionIdentifier, updatePortDisplay);
}

/**
 * This delegate method is invoked every time a node has started executing.
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier)
{
	void (^updateNodeDisplay)(VuoEditorComposition *) = ^void (VuoEditorComposition *matchingComposition)
	{
		if (matchingComposition->showEventsMode)
		{
			dispatch_async(this->runCompositionQueue, ^{
				if (this->isRunningThreadUnsafe())
				{
					matchingComposition->identifierCache->doForNodeWithIdentifier(nodeIdentifier, [](VuoNode *node) {
						node->getRenderer()->setExecutionBegun();
					});
				}
			});
		}
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedCompositionWithIdentifier(this, compositionIdentifier, updateNodeDisplay);
}

/**
 * This delegate method is invoked every time a node has finished executing.
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier)
{
	void (^updateNodeDisplay)(VuoEditorComposition *) = ^void (VuoEditorComposition *matchingComposition)
	{
		if (matchingComposition->showEventsMode)
		{
			dispatch_async(this->runCompositionQueue, ^{
				if (this->isRunningThreadUnsafe())
				{
					matchingComposition->identifierCache->doForNodeWithIdentifier(nodeIdentifier, [](VuoNode *node) {
						node->getRenderer()->setExecutionEnded();
					});
				}
		   });
		}
	};
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedCompositionWithIdentifier(this, compositionIdentifier, updateNodeDisplay);
}

/**
 * Called if the user quits the composition or the composition crashes. Updates the UI to
 * show that the composition has stopped.
 *
 * Implementation of the virtual VuoRunnerDelegate function.
 */
void VuoEditorComposition::lostContactWithComposition(void)
{
	emit compositionStoppedItself();
}

/**
 * Prepares every component in the composition to be repainted (slot version
 * that simply calls the VuoRendererComposition non-slot version of the
 * same method).
 */
void VuoEditorComposition::updateGeometryForAllComponents()
{
	VuoRendererComposition::updateGeometryForAllComponents();
}

/**
 * Returns a boolean indicating whether this composition is currently in 'Show Events' mode.
 */
bool VuoEditorComposition::getShowEventsMode()
{
	return showEventsMode;
}

/**
 * Sets the boolean indicating whether this composition is currently in 'Show Events' mode.
 */
void VuoEditorComposition::setShowEventsMode(bool showEventsMode)
{
	this->showEventsMode = showEventsMode;

	if (showEventsMode)
	{
		beginDisplayingActivity();

		void (^subscribe)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
		{
			dispatch_sync(topLevelComposition->runCompositionQueue, ^{
				if (topLevelComposition->isRunningThreadUnsafe())
					topLevelComposition->runner->subscribeToEventTelemetry(thisCompositionIdentifier);
			});
		};
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, subscribe);
	}
	else
	{
		stopDisplayingActivity();

		void (^unsubscribe)(VuoEditorComposition *, string) = ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
		{
			dispatch_sync(topLevelComposition->runCompositionQueue, ^{
				if (topLevelComposition->isRunningThreadUnsafe())
					topLevelComposition->runner->unsubscribeFromEventTelemetry(thisCompositionIdentifier);
			});
		};
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(this, unsubscribe);
	}
}

/**
 * Returns a boolean indicating whether the composition contains any hidden (wireless) internal cables.
 */
bool VuoEditorComposition::hasHiddenInternalCables()
{
	foreach (VuoCable *cable, getBase()->getCables())
	{
		if (cable->getCompiler()->getHidden() && !cable->isPublished())
			return true;
	}

	return false;
}

/**
 * Returns a boolean indicating whether the composition contains any hidden (wireless) published cables.
 */
bool VuoEditorComposition::hasHiddenPublishedCables()
{
	foreach (VuoCable *cable, getBase()->getCables())
	{
		if (cable->hasRenderer() && cable->getRenderer()->getEffectivelyWireless() && cable->isPublished())
			return true;
	}

	return false;
}

/**
 * Defines the initial parameters and makes the necessary connections for
 * the given @c port 's 'Show Events'-mode animation.
 */
QGraphicsItemAnimation * VuoEditorComposition::setUpAnimationForPort(QGraphicsItemAnimation *animation, VuoRendererPort *port)
{
	VuoRendererPort *animatedPort = new VuoRendererPort(new VuoPort(port->getBase()->getClass()),
														NULL,
														port->getOutput(),
														port->getRefreshPort(),
														port->getFunctionPort());
	animatedPort->setAnimated(true);
	animatedPort->setZValue(VuoRendererItem::triggerAnimationZValue);
	animatedPort->setParentItem(port->getRenderedParentNode());

	animation->setItem(animatedPort);
	animation->setScaleAt(0.0, 1, 1);
	animation->setScaleAt(0.999, 3, 3);
	animation->setScaleAt(1.0, 1, 1);

	QTimeLine *animationTimeline = animation->timeLine();
	animationTimeline->setFrameRange(0, 100);
	animationTimeline->setUpdateInterval(showEventsModeUpdateInterval);
	animationTimeline->setCurveShape(QTimeLine::LinearCurve);

	preparedAnimations.insert(animation);
	animationForTimeline[animation->timeLine()] = animation;

	connect(animationTimeline, &QTimeLine::valueChanged, this, &VuoEditorComposition::updatePortAnimation);
	connect(animationTimeline, &QTimeLine::finished, this, &VuoEditorComposition::endPortAnimation);

	return animation;
}

/**
 * Initiates the given @c port 's 'Show Events'-mode animation.
 */
void VuoEditorComposition::animatePort(VuoRendererPort *port)
{
	dispatch_async(dispatch_get_main_queue(), ^{
	QGraphicsItemAnimation *animation = getAvailableAnimationForPort(port);
	if (! animation)
		return;

	VuoRendererPort *animatedPort = (VuoRendererPort *)(animation->item());

	if (animation->timeLine()->state() == QTimeLine::Running)
		animation->timeLine()->setCurrentTime(0);

	else
	{
		animatedPort->setPos(port->pos());
		animatedPort->setVisible(true);
		animation->timeLine()->start();
	}
	});
}

/**
 * Returns an available animation for the provided @c port, or NULL if no
 * animation is currently available.
 */
QGraphicsItemAnimation * VuoEditorComposition::getAvailableAnimationForPort(VuoRendererPort *port)
{
	vector<QGraphicsItemAnimation *> animations = port->getAnimations();

	QGraphicsItemAnimation *mostAdvancedAnimation = NULL;
	qreal maxPercentAdvanced = -1;

	for (int i = 0; i < animations.size(); ++i)
	{
		QGraphicsItemAnimation *animation = animations[i];
		bool animationPrepared = (preparedAnimations.find(animation) != preparedAnimations.end());
		bool animationRunning = (animation->timeLine()->state() == QTimeLine::Running);

		if (! animationPrepared)
			return setUpAnimationForPort(animation, port);

		else if (! animationRunning)
			return animation;

		// If all of the port's animations are already running, return the
		// one that has been running the longest.
		qreal percentAdvanced = animation->timeLine()->currentValue();
		if (percentAdvanced > maxPercentAdvanced)
		{
			mostAdvancedAnimation = animation;
			maxPercentAdvanced = percentAdvanced;
		}
	}

	// If no animation is even halfway complete, return NULL to indicate
	// that no animation is currently available.
	return (maxPercentAdvanced >= 0.5? mostAdvancedAnimation : NULL);
}

/**
 * Updates the animation of the port associated with the sender *QTimeLine
 * that signalled this slot.  The port's fade percentage is updated to
 * to reflect the provided @c value (expected range [0.0-1.0]).
 */
void VuoEditorComposition::updatePortAnimation(qreal value)
{
	QTimeLine *animationTimeline = (QTimeLine *)sender();
	QGraphicsItemAnimation *animation = animationForTimeline[animationTimeline];
	VuoRendererPort *animatedPort = (VuoRendererPort *)(animation->item());
	const qreal multiplier = 1000.;
	animatedPort->setFadePercentageSinceEventFired(pow((multiplier*value),2)/pow(multiplier,2));
}

/**
 * Ends the animation of the port associated with the sender *QTimeLine
 * that signalled this slot.
 */
void VuoEditorComposition::endPortAnimation(void)
{
	QTimeLine *animationTimeline = (QTimeLine *)sender();
	QGraphicsItemAnimation *animation = animationForTimeline[animationTimeline];
	VuoRendererPort *animatedPort = (VuoRendererPort *)(animation->item());
	animatedPort->setVisible(false);
}

/**
* Sets the boolean indicating whether to disable drag stickiness within the canvas.
*/
void VuoEditorComposition::setDisableDragStickiness(bool disable)
{
	this->dragStickinessDisabled = disable;
}

/**
* Sets the boolean indicating whether application state change events should currently be ignored.
*/
void VuoEditorComposition::setIgnoreApplicationStateChangeEvents(bool ignore)
{
	this->ignoreApplicationStateChangeEvents = ignore;
}

/**
* Sets the boolean indicating whether to display popovers for
* components within the canvas in response to mouse events from
* this point on. This will not affect existing popovers.
*/
void VuoEditorComposition::setPopoverEventsEnabled(bool enable)
{
	this->popoverEventsEnabled = enable;
}

/**
 * Begin reflecting events and executions in the rendering of the composition.
 */
void VuoEditorComposition::beginDisplayingActivity(bool includePorts)
{
	setRenderActivity(true, includePorts);
	refreshComponentAlphaLevelTimer->start();
}

/**
 * Stop reflecting events and executions in the rendering of the composition.
 */
void VuoEditorComposition::stopDisplayingActivity()
{
	refreshComponentAlphaLevelTimer->stop();
	setRenderActivity(false);
}

/**
 * Checks whether the composition superficially meets the requirements
 * as an image filter/generator/transition protocol composition eligible for movie or plugin export.
 * This check may be used to prevent the most commonly anticipated situations
 * in which export might fail; it is not authoritative.
 *
 * If a problem is detected, shows an error dialog and returns false.
 *
 * For Image Filters, checks whether the `image` and `outputImage` ports both have a connected cable.
 *
 * For Image Generators, checks whether the `time` and `outputImage` ports both have a connected cable.
 *
 * For Image Transitions, checks whether the `progress` and `outputImage` ports both have a connected cable.
 *
 * @todo: More comprehensive check to be implemented for https://b33p.net/kosada/node/9807 .
 */
bool VuoEditorComposition::validateProtocol(VuoEditorWindow *window, bool isExportingMovie)
{
	// This should never happen if we've enabled the "Export" menu options in the appropriate contexts.
	if (!activeProtocol)
	{
		VuoErrorDialog::show(window, "To export, activate a protocol.", "");
		return false;
	}

	// Can events from at least one trigger reach at least one published output port? If not, report an error.
	if (! getBase()->getCompiler()->getCachedGraph()->mayEventsReachPublishedOutputPorts())
	{
		QString errorHeadline = tr("<b>This composition doesn't send any images to <code>outputImage</code>.</b>");
		QString errorDetails = tr("<p>To export, your composition should use the data and events from the published input ports "
				"to output a stream of images through the <code>outputImage</code> published output port.</p>");

		if (isExportingMovie)
			errorDetails.append("<p>Alternatively, you can record a realtime movie by running the composition and selecting File > Start Recording.</p>");

		VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();
		QMessageBox messageBox(window);
		messageBox.setWindowFlags(Qt::Sheet);
		messageBox.setWindowModality(Qt::WindowModal);
		messageBox.setFont(fonts->dialogHeadingFont());
		messageBox.setTextFormat(Qt::RichText);

		messageBox.setStandardButtons(QMessageBox::Help | QMessageBox::Ok);
		messageBox.setButtonText(QMessageBox::Help, tr("Open an Example"));
		messageBox.setButtonText(QMessageBox::Ok, tr("OK"));
		messageBox.setDefaultButton(QMessageBox::Ok);

		messageBox.setText(errorHeadline);
		messageBox.setInformativeText("<style>p{" + fonts->getCSS(fonts->dialogBodyFont()) + "}</style>" + errorDetails);

		if (messageBox.exec() == QMessageBox::Help)
		{
			map<QString, QString> examples = static_cast<VuoEditor *>(qApp)->getExampleCompositionsForProtocol(activeProtocol);
			map<QString, QString>::iterator i = examples.begin();
			if (i != examples.end())
				QDesktopServices::openUrl(QUrl(VuoEditor::getURLForExampleComposition(i->first, i->second)));
		}
		return false;
	}

	return true;
}

/**
 * Returns a string representation of the composition (to save its current state).
 */
string VuoEditorComposition::takeSnapshot(void)
{
	return (getBase()->hasCompiler()? getBase()->getCompiler()->getGraphvizDeclaration(getActiveProtocol(), generateCompositionHeader()) : "");
}

/**
 * Generates a header containing this composition's metadata.
 */
string VuoEditorComposition::generateCompositionHeader()
{
	return getBase()->getMetadata()->toCompositionHeader() + VuoCompilerComposition::defaultGraphDeclaration;
}

/**
 * Returns the name to be used for a composition located at @a compositionPath if the user hasn't specified a name.
 */
string VuoEditorComposition::getDefaultNameForPath(const string &compositionPath)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
	return file;
}

/**
 * Returns the formatted name of the composition, either pre-formatted by the user or,
 * otherwise, formatted automatically.
 *
 * @return The human-readable composition file name.
 */
QString VuoEditorComposition::getFormattedName()
{
	string customizedName = getBase()->getMetadata()->getCustomizedName();
	if (! customizedName.empty())
		return QString::fromStdString(customizedName);

	string name = getBase()->getMetadata()->getName();
	return formatCompositionFileNameForDisplay(QString::fromStdString(name));
}

/**
 * Formats the input @c unformattedCompositionFileName for human-readable display
 * by stripping the .vuo extension, capitalizing the first word, and inserting spaces
 * between CamelCase transitions.
 *
 * @return The human-readable composition file name.
 */
QString VuoEditorComposition::formatCompositionFileNameForDisplay(QString unformattedCompositionFileName)
{
	// Remove the file extension. Do this correctly even for subcompositions whose filenames contain dot-delimited segments.
	// If the extensionless filename contains dot-delimited segments, use only the final segment.
	vector<string> fileNameParts = VuoStringUtilities::split(unformattedCompositionFileName.toUtf8().constData(), '.');
	string fileNameContentPart = (fileNameParts.size() >= 2 && fileNameParts[fileNameParts.size()-1] == "vuo"?
									 fileNameParts[fileNameParts.size()-2] :
								 (fileNameParts.size() >= 1? fileNameParts[fileNameParts.size()-1] : ""));

	// If the filename already contains spaces, init-cap the first word but otherwise leave the formatting alone.
	if (QRegExp("\\s").indexIn(fileNameContentPart.c_str()) != -1)
	{
		string formattedName = fileNameContentPart;
		if (formattedName.size() >= 1)
			formattedName[0] = toupper(formattedName[0]);

		return QString(formattedName.c_str());
	}

	// Otherwise, init-cap the first word and insert spaces among CamelCase transitions.
	return QString(VuoStringUtilities::expandCamelCase(fileNameContentPart).c_str());
}

/**
 * Formats the input @c nodeSetName for human-readable display.
 *
 * @return The human-readable node set name.
 */
QString VuoEditorComposition::formatNodeSetNameForDisplay(QString nodeSetName)
{
	QStringList wordsInName = nodeSetName.split(QRegularExpression("\\."));
	if (wordsInName.size() < 2 || wordsInName[0] != "vuo")
	{
		// If not an official Vuo nodeset, return the name as-is.
		return nodeSetName;
	}

	map<QString, QString> wordsToReformat;
	wordsToReformat["artnet"] = "Art-Net";
	wordsToReformat["bcf2000"] = "BCF2000";
	wordsToReformat["hid"] = "HID";
	wordsToReformat["midi"] = "MIDI";
	wordsToReformat["ndi"] = "NDI";
	wordsToReformat["osc"] = "OSC";
	wordsToReformat["rss"] = "RSS";
	wordsToReformat["ui"] = "UI";
	wordsToReformat["url"] = "URL";

	QString nodeSetDisplayName = "";
	for (int i = 1; i < wordsInName.size(); ++i)
	{
		QString currentWord = wordsInName[i];
		if (currentWord.size() >= 1)
		{
			if (wordsToReformat.find(currentWord.toLower()) != wordsToReformat.end())
				currentWord = wordsToReformat.at(currentWord.toLower());
			else
				currentWord[0] = currentWord[0].toUpper();

			nodeSetDisplayName += currentWord;

			if (i < wordsInName.size()-1)
				nodeSetDisplayName += " ";
		}
	}
	return nodeSetDisplayName;
}

/**
 * Returns a human-readable title for the concrete type (not unspecialized generic) with module key @a typeName.
 */
QString VuoEditorComposition::formatTypeNameForDisplay(string typeName)
{
	set<string> listTypes = moduleManager->getKnownListTypeNames(false);
	auto foundListType = listTypes.find(typeName);
	if (foundListType != listTypes.end())
	{
		// Don't request the list type from the compiler, since that causes a delay if the type is not already generated.
		auto getVuoType = [this] (const string &typeName) -> VuoCompilerType *
		{
			map<string, VuoCompilerType *> singletonTypes = moduleManager->getLoadedSingletonTypes(false);
			VuoCompilerType *singletonType = singletonTypes[typeName];
			if (singletonType)
				return singletonType;

			map<string, VuoCompilerType *> compoundTypes = moduleManager->getLoadedGenericCompoundTypes();
			VuoCompilerType *compoundType = compoundTypes[typeName];
			if (compoundType)
				return compoundType;

			return nullptr;
		};
		string defaultTitle = VuoCompilerCompoundType::buildDefaultTitle(typeName, getVuoType);
		return QString::fromStdString(defaultTitle);
	}

	map<string, VuoCompilerType *> singletonTypes = moduleManager->getLoadedSingletonTypes(false);
	VuoCompilerType *singletonType = singletonTypes[typeName];
	return formatTypeNameForDisplay(singletonType ? singletonType->getBase() : nullptr);
}

/**
 * Returns a human-readable title for @a type.
 */
QString VuoEditorComposition::formatTypeNameForDisplay(VuoType *type)
{
	if (! type)
		return "(none)";

	string defaultTitle = type->getDefaultTitle();
	return QString::fromStdString(defaultTitle);
}

/**
 * Returns a suggested name for a published port of the provided type.
 */
string VuoEditorComposition::getDefaultPublishedPortNameForType(VuoType *type)
{
	if (!type)
		return "Event";

	// Special handling for points and transforms so that the initial numeral in their display name doesn't get sanitized away.
	else if (type->getDefaultTitle() == "2D Point")
		return "Point2D";
	else if (type->getDefaultTitle() == "3D Point")
		return "Point3D";
	else if (type->getDefaultTitle() == "4D Point")
		return "Point4D";
	else if (type->getDefaultTitle() == "2D Transform")
		return "Transform2D";
	else if (type->getDefaultTitle() == "3D Transform")
		return "Transform3D";

	return VuoRendererPort::sanitizePortName(formatTypeNameForDisplay(type)).toUtf8().constData();
}

/**
 * Comparison function for QGraphicsItem pointers.
 * Sorts primarily by y-coordinate (top to bottom), and secondarily by x-coordinate (left to right).
 */
bool VuoEditorComposition::itemHigherOnCanvas(QGraphicsItem *item1, QGraphicsItem *item2)
{
	qreal item1Y = item1->scenePos().y();
	qreal item2Y = item2->scenePos().y();

	qreal item1X = item1->scenePos().x();
	qreal item2X = item2->scenePos().x();

	if (item1Y == item2Y)
		return (item1X < item2X);

	return item1Y < item2Y;
}

/**
 * Calculates a similarity score between the two provided node classes.
 *
 * Returns a score between 0 (not similar at all) and 1 (the same, as far as
 * this metric is concerned).
 */
double VuoEditorComposition::calculateNodeSimilarity(VuoNodeClass *node1, VuoNodeClass *node2)
{
	// Assign replacement (successor) node classes a perfect match score.
	{
		string originalGenericNode1ClassName, originalGenericNode2ClassName;
		if (node1->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(node1->getCompiler()))
			originalGenericNode1ClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node1->getCompiler())->getOriginalGenericNodeClassName();
		else
			originalGenericNode1ClassName = node1->getClassName();

		if (node2->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(node2->getCompiler()))
			originalGenericNode2ClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node2->getCompiler())->getOriginalGenericNodeClassName();
		else
			originalGenericNode2ClassName = node2->getClassName();

		if (VuoEditorUtilities::isNodeClassSuccessorTo(originalGenericNode1ClassName.c_str(), originalGenericNode2ClassName.c_str()))
			return 1;
	}

	// Compare keywords.
	vector<string> node1Keywords = node1->getKeywords();
	vector<string> node2Keywords = node2->getKeywords();

	// Compare node set names.
	if (node1->getNodeSet())
		node1Keywords.push_back(node1->getNodeSet()->getName());

	if (node2->getNodeSet())
		node2Keywords.push_back(node2->getNodeSet()->getName());

	// Compare tokens in node class display names.
	foreach (QString nodeTitleToken, VuoNodeLibrary::tokenizeNodeName(node1->getDefaultTitle().c_str(), ""))
		if (!VuoNodeLibrary::isStopWord(nodeTitleToken.toLower()))
			node1Keywords.push_back(nodeTitleToken.toLower().toUtf8().constData());

	foreach (QString nodeTitleToken, VuoNodeLibrary::tokenizeNodeName(node2->getDefaultTitle().c_str(), ""))
		if (!VuoNodeLibrary::isStopWord(nodeTitleToken.toLower()))
			node2Keywords.push_back(nodeTitleToken.toLower().toUtf8().constData());

	set<string> node1KeywordSet(node1Keywords.begin(), node1Keywords.end());
	set<string> node2KeywordSet(node2Keywords.begin(), node2Keywords.end());

	set<string> nodeKeywordsIntersection;
	std::set_intersection(node1KeywordSet.begin(), node1KeywordSet.end(),
						  node2KeywordSet.begin(), node2KeywordSet.end(),
						  std::inserter(nodeKeywordsIntersection, nodeKeywordsIntersection.end()));

	set<string> nodeKeywordsUnion = node1KeywordSet;
	nodeKeywordsUnion.insert(node2KeywordSet.begin(), node2KeywordSet.end());

	// Avoid division by zero.
	if (nodeKeywordsUnion.size() == 0)
		return 0;

	// Calculate Jaccard similarity.
	double nodeSimilarity = nodeKeywordsIntersection.size()/(1.0*nodeKeywordsUnion.size());

	return nodeSimilarity;
}

/**
 * Emits a @c compositionOnTop signal.
 */
void VuoEditorComposition::emitCompositionOnTop(bool top)
{
	emit compositionOnTop(top);
}

/**
 * Emits a @c publishedPortNameEditorRequested signal.
 */
void VuoEditorComposition::emitPublishedPortNameEditorRequested(VuoRendererPublishedPort *port)
{
	emit publishedPortNameEditorRequested(port, false);
}

VuoEditorComposition::~VuoEditorComposition()
{
	dispatch_sync(runCompositionQueue, ^{});
	dispatch_release(runCompositionQueue);

	preparedAnimations.clear();
	animationForTimeline.clear();

	delete identifierCache;

	delete moduleManager;  // deletes compiler
}

/**
 * Makes the widget dark.
 */
void VuoEditorComposition::setColor(bool isDark)
{
	// Update the canvas color.
	setBackgroundTransparent(false);

	// Force repainting the entire canvas.
	setComponentCaching(QGraphicsItem::NoCache);
	updateGeometryForAllComponents();
	setComponentCaching(getCurrentDefaultCacheMode());
}

/**
 * Publishes the ports with the provided identifiers in order of their y-coordinates on canvas,
 * from top to bottom.
 * Returns the mappings between the original port identifiers and their published identifiers.
 */
map<string, string> VuoEditorComposition::publishPorts(set<string> portsToPublish)
{
	vector<VuoRendererPort *> sortedPortsToPublish;
	foreach (string portID, portsToPublish)
	{
		VuoPort *port = getPortWithStaticIdentifier(portID);
		if (port && port->hasRenderer())
			sortedPortsToPublish.push_back(port->getRenderer());
	}
	std::sort(sortedPortsToPublish.begin(), sortedPortsToPublish.end(), itemHigherOnCanvas);

	map<string, string> publishedPortNames;
	foreach (VuoRendererPort *rp, sortedPortsToPublish)
	{
		rp->updateGeometry();
		VuoType *publishedPortType = ((VuoCompilerPortClass *)(rp->getBase()->getClass()->getCompiler()))->getDataVuoType();

		string specializedPublishedPortName = generateSpecialPublishedNameForPort(rp->getBase());
		string publishedPortName = (!specializedPublishedPortName.empty()?
										specializedPublishedPortName :
										VuoRendererPort::sanitizePortName(rp->getPortNameToRenderWhenDisplayed().c_str()).toUtf8().constData());

		bool forceEventOnlyPublication = rp->effectivelyHasConnectedDataCable(false);
		VuoRendererPort *publishedPort = publishInternalPort(rp->getBase(), forceEventOnlyPublication, publishedPortName, publishedPortType, false);

		publishedPortNames[getIdentifierForStaticPort(rp->getBase())] = publishedPort->getBase()->getClass()->getName();
	}

	return publishedPortNames;
}

/**
 * If the provided port is one of the handful of port types whose default published name
 * should be something other than the port's own name, returns that specialized name.
 * Otherwise, returns the empty string.
 */
string VuoEditorComposition::generateSpecialPublishedNameForPort(VuoPort *port)
{
	if (!port || !port->hasRenderer() || !port->getRenderer()->getUnderlyingParentNode())
		return "";

	// If publishing a port on a "Share Value" node and the node's title has been
	// customized, request that title as the published port name.
	if (VuoStringUtilities::beginsWith(port->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(), "vuo.data.share") &&
			(port->getRenderer()->getUnderlyingParentNode()->getBase()->getTitle() !=
			port->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getDefaultTitle()))
	{
		return VuoRendererPort::sanitizePortName(port->getRenderer()->getUnderlyingParentNode()->getBase()->getTitle().c_str()).toUtf8().constData();
	}

	return "";
}

/**
 * Prevents non-detached popovers from being cut off at the right or bottom edges of the canvas after being resized.
 */
void VuoEditorComposition::repositionPopover()
{
		VuoPortPopover *popover = static_cast<VuoPortPopover *>(QObject::sender());
		if (popover && !popover->getDetached())
		{
			const int cutoffMargin = 16;
			if (popover->pos().x()+popover->size().width()+cutoffMargin > views()[0]->viewport()->rect().right())
				popover->move(QPoint(views()[0]->viewport()->rect().right()-popover->size().width()-cutoffMargin, popover->pos().y()));

			if (popover->pos().y()+popover->size().height()+cutoffMargin > views()[0]->viewport()->rect().bottom())
				popover->move(QPoint(popover->pos().x(), views()[0]->viewport()->rect().bottom()-popover->size().height()-cutoffMargin));
		}
}
