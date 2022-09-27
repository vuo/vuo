/**
 * @file
 * VuoEditorWindow implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorWindow.hh"
#include "ui_VuoEditorWindow.h"

#include "VuoCompilerCable.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoComment.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoGenericType.hh"
#include "VuoCommandMove.hh"
#include "VuoCommandAdd.hh"
#include "VuoCommandAddPublishedPort.hh"
#include "VuoCommandRemoveProtocolPort.hh"
#include "VuoCommandRemove.hh"
#include "VuoCommandReorderPublishedPorts.hh"
#include "VuoCommandResizeComment.hh"
#include "VuoCommandConnect.hh"
#include "VuoCommandSetCableHidden.hh"
#include "VuoCommandSetCommentText.hh"
#include "VuoCommandSetPortConstant.hh"
#include "VuoCommandSetPublishedPortDetails.hh"
#include "VuoCommandSetItemTint.hh"
#include "VuoCommandSetNodeTitle.hh"
#include "VuoCommandSetPublishedPortName.hh"
#include "VuoCommandSetTriggerThrottling.hh"
#include "VuoCommandChangeNode.hh"
#include "VuoCommandPublishPort.hh"
#include "VuoCommandUnpublishPort.hh"
#include "VuoCommandReplaceNode.hh"
#include "VuoCommandSetMetadata.hh"
#include "VuoCommentEditor.hh"
#include "VuoCompilerGraphvizParser.hh"
#include "VuoCompositionMetadataPanel.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorComposition.hh"
#include "VuoConsole.hh"
#include "VuoErrorDialog.hh"
#include "VuoGlPool.h"
#include "VuoMetadataEditor.hh"
#include "VuoModuleManager.hh"
#include "VuoRendererComment.hh"
#include "VuoRendererCommon.hh"
#include "VuoRendererFonts.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoSearchBox.hh"
#include "VuoSubcompositionSaveAsDialog.hh"
#include "VuoEditor.hh"
#include "VuoEditorCocoa.hh"
#include "VuoEditorWindowToolbar.hh"
#include "VuoExampleMenu.hh"
#include "VuoInfoDialog.hh"
#include "VuoInputEditorManager.hh"
#include "VuoNodeClass.hh"
#include "VuoProtocol.hh"
#include "VuoPublishedPortSidebar.hh"
#include "VuoRecentFileMenu.hh"
#include "VuoStringUtilities.hh"
#include "VuoSubcompositionMessageRouter.hh"
#include "VuoCodeWindow.hh"
#include "VuoShaderFile.hh"
#include "VuoTitleEditor.hh"
#include "VuoInputEditorSession.hh"

#include "type.h"

#ifdef __APPLE__
#include <objc/objc-runtime.h>
#undef check  // Prevent macro defined in AssertMacros.h from overriding VuoCompilerComposition::check
#endif

#include <fstream>
#include <sstream>
#include <sys/stat.h>


const qreal VuoEditorWindow::viewportStepRate = 1;
const qreal VuoEditorWindow::viewportStepRateMultiplier = 5;
const qreal VuoEditorWindow::zoomRate = 1.2;
const qreal VuoEditorWindow::pastedComponentOffset = 20;
const qreal VuoEditorWindow::compositionMargin = 20;
QString VuoEditorWindow::untitledComposition;
VuoEditorWindow *VuoEditorWindow::mostRecentActiveEditorWindow = NULL;

/**
 * Creates a new window for editing a Vuo Composition. If @a compositionAsString is not empty, that composition is loaded into the window.
 *
 * The title of the composition is based on @a documentIdentifier,
 * which can be either the path of the composition or something like "Untitled Composition".
 *
 * The compiler is configured with @a compositionPath,
 * which can be the full path of the composition, the path of the directory containing the composition, or empty if no path is known yet.
 */
VuoEditorWindow::VuoEditorWindow(QString documentIdentifier, QString compositionPath,
								 const string &compositionAsString,
								 VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode,
								 VuoNodeLibrary::nodeLibraryState nodeLibraryState,
								 VuoNodeLibrary *floater,
								 VuoProtocol *activeProtocol,
								 string nodeClassToHighlight) :
	ui(new Ui::VuoEditorWindow)
{
	doneInitializing = false;

	// Initialize the compiler.
	this->compiler = new VuoCompiler(compositionPath.toStdString());
	this->composition = nullptr;  // Initialized later in this constructor.

	this->compositionUpgradedSinceLastSave = false;
	this->protocolComplianceReevaluationPending = false;
	this->ignoreItemMoveSignals = false;
	this->closing = false;
	this->containedPrepopulatedContent = (!compositionAsString.empty() || activeProtocol);
	this->publishedPortNearCursorPreviously = false;
	this->zoomOutToFitOnNextShowEvent = nodeClassToHighlight.empty();
	this->includeInRecentFileMenu = true;

	// Initialize the module manager — before the compiler loads any composition-local modules,
	// so the module manager will catch any issues with them.
	VuoModuleManager *moduleManager = new VuoModuleManager(compiler);

	ui->setupUi(this);

	// Set keyboard shortcuts.
	// "On Mac OS X, references to "Ctrl", Qt::CTRL, Qt::Control and Qt::ControlModifier correspond
	// to the Command keys on the Macintosh keyboard, and references to "Meta", Qt::META, Qt::Meta
	// and Qt::MetaModifier correspond to the Control keys. Developers on Mac OS X can use the same
	// shortcut descriptions across all platforms, and their applications will automatically work as
	// expected on Mac OS X." -- http://qt-project.org/doc/qt-4.8/qkeysequence.html
	ui->newComposition->setShortcut(QKeySequence("Ctrl+N"));
	ui->openComposition->setShortcut(QKeySequence("Ctrl+O"));
	ui->saveComposition->setShortcut(QKeySequence("Ctrl+S"));
	ui->saveCompositionAs->setShortcut(QKeySequence("Ctrl+Shift+S"));
	ui->closeComposition->setShortcut(QKeySequence("Ctrl+W"));
	ui->selectAll->setShortcut(QKeySequence("Ctrl+A"));
	ui->selectNone->setShortcut(QKeySequence("Ctrl+Shift+A"));
	ui->cutCompositionComponents->setShortcut(QKeySequence("Ctrl+X"));
	ui->copyCompositionComponents->setShortcut(QKeySequence("Ctrl+C"));
	ui->duplicateCompositionComponents->setShortcut(QKeySequence("Ctrl+D"));
	ui->paste->setShortcut(QKeySequence("Ctrl+V"));
	ui->deleteCompositionComponents->setShortcut(QKeySequence("Backspace"));
	ui->zoomIn->setShortcut(QKeySequence("Ctrl+="));
	ui->zoomOut->setShortcut(QKeySequence("Ctrl+-"));
	ui->zoom11->setShortcut(QKeySequence("Ctrl+0"));
	isZoomedToFit = false;
	addAction(ui->stopComposition);
	ui->showNodeLibrary->setShortcut(QKeySequence("Ctrl+Return"));

	// Trigger app-wide menu with our local window menu.
	connect(ui->newComposition,  &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::newComposition);
	connect(ui->openComposition, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::openFile);

	// "About" menu item.
	// On macOS, this menu item will automatically be moved from the "File" menu to the Application menu by way of `QAction::AboutRole`.
	QAction *aboutAction = new QAction(nullptr);
	aboutAction->setText(tr("About Vuo…"));
	connect(aboutAction, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::about);
	aboutAction->setMenuRole(QAction::AboutRole);
	ui->menuFile->addAction(aboutAction);

	// Connect the "Quit" menu item action to our customized quit method.
	// On macOS, this menu item will automatically be moved from the "File" menu to the Application menu by way of `QAction::QuitRole`.
	quitAction = new QAction(nullptr);
	quitAction->setText(tr("&Quit"));
	quitAction->setShortcut(QKeySequence("Ctrl+Q"));
	connect(quitAction, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::quitCleanly);
	quitAction->setMenuRole(QAction::QuitRole);
	ui->menuFile->addAction(quitAction);

	VuoEditor *editor = (VuoEditor *)qApp;

	ui->menuView->addSeparator();

#if VUO_PRO
	// "Dark Interface" menu item
	ui->menuView->addAction(editor->darkInterfaceAction);

	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoEditorWindow::updateColor);
	updateColor(editor->isInterfaceDark());
#endif

	// "Grid" menu
	QMenu *menuGrid = new QMenu(ui->menuBar);
	menuGrid->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuGrid->setTitle(tr("&Grid"));
	menuGrid->addAction(editor->snapToGridAction);

	menuGrid->addSeparator();

	menuGrid->addAction(editor->showGridLinesAction);
	menuGrid->addAction(editor->showGridPointsAction);
	connect(editor, &VuoEditor::showGridToggled, this, &VuoEditorWindow::updateGrid);

	ui->menuView->addMenu(menuGrid);

	// "Canvas Transparency" menu
	QMenu *menuCanvasTransparency = new QMenu(ui->menuBar);
	menuCanvasTransparency->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	menuCanvasTransparency->setTitle(tr("&Canvas Transparency"));
	((VuoEditor *)qApp)->populateCanvasTransparencyMenu(menuCanvasTransparency);
	connect(editor, &VuoEditor::canvasOpacityChanged, this, &VuoEditorWindow::updateCanvasOpacity);

	ui->menuView->addMenu(menuCanvasTransparency);

	// "New Composition from Template" menu
	menuNewCompositionWithTemplate = new QMenu(tr("New Composition from Template"));
	menuNewCompositionWithTemplate->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	((VuoEditor *)qApp)->populateNewCompositionWithTemplateMenu(menuNewCompositionWithTemplate);

	// "New Shader" menu
	QMenu *menuNewShader = new QMenu(tr("New Shader"));
	menuNewShader->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	((VuoEditor *)qApp)->populateNewShaderMenu(menuNewShader);

	// Insert the "New Composition from Template" and "New Shader" menus immediately after the "New Composition" menu item.
	for (int menuFileIndex = 0; menuFileIndex < ui->menuFile->actions().count(); ++menuFileIndex)
	{
		if (ui->menuFile->actions().at(menuFileIndex) == ui->newComposition)
		{
			ui->menuFile->insertMenu(ui->menuFile->actions().at(menuFileIndex+1), menuNewCompositionWithTemplate);
			ui->menuFile->insertMenu(ui->menuFile->actions().at(menuFileIndex+2), menuNewShader);
			break;
		}
	}

	// "Open Example" menu
	menuOpenExample = new VuoExampleMenu(ui->menuFile, compiler);
	connect(menuOpenExample, &VuoExampleMenu::exampleSelected, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

	menuOpenRecent = new VuoRecentFileMenu();
	connect(menuOpenRecent, &VuoRecentFileMenu::recentFileSelected, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

	// Insert the "Open Recent" and "Open Example" menus immediately after the "Open..." menu item.
	for (int menuFileIndex = 0; menuFileIndex < ui->menuFile->actions().count(); ++menuFileIndex)
		if (ui->menuFile->actions().at(menuFileIndex) == ui->openComposition)
		{
			ui->menuFile->insertMenu(ui->menuFile->actions().at(menuFileIndex+1), menuOpenRecent);
			ui->menuFile->insertMenu(ui->menuFile->actions().at(menuFileIndex+2), menuOpenExample);
		}

	// Ensure that the keyboard shortcuts for the "Open Recent" and "Open Example" submenus work
	// even when no windows are open.
	VuoEditorCocoa_detectSubmenuKeyEvents();

	// "Protocols" menu
	menuProtocols = new QMenu(tr("Protocols"));
	menuProtocols->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
	populateProtocolsMenu(menuProtocols);

	// Insert the "Protocols" menu immediately before the "Composition Information" menu item.
	ui->menuEdit->insertMenu(ui->compositionInformation, menuProtocols);

	// Populate the "Help" menu.
	((VuoEditor *)qApp)->populateHelpMenu(ui->menuHelp);

	// Prevent "Help > Search" from triggering lookup of all customized example composition titles at once.
	connect(ui->menuHelp, &QMenu::aboutToShow, menuOpenExample, &VuoExampleMenu::disableExampleTitleLookup);
	connect(ui->menuHelp, &QMenu::aboutToHide, menuOpenExample, &VuoExampleMenu::enableExampleTitleLookup);

	connect(ui->menuHelp, &QMenu::aboutToShow, [editor, this] { editor->populateHelpMenu(ui->menuHelp); });

	// Initialize the composition.
	VuoCompilerComposition *compilerComposition = (compositionAsString.empty() ?
													   new VuoCompilerComposition(new VuoComposition(), NULL) :
													   VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionAsString, compiler));

	compilerComposition->getBase()->setDirectory(VuoEditor::getDefaultCompositionStorageDirectory().toUtf8().constData());

	composition = new VuoEditorComposition(this, compilerComposition->getBase());
	composition->setCompiler(this->compiler);
	composition->setModuleManager(moduleManager);

	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->addComposition(this);

	if (!compositionAsString.empty())
	{
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath.toStdString(), dir, file, extension);

		if (!dir.empty())
			compilerComposition->getBase()->setDirectory(dir);

		string defaultName = VuoEditorComposition::getDefaultNameForPath(documentIdentifier.toUtf8().data());
		compilerComposition->getBase()->getMetadata()->setDefaultName(defaultName);
	}

	ui->graphicsView->setScene(composition);
	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	ui->graphicsView->setMouseTracking(true);
	composition->installEventFilter(this);
	ui->graphicsView->viewport()->installEventFilter(this);

	connect(ui->cutCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::cutSelectedCompositionComponents);
	connect(ui->copyCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::copySelectedCompositionComponents);
	connect(ui->duplicateCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::duplicateSelectedCompositionComponentsByMenuItem);
	connect(ui->paste, &QAction::triggered, this, &VuoEditorWindow::disambiguatePasteRequest);
	connect(ui->deleteCompositionComponents, &QAction::triggered, composition, static_cast<void (VuoEditorComposition::*)()>(&VuoEditorComposition::deleteSelectedCompositionComponents));
	connect(ui->renameNodes, &QAction::triggered, composition, &VuoEditorComposition::renameSelectedNodes);
	connect(ui->refactor, &QAction::triggered, this, &VuoEditorWindow::refactorSelectedItems);
	connect(ui->selectAll, &QAction::triggered, composition, &VuoEditorComposition::selectAllCompositionComponents);
	connect(ui->selectAllComments, &QAction::triggered, composition, &VuoEditorComposition::selectAllComments);

	// Initialize the action associated with raising this window to the application foreground.
	raiseDocumentAction = new QAction(this);
	raiseDocumentAction->setCheckable(true);
	connect(raiseDocumentAction, &QAction::triggered, this, &VuoEditorWindow::setAsActiveWindow);
	connect(ui->menuWindow, &QMenu::aboutToShow, this, &VuoEditorWindow::updateUI);

	// Prepare the input editors.
	inputEditorManager = new VuoInputEditorManager();
	composition->setInputEditorManager(inputEditorManager);
	inputEditorSession = nullptr;
	connect(composition, &VuoEditorComposition::portConstantChangeRequested, this, &VuoEditorWindow::setPortConstant);
	connect(composition, &VuoEditorComposition::inputEditorRequested, this, static_cast<void (VuoEditorWindow::*)(VuoRendererPort *)>(&VuoEditorWindow::showInputEditor));
	connect(composition, &VuoEditorComposition::nodeTitleEditorRequested, this, &VuoEditorWindow::showNodeTitleEditor);
	connect(composition, &VuoEditorComposition::commentEditorRequested, this, &VuoEditorWindow::showCommentEditor);
	connect(composition, &VuoEditorComposition::commentZoomRequested, this, &VuoEditorWindow::zoomToFitComment);

	connect(composition, &VuoEditorComposition::commentInsertionRequested, this, &VuoEditorWindow::insertCommentAtPos);
	connect(composition, &VuoEditorComposition::subcompositionInsertionRequested, this, &VuoEditorWindow::insertSubcompositionAtPos);
	connect(composition, &VuoEditorComposition::nodeSourceEditorRequested, this, &VuoEditorWindow::openEditableSourceForNode);
	connect(composition, &VuoEditorComposition::emptyCanvasLocationLeftClicked, this, &VuoEditorWindow::displayAppropriateDocumentation);

	connect(composition, &VuoEditorComposition::refactorRequested, this, &VuoEditorWindow::refactorSelectedItems);

	// Initialize the 'undo' stack.
	undoStack = new QUndoStack(this);
	connect(undoStack, &QUndoStack::cleanChanged, this, &VuoEditorWindow::undoStackCleanStateChanged);

	undoAction = undoStack->createUndoAction(this);
	undoAction->setText(tr("Undo"));
	undoAction->setShortcut(QKeySequence::Undo);

	redoAction = undoStack->createRedoAction(this);
	redoAction->setText(tr("Redo"));
	redoAction->setShortcut(QKeySequence::Redo);

	metadataEditor = new VuoMetadataEditor(composition, this, Qt::Sheet);
	metadataEditor->setWindowModality(Qt::WindowModal);
	connect(metadataEditor, &VuoMetadataEditor::finished, this, &VuoEditorWindow::editMetadata);

	searchBox = new VuoSearchBox(composition, this, Qt::Widget);
	searchBox->setVisible(false);
	connect(searchBox, &VuoSearchBox::searchPerformed, this, &VuoEditorWindow::updateUI);

	canvasDragEnabled = false;
	canvasDragInProgress = false;
	scrollInProgress = false;
	timeOfLastScroll = 0;
	consumeNextMouseReleaseToCanvas = false;
	lastLeftMousePressHadOptionModifier = false;
	rubberBandSelectionInProgress = false;
	previousDragMoveWasOverSidebar = false;
	duplicationMacroInProgress = false;
	itemDragMacroInProgress = false;
	itemDragDx = 0;
	itemDragDy = 0;
	latestDragTime = 0;
	commentResizeMacroInProgress = false;
	commentBeingResized = NULL;
	commentResizeDx = 0;
	commentResizeDy = 0;
	forwardingEventsToCanvas = false;

	ui->menuEdit->insertAction(ui->menuEdit->actions()[0], redoAction);
	ui->menuEdit->insertAction(redoAction, undoAction);

	contextMenuTints = composition->getContextMenuTints();
	ui->menuEdit->insertMenu(ui->changeNodePlaceholder, contextMenuTints);

	menuChangeNode = new QMenu(ui->menuEdit);
	menuChangeNode->setTitle(tr("Change To"));

	connect(ui->menuEdit, &QMenu::aboutToShow, this, &VuoEditorWindow::updateUI);
	connect(ui->menuEdit, &QMenu::aboutToShow, this, &VuoEditorWindow::updateChangeNodeMenu);

	foreach (QMenu *menu, ui->menuBar->findChildren<QMenu*>())
		menu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

	connect(composition, &VuoEditorComposition::itemsMoved, this, &VuoEditorWindow::itemsMoved);
	connect(composition, &VuoEditorComposition::commentResized, this, &VuoEditorWindow::commentResized);
	connect(composition, &VuoEditorComposition::leftMouseButtonReleased, this, &VuoEditorWindow::resetUndoStackMacros);
	connect(composition, &VuoEditorComposition::connectionCompletedByDragging, this, &VuoEditorWindow::connectionCompleted);
	connect(composition, &VuoEditorComposition::componentsRemoved, this, &VuoEditorWindow::componentsRemoved);
	connect(composition, &VuoEditorComposition::componentsAdded, this, &VuoEditorWindow::componentsAdded);
	connect(composition, &VuoEditorComposition::selectedComponentsDuplicated, this, &VuoEditorWindow::duplicateSelectedCompositionComponentsByDrag);
	connect(composition, &VuoEditorComposition::duplicationOperationCancelled, this, &VuoEditorWindow::cleanUpCancelledDuplication);
	connect(composition, SIGNAL(portPublicationRequested(VuoPort *, bool)), this, SLOT(internalPortPublished(VuoPort *, bool)));
	connect(composition, &VuoEditorComposition::publishedPortNameEditorRequested, this, &VuoEditorWindow::showPublishedPortNameEditor);
	connect(composition, &VuoEditorComposition::portUnpublicationRequested, this, &VuoEditorWindow::internalPortUnpublished);
	connect(composition, &VuoEditorComposition::protocolPortChangesRequested, this, &VuoEditorWindow::makeProtocolPortChanges);
	connect(composition, &VuoEditorComposition::activeProtocolChanged, this, &VuoEditorWindow::updateActiveProtocolDisplay);
	connect(composition, &VuoEditorComposition::publishedPortModified, this, &VuoEditorWindow::registerProtocolComplianceEvaluationRequest);
	connect(composition, &VuoEditorComposition::triggerThrottlingUpdated, this, &VuoEditorWindow::setTriggerThrottling);
	connect(composition, &VuoEditorComposition::inputPortCountAdjustmentRequested, this, &VuoEditorWindow::adjustInputPortCountForNode);
	connect(composition, &VuoEditorComposition::nodeSwapRequested, this, &VuoEditorWindow::swapNodes);
	connect(composition, &VuoEditorComposition::specializePort, this, static_cast<VuoRendererNode *(VuoEditorWindow::*)(VuoRendererPort *port, string specializedTypeName)>(&VuoEditorWindow::specializePortNetwork));
	connect(composition, &VuoEditorComposition::unspecializePort, this, static_cast<VuoRendererNode *(VuoEditorWindow::*)(VuoRendererPort *port)>(&VuoEditorWindow::unspecializePortNetwork));
	connect(composition, &VuoEditorComposition::respecializePort, this, static_cast<VuoRendererNode *(VuoEditorWindow::*)(VuoRendererPort *port, string specializedTypeName)>(&VuoEditorWindow::respecializePortNetwork));
	connect(composition, &VuoEditorComposition::tintSelectedItemsRequested, this, &VuoEditorWindow::tintSelectedItems);
	connect(composition, &VuoEditorComposition::selectedInternalCablesHidden, this, &VuoEditorWindow::hideSelectedInternalCables);
	connect(composition, &VuoEditorComposition::cablesHidden, this, &VuoEditorWindow::hideCables);
	connect(composition, &VuoEditorComposition::cablesUnhidden, this, &VuoEditorWindow::unhideCables);
	connect(composition, &VuoEditorComposition::changeInHiddenCables, this, &VuoEditorWindow::updateUI);
	connect(composition, &VuoEditorComposition::cableDragInitiated, this, &VuoEditorWindow::updateWindowForNewCableDrag);
	connect(composition, &VuoEditorComposition::cableDragEnded, this, &VuoEditorWindow::updateUI);
	connect(composition, &VuoEditorComposition::cableDragEnded, this, &VuoEditorWindow::updateLatestDragTime);
	connect(composition, &VuoEditorComposition::compositionStoppedItself, this, &VuoEditorWindow::on_stopComposition_triggered);
	connect(composition, &VuoEditorComposition::buildStarted, this, &VuoEditorWindow::showBuildActivityIndicator);
	connect(composition, &VuoEditorComposition::buildFinished, this, &VuoEditorWindow::hideBuildActivityIndicator);
	connect(composition, &VuoEditorComposition::stopFinished, this, &VuoEditorWindow::hideStopActivityIndicator);

	// Avoid bug where the first time the application is de-activated after a popover is detached,
	// the application and its widgets (including the popover) fail to receive any notification of
	// the de-activation unless this window has first been re-activated in place of the popover.
	// See https://b33p.net/kosada/node/6281 .
	connect(composition, &VuoEditorComposition::popoverDetached, this, &VuoEditorWindow::setAsActiveWindow);

	// Uncomment to display undo stack for debugging purposes:
	//undoView = new QUndoView(undoStack);
	//undoView->show();

	// Show the VuoSearchBox over just the composition area (not the node library or published port sidebar areas).
	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);

	connect(ui->toggleNodeLibraryDocking, &QAction::triggered, this, &VuoEditorWindow::toggleNodeLibraryDockedState);

	QActionGroup* toggleDisplay= new QActionGroup(this);
	toggleDisplay->addAction(ui->actionShowNodeNames);
	toggleDisplay->addAction(ui->actionShowNodeClassNames);

	// Initialize published port sidebars.
	inputPortSidebar = new VuoPublishedPortSidebar(this, composition, true);
	outputPortSidebar = new VuoPublishedPortSidebar(this, composition, false);
	inputPortSidebar->setVisible(false);
	outputPortSidebar->setVisible(false);
	populateProtocolsMenu(inputPortSidebar->getProtocolsContextMenu());
	populateProtocolsMenu(outputPortSidebar->getProtocolsContextMenu());

	connect(inputPortSidebar->getRemoveProtocolAction(), &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);
	connect(outputPortSidebar->getRemoveProtocolAction(), &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);

	connect(inputPortSidebar,  &VuoPublishedPortSidebar::closed, this, &VuoEditorWindow::closePublishedPortSidebars);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::closed, this, &VuoEditorWindow::closePublishedPortSidebars);

	connect(inputPortSidebar,  &VuoPublishedPortSidebar::visibilityChanged, this, &VuoEditorWindow::conditionallyShowPublishedPortSidebars);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::visibilityChanged, this, &VuoEditorWindow::conditionallyShowPublishedPortSidebars);

	connect(inputPortSidebar,  &VuoPublishedPortSidebar::publishedPortPositionsUpdated, this, &VuoEditorWindow::updatePublishedCableGeometry);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::publishedPortPositionsUpdated, this, &VuoEditorWindow::updatePublishedCableGeometry);
	connect(inputPortSidebar,  &VuoPublishedPortSidebar::publishedPortsReordered, this, &VuoEditorWindow::updatePublishedPortOrder);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::publishedPortsReordered, this, &VuoEditorWindow::updatePublishedPortOrder);

	connect(ui->graphicsView, &VuoEditorGraphicsView::viewResized, inputPortSidebar, &VuoPublishedPortSidebar::externalMoveEvent);
	connect(ui->graphicsView, &VuoEditorGraphicsView::viewResized, outputPortSidebar, &VuoPublishedPortSidebar::externalMoveEvent);
	connect(ui->graphicsView, &VuoEditorGraphicsView::viewResized, this, &VuoEditorWindow::viewportFitReset);
	connect(ui->graphicsView->horizontalScrollBar(), &QScrollBar::valueChanged, this, &VuoEditorWindow::viewportFitReset);
	connect(ui->graphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), outputPortSidebar, SLOT(externalMoveEvent()));
	connect(ui->graphicsView->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), outputPortSidebar, SLOT(externalMoveEvent()));
	connect(ui->graphicsView->verticalScrollBar(), &QScrollBar::valueChanged, this, &VuoEditorWindow::viewportFitReset);
	connect(ui->graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), outputPortSidebar, SLOT(externalMoveEvent()));
	connect(ui->graphicsView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), outputPortSidebar, SLOT(externalMoveEvent()));
	connect(ui->graphicsView, &VuoEditorGraphicsView::rubberBandChanged, this, &VuoEditorWindow::updateRubberBandSelectionMode);

	connect(composition, &VuoEditorComposition::sceneRectChanged, this, &VuoEditorWindow::viewportFitReset);
	connect(composition, &VuoEditorComposition::selectionChanged, this, &VuoEditorWindow::viewportFitReset);
	connect(composition, &VuoEditorComposition::highlightPublishedSidebarDropLocationsRequested, inputPortSidebar, &VuoPublishedPortSidebar::highlightEligibleDropLocations);
	connect(composition, &VuoEditorComposition::highlightPublishedSidebarDropLocationsRequested, outputPortSidebar, &VuoPublishedPortSidebar::highlightEligibleDropLocations);
	connect(composition, &VuoEditorComposition::clearPublishedSidebarDropLocationHighlightingRequested, inputPortSidebar, &VuoPublishedPortSidebar::clearEligibleDropLocationHighlighting);
	connect(composition, &VuoEditorComposition::clearPublishedSidebarDropLocationHighlightingRequested, outputPortSidebar, &VuoPublishedPortSidebar::clearEligibleDropLocationHighlighting);
	connect(composition, SIGNAL(portPublicationRequested(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)), this, SLOT(internalExternalPortPairPublished(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)));

	connect(composition, &VuoEditorComposition::undoStackMacroBeginRequested, this, &VuoEditorWindow::beginUndoStackMacro);
	connect(composition, &VuoEditorComposition::undoStackMacroEndRequested, this, &VuoEditorWindow::endUndoStackMacro);

	connect(composition, SIGNAL(publishedPortModified()), inputPortSidebar, SLOT(updatePortList()), Qt::QueuedConnection);
	connect(composition, SIGNAL(publishedPortModified()), outputPortSidebar, SLOT(updatePortList()), Qt::QueuedConnection);

	connect(inputPortSidebar, &VuoPublishedPortSidebar::newPublishedPortRequested, this, &VuoEditorWindow::createIsolatedExternalPublishedPort);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::newPublishedPortRequested, this, &VuoEditorWindow::createIsolatedExternalPublishedPort);

	connect(inputPortSidebar,  &VuoPublishedPortSidebar::portPublicationRequestedViaDropBox, this, &VuoEditorWindow::internalPortPublishedViaDropBox);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::portPublicationRequestedViaDropBox, this, &VuoEditorWindow::internalPortPublishedViaDropBox);
	connect(inputPortSidebar, SIGNAL(portPublicationRequestedViaSidebarPort(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)), this, SLOT(internalExternalPortPairPublished(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)));
	connect(outputPortSidebar, SIGNAL(portPublicationRequestedViaSidebarPort(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)), this, SLOT(internalExternalPortPairPublished(VuoPort *, VuoPublishedPort *, bool, VuoPort *, string, string, bool)));
	connect(inputPortSidebar,  &VuoPublishedPortSidebar::componentsRemoved, this, &VuoEditorWindow::componentsRemoved);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::componentsRemoved, this, &VuoEditorWindow::componentsRemoved);

	connect(inputPortSidebar,  &VuoPublishedPortSidebar::externalPortUnpublicationRequested, this, &VuoEditorWindow::externalPortUnpublished);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::externalPortUnpublicationRequested, this, &VuoEditorWindow::externalPortUnpublished);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::publishedPortNameEditorRequested, this, &VuoEditorWindow::showPublishedPortNameEditor);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::publishedPortNameEditorRequested, this, &VuoEditorWindow::showPublishedPortNameEditor);
	connect(inputPortSidebar,  SIGNAL(publishedPortDetailsChangeRequested(VuoRendererPublishedPort *, json_object *)), this, SLOT(changePublishedPortDetails(VuoRendererPublishedPort *, json_object *)));
	connect(outputPortSidebar,  SIGNAL(publishedPortDetailsChangeRequested(VuoRendererPublishedPort *, json_object *)), this, SLOT(changePublishedPortDetails(VuoRendererPublishedPort *, json_object *)));

	connect(inputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroBeginRequested, this, &VuoEditorWindow::beginUndoStackMacro);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroBeginRequested, this, &VuoEditorWindow::beginUndoStackMacro);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroEndRequested, this, &VuoEditorWindow::endUndoStackMacro);
	connect(outputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroEndRequested, this, &VuoEditorWindow::endUndoStackMacro);

	connect(inputPortSidebar, &VuoPublishedPortSidebar::inputEditorRequested, this, static_cast<void (VuoEditorWindow::*)(VuoRendererPort *)>(&VuoEditorWindow::showInputEditor));

	connect(undoStack, &QUndoStack::indexChanged, searchBox, &VuoSearchBox::refreshResults);
	connect(undoStack, &QUndoStack::indexChanged, this, &VuoEditorWindow::coalescedUpdateRunningComposition);
	connect(undoStack, &QUndoStack::indexChanged, this, &VuoEditorWindow::handlePendingProtocolComplianceReevaluationRequests);
	connect(undoStack, SIGNAL(indexChanged(int)), inputPortSidebar, SLOT(updatePortList()));
	connect(undoStack, SIGNAL(indexChanged(int)), outputPortSidebar, SLOT(updatePortList()));
	// Use a queued connection to avoid mutual recursion between QUndoStack::indexChanged() and VuoEditorComposition::updateFeedbackErrors().
	connect(undoStack, SIGNAL(indexChanged(int)), composition, SLOT(updateFeedbackErrors()), Qt::QueuedConnection);

#ifdef VUO_PRO
	VuoEditorWindow_Pro();
#endif

	coalescedOldCompositionSnapshot = "";
	coalescedNewCompositionSnapshot = "";
	coalescedDiffInfo = nullptr;

	toolbar = NULL;

	metadataPanel = new VuoCompositionMetadataPanel(composition->getBase());
	connect(metadataPanel, &VuoCompositionMetadataPanel::textSelectionChanged, this, &VuoEditorWindow::updateUI);
	connect(metadataPanel, &VuoCompositionMetadataPanel::metadataEditRequested, this, &VuoEditorWindow::on_compositionInformation_triggered);

	initializeNodeLibrary(this->compiler, nodeLibraryDisplayMode, nodeLibraryState, floater);
	moduleManager->setNodeLibrary(ownedNodeLibrary);
	moduleManager->updateWithAlreadyLoadedModules();
	connect(editor, &VuoEditor::globalNodeLibraryStateChanged, this, &VuoEditorWindow::conformToGlobalNodeLibraryVisibility);

	// Dynamically resize the sceneRect to accommodate current canvas items.
	connect(composition, &VuoEditorComposition::changed, this, &VuoEditorWindow::ensureSceneRectContainsRegion);

	// Update relevant menu items when there is a change to the set of currently selected composition components.
	connect(composition, &VuoEditorComposition::selectionChanged, this, &VuoEditorWindow::updateSelectedComponentMenuItems);

	// Update relevant menu items when the trigger port to manually re-fire changes.
	connect(composition, &VuoEditorComposition::refirePortChanged, this, &VuoEditorWindow::updateRefireAction);

	// Update relevant menu items when clipboard data is modified.
	connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &VuoEditorWindow::updateUI);

	// Performance optimizations
	ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);

	ui->graphicsView->setAttribute(Qt::WA_NoBackground, true);
	ui->graphicsView->setAttribute(Qt::WA_OpaquePaintEvent);
	ui->graphicsView->setAttribute(Qt::WA_NoSystemBackground);

	if (nl)
	{
		nl->setAttribute(Qt::WA_NoBackground, true);
		nl->setAttribute(Qt::WA_OpaquePaintEvent);
		nl->setAttribute(Qt::WA_NoSystemBackground);
	}

	if (activeProtocol)
	{
		composition->addActiveProtocol(activeProtocol, false);

		if (compositionAsString.empty())
		{
			// Automatically add an "Allow First Event" node to new protocol compositions
			// and connect it to the published "time" input port.
			VuoRendererNode *allowFirstEventNode = composition->createNode("vuo.event.allowFirst", "", 30, 3*VuoRendererComposition::majorGridLineSpacing);
			composition->addNode(allowFirstEventNode->getBase());

			if (composition->getBase()->getPublishedInputPortWithName("time") &&
					allowFirstEventNode->getBase()->getInputPortWithName("event"))
			{
				composition->publishInternalPort(allowFirstEventNode->getBase()->getInputPortWithName("event"), false, "time", NULL, true);
			}
		}
	}
	else
	{
		if (compositionAsString.empty())
		{
			// Automatically add a "Fire on Start" node to new compositions,
			// except those initialized under an active protocol.
			// Select x- and y- coordinates that don't cause the resulting sceneRect update
			// to visibly jiggle the node in either node-library-floating or -docked mode.
			VuoRendererNode *fireOnStartNode = composition->createNode("vuo.event.fireOnStart", "", 30, 50);
			composition->addNode(fireOnStartNode->getBase());
			composition->setTriggerPortToRefire(fireOnStartNode->getBase()->getOutputPortWithName("started"));
		}
		else
			evaluateCompositionForProtocolPromotion();
	}

	inputPortSidebar->updateActiveProtocol();
	outputPortSidebar->updateActiveProtocol();

	// By default, display the published port sidebars if and only if the composition has any published ports.
	bool displayPublishedPorts = ((! composition->getBase()->getPublishedInputPorts().empty()) ||
								  (! composition->getBase()->getPublishedOutputPorts().empty()));
	setPublishedPortSidebarVisibility(displayPublishedPorts);

	// The toolbar must be initialized after the composition, since it triggers moveEvent(), which assumes the composition exists.
	toolbar = VuoEditorWindowToolbar::create(this);

	doneInitializing = true;
	updateColor(editor->isInterfaceDark());
	updateCanvasOpacity();

	string dir, file, ext;
	VuoFileUtilities::splitPath(documentIdentifier.toUtf8().constData(), dir, file, ext);

	// Case: Creating a new composition (which may or may not have pre-populated content)
	if (dir.empty())
	{
		setWindowTitle(documentIdentifier + "[*]");
#if VUO_PRO
		toolbar->updateTitle();
#endif

		// Generate default description.
		composition->getBase()->getMetadata()->setDescription("");

		// Generate version of Vuo used to create composition.
		composition->getBase()->getMetadata()->setCreatedInVuoVersion(VUO_VERSION_STRING);

		// Generate author link and default copyright.
		const string user = static_cast<VuoEditor *>(qApp)->getUserName();
		const string userProfileURL = static_cast<VuoEditor *>(qApp)->getStoredUserProfileURL();
		const string userProfileLink = (userProfileURL.empty()? user : "[" + user + "](" + userProfileURL + ")");
		composition->getBase()->getMetadata()->setAuthor(userProfileLink);
		composition->getBase()->getMetadata()->setCopyright(generateCurrentDefaultCopyright());
	}

	// Case: Opening a pre-existing composition from the filesystem
	else
	{
		setWindowFilePath(documentIdentifier);
		setFocus();
	}

	// Don't display the "Edit..." link in the metadata panel for example compositions.
	QDir compositionDir(QDir(composition->getBase()->getDirectory().c_str()).canonicalPath());
	bool tmpFile = (compositionDir.canonicalPath() == QDir(VuoFileUtilities::getTmpDir().c_str()).canonicalPath());
	metadataPanel->setIsUserComposition(!tmpFile);

	displayAppropriateDocumentation();
	updateSceneRect();

	if (!nodeClassToHighlight.empty())
		highlightNodeClass(nodeClassToHighlight);

	updateUI();
	updateRefireAction();

	// Schedule this after the constructor returns so there's not a brief glitchy grayness when showing the window.
	QMetaObject::invokeMethod(this, "showUpdateHelpDialog", Qt::QueuedConnection);
}

VuoEditorWindow::~VuoEditorWindow()
{
	VUserLog("%s:      Close", getWindowTitleWithoutPlaceholder().toUtf8().data());

	disconnect(inputPortSidebar->getRemoveProtocolAction(), &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);
	disconnect(outputPortSidebar->getRemoveProtocolAction(), &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);
	disconnect(inputPortSidebar,  &VuoPublishedPortSidebar::publishedPortPositionsUpdated, this, &VuoEditorWindow::updatePublishedCableGeometry);
	disconnect(outputPortSidebar, &VuoPublishedPortSidebar::publishedPortPositionsUpdated, this, &VuoEditorWindow::updatePublishedCableGeometry);
	disconnect(inputPortSidebar,  &VuoPublishedPortSidebar::publishedPortsReordered, this, &VuoEditorWindow::updatePublishedPortOrder);
	disconnect(outputPortSidebar, &VuoPublishedPortSidebar::publishedPortsReordered, this, &VuoEditorWindow::updatePublishedPortOrder);

	disconnect(inputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroBeginRequested, this, &VuoEditorWindow::beginUndoStackMacro);
	disconnect(outputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroBeginRequested, this, &VuoEditorWindow::beginUndoStackMacro);
	disconnect(inputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroEndRequested, this, &VuoEditorWindow::endUndoStackMacro);
	disconnect(outputPortSidebar, &VuoPublishedPortSidebar::undoStackMacroEndRequested, this, &VuoEditorWindow::endUndoStackMacro);

	disconnect(inputPortSidebar, &VuoPublishedPortSidebar::inputEditorRequested, this, static_cast<void (VuoEditorWindow::*)(VuoRendererPort *)>(&VuoEditorWindow::showInputEditor));

	disconnect(ui->graphicsView, SIGNAL(viewResized()), outputPortSidebar, SLOT(externalMoveEvent()));
	disconnect(ui->graphicsView, &VuoEditorGraphicsView::viewResized, this, &VuoEditorWindow::viewportFitReset);
	disconnect(ui->graphicsView->horizontalScrollBar(), &QScrollBar::valueChanged, this, &VuoEditorWindow::viewportFitReset);
	disconnect(ui->graphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), outputPortSidebar, SLOT(externalMoveEvent()));
	disconnect(ui->graphicsView->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), outputPortSidebar, SLOT(externalMoveEvent()));
	disconnect(ui->graphicsView->verticalScrollBar(), &QScrollBar::valueChanged, this, &VuoEditorWindow::viewportFitReset);
	disconnect(ui->graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), outputPortSidebar, SLOT(externalMoveEvent()));
	disconnect(ui->graphicsView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), outputPortSidebar, SLOT(externalMoveEvent()));
	disconnect(ui->graphicsView, &VuoEditorGraphicsView::rubberBandChanged, this, &VuoEditorWindow::updateRubberBandSelectionMode);

	disconnect(ui->deleteCompositionComponents, &QAction::triggered, composition, static_cast<void (VuoEditorComposition::*)()>(&VuoEditorComposition::deleteSelectedCompositionComponents));
	disconnect(ui->renameNodes, &QAction::triggered, composition, &VuoEditorComposition::renameSelectedNodes);
	disconnect(ui->selectAll, &QAction::triggered, composition, &VuoEditorComposition::selectAllCompositionComponents);
	disconnect(ui->selectAllComments, &QAction::triggered, composition, &VuoEditorComposition::selectAllComments);

	disconnect(ui->newComposition,  &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::newComposition);
	disconnect(ui->openComposition, &QAction::triggered, static_cast<VuoEditor *>(qApp), &VuoEditor::openFile);
	disconnect(ui->cutCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::cutSelectedCompositionComponents);
	disconnect(ui->copyCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::copySelectedCompositionComponents);
	disconnect(ui->duplicateCompositionComponents, &QAction::triggered, this, &VuoEditorWindow::duplicateSelectedCompositionComponentsByMenuItem);
	disconnect(ui->paste, &QAction::triggered, this, &VuoEditorWindow::disambiguatePasteRequest);
	disconnect(ui->menuWindow, &QMenu::aboutToShow, this, &VuoEditorWindow::updateUI);
	disconnect(ui->menuEdit, &QMenu::aboutToShow, this, &VuoEditorWindow::updateUI);
	disconnect(ui->menuEdit, &QMenu::aboutToShow, this, &VuoEditorWindow::updateChangeNodeMenu);
	disconnect(ui->menuHelp, &QMenu::aboutToShow, menuOpenExample, &VuoExampleMenu::disableExampleTitleLookup);
	disconnect(ui->menuHelp, &QMenu::aboutToHide, menuOpenExample, &VuoExampleMenu::enableExampleTitleLookup);

	disconnect(undoStack, &QUndoStack::cleanChanged, this, &VuoEditorWindow::undoStackCleanStateChanged);
	disconnect(undoStack, &QUndoStack::indexChanged, searchBox, &VuoSearchBox::refreshResults);
	disconnect(undoStack, &QUndoStack::indexChanged, this, &VuoEditorWindow::coalescedUpdateRunningComposition);
	disconnect(undoStack, &QUndoStack::indexChanged, this, &VuoEditorWindow::handlePendingProtocolComplianceReevaluationRequests);
	disconnect(undoStack, SIGNAL(indexChanged(int)), inputPortSidebar, SLOT(updatePortList()));
	disconnect(undoStack, SIGNAL(indexChanged(int)), outputPortSidebar, SLOT(updatePortList()));
	disconnect(undoStack, SIGNAL(indexChanged(int)), composition, SLOT(updateFeedbackErrors()));

	disconnect(composition, &VuoEditorComposition::sceneRectChanged, this, &VuoEditorWindow::viewportFitReset);
	disconnect(composition, &VuoEditorComposition::selectionChanged, this, &VuoEditorWindow::viewportFitReset);
	disconnect(composition, &VuoEditorComposition::selectionChanged, this, &VuoEditorWindow::updateSelectedComponentMenuItems);
	disconnect(composition, &VuoEditorComposition::highlightPublishedSidebarDropLocationsRequested, inputPortSidebar, &VuoPublishedPortSidebar::highlightEligibleDropLocations);
	disconnect(composition, &VuoEditorComposition::highlightPublishedSidebarDropLocationsRequested, outputPortSidebar, &VuoPublishedPortSidebar::highlightEligibleDropLocations);
	disconnect(composition, &VuoEditorComposition::clearPublishedSidebarDropLocationHighlightingRequested, inputPortSidebar, &VuoPublishedPortSidebar::clearEligibleDropLocationHighlighting);
	disconnect(composition, &VuoEditorComposition::clearPublishedSidebarDropLocationHighlightingRequested, outputPortSidebar, &VuoPublishedPortSidebar::clearEligibleDropLocationHighlighting);

	transitionNodeLibraryConnections(nl, NULL);
	disconnect(static_cast<VuoEditor *>(qApp), &VuoEditor::globalNodeLibraryStateChanged, this, &VuoEditorWindow::conformToGlobalNodeLibraryVisibility);

	delete toolbar;
	delete metadataPanel;

	// If this is a subcomposition, revert any unsaved changes.
	VuoSubcompositionMessageRouter *subcompositionRouter = static_cast<VuoEditor *>(qApp)->getSubcompositionRouter();
	subcompositionRouter->applyIfInstalledAsSubcomposition(getComposition(), ^(VuoEditorComposition *subcomposition, string subcompositionPath) {

		map<string, string> constantPortIdentifiersAndValues;
		string revertedSourceCode;
		try
		{
			revertedSourceCode = VuoFileUtilities::readFileToString(subcompositionPath);
		}
		catch (std::exception const &e)
		{
			// The user may have deleted this subcomposition via Finder before closing its window.
			// https://b33p.net/kosada/node/16404
			return;
		}

		VuoCompilerComposition *revertedComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(revertedSourceCode, compiler);
		for (VuoNode *node : revertedComposition->getBase()->getNodes())
		{
			for (VuoPort *port : node->getInputPorts())
			{
				if (port->hasCompiler())
				{
					VuoCompilerInputEventPort *compilerPort = static_cast<VuoCompilerInputEventPort *>(port->getCompiler());
					if (compilerPort->getDataVuoType() && ! compilerPort->hasConnectedDataCable())
					{
						compilerPort->setNodeIdentifier(node->getCompiler()->getIdentifier());
						constantPortIdentifiersAndValues[compilerPort->getIdentifier()] = compilerPort->getData()->getInitialValue();
					}
				}
			}
		}
		delete revertedComposition;

		subcompositionRouter->applyToAllOtherTopLevelCompositions(getComposition(), ^(VuoEditorComposition *topLevelComposition) {

			string nodeClassName = VuoCompiler::getModuleKeyForPath(subcompositionPath);
			topLevelComposition->getModuleManager()->doNextTimeNodeClassIsLoaded(nodeClassName, ^{

				for (auto i : constantPortIdentifiersAndValues)
					topLevelComposition->updateInternalPortConstantInSubcompositionInstances(subcompositionPath, i.first, i.second);
			});
		});

		compiler->revertOverriddenNodeClass(subcompositionPath);
	});

	subcompositionRouter->removeComposition(this);

	delete composition;
	delete ui;

	// Drain the documentation queue, to ensure any pending
	// node library documentation requests have completed
	// before QWidget deallocates the node library.
	// https://b33p.net/kosada/node/15623
	dispatch_sync(((VuoEditor *)qApp)->getDocumentationQueue(), ^{});
}

/**
 * If this composition was created prior to Vuo 2.0 and it contains subcompositions or community nodes,
 * point the user to the FAQ on upgrading compositions.
 */
void VuoEditorWindow::showUpdateHelpDialog()
{
	if (composition->getBase()->getMetadata()->getLastSavedInVuoVersion().empty() && !windowFilePath().isEmpty())
	{
		bool foundCommunityNode = false;
		for (VuoNode *node : composition->getBase()->getNodes())
		{
			if (node->hasCompiler() && ! VuoStringUtilities::beginsWith(node->getNodeClass()->getClassName(), "vuo."))
			{
				foundCommunityNode = true;
				break;
			}
		}

		if (foundCommunityNode)
		{
			QString summary = tr("This composition was created in an earlier version of Vuo. It might behave differently now.");
			QString details = tr("<p><a href=\"%1\">How do I update my compositions from Vuo 1.x to Vuo 2.0?</a></p>")
							  .arg("https://vuo.org/node/2376");
			QString checkboxLabel = tr("Show this window when opening compositions.");
			QString settingsKey = "showUpdateHelp";
			VuoInfoDialog *d = new VuoInfoDialog(this, summary, details, checkboxLabel, settingsKey);
			d->show();
		}
	}
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) based on the application's state.
 */
void VuoEditorWindow::updateUI()
{
	updateSelectedComponentMenuItems();
	updateToolbarElementUI();

	ui->paste->setEnabled(!VuoEditor::getClipboardText().isEmpty());
	ui->compositionInformation->setEnabled(true);

	if (toolbar && toolbar->isStopInProgress())
	{
		ui->runComposition->setEnabled(!toolbar->isBuildPending());
		ui->stopComposition->setEnabled(false);
		ui->restartComposition->setEnabled(false);
	}
	else if (toolbar && toolbar->isBuildInProgress())
	{
		ui->runComposition->setEnabled(false);
		ui->stopComposition->setEnabled(true);
		ui->restartComposition->setEnabled(true);
	}
	else if (toolbar && toolbar->isRunning())
	{
		ui->runComposition->setEnabled(false);
		ui->stopComposition->setEnabled(true);
		ui->restartComposition->setEnabled(true);
	}
	else
	{
		ui->runComposition->setEnabled(true);
		ui->stopComposition->setEnabled(false);
		ui->restartComposition->setEnabled(false);
	}

	ui->runComposition->setText(composition->getDriverForActiveProtocol()
		? tr("Run as") + " " + QString::fromStdString(composition->getActiveProtocol()->getName())
		: tr("Run"));

	{
		auto openWindows = VuoEditorUtilities::getOpenCompositionEditingWindows();
		auto found = std::find_if(openWindows.begin(), openWindows.end(), [](VuoEditorWindow *w){ return w->metadataEditor->isVisible(); });
		bool isCompositionInformationOpen = (found != openWindows.end());
		quitAction->setEnabled(! isCompositionInformationOpen);
	}

	QString savedPath = windowFilePath();
	string savedDir, savedFile, savedExt;
	VuoFileUtilities::splitPath(savedPath.toUtf8().constData(), savedDir, savedFile, savedExt);

	bool enableSaveMenuItem = (isWindowModified() || !VuoFileUtilities::fileExists(savedPath.toStdString()));
	ui->saveComposition->setEnabled(enableSaveMenuItem);

	bool alreadyInstalledAsUserSubcomposition = (VuoFileUtilities::getUserModulesPath().c_str() == QFileInfo(savedDir.c_str()).canonicalFilePath());
	bool hasErrors = true;
	if (! alreadyInstalledAsUserSubcomposition)
	{
		try
		{
			VuoCompilerIssues issues;
			composition->getBase()->getCompiler()->check(&issues);
			hasErrors = false;
		}
		catch (VuoCompilerException &e) {}
	}

	ui->saveComposition->setText(tr("Save"));
	ui->saveCompositionAs->setText(tr("Save As…"));
	ui->installSubcomposition->setEnabled(!alreadyInstalledAsUserSubcomposition && !hasErrors);
	ui->installSubcomposition->setText(VuoFileUtilities::fileExists(windowFilePath().toStdString())?
										   tr("Move to User Library") :
										   tr("Save to User Library"));

	// Update the "Protocols" menus to reflect the currently active protocol(s).
	updateProtocolsMenu(menuProtocols);
	updateProtocolsMenu(inputPortSidebar->getProtocolsContextMenu());
	updateProtocolsMenu(outputPortSidebar->getProtocolsContextMenu());

	if (nl)
	{
		if (nl->getHumanReadable())
			ui->actionShowNodeNames->setChecked(true);
		else
			ui->actionShowNodeClassNames->setChecked(true);

		ui->toggleNodeLibraryDocking->setEnabled(!nl->isHidden());
		ui->toggleNodeLibraryDocking->setText(nl->isFloating() ? tr("Attach to Editor Window") : tr("Detach from Editor Window"));
	}

	if (inputPortSidebar && outputPortSidebar)
	{
		bool publishedPortSidebarsDisplayed = (! inputPortSidebar->isHidden());
		ui->showPublishedPorts->setText(publishedPortSidebarsDisplayed ? tr("Hide Published Ports") : tr("Show Published Ports"));
		ui->graphicsView->setVerticalScrollBarPolicy(publishedPortSidebarsDisplayed? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	}

	bool displaySearchTraversalOptions = (!searchBox->isHidden() && searchBox->traversalButtonsEnabled());
	ui->findNext->setEnabled(displaySearchTraversalOptions);
	ui->findPrevious->setEnabled(displaySearchTraversalOptions);

	bool cableDragInProgress = composition->getCableInProgress();
	undoAction->setEnabled(!cableDragInProgress && undoStack->canUndo());
	redoAction->setEnabled(!cableDragInProgress && undoStack->canRedo());

	bool showingHiddenCables = composition->getRenderHiddenCables();
	if (!showingHiddenCables)
	{
		ui->showHiddenCables->setText(tr("Show Hidden Cables"));

		// In checking for hidden cables, include published cables, since selecting "Show Hidden Cables"
		// will display the published port sidebars if there are any hidden published cables.
		ui->showHiddenCables->setEnabled(composition->hasHiddenInternalCables() || composition->hasHiddenPublishedCables());
	}
	else
	{
		ui->showHiddenCables->setText(tr("Hide Hidden Cables"));

		// Always enable, to prevent the user from getting stuck in "Show Hidden Cables" mode and being
		// unable to use context menu items to modify the hidden-status of cables in the future.
		ui->showHiddenCables->setEnabled(true);
	}

#ifdef VUO_PRO
	updateUI_Pro();
#else
	ui->exportMovie->setEnabled(false);
	ui->exportMacScreenSaver->setEnabled(false);
	ui->exportMacFFGL->setEnabled(false);
	ui->exportFxPlug->setEnabled(false);
#endif

	// Update this document's name in the "Window" menu.
	raiseDocumentAction->setText(getWindowTitleWithoutPlaceholder());

	// Update the list of open documents in the "Windows" menu.
	ui->menuWindow->clear();
	static_cast<VuoEditor *>(qApp)->populateWindowMenu(ui->menuWindow, this);

	// In open editing windows for nodes contained in this composition,
	// update UI actions that depend on whether this composition is running.
	for (QMainWindow *window : VuoEditorUtilities::getOpenEditingWindows())
	{
		VuoCodeWindow *codeWindow = dynamic_cast<VuoCodeWindow *>(window);
		if (codeWindow)
			codeWindow->updateReloadAction();

		VuoEditorWindow *editorWindow = dynamic_cast<VuoEditorWindow *>(window);
		if (editorWindow && editorWindow != this)
			editorWindow->updateRefireAction();
	}

	updateCursor();
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) in the toolbar, and all
 * synced menu items, based on the application's state.
 */
void VuoEditorWindow::updateToolbarElementUI()
{
	if (toolbar)
		toolbar->update(composition->getShowEventsMode(), ui->graphicsView->transform().isIdentity(), isZoomedToFit);

	ui->zoom11->setEnabled(! ui->graphicsView->transform().isIdentity());
	ui->zoomToFit->setEnabled(! isZoomedToFit);
	ui->showEvents->setText(composition->getShowEventsMode() ? tr("Hide Events") : tr("Show Events"));
}

/**
 * Enables/disables the Re-fire Event action based on the composition's state.
 */
void VuoEditorWindow::updateRefireAction()
{
	__block bool isTopLevelCompositionRunning = false;
	static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToLinkedTopLevelComposition(composition, ^void (VuoEditorComposition *topLevelComposition, string thisCompositionIdentifier)
	{
		isTopLevelCompositionRunning = topLevelComposition->isRunning();
	});

	ui->refireEvent->setEnabled(isTopLevelCompositionRunning ? (bool)composition->getTriggerPortToRefire() : false);
}

/**
 * Updates the UI elements and published port sidebar visibility in response to initiation of a cable drag.
 */
void VuoEditorWindow::updateWindowForNewCableDrag()
{
	if (composition->getCableInProgress() && composition->getCableInProgress()->isPublished())
		showPublishedPortSidebars();

	updateUI();
}

/**
 * Updates the cursor based on the window's state.
 */
void VuoEditorWindow::updateCursor()
{
	QCursor updatedCursor = (canvasDragInProgress?	Qt::ClosedHandCursor :
							(canvasDragEnabled?		Qt::OpenHandCursor :
													Qt::ArrowCursor));

	// Workaround to force a cursor update when the cursor believes it already has the correct updated
	// shape, but it actually does not.  See https://b33p.net/kosada/node/7718#comment-27792 .
	setCursor(Qt::ArrowCursor);

	setCursor(updatedCursor);
}

/**
 * Returns an existing window displaying an unsaved composition that was
 * contentless when initialized and unmodified currently, if one exists. Otherwise returns NULL.
 *
 * Compositions that contain only the default "Fire on Start" node are considered contentless;
 * compositions initialized from a protocol or window template are not.
 */
VuoEditorWindow * VuoEditorWindow::existingWindowWithNewFile()
{
	foreach (QWidget *widget, qApp->topLevelWidgets())
	{
		VuoEditorWindow *w = qobject_cast<VuoEditorWindow *>(widget);
		if (w && w->isVisible() && !w->containedPrepopulatedContent && (w->windowFilePath() == "") && (!w->isWindowModified()))
			return w;
	}
	return NULL;
}

/**
 * Moves input @c nodes and @c comments @c dx points horizontally and @c dy points vertically,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::itemsMoved(set<VuoRendererNode *> nodes, set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging)
{
	if ((!nodes.empty() || !comments.empty()) && !ignoreItemMoveSignals)
	{
		// Aggregate incremental moves of multiple items by mouse drag.
		if (movedByDragging && !duplicationMacroInProgress)
		{
			itemDragMacroInProgress = true;
			updateLatestDragTime();

			// Assumption: Notifications of item drags come item-by-item.
			QGraphicsItem *firstItem = (!nodes.empty()? static_cast<QGraphicsItem *>(*nodes.begin()) :
														static_cast<QGraphicsItem *>(*comments.begin()));

			if (std::find(itemsBeingDragged.begin(), itemsBeingDragged.end(), firstItem) == itemsBeingDragged.end())
				itemsBeingDragged.push_back(firstItem);

			if (firstItem == itemsBeingDragged[0])
			{
				itemDragDx += dx;
				itemDragDy += dy;
			}
		}

		if (!itemDragMacroInProgress)
			undoStack->push(new VuoCommandMove(nodes, comments, dx, dy, this, movedByDragging));
	}
}

/**
 * Resizes the input @c comment by @c dx points horizontally and @c dy points vertically,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::commentResized(VuoRendererComment *comment, qreal dx, qreal dy)
{
	commentResizeMacroInProgress = true;
	commentBeingResized = comment;
	commentResizeDx += dx;
	commentResizeDy += dy;
}

/**
 * Adds the input @c components to the scene,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::componentsAdded(QList<QGraphicsItem *> components, VuoEditorComposition *target)
{
	if ((target == composition) && (! components.empty()))
		undoStack->push(new VuoCommandAdd(components, this, "Add"));
}

/**
 * Returns Graphviz-formatted text representing the set of currently selected
 * comments and nodes, and any cables connecting selected nodes.
 */
string VuoEditorWindow::getMaximumSubcompositionFromSelection(bool includePublishedPorts, bool includeHeader)
{
	QList<QGraphicsItem *> selectedComponents = composition->selectedItems();

	// Make sure to include drawers attached to selected nodes.
	QList<QGraphicsItem *> selectedComponentsWithAttachments;
	foreach (QGraphicsItem *item, selectedComponents)
	{
		selectedComponentsWithAttachments.append(item);
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(item);
		if (rn)
		{
			set<QGraphicsItem *> dependentAttachments = composition->getDependentAttachmentsForNode(rn, false);
			foreach (QGraphicsItem *attachment, dependentAttachments)
				selectedComponentsWithAttachments.push_back(attachment);
		}
	}

	QList<QGraphicsItem *> selectedNonStrandedCompositionComponents;

	QStringList nodeDeclarations;
	vector<VuoRendererNode *> nodesToCopy;
	vector<QGraphicsItem *> potentialTypecastNodesToCopy;
	map<string, bool> nodeRepresented;
	vector<VuoRendererComment *> commentsToCopy;

	for (QList<QGraphicsItem *>::iterator i = selectedComponentsWithAttachments.begin(); i != selectedComponentsWithAttachments.end(); ++i)
	{
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);
		VuoRendererInputAttachment *attachment = dynamic_cast<VuoRendererInputAttachment *>(*i);
		bool strandedAttachment = (attachment && isStrandedAttachment(attachment, selectedComponentsWithAttachments));

		if (rn && rn->getBase()->hasCompiler() && !strandedAttachment)
		{
			if (! nodeRepresented[rn->getBase()->getCompiler()->getGraphvizIdentifier()])
			{
				nodesToCopy.push_back(rn);
				nodeRepresented[rn->getBase()->getCompiler()->getGraphvizIdentifier()] = true;
				selectedNonStrandedCompositionComponents.append(rn);
			}

			// Check for collapsed typecasts within the set of selected components.
			vector<VuoPort *> inputPorts = rn->getBase()->getInputPorts();
			for (vector<VuoPort *>::iterator inputPort = inputPorts.begin(); inputPort != inputPorts.end(); ++inputPort)
			{
				VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
				if (typecastPort)
					potentialTypecastNodesToCopy.push_back(typecastPort->getUncollapsedTypecastNode());
			}
		}
		else
		{
			VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(*i);
			if (rcomment)
				commentsToCopy.push_back(rcomment);
		}
	}

	selectedNonStrandedCompositionComponents.append(QList<QGraphicsItem *>::fromVector(QVector<QGraphicsItem *>::fromStdVector(potentialTypecastNodesToCopy)));
	set<VuoRendererCable *> internalCableSet = composition->getCablesInternalToSubcomposition(selectedNonStrandedCompositionComponents);

	// Decide which of the original collapsed typecasts to copy.
	for (vector<QGraphicsItem *>::iterator node = potentialTypecastNodesToCopy.begin(); node != potentialTypecastNodesToCopy.end(); ++node)
	{
		bool inputCableCopied = false;
		bool childPortIsPublished = false;
		vector<VuoPort *> inputPorts = ((VuoRendererNode *)(*node))->getBase()->getInputPorts();
		for(vector<VuoPort *>::iterator inputPort = inputPorts.begin(); inputPort != inputPorts.end(); ++inputPort)
		{
			vector<VuoCable *> inputCables = (*inputPort)->getConnectedCables(true);
			for (vector<VuoCable *>::iterator inputCable = inputCables.begin(); inputCable != inputCables.end(); ++inputCable)
			{
				if (std::find(internalCableSet.begin(), internalCableSet.end(), (*inputCable)->getRenderer()) != internalCableSet.end())
					inputCableCopied = true;
				if ((*inputCable)->isPublished())
					childPortIsPublished = true;
			}
		}

		// Include the collapsed typecast if the cable connected to its child input port was also included in the set of
		// selected components, or if the typecast child port was connected to a published input port.
		if (inputCableCopied || (childPortIsPublished && includePublishedPorts))
		{
			if (! nodeRepresented[((VuoRendererNode *)(*node))->getBase()->getCompiler()->getGraphvizIdentifier()])
			{
				nodesToCopy.push_back((VuoRendererNode *)(*node));
				nodeRepresented[((VuoRendererNode *)(*node))->getBase()->getCompiler()->getGraphvizIdentifier()] = true;
			}
		}

		// Otherwise, exclude the collapsed typecast and its connected cables.
		else
		{
			set<VuoCable *> connectedCables = ((VuoRendererNode *)(*node))->getConnectedCables(false);
			for (set<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
				internalCableSet.erase((*cable)->getRenderer());
		}
	}

	vector<VuoRendererCable *> cablesToCopy(internalCableSet.begin(), internalCableSet.end());
	QPointF viewportTopLeft = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect().topLeft();

	set<VuoNode *> baseNodesToCopy;
	for (vector<VuoRendererNode *>::iterator i = nodesToCopy.begin(); i != nodesToCopy.end(); ++i)
		baseNodesToCopy.insert( (*i)->getBase() );
	set<VuoCable *> baseCablesToCopy;
	for (vector<VuoRendererCable *>::iterator i = cablesToCopy.begin(); i != cablesToCopy.end(); ++i)
		baseCablesToCopy.insert( (*i)->getBase() );
	set<VuoComment *> baseCommentsToCopy;
	for (vector<VuoRendererComment *>::iterator i = commentsToCopy.begin(); i != commentsToCopy.end(); ++i)
		baseCommentsToCopy.insert( (*i)->getBase() );

	// Determine which of the composition's published ports are contained within the subcomposition.
	set<VuoPublishedPort *> subcompositionPublishedInputPortSet;
	set<VuoPublishedPort *> subcompositionPublishedOutputPortSet;

	if (includePublishedPorts)
	{
		foreach (VuoPublishedPort *publishedInputPort, composition->getBase()->getPublishedInputPorts())
		{
			foreach (VuoCable *cable, publishedInputPort->getConnectedCables())
			{
				if (baseNodesToCopy.find(cable->getToNode()) != baseNodesToCopy.end())
				{
					subcompositionPublishedInputPortSet.insert(publishedInputPort);
					baseCablesToCopy.insert(cable);
				}
			}
		}

		foreach (VuoPublishedPort *publishedOutputPort, composition->getBase()->getPublishedOutputPorts())
		{
			foreach (VuoCable *cable, publishedOutputPort->getConnectedCables())
			{
				if (baseNodesToCopy.find(cable->getFromNode()) != baseNodesToCopy.end())
				{
					subcompositionPublishedOutputPortSet.insert(publishedOutputPort);
					baseCablesToCopy.insert(cable);
				}
			}
		}
	}

	// Preserve the ordering of the published ports.
	vector<VuoPublishedPort *> subcompositionPublishedInputPorts;
	foreach (VuoPublishedPort *port, composition->getBase()->getProtocolAwarePublishedPortOrder(composition->getActiveProtocol(), true))
		if (subcompositionPublishedInputPortSet.find(port) != subcompositionPublishedInputPortSet.end())
			subcompositionPublishedInputPorts.push_back(port);

	vector<VuoPublishedPort *> subcompositionPublishedOutputPorts;
	foreach (VuoPublishedPort *port, composition->getBase()->getProtocolAwarePublishedPortOrder(composition->getActiveProtocol(), false))
		if (subcompositionPublishedOutputPortSet.find(port) != subcompositionPublishedOutputPortSet.end())
			subcompositionPublishedOutputPorts.push_back(port);

	string outputCompositionText = composition->getBase()->getCompiler()->getGraphvizDeclarationForComponents(baseNodesToCopy,
																											  baseCablesToCopy,
																											  baseCommentsToCopy,
																											  subcompositionPublishedInputPorts,
																											  subcompositionPublishedOutputPorts,
																											  (includeHeader? composition->generateCompositionHeader() : ""),
																											  "",
																											  -viewportTopLeft.x(),
																											  -viewportTopLeft.y());

	return outputCompositionText;
}

/**
 * Returns a boolean indicating whether the provided @c attachment is stranded, i.e.,
 * is contained in the provided list of @c selectedItems in the absence of its host
 * node or co-attachments.
 */
bool VuoEditorWindow::isStrandedAttachment(VuoRendererInputAttachment *attachment, QList<QGraphicsItem *> selectedItems)
{
	VuoNode *renderedHostNode = attachment->getRenderedHostNode();
	if (!(renderedHostNode && renderedHostNode->hasRenderer() && selectedItems.contains(renderedHostNode->getRenderer())))
		return true;

	set<VuoNode *> coattachments = attachment->getCoattachments();
	foreach (VuoNode *coattachment, coattachments)
	{
		if (!(coattachment->hasRenderer() && selectedItems.contains(coattachment->getRenderer())))
			return true;
	}

	return false;
}

/**
 * Copies the currently selected content.
 * If the node library documentation pane currently has selected text (only possible
 * if it is the active widget), copies that text.
 * Otherwise, copies the currently selected comments, nodes, and any cables connecting those nodes internally.
 */
void VuoEditorWindow::copySelectedCompositionComponents()
{
	QClipboard *clipboard = QApplication::clipboard();
	QMimeData *mimeData = new QMimeData();

	if (nl && !nl->getSelectedDocumentationText().isEmpty())
		mimeData->setText(nl->getSelectedDocumentationText());

	else
	{
		mimeData->setText(getMaximumSubcompositionFromSelection(false, true).c_str());
		cursorPosAtLastComponentCopy = getCursorScenePos();
	}

	clipboard->setMimeData(mimeData);
}

/**
 * Attempts to determine whether or not the clipboard contains .vuo
 * composition source text.  If so, pastes it onto the canvas; if not, activates
 * the node library text filter and pastes the clipboard contents there.
 */
void VuoEditorWindow::disambiguatePasteRequest()
{
	QString clipboardText = VuoEditor::getClipboardText();

	if (containsLikelyVuoComposition(clipboardText))
		pasteCompositionComponents();
	else if (!clipboardText.isEmpty())
	{
		on_showNodeLibrary_triggered();
		nl->searchForText(clipboardText);
	}
}

/**
 * Pastes composition components from the clipboard onto the canvas.
 */
void VuoEditorWindow::pasteCompositionComponents()
{
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();

	int publishedPortsBeforePaste = composition->getBase()->getPublishedInputPorts().size() +
									composition->getBase()->getPublishedOutputPorts().size();

	int publishedCablesBeforePaste = 0;
	foreach (VuoPublishedPort *publishedInput, composition->getBase()->getPublishedInputPorts())
		publishedCablesBeforePaste += publishedInput->getConnectedCables(true).size();
	foreach (VuoPublishedPort *publishedOutput, composition->getBase()->getPublishedOutputPorts())
		publishedCablesBeforePaste += publishedOutput->getConnectedCables(true).size();

	if (mimeData->hasFormat("text/plain"))
	{
		// Paste at the cursor location only if the cursor is currently within the viewport bounds
		// and has moved since the most recent copy operation.
		QRectF viewportRect = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();
		QPointF cursorPos = getCursorScenePos();
		bool pasteAtCursorLoc = ((cursorPos != cursorPosAtLastComponentCopy) && viewportRect.contains(cursorPos));

		QString compositionText = mimeData->text();
		mergeCompositionComponentsFromString(compositionText.toUtf8().constData(), pasteAtCursorLoc, !pasteAtCursorLoc, "Paste");
	}

	int publishedPortsAfterPaste = composition->getBase()->getPublishedInputPorts().size() +
									composition->getBase()->getPublishedOutputPorts().size();

	int publishedCablesAfterPaste = 0;
	foreach (VuoPublishedPort *publishedInput, composition->getBase()->getPublishedInputPorts())
		publishedCablesAfterPaste += publishedInput->getConnectedCables(true).size();
	foreach (VuoPublishedPort *publishedOutput, composition->getBase()->getPublishedOutputPorts())
		publishedCablesAfterPaste += publishedOutput->getConnectedCables(true).size();

	// Display the published port sidebars if the composition has any new published ports or cables.
	if ((publishedPortsAfterPaste > publishedPortsBeforePaste) || (publishedCablesAfterPaste > publishedCablesBeforePaste))
		setPublishedPortSidebarVisibility(true);
}

/**
 * Duplicates the currently selected comments, nodes, and any cables connecting those nodes internally.
 *
 * Should be called when the duplication is to be performed as part of a mouse drag, and the
 * operations should be aggregated on the Undo stack.
 */
void VuoEditorWindow::duplicateSelectedCompositionComponentsByDrag()
{
	// Compose a macro to merge the duplication and subsequent mouse drag operation of duplicated components.
	if (! duplicationMacroInProgress)
	{
		undoStack->beginMacro(tr("Duplication"));
		duplicationMacroInProgress = true;
	}

	string subcompositionText = getMaximumSubcompositionFromSelection(false, true);
	QList<QGraphicsItem *> newComponents = mergeCompositionComponentsFromString(subcompositionText, false, false, "Duplication");

	// Clear the previous selection and select the duplicated components.
	composition->clearSelection();
	for (QList<QGraphicsItem *>::iterator i = newComponents.begin(); i != newComponents.end(); ++i)
		(*i)->setSelected(true);
}

/**
  * Calls attention to nodes of class @c nodeClass within the composition
  * by selecting them and centering the view on them.
  */
void VuoEditorWindow::highlightNodeClass(string nodeClass)
{
	foreach (VuoNode *node, composition->getBase()->getNodes())
	{
		VuoNodeClass *currentNodeClass = node->getNodeClass();
		string genericNodeClassName;
		if (currentNodeClass->hasCompiler() && dynamic_cast<VuoCompilerSpecializedNodeClass *>(currentNodeClass->getCompiler()))
			genericNodeClassName = dynamic_cast<VuoCompilerSpecializedNodeClass *>(currentNodeClass->getCompiler())->getOriginalGenericNodeClassName();
		else
			genericNodeClassName = currentNodeClass->getClassName();

		if ((genericNodeClassName == nodeClass) && node->hasRenderer())
			node->getRenderer()->setSelected(true);
	}

	// Try to keep selected components visible.
	QRectF selectedItemsRect;
	foreach (QGraphicsItem *selectedComponent, composition->selectedItems())
		selectedItemsRect |= selectedComponent->sceneBoundingRect();

	if (!selectedItemsRect.isNull())
		ui->graphicsView->centerOn(selectedItemsRect.center());
}

/**
 * Duplicates the currently selected nodes and any cables connecting them internally, inserting
 * the new components at an offset from the originals.
 *
 * Should be called when the duplication is to be performed as a stand-alone operation
 * (e.g., in response to a menu item selection, not as part of a mouse drag).
 */
void VuoEditorWindow::duplicateSelectedCompositionComponentsByMenuItem()
{
	string subcompositionText = getMaximumSubcompositionFromSelection(false, true);
	QList<QGraphicsItem *> newComponents = mergeCompositionComponentsFromString(subcompositionText, false, true, "Duplication");

	// Clear the previous selection and select the duplicated components.
	composition->clearSelection();
	for (QList<QGraphicsItem *>::iterator i = newComponents.begin(); i != newComponents.end(); ++i)
		(*i)->setSelected(true);
}

/**
 * Cleans up the 'Undo' stack after a cancelled duplication.
 */
void VuoEditorWindow::cleanUpCancelledDuplication()
{
	// End composition of duplication macro.
	resetUndoStackMacros();

	// Undo the composite duplication operation already on the 'Undo' stack.
	undoAction->trigger();
}

/**
 * Clears the composition of its current components and re-populates it from the provided @a snapshot.
 */
void VuoEditorWindow::resetCompositionWithSnapshot(string snapshot)
{
	if (snapshot != composition->takeSnapshot())
	{
		composition->modifyComponents(^{
			composition->clear();
			instantiateNewCompositionComponentsFromString(snapshot);
		});
		updateUI();
	}
}

/**
 * Instantiates the composition components specified in the .dot-format input @c compositionText,
 * including all published ports and published cables.
 */
void VuoEditorWindow::instantiateNewCompositionComponentsFromString(string compositionText)
{
	VuoCompilerGraphvizParser *graphParser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionText, compiler);

	foreach (VuoNode *node, graphParser->getNodes())
		composition->addNode(node);

	foreach (VuoPublishedPort *publishedInputPort, graphParser->getPublishedInputPorts())
	{
		composition->addPublishedPort(publishedInputPort, true);
		composition->createRendererForPublishedPortInComposition(publishedInputPort, true);
	}

	foreach (VuoPublishedPort *publishedOutputPort, graphParser->getPublishedOutputPorts())
	{
		composition->addPublishedPort(publishedOutputPort, false);
		composition->createRendererForPublishedPortInComposition(publishedOutputPort, false);
	}

	foreach (VuoCable *cable, graphParser->getCables())
	{
		composition->addCable(cable, false);

		if (cable->isPublishedInputCable())
			cable->setFrom(composition->getPublishedInputNode(), cable->getFromPort());
		if (cable->isPublishedOutputCable())
			cable->setTo(composition->getPublishedOutputNode(), cable->getToPort());
	}

	foreach (VuoComment *comment, graphParser->getComments())
		composition->addComment(comment);

	// Collapse any typecasts possible.
	composition->collapseTypecastNodes();

	// Now that all renderer components have been created, calculate
	// the final positions of collapsed "Make List" drawers.
	foreach (VuoNode *node, graphParser->getNodes())
		node->getRenderer()->layoutConnectedInputDrawers();

	// @todo https://b33p.net/kosada/node/10638
	//delete graphParser;

	updateUI();
}

/**
 * Instantiates the composition components specified in the .dot-format input @c compositionText
 * and merges them into the existing composition, pushing the changes onto the Undo stack
 * under the provided `commandDescription`.
 * Returns a list of the newly created nodes, cables, and comments, excluding published ports and published cables.
 */
QList<QGraphicsItem *> VuoEditorWindow::mergeCompositionComponentsFromString(string compositionText, bool pasteAtCursorLoc, bool pasteWithOffset, string commandDescription)
{
	VuoCompilerComposition *pastedComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionText, compiler);
	QList<QGraphicsItem *> pastedComponents = QList<QGraphicsItem *>();

	QPointF startPos = (pasteWithOffset? QPointF(pastedComponentOffset, pastedComponentOffset) : QPointF(0,0));

	// Replace nodes that can't be legally added to this composition with implementation-less nodes.
	for (VuoNode *pastedNode : pastedComposition->getBase()->getNodes())
	{
		if (pastedNode->getNodeClass()->hasCompiler())
		{
			VuoNode *allowedNode = composition->createBaseNode(pastedNode->getNodeClass()->getCompiler(), pastedNode);
			if (! allowedNode->getNodeClass()->hasCompiler())
			{
				pastedComposition->replaceNode(pastedNode, allowedNode);
				allowedNode->setRawGraphvizDeclaration(pastedNode->getRawGraphvizDeclaration());
				delete pastedNode;
			}
		}
	}

	set<VuoNode *> pastedNodes = pastedComposition->getBase()->getNodes();
	for (VuoNode *node : pastedNodes)
	{
		VuoRendererNode *rn = ((VuoRendererComposition *)composition)->createRendererNode(node);
		pastedComponents.append(rn);
	}

	set<VuoComment *> pastedComments = pastedComposition->getBase()->getComments();
	for (VuoComment *comment : pastedComments)
	{
		VuoRendererComment *rc = ((VuoRendererComposition *)composition)->createRendererComment(comment);
		pastedComponents.append(rc);
	}

	qreal minX = std::numeric_limits<qreal>::max();
	qreal minY = std::numeric_limits<qreal>::max();

	if (pasteAtCursorLoc)
	{
		// Find the top-left of the pasted component cluster, to position at the cursor location.
		for (VuoNode *node : pastedNodes)
		{
			// Disregard the positions of nodes that will be rendered as attachments.
			if (dynamic_cast<VuoRendererInputAttachment *>(node->getRenderer()))
				continue;

			if (node->getX() < minX)
				minX = node->getX();

			if (node->getY() < minY)
				minY = node->getY();
		}

		for (VuoComment *comment : pastedComments)
		{
			if (comment->getX() < minX)
				minX = comment->getX();

			if (comment->getY() < minY)
				minY = comment->getY();
		}

		startPos += (getFittedScenePos(getCursorScenePos()) - QPointF(minX, minY));
	}
	else
	{
		QPointF viewportTopLeft = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect().topLeft();
		startPos += viewportTopLeft;
	}

	// Set new node and comment positions.
	this->ignoreItemMoveSignals = true;
	for (VuoNode *node : pastedNodes)
	{
		node->setX(startPos.x() + node->getX());
		node->setY(startPos.y() + node->getY());

		node->getRenderer()->setPos(node->getX(), node->getY());
	}

	for (VuoComment *comment : pastedComments)
	{
		comment->setX(startPos.x() + comment->getX());
		comment->setY(startPos.y() + comment->getY());

		comment->getRenderer()->setPos(comment->getX(), comment->getY());
	}
	this->ignoreItemMoveSignals = false;

	set<VuoCable *> pastedCables = pastedComposition->getBase()->getCables();
	for (VuoCable *cable : pastedCables)
	{
		if (! cable->isPublished())
		{
			VuoRendererCable *rc = new VuoRendererCable(cable);
			pastedComponents.append(rc);
		}
	}

	// Remove the pre-existing published cable connections from the pasted components.
	// We will make use of our own customized published port merging algorithm and will
	// create and connect our own published cables.
	// But first, document some information about the connections.
	map<VuoPublishedPort *, vector<pair<VuoPort *, bool> > > connectionsForPublishedPort;
	map<VuoPublishedPort *, bool> publishedPortHadConnectedDataCable;

	foreach (VuoPublishedPort *publishedInput, pastedComposition->getBase()->getPublishedInputPorts())
	{
		bool foundConnectedDataCable = false;
		vector<VuoCable *> publishedInputCables = publishedInput->getConnectedCables(true);
		foreach (VuoCable *cable, publishedInputCables)
		{
			VuoPort *toPort = cable->getToPort();
			VuoCompilerPort *compilerToPort = static_cast<VuoCompilerPort *>(toPort->getCompiler());
			bool alwaysEventOnly = cable->getCompiler()->getAlwaysEventOnly();

			bool cableCarriesData = (compilerToPort && compilerToPort->getDataVuoType() && !alwaysEventOnly);
			if (cableCarriesData)
				foundConnectedDataCable = true;

			connectionsForPublishedPort[publishedInput].push_back(make_pair(toPort, alwaysEventOnly));

			cable->setFrom(NULL, NULL);
			cable->setTo(NULL, NULL);
		}

		publishedPortHadConnectedDataCable[publishedInput] = foundConnectedDataCable;
	}

	foreach (VuoPublishedPort *publishedOutput, pastedComposition->getBase()->getPublishedOutputPorts())
	{
		bool foundConnectedDataCable = false;
		vector<VuoCable *> publishedOutputCables = publishedOutput->getConnectedCables(true);
		foreach (VuoCable *cable, publishedOutputCables)
		{
			VuoPort *fromPort = cable->getFromPort();
			VuoCompilerPort *compilerFromPort = static_cast<VuoCompilerPort *>(fromPort->getCompiler());
			bool alwaysEventOnly = cable->getCompiler()->getAlwaysEventOnly();

			bool cableCarriesData = (compilerFromPort && compilerFromPort->getDataVuoType() && !alwaysEventOnly);
			if (cableCarriesData)
				foundConnectedDataCable = true;

			connectionsForPublishedPort[publishedOutput].push_back(make_pair(fromPort, alwaysEventOnly));

			cable->setFrom(NULL, NULL);
			cable->setTo(NULL, NULL);
		}

		publishedPortHadConnectedDataCable[publishedOutput] = foundConnectedDataCable;
	}

	// Begin an 'Undo' macro to aggregate the instantiation of new composition components with the
	// publication of relevant ports.
	undoStack->beginMacro(tr(commandDescription.c_str()));

	// Add pasted components to composition.
	componentsPasted(pastedComponents, commandDescription);

	// Publish any input ports that were published in their source environment.
	vector<VuoPublishedPort *> unmergedPublishedInputPorts;

	// First pass: Check for any pasted published input ports that have identically named ports in the
	// target environment that can accommodate merging.
	foreach (VuoPublishedPort *publishedInputPort, pastedComposition->getBase()->getPublishedInputPorts())
	{
		string pastedPublishedPortName = publishedInputPort->getClass()->getName();
		VuoPublishedPort *existingPortWithSameName = composition->getBase()->getPublishedInputPortWithName(pastedPublishedPortName);

		if (existingPortWithSameName && dynamic_cast<VuoRendererPublishedPort *>(existingPortWithSameName->getRenderer())->canBeMergedWith(publishedInputPort,
																								 publishedPortHadConnectedDataCable[publishedInputPort]))
		{
			vector<pair<VuoPort *, bool> > internalConnections = connectionsForPublishedPort[publishedInputPort];
			for (vector<pair<VuoPort *, bool> >::iterator i = internalConnections.begin(); i != internalConnections.end(); ++i)
			{
				VuoPort *connectedPort = i->first;
				bool forceEventOnlyPublication = i->second;

				internalPortPublished(connectedPort, forceEventOnlyPublication, pastedPublishedPortName, true);
			}
		}
		else
			unmergedPublishedInputPorts.push_back(publishedInputPort);
	}

	// Second pass: Re-name and publish the remaining pasted published input ports.
	foreach (VuoPublishedPort *unmergedPublishedInputPort, unmergedPublishedInputPorts)
	{
		string uniquePublishedPortName = composition->getUniquePublishedPortName(unmergedPublishedInputPort->getClass()->getName());

		vector<pair<VuoPort *, bool> > internalConnections = connectionsForPublishedPort[unmergedPublishedInputPort];
		if (!internalConnections.empty())
		{
			for (vector<pair<VuoPort *, bool> >::iterator i = internalConnections.begin(); i != internalConnections.end(); ++i)
			{
				VuoPort *connectedPort = i->first;
				bool forceEventOnlyPublication = i->second;

				internalPortPublished(connectedPort, forceEventOnlyPublication, uniquePublishedPortName, true);
			}
		}

		else
		{
			unmergedPublishedInputPort->getClass()->setName(uniquePublishedPortName);
			composition->addPublishedPort(unmergedPublishedInputPort, true);
			composition->createRendererForPublishedPortInComposition(unmergedPublishedInputPort, true);
		}
	}

	// Publish any output ports that were published in their source environment.
	vector<VuoPublishedPort *> unmergedPublishedOutputPorts;

	// First pass: Check for any pasted published output ports that have identically named ports in the
	// target environment that can accommodate merging.
	foreach (VuoPublishedPort *publishedOutputPort, pastedComposition->getBase()->getPublishedOutputPorts())
	{
		string pastedPublishedPortName = publishedOutputPort->getClass()->getName();
		VuoPublishedPort *existingPortWithSameName = composition->getBase()->getPublishedOutputPortWithName(pastedPublishedPortName);

		if (existingPortWithSameName && dynamic_cast<VuoRendererPublishedPort *>(existingPortWithSameName->getRenderer())->canBeMergedWith(publishedOutputPort,
																								 publishedPortHadConnectedDataCable[publishedOutputPort]))
		{
			vector<pair<VuoPort *, bool> > internalConnections = connectionsForPublishedPort[publishedOutputPort];
			for (vector<pair<VuoPort *, bool> >::iterator i = internalConnections.begin(); i != internalConnections.end(); ++i)
			{
				VuoPort *connectedPort = i->first;
				bool forceEventOnlyPublication = i->second;

				internalPortPublished(connectedPort, forceEventOnlyPublication, pastedPublishedPortName, true);
			}
		}
		else
			unmergedPublishedOutputPorts.push_back(publishedOutputPort);
	}

	// Second pass: Re-name and publish the remaining pasted published output ports.
	foreach (VuoPublishedPort *unmergedPublishedOutputPort, unmergedPublishedOutputPorts)
	{
		string uniquePublishedPortName = composition->getUniquePublishedPortName(unmergedPublishedOutputPort->getClass()->getName());

		vector<pair<VuoPort *, bool> > internalConnections = connectionsForPublishedPort[unmergedPublishedOutputPort];
		if (!internalConnections.empty())
		{
			for (vector<pair<VuoPort *, bool> >::iterator i = internalConnections.begin(); i != internalConnections.end(); ++i)
			{
				VuoPort *connectedPort = i->first;
				bool forceEventOnlyPublication = i->second;

				internalPortPublished(connectedPort, forceEventOnlyPublication, uniquePublishedPortName, true);
			}
		}

		else
		{
			unmergedPublishedOutputPort->getClass()->setName(uniquePublishedPortName);
			composition->addPublishedPort(unmergedPublishedOutputPort, false);
			composition->createRendererForPublishedPortInComposition(unmergedPublishedOutputPort, false);
		}
	}

	delete pastedComposition;

	undoStack->endMacro();

	return pastedComponents;
}

/**
 * Pastes the input @c components into the scene,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::componentsPasted(QList<QGraphicsItem *> components, string commandDescription)
{
	if (components.empty())
		return;

	// Add the components to the scene.
	undoStack->push(new VuoCommandAdd(components, this, commandDescription));

	// Clear the previous selection and select the pasted components.
	composition->clearSelection();
	for (QList<QGraphicsItem *>::iterator i = components.begin(); i != components.end(); ++i)
		(*i)->setSelected(true);
}

/**
 * Copies currenly selected comments, nodes, and any cables connecting those nodes internally;
 * deletes all currently selected comments, nodes, and cables.
 */
void VuoEditorWindow::cutSelectedCompositionComponents()
{
	copySelectedCompositionComponents();
	composition->deleteSelectedCompositionComponents("Cut");
}

/**
 * Completes the connection for the input @c cableInProgress
 * to port @c targetPort.  This operation may also involve:
 * - Specializing or re-specializing the @c portToSpecialize (if non-NULL) by
 *   assigning it a type of @c specializedTypeName;
 * - Displacing the @c dataCableToDisplace (if non-NULL);
 * - Replacing the @c cableToReplace (if non-NULL);
 * - Deleting the @c typecastNodeToDelete (if non-NULL);
 * - Unpublishing the @c portToUnpublish (if non-NULL); and
 * - Bridging the connection with a typecast of class name @c typecastToInsert (if non-empty).
 *
 * The @c cableArgs pair is expected to contain (dataCableToDisplace, cableToReplace).
 * The @c typeArgs pair is expected to contain (typecastToInsert, specializedTypeName).
 * The @c portArgs pair is expected to contain (portToUnpublish, portToSpecialize).
 *
 * Pushes the sequence of operations onto the Undo stack, *without* coalescing them
 * into a macro. To do so, call beginUndoStackMacro() and endUndoStackMacro()
 * before and after calling this function.
 */
void VuoEditorWindow::connectionCompleted(VuoRendererCable *cableInProgress,
										  VuoRendererPort *targetPort,
										  pair<VuoRendererCable *, VuoRendererCable *> cableArgs,
										  VuoRendererNode *typecastNodeToDelete,
										  pair<string, string> typeArgs,
										  pair<VuoRendererPort *, VuoRendererPort *> portArgs)
{
	updateLatestDragTime();

	// Unpack arguments that were packed into pairs to avoid Qt's parameter count limit for signals/slots.
	VuoRendererCable *dataCableToDisplace = cableArgs.first;
	VuoRendererCable *cableToReplace = cableArgs.second;
	string typecastToInsert = typeArgs.first;
	string specializedTypeName = typeArgs.second;
	VuoRendererPort *portToUnpublish = portArgs.first;
	VuoRendererPort *portToSpecialize = portArgs.second;

	// Reconstruct the state of the composition before the beginning of the cable drag
	// that concluded with this connection, for the composition's initial "Before" snapshot
	// in the following sequence of operations.
	if (!cableInProgress->getBase()->getToPort() &&
			!composition->getCableInProgressWasNew() &&
			cableInProgress->getFloatingEndpointPreviousToPort())
	{
		bool cableInProgressAlwaysEventOnly = cableInProgress->getBase()->getCompiler()->getAlwaysEventOnly();
		cableInProgress->setTo(composition->getUnderlyingParentNodeForPort(cableInProgress->getFloatingEndpointPreviousToPort(), composition),
							   cableInProgress->getFloatingEndpointPreviousToPort());
		cableInProgress->getBase()->getCompiler()->setAlwaysEventOnly(cableInProgress->getPreviouslyAlwaysEventOnly());

		// Execute an identity Undo stack command simply to record the composition's screenshot
		// before the cable drag began.
		set<VuoRendererNode *> emptyNodeSet;
		set<VuoRendererComment *> emptyCommentSet;
		undoStack->push(new VuoCommandMove(emptyNodeSet, emptyCommentSet, 0, 0, this, false));

		// Now reconstruct the state of the composition mid-cable drag and carry on.
		cableInProgress->getBase()->getCompiler()->setAlwaysEventOnly(cableInProgressAlwaysEventOnly);
		cableInProgress->setTo(NULL, NULL);
	}

	// Create the requested typecast node for eventual insertion.
	QList<QGraphicsItem *> typecastRelatedComponentsToAdd = QList<QGraphicsItem *>();
	VuoRendererNode *typecastNodeToAdd = NULL;
	VuoPort *typecastInPort = NULL;
	VuoPort *typecastOutPort = NULL;
	if (!typecastToInsert.empty())
	{
		typecastNodeToAdd = composition->createNode(typecastToInsert.c_str(), "",
					targetPort->scenePos().x()+100,
					targetPort->scenePos().y()+100);

		typecastRelatedComponentsToAdd.append(typecastNodeToAdd);

		typecastInPort = typecastNodeToAdd->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
		typecastOutPort = typecastNodeToAdd->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

		// Apply NULL checks liberally to avoid currently undiagnosed getRenderer()-related crash. https://b33p.net/kosada/node/9053
		if (!(typecastInPort && typecastInPort->hasRenderer() && typecastOutPort && typecastOutPort->hasRenderer()))
			return;
	}

	VuoRendererPort *unadjustedFromPort = NULL;
	VuoRendererPort *unadjustedToPort = NULL;

	// Apply NULL checks liberally to avoid currently undiagnosed getRenderer()-related crash. https://b33p.net/kosada/node/9053
	if (targetPort->getInput() && !(cableInProgress->getBase()->getFromPort() && cableInProgress->getBase()->getFromPort()->hasRenderer()))
		return;
	if (!targetPort->getInput() && !(cableInProgress->getBase()->getToPort() && cableInProgress->getBase()->getToPort()->hasRenderer()))
		return;

	// Specialize the port and its network, if applicable.
	if (portToSpecialize)
	{
		bool currentlyGeneric = dynamic_cast<VuoGenericType *>(portToSpecialize->getDataType());

		VuoRendererNode *specializedNode = NULL;
		if (currentlyGeneric)
			specializedNode = specializePortNetwork(portToSpecialize, specializedTypeName, false);
		else
			specializedNode = respecializePortNetwork(portToSpecialize, specializedTypeName, false);

		if (!specializedNode)
			return;

		if (portToSpecialize == targetPort)
			targetPort = (targetPort->getInput()? specializedNode->getBase()->getInputPortWithName(targetPort->getBase()->getClass()->getName())->getRenderer() :
												  specializedNode->getBase()->getOutputPortWithName(targetPort->getBase()->getClass()->getName())->getRenderer());
	}

	if (targetPort->getInput())
	{
		unadjustedFromPort = cableInProgress->getBase()->getFromPort()->getRenderer();
		unadjustedToPort = targetPort;
	}
	else
	{
		unadjustedFromPort = targetPort;
		unadjustedToPort = cableInProgress->getBase()->getToPort()->getRenderer();
	}

	// @todo: Possibly re-use logic from VuoCompilerCable::carriesData() even though we don't yet have a connected cable.
	// Should really only matter if we implement published cable connection by dragging before we implement external published port types
	// that are independent of their internal connected port types, since then we must take into account that the cable may carry data even
	// if the (published) 'From' port technically does not.
	bool cableWillCarryData = (cableInProgress->effectivelyCarriesData() &&
							   (dynamic_cast<VuoRendererPublishedPort *>(unadjustedFromPort)?
								   static_cast<VuoCompilerPortClass *>(unadjustedFromPort->getBase()->getClass()->getCompiler())->getDataVuoType() :
								   unadjustedFromPort->getDataType()) &&
							   (dynamic_cast<VuoRendererPublishedPort *>(unadjustedToPort)?
									static_cast<VuoCompilerPortClass *>(unadjustedToPort->getBase()->getClass()->getCompiler())->getDataVuoType() :
								   unadjustedToPort->getDataType())
							   );

	VuoPort *previousToPort = cableInProgress->getFloatingEndpointPreviousToPort();

	// If connecting a data+event cable, and the intended input port had a previously connected
	// collapsed "Make List" node, delete it.
	VuoRendererInputDrawer *attachedDrawer = unadjustedToPort->getAttachedInputDrawer();
	if (attachedDrawer && cableWillCarryData)
	{
		QList<QGraphicsItem *> drawerRelatedComponentsToRemove;
		drawerRelatedComponentsToRemove.append(attachedDrawer);
		bool disableAutomaticAttachmentInsertion = true;
		undoStack->push(new VuoCommandRemove(drawerRelatedComponentsToRemove, this, inputEditorManager, "", disableAutomaticAttachmentInsertion));
	}

	VuoRendererPort *adjustedTargetPort = targetPort;

	if (typecastNodeToDelete)
	{
		QList<QGraphicsItem *> typecastRelatedComponentsToRemove = QList<QGraphicsItem *>();
		typecastRelatedComponentsToRemove.append(typecastNodeToDelete);
		typecastRelatedComponentsToRemove.append(dataCableToDisplace);

		undoStack->push(new VuoCommandRemove(typecastRelatedComponentsToRemove, this, inputEditorManager, "", true));

		dataCableToDisplace = NULL;
	}

	// Replace a pre-existing cable that connected the same two ports (but that had a different data-carrying status).
	if (cableToReplace)
	{
		QList<QGraphicsItem *> componentsToReplace = QList<QGraphicsItem *>();
		componentsToReplace.append(cableToReplace);
		undoStack->push(new VuoCommandRemove(componentsToReplace, this, inputEditorManager, "", false));
	}

	if (! typecastToInsert.empty())
	{
		// Insert an extra cable leading to or from the typecast as appropriate.
		VuoRendererCable *newCableConnectingTypecast;
		VuoRendererCable *incomingTypecastCable;

		// Case: making a "forward" cable connection (from an output port to an input port)
		if (targetPort->getInput())
		{
			// Prepare to add a cable connecting the output port of the newly inserted
			// typecast to the originally intended input port.
			VuoCompilerNode *fromNode = typecastNodeToAdd->getBase()->getCompiler();
			VuoCompilerPort *fromPort = (VuoCompilerPort *)typecastOutPort->getCompiler();
			VuoCompilerNode *toNode = targetPort->getUnderlyingParentNode()->getBase()->getCompiler();
			VuoCompilerPort *toPort = (VuoCompilerPort *)targetPort->getBase()->getCompiler();

			// Prepare to re-route the dragged cable's loose end to connect to the typecast's input port.
			adjustedTargetPort = typecastInPort->getRenderer();

			newCableConnectingTypecast = new VuoRendererCable((new VuoCompilerCable(fromNode, fromPort, toNode, toPort))->getBase());
			incomingTypecastCable = cableInProgress;
		}

		// Case: making a "backward" cable connection (from an input port to an output port)
		else
		{
			// Prepare to add a cable connecting the originally intended output port
			// to the input port of the newly inserted typecast.
			VuoCompilerNode *fromNode = targetPort->getUnderlyingParentNode()->getBase()->getCompiler();
			VuoCompilerPort *fromPort = (VuoCompilerPort *)targetPort->getBase()->getCompiler();
			VuoCompilerNode *toNode = typecastNodeToAdd->getBase()->getCompiler();
			VuoCompilerPort *toPort = (VuoCompilerPort *)typecastInPort->getCompiler();

			// Prepare to re-route the dragged cable's loose end to connect to the typecast's output port.
			adjustedTargetPort = typecastOutPort->getRenderer();

			newCableConnectingTypecast = new VuoRendererCable((new VuoCompilerCable(fromNode, fromPort, toNode, toPort))->getBase());
			incomingTypecastCable = newCableConnectingTypecast;
		}

		typecastRelatedComponentsToAdd.append(newCableConnectingTypecast);

		bool disableAutomaticAttachmentInsertion = incomingTypecastCable->effectivelyCarriesData();
		undoStack->push(new VuoCommandAdd(typecastRelatedComponentsToAdd, this, "", disableAutomaticAttachmentInsertion));
	}

	undoStack->push(new VuoCommandConnect(cableInProgress, adjustedTargetPort, dataCableToDisplace, portToUnpublish, this, inputEditorManager));

	// If re-connecting a cable whose previous 'To' port had a collapsed typecast attached, decide what
	// to do with the typecast.
	if (targetPort->getInput() && previousToPort && (previousToPort != targetPort->getBase()))
	{
		VuoRendererTypecastPort *previousTypecastToPort = (VuoRendererTypecastPort *)(previousToPort->getRenderer()->getTypecastParentPort());
		if (previousTypecastToPort)
		{
			// If the typecast does not have any remaining incoming cables, delete it.
			VuoRendererPort *childPort = previousTypecastToPort->getChildPort();
			if (childPort->getBase()->getConnectedCables(true).size() < 1)
			{
					VuoRendererNode *uncollapsedTypecastToPort = composition->uncollapseTypecastNode(previousTypecastToPort);
					QList<QGraphicsItem *> typecastRelatedComponentsToRemove = QList<QGraphicsItem *>();
					typecastRelatedComponentsToRemove.append(uncollapsedTypecastToPort);
					undoStack->push(new VuoCommandRemove(typecastRelatedComponentsToRemove, this, inputEditorManager, ""));
			}

			// If the typecast does have remaining incoming cables but none of them carry data, uncollapse it.
			// @todo For consistency with cable deletion, we should really reroute the event-only cables
			// to the typecast host port.
			else if (!childPort->effectivelyHasConnectedDataCable(true))
				composition->uncollapseTypecastNode(previousTypecastToPort);

			composition->clearCableEndpointEligibilityHighlighting();
		}
	}

	// If the target port is generic and, as a result of this connection, has only a single
	// compatible specialized type, specialize its network.
	VuoGenericType *genericType = dynamic_cast<VuoGenericType *>((static_cast<VuoCompilerPort *>(targetPort->getBase()->getCompiler()))->getDataVuoType());
	if (genericType)
	{
		VuoGenericType::Compatibility compatibility;
		vector<string> compatibleSpecializedTypes = genericType->getCompatibleSpecializedTypes(compatibility);
		if (compatibleSpecializedTypes.size() == 1)
		{
			string loneCompatibleSpecializedType = *compatibleSpecializedTypes.begin();
			specializePortNetwork(targetPort, loneCompatibleSpecializedType, false);
		}
	}
}

/**
 * Removes the input @c components from the scene,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::componentsRemoved(QList<QGraphicsItem *> components, string commandDescription)
{
	if (! components.empty())
		undoStack->push(new VuoCommandRemove(components, this, inputEditorManager, commandDescription));
}

/**
 * Updates the trigger port's event-throttling behavior.
 */
void VuoEditorWindow::setTriggerThrottling(VuoPort *triggerPort, enum VuoPortClass::EventThrottling eventThrottling)
{
	if (triggerPort->getEventThrottling() != eventThrottling)
		undoStack->push(new VuoCommandSetTriggerThrottling(triggerPort, eventThrottling, this));
}

/**
 * Adjusts the input port count for the provided @c node by @c inputPortCountDelta , provided
 * the node is of a class (e.g., "Make List") eligible for such an operation.
 */
void VuoEditorWindow::adjustInputPortCountForNode(VuoRendererNode *node, int inputPortCountDelta, bool adjustmentedRequestedByDragging)
{
	VuoCompilerNodeClass *origNodeClass = node->getBase()->getNodeClass()->getCompiler();
	VuoCompilerNodeClass *newNodeClass = NULL;

	// Case: "Make List" node
	if (VuoCompilerMakeListNodeClass::isMakeListNodeClassName(origNodeClass->getBase()->getClassName()))
	{
		int origNumPorts = ((VuoCompilerMakeListNodeClass *)(origNodeClass))->getItemCount();
		VuoCompilerType *listType = ((VuoCompilerMakeListNodeClass *)(origNodeClass))->getListType();
		string newMakeListNodeClassName = VuoCompilerMakeListNodeClass::getNodeClassName(origNumPorts+inputPortCountDelta, listType);
		newNodeClass = compiler->getNodeClass(newMakeListNodeClassName);
	}

	if (newNodeClass)
	{
		VuoNode *newNode = newNodeClass->newNode(node->getBase());
		string commandText = (inputPortCountDelta == -1? "Remove Input Port" : (inputPortCountDelta == 1? "Add Input Port" : ""));
		undoStack->push(new VuoCommandReplaceNode(node, composition->createRendererNode(newNode), this, commandText));

		if (adjustmentedRequestedByDragging)
		{
			VuoRendererInputListDrawer *oldMakeListNode = dynamic_cast<VuoRendererInputListDrawer *>(node);
			VuoRendererInputListDrawer *newMakeListNode = dynamic_cast<VuoRendererInputListDrawer *>(newNode->getRenderer());

			if (oldMakeListNode)
			{
				if (composition->mouseGrabberItem() == oldMakeListNode)
					oldMakeListNode->ungrabMouse();

				oldMakeListNode->setDragInProgress(false);
			}

			if (newMakeListNode)
			{
				newMakeListNode->setDragInProgress(true);
				newMakeListNode->grabMouse();
			}
		}
	}
}

/**
 * Replaces the provided composition node with another node of the specified class, preserving
 * whatever connections possible.
 */
void VuoEditorWindow::swapNodes(VuoRendererNode *node, string newNodeClass)
{
	VuoCompilerNodeClass *newNodeCompilerClass = compiler->getNodeClass(newNodeClass);
	if (!newNodeCompilerClass)
		return;

	VuoRendererNode *newNode = composition->createNode(newNodeClass.c_str(), "", node->getBase()->getX(), node->getBase()->getY());
	newNode->getBase()->setTintColor(node->getBase()->getTintColor());
	undoStack->push(new VuoCommandChangeNode(node, newNode, this));
}

/**
 * Specializes the provided @c port by replacing all networked ports of its generic type
 * with ports of type @c specializedTypeName, meanwhile pushing the action onto the undo stack.
 * Coalesces all port specializations into a single Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after specialization.
 */
VuoRendererNode * VuoEditorWindow::specializePortNetwork(VuoRendererPort *port, string specializedTypeName)
{
	return specializePortNetwork(port, specializedTypeName, true);
}

/**
 * Specializes the provided @c port by replacing all networked ports of its generic type
 * with ports of type @c specializedTypeName, meanwhile pushing the action onto the undo stack.
 * If @c encapsulateInMacro is true, coalesces all port specializations into a single
 * Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after specialization.
 */
VuoRendererNode * VuoEditorWindow::specializePortNetwork(VuoRendererPort *port, string specializedTypeName, bool encapsulateInMacro)
{
	string commandText = "Port Specialization";

	if (encapsulateInMacro)
		undoStack->beginMacro(tr(commandText.c_str()));

	VuoRendererNode *originalNode = port->getUnderlyingParentNode();
	set<VuoPort *> networkedGenericPorts = composition->getBase()->getCompiler()->getCorrelatedGenericPorts(originalNode->getBase(), port->getBase(), false);

	VuoRendererNode *newNode = NULL;
	try
	{
		// Specialize the parent node of the target port.
		string innermostSpecializedTypeName = (VuoType::isListTypeName(port->getDataType()->getModuleKey())? VuoType::extractInnermostTypeName(specializedTypeName) : specializedTypeName);
		newNode = specializeSinglePort(port, innermostSpecializedTypeName);
		if (newNode)
		{

			map<VuoRendererNode *, VuoRendererNode *> nodesToSpecialize;
			nodesToSpecialize[originalNode] = newNode;

			// Specialize the parent node of each port in the target port's connected generic network.
			foreach (VuoPort *networkedPort, networkedGenericPorts)
			{
				// If we have specialized this port already, there is no need to do so again.
				VuoRendererNode *originalNetworkedNode = networkedPort->getRenderer()->getUnderlyingParentNode();
				VuoRendererNode *mostRecentVersionOfNetworkedNode = originalNetworkedNode;
				VuoPort *mostRecentVersionOfNetworkedPort = networkedPort;
				map<VuoRendererNode *, VuoRendererNode *>::iterator i = nodesToSpecialize.find(originalNetworkedNode);
				if (i != nodesToSpecialize.end())
				{
					VuoRendererNode *specializedNode = i->second;
					if (!specializedNode)
					{
						VuoCompilerIssue issue(VuoCompilerIssue::Error, "specializing node", "",
											   tr("Couldn't specialize node '%1' to type '%2'.")
												   .arg(QString::fromStdString(originalNetworkedNode->getBase()->getTitle()),
														QString::fromStdString(specializedTypeName)).toStdString(),
											   tr("The node might have failed to compile.  Check the console log for details.").toStdString());
						throw VuoCompilerException(issue);
					}
					VuoPort *networkedPortInSpecializedNode = (networkedPort->getRenderer()->getInput()? specializedNode->getBase()->getInputPortWithName(networkedPort->getClass()->getName()) :
																										 specializedNode->getBase()->getOutputPortWithName(networkedPort->getClass()->getName()));

					mostRecentVersionOfNetworkedNode = specializedNode;
					mostRecentVersionOfNetworkedPort = networkedPortInSpecializedNode;

					string originalTypeName = ((VuoCompilerPortClass *)(networkedPort->getClass()->getCompiler()))->getDataVuoType()->getModuleKey();
					string specializedTypeName = ((VuoCompilerPortClass *)(networkedPortInSpecializedNode->getClass()->getCompiler()))->getDataVuoType()->getModuleKey();
					if (specializedTypeName != originalTypeName)
						continue;
				}

				VuoRendererNode *newNetworkedNode = specializeSinglePort(mostRecentVersionOfNetworkedPort->getRenderer(), innermostSpecializedTypeName);
				nodesToSpecialize[originalNetworkedNode] = newNetworkedNode;
			}

			bool preserveDanglingCables = true;
			undoStack->push(new VuoCommandReplaceNode(nodesToSpecialize, this, commandText, preserveDanglingCables));
		}
	}
	catch (VuoCompilerException &e)
	{
		VuoErrorDialog::show(this, tr("Can't set data type"), e.what());
	}

	if (encapsulateInMacro)
		undoStack->endMacro();

	return newNode;
}

/**
 * Specializes the provided generic @c port by assigning it new type @c specializedTypeName.
 *
 * Returns a pointer to the new parent node of the given @c port after specialization.
 */
VuoRendererNode * VuoEditorWindow::specializeSinglePort(VuoRendererPort *port, string specializedTypeName)
{
	VuoRendererNode *node = port->getUnderlyingParentNode();
	string originalTypeName = ((VuoCompilerPortClass *)(port->getBase()->getClass()->getCompiler()))->getDataVuoType()->getModuleKey();
	string innermostOriginalTypeName = VuoType::extractInnermostTypeName(originalTypeName);

	VuoCompilerSpecializedNodeClass *specializableNodeClass = dynamic_cast<VuoCompilerSpecializedNodeClass *>(node->getBase()->getNodeClass()->getCompiler());
	if (!specializableNodeClass)
		return NULL;

	string newNodeClassName = specializableNodeClass->createSpecializedNodeClassNameWithReplacement(innermostOriginalTypeName, specializedTypeName);
	VuoCompilerNodeClass *newNodeClass = compiler->getNodeClass(newNodeClassName);

	if (!newNodeClass)
		return NULL;

	VuoNode *newNode = newNodeClass->newNode(node->getBase());
	return composition->createRendererNode(newNode);
}

/**
 * Reverts the provided @c port and all ports within its connected network to
 * their original generic origins, meanwhile pushing the action onto the Undo stack.
 * Coalesces all port specializations into a single Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after unspecialization.
 */
VuoRendererNode * VuoEditorWindow::unspecializePortNetwork(VuoRendererPort *port)
{
	return unspecializePortNetwork(port, true);
}

/**
 * Reverts the provided @c port and all ports within its connected network to
 * their original generic origins, meanwhile pushing the action onto the Undo stack.
 * If @c encapsulateInMacro is true, coalesces all port specializations into a single
 * Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after unspecialization.
 */
VuoRendererNode * VuoEditorWindow::unspecializePortNetwork(VuoRendererPort *port, bool encapsulateInMacro)
{
	QString commandText = tr("Port Specialization");
	bool preserveDanglingCables = true;
	bool resetConstantValues = false;

	if (encapsulateInMacro)
		undoStack->beginMacro(commandText);

	VuoRendererNode *originalParentNode = port->getUnderlyingParentNode();
	VuoRendererNode *unspecializedParentNode = NULL;

	// Retrieve the set of networked nodes to be reverted and resulting cables to be disconnected.
	map<VuoNode *, string> nodesToReplace;
	set<VuoCable *> cablesToDelete;
	composition->createReplacementsToUnspecializePort(port->getBase(), false, nodesToReplace, cablesToDelete);

	// Disconnect the necessary cables.
	QList<QGraphicsItem *> rendererCablesToDelete;
	foreach (VuoCable *cableToDelete, cablesToDelete)
	{
		// Uncollapse any attached typecasts so that they are not automatically deleted with their incoming cables.
		if (cableToDelete->getToNode() && cableToDelete->getToNode()->hasRenderer())
			composition->uncollapseTypecastNode(cableToDelete->getToNode()->getRenderer());

		rendererCablesToDelete.append(cableToDelete->getRenderer());
	}

	undoStack->push(new VuoCommandRemove(rendererCablesToDelete, this, inputEditorManager, commandText.toStdString()));

	// Call createReplacementsToUnspecializePort(...) a second time to retrieve an up-to-date set of nodes
	// to replace, since nodes may have been added (in the case of collapsed drawers) or removed
	// (in the case of collapsed typecasts) as a result of the cable deletions just performed.
	nodesToReplace.clear();
	composition->createReplacementsToUnspecializePort(port->getBase(), true, nodesToReplace, cablesToDelete);

	map<VuoRendererNode *, VuoRendererNode *> nodesToUnspecialize;

	// Revert the parent node of each port in the target port's connected generic network.
	for (map<VuoNode *, string>::iterator i = nodesToReplace.begin(); i != nodesToReplace.end(); ++i)
	{
		VuoNode *nodeToRevert = i->first;
		string revertedNodeClass = i->second;

		VuoRendererNode *revertedNode = unspecializeSingleNode(nodeToRevert->getRenderer(), revertedNodeClass);
		nodesToUnspecialize[nodeToRevert->getRenderer()] = revertedNode;

		if (nodeToRevert->getRenderer() == originalParentNode)
			unspecializedParentNode = revertedNode;
	}

	undoStack->push(new VuoCommandReplaceNode(nodesToUnspecialize, this, commandText.toStdString(), preserveDanglingCables, resetConstantValues));

	if (encapsulateInMacro)
		undoStack->endMacro();

	return unspecializedParentNode;
}

/**
 * Re-specializes the provided @c port by replacing all networked ports of its re-specialized type
 * with ports of type @c specializedTypeName.
 *
 * Coalesces all port re-specializations into a single Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after re-specialization.
 */
VuoRendererNode * VuoEditorWindow::respecializePortNetwork(VuoRendererPort *port, string specializedTypeName)
{
	return respecializePortNetwork(port, specializedTypeName, true);
}

/**
 * Re-specializes the provided @c port by replacing all networked ports of its re-specialized type
 * with ports of type @c specializedTypeName.
 *
 * If @c encapsulateInMacro is true, coalesces all port re-specializations into a single
 * Undo stack macro.
 *
 * Returns a pointer to the new parent node of the given @c port after re-specialization.
 */
VuoRendererNode * VuoEditorWindow::respecializePortNetwork(VuoRendererPort *port, string specializedTypeName, bool encapsulateInMacro)
{
	// If the port already has the requested type, no action is required.
	if (port->getDataType()->getModuleKey() == specializedTypeName)
		return port->getUnderlyingParentNode();

	string commandText = "Port Re-specialization";
	VuoRendererNode *specializedParentNode = NULL;

	if (encapsulateInMacro)
		undoStack->beginMacro(tr(commandText.c_str()));

	VuoRendererNode *revertedNode = unspecializePortNetwork(port, false);
	if (revertedNode)
	{
		VuoPort *revertedPort = (port->getInput()? revertedNode->getBase()->getInputPortWithName(port->getBase()->getClass()->getName()) :
														  revertedNode->getBase()->getOutputPortWithName(port->getBase()->getClass()->getName()));
		if (revertedPort)
			specializedParentNode = specializePortNetwork(revertedPort->getRenderer(), specializedTypeName, false);
	}

	if (encapsulateInMacro)
		undoStack->endMacro();

	return specializedParentNode;
}

/**
 * Reverts the provided specialized @c node to its generic origins.
 *
 * Returns a pointer to the reverted node.
 */
VuoRendererNode * VuoEditorWindow::unspecializeSingleNode(VuoRendererNode *node, string revertedNodeClassName)
{
	VuoCompilerNodeClass *revertedNodeClass = compiler->getNodeClass(revertedNodeClassName);

	if (!revertedNodeClass)
		return NULL;

	VuoNode *revertedNode = revertedNodeClass->newNode(node->getBase());
	return composition->createRendererNode(revertedNode);
}

/**
 * Sets the tint color of the selected nodes and comments to @c tintColor,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::tintSelectedItems(VuoNode::TintColor tintColor)
{
	undoStack->beginMacro(tr("Tint"));

	QList<QGraphicsItem *> selectedCompositionComponents = composition->selectedItems();
	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin(); i != selectedCompositionComponents.end(); ++i)
	{
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);
		VuoRendererInputAttachment *attachment = dynamic_cast<VuoRendererInputAttachment *>(*i);
		VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(*i);
		if ((rn || rcomment) && (!attachment || (attachment->getRenderedHostNode() &&
								   attachment->getRenderedHostNode()->hasRenderer() &&
								   selectedCompositionComponents.contains(attachment->getRenderedHostNode()->getRenderer()))))
			undoStack->push(new VuoCommandSetItemTint(*i, tintColor, this));
	}

	undoStack->endMacro();
}

/**
 * Makes currently selected internal cables wireless.
 */
void VuoEditorWindow::hideSelectedInternalCables()
{
	hideCables(composition->getSelectedCables(false));
}

/**
 * Makes the provided set of cables wireless (hidden).
 */
void VuoEditorWindow::hideCables(set<VuoRendererCable *> cables)
{
	undoStack->beginMacro(tr("Hide"));

	foreach (VuoRendererCable *cable, cables)
		undoStack->push(new VuoCommandSetCableHidden(cable, true, this));

	undoStack->endMacro();
	updateUI();
}

/**
 * Makes the provided set of cables wired (unhidden).
 */
void VuoEditorWindow::unhideCables(set<VuoRendererCable *> cables)
{
	bool publishedCableToUnhide = false;
	undoStack->beginMacro(tr("Unhide"));

	foreach (VuoRendererCable *cable, cables)
	{
		if (cable->getBase()->isPublished())
			publishedCableToUnhide = true;
		else
			undoStack->push(new VuoCommandSetCableHidden(cable, false, this));
	}

	if ((inputPortSidebar->isHidden() || outputPortSidebar->isHidden()) && publishedCableToUnhide)
		on_showPublishedPorts_triggered();

	undoStack->endMacro();
	updateUI();
}

/**
 * Creates a new external published port, not connected to any internal ports.
 */
void VuoEditorWindow::createIsolatedExternalPublishedPort(string typeName, bool isInput)
{
	VuoType *type = (typeName.empty()? NULL : (compiler->getType(typeName)? compiler->getType(typeName)->getBase() : NULL));
	if (!typeName.empty() && !type)
		return;

	undoStack->beginMacro(tr("Add Published Port"));

	string portName = composition->getDefaultPublishedPortNameForType(type);
	VuoPublishedPort *publishedPort = static_cast<VuoPublishedPort *>(VuoCompilerPublishedPort::newPort(composition->getUniquePublishedPortName(portName), type)->getBase());
	VuoRendererPublishedPort *rpp = composition->createRendererForPublishedPortInComposition(publishedPort, isInput);
	undoStack->push(new VuoCommandAddPublishedPort(rpp, this));
	showPublishedPortNameEditor(rpp, true);

	undoStack->endMacro();
}

/**
 * Displays a name editor for the provided published port and applies the name change.
 */
void VuoEditorWindow::showPublishedPortNameEditor(VuoRendererPublishedPort *port, bool useUndoStack)
{
	string originalName = port->getBase()->getClass()->getName();
	bool isPublishedOutput = port->getInput();
	(isPublishedOutput? outputPortSidebar : inputPortSidebar)->updatePortList();
	string newName = (isPublishedOutput? outputPortSidebar : inputPortSidebar)->showPublishedPortNameEditor(port);

	if (originalName != newName)
		changePublishedPortName(port, newName, useUndoStack);
}

/**
 * Publishes the provided @c internalPort in association with the provided @c externalPort,
 * meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::internalExternalPortPairPublished(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoPort *portToSpecialize, string specializedTypeName, string typecastToInsert, bool useUndoStackMacro)
{
	internalPortPublished(internalPort, forceEventOnlyPublication, externalPort->getClass()->getName(), true, portToSpecialize, specializedTypeName, typecastToInsert, useUndoStackMacro);
}

/**
 * Publishes the provided @c port, meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::internalPortPublishedViaDropBox(VuoPort *port, bool forceEventOnlyPublication, bool useUndoStackMacro)
{
	internalPortPublished(port, forceEventOnlyPublication, "", false, NULL, "", "", useUndoStackMacro);
}

/**
 * Publishes the provided @c port, meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::internalPortPublished(VuoPort *port, bool forceEventOnlyPublication, string name, bool merge, VuoPort *portToSpecialize, string specializedTypeName, string typecastToInsert, bool useUndoStackMacro)
{
	// Display the published port sidebars whenever a port is published.
	if (inputPortSidebar->isHidden() || outputPortSidebar->isHidden())
		on_showPublishedPorts_triggered();

	if (name.empty())
		name = composition->generateSpecialPublishedNameForPort(port);

	// If the provided port is already published under the requested name,
	// don't attempt to re-publish it.
	// @todo: Once re-connection of published cables is allowed, account for possibility that ports were already
	// connected, but with a cable of a different type. https://b33p.net/kosada/node/5142
	VuoRendererPort *targetPort = port->getRenderer();
	vector<VuoRendererPublishedPort *> preexistingPublishedPorts = targetPort->getPublishedPorts();
	foreach (VuoRendererPublishedPort * publishedPort, preexistingPublishedPorts)
	{
		if (publishedPort->getBase()->getClass()->getName() == name)
			return;
	}

	if (useUndoStackMacro)
		undoStack->beginMacro(tr("Publish Port"));

	// Specialize the port and its network, if applicable.
	if (portToSpecialize && portToSpecialize->hasRenderer())
	{
		bool currentlyGeneric = dynamic_cast<VuoGenericType *>(portToSpecialize->getRenderer()->getDataType());

		VuoRendererNode *specializedNode = NULL;
		if (currentlyGeneric)
			specializedNode = specializePortNetwork(portToSpecialize->getRenderer(), specializedTypeName, false);
		else
			specializedNode = respecializePortNetwork(portToSpecialize->getRenderer(), specializedTypeName, false);

		if (specializedNode && (portToSpecialize == targetPort->getBase()))
			targetPort = (targetPort->getInput()? specializedNode->getBase()->getInputPortWithName(targetPort->getBase()->getClass()->getName())->getRenderer() :
												  specializedNode->getBase()->getOutputPortWithName(targetPort->getBase()->getClass()->getName())->getRenderer());
	}

	// Determine whether we need to uncollapse any attached typecasts, delete any attached
	// "Make List" nodes, or disconnect any cables in order to publish this port.
	VuoRendererCable *displacedCable = NULL;

	if (targetPort->getInput())
	{
		bool publishedCableExpectedToHaveData = false;
		bool publishedPortExpectedToBeMerged = false;

		VuoType *internalPortDataType = (dynamic_cast<VuoRendererPublishedPort *>(targetPort)?
											 static_cast<VuoCompilerPortClass *>(targetPort->getBase()->getClass()->getCompiler())->getDataVuoType() :
											 targetPort->getDataType());
		if (merge)
		{
			VuoPublishedPort *publishedPortWithTargetName = (targetPort->getInput()?
								 composition->getBase()->getPublishedInputPortWithName(name) :
								 composition->getBase()->getPublishedOutputPortWithName(name));

			if (publishedPortWithTargetName &&
					dynamic_cast<VuoRendererPublishedPort *>(publishedPortWithTargetName->getRenderer())->canAccommodateInternalPort(targetPort, forceEventOnlyPublication))
			{
				publishedPortExpectedToBeMerged = true;
				VuoType *type = static_cast<VuoCompilerPortClass *>(publishedPortWithTargetName->getClass()->getCompiler())->getDataVuoType();
				publishedCableExpectedToHaveData = (!forceEventOnlyPublication && internalPortDataType && type);
			}
		}

		if (!publishedPortExpectedToBeMerged)
		{
			publishedCableExpectedToHaveData = (!forceEventOnlyPublication && internalPortDataType);
		}

		// If input port has a connected collapsed typecast, uncollapse it.
		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>(targetPort);
		if (typecastPort)
		{
			targetPort = typecastPort->getReplacedPort();
			composition->uncollapseTypecastNode(typecastPort);
		}

		// If the published input cable will have data...
		if (publishedCableExpectedToHaveData)
		{
			// If the internal input port has a connected drawer, delete it.
			VuoRendererInputDrawer *attachedDrawer = targetPort->getAttachedInputDrawer();
			if (attachedDrawer)
			{
				QList<QGraphicsItem *> componentsToRemove;
				componentsToRemove.append(attachedDrawer);
				bool disableAutomaticAttachmentInsertion = true;
				undoStack->push(new VuoCommandRemove(componentsToRemove, this, inputEditorManager, "", disableAutomaticAttachmentInsertion));
			}

			// If the internal input port has a connected data cable, displace it.
			else
			{
				vector<VuoCable *> connectedCables = targetPort->getBase()->getConnectedCables(true);
				for (vector<VuoCable *>::iterator cable = connectedCables.begin(); (! displacedCable) && (cable != connectedCables.end()); ++cable)
					if ((*cable)->getRenderer()->effectivelyCarriesData())
						displacedCable = (*cable)->getRenderer();

			}
		}
	}

	VuoRendererPort *adjustedTargetPort = targetPort;
	QList<QGraphicsItem *> typecastRelatedComponentsToAdd = QList<QGraphicsItem *>();
	if (! typecastToInsert.empty())
	{
		// Insert the necessary typecast node.
		VuoRendererNode *typecast = composition->createNode(typecastToInsert.c_str(), "",
					targetPort->scenePos().x()+100,
					targetPort->scenePos().y()+100);

		typecastRelatedComponentsToAdd.append(typecast);

		VuoPort *typecastInPort = typecast->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
		VuoPort *typecastOutPort = typecast->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

		// Insert an extra cable leading to or from the typecast as appropriate.
		VuoRendererCable *newCableConnectingTypecast;

		// Case: Publishing an internal input port
		if (targetPort->getInput())
		{
			// Prepare to add a cable connecting the output port of the newly inserted
			// typecast to the originally intended input port.
			VuoCompilerNode *fromNode = typecast->getBase()->getCompiler();
			VuoCompilerPort *fromPort = (VuoCompilerPort *)typecastOutPort->getCompiler();
			VuoCompilerNode *toNode = targetPort->getUnderlyingParentNode()->getBase()->getCompiler();
			VuoCompilerPort *toPort = (VuoCompilerPort *)targetPort->getBase()->getCompiler();

			// Prepare to re-route the dragged cable's loose end to connect to the typecast's input port.
			adjustedTargetPort = typecastInPort->getRenderer();
			newCableConnectingTypecast = new VuoRendererCable((new VuoCompilerCable(fromNode, fromPort, toNode, toPort))->getBase());
		}

		// Case: Publishing an internal output port
		else
		{
			// Prepare to add a cable connecting the originally intended output port
			// to the input port of the newly inserted typecast.
			VuoCompilerNode *fromNode = targetPort->getUnderlyingParentNode()->getBase()->getCompiler();
			VuoCompilerPort *fromPort = (VuoCompilerPort *)targetPort->getBase()->getCompiler();
			VuoCompilerNode *toNode = typecast->getBase()->getCompiler();
			VuoCompilerPort *toPort = (VuoCompilerPort *)typecastInPort->getCompiler();

			// Prepare to re-route the dragged cable's loose end to connect to the typecast's output port.
			adjustedTargetPort = typecastOutPort->getRenderer();

			newCableConnectingTypecast = new VuoRendererCable((new VuoCompilerCable(fromNode, fromPort, toNode, toPort))->getBase());
		}

		typecastRelatedComponentsToAdd.append(newCableConnectingTypecast);

		bool disableAutomaticAttachmentInsertion = true;
		undoStack->push(new VuoCommandAdd(typecastRelatedComponentsToAdd, this, "", disableAutomaticAttachmentInsertion));
	}

	undoStack->push(new VuoCommandPublishPort(adjustedTargetPort->getBase(), displacedCable, this, forceEventOnlyPublication, name, merge));

	// @todo: If re-connecting a data+event cable whose previous 'To' port had a collapsed typecast attached, delete that typecast.
	// Re-connection of published cables is not currently possible (https://b33p.net/kosada/node/5142), so we can skip this for now.

	// If the target port is generic and, as a result of this connection, has only a single
	// compatible specialized type, specialize its network.
	VuoGenericType *genericType = dynamic_cast<VuoGenericType *>((static_cast<VuoCompilerPort *>(targetPort->getBase()->getCompiler()))->getDataVuoType());
	if (genericType)
	{
		VuoGenericType::Compatibility compatibility;
		vector<string> compatibleSpecializedTypes = genericType->getCompatibleSpecializedTypes(compatibility);
		if (compatibleSpecializedTypes.size() == 1)
		{
			string loneCompatibleSpecializedType = *compatibleSpecializedTypes.begin();
			specializePortNetwork(targetPort, loneCompatibleSpecializedType, false);
		}
	}

	if (useUndoStackMacro)
		undoStack->endMacro();
}

/**
 * Unpublishes the provided externally visible published @c port, meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::externalPortUnpublished(VuoRendererPublishedPort *port)
{
	// Display the published port sidebars whenever a port is unpublished.
	if (inputPortSidebar->isHidden() || outputPortSidebar->isHidden())
		on_showPublishedPorts_triggered();

	undoStack->beginMacro(tr("Delete"));

	// Remove any connected data+event cables using a separate VuoCommandRemove so that typecast
	// deletion, list insertion, etc. are handled correctly.
	bool isPublishedInput = !port->getInput();
	if (isPublishedInput)
	{
		vector<VuoCable *> publishedInputCables = port->getBase()->getConnectedCables(true);
		QList<QGraphicsItem *> removedDataCables;
		foreach (VuoCable *cable, publishedInputCables)
		{
			if (cable->getRenderer()->effectivelyCarriesData())
				removedDataCables.append(cable->getRenderer());
		}

		if (!removedDataCables.empty())
			undoStack->push(new VuoCommandRemove(removedDataCables, this, inputEditorManager, "Delete"));
	}

	undoStack->push(new VuoCommandUnpublishPort(dynamic_cast<VuoPublishedPort *>(port->getBase()), this));
	undoStack->endMacro();
}

/**
 * Unpublishes the provided internally visible published @c port, meanwhile pushing the action onto the undo stack.
 */
void VuoEditorWindow::internalPortUnpublished(VuoPort *port)
{
	// Display the published port sidebars whenever a port is unpublished.
	if (inputPortSidebar->isHidden() || outputPortSidebar->isHidden())
		on_showPublishedPorts_triggered();

	undoStack->beginMacro(tr("Delete"));

	// If unpublishing a port with a list input, insert a new attached "Make List" node.
	// @todo: Eventually, account for the fact that the published cable might be
	// event-only even though the port is not.
	QList<QGraphicsItem *> makeListComponents = createAnyNecessaryMakeListComponents(port);
	if (! makeListComponents.empty())
		undoStack->push(new VuoCommandAdd(makeListComponents, this, ""));

	undoStack->push(new VuoCommandUnpublishPort(port, this));
	undoStack->endMacro();
}

/**
 * Renames, removes, and adds the provided sets of published ports, in that order, pushing each
 * operation onto the Undo stack.
 */
void VuoEditorWindow::makeProtocolPortChanges(map<VuoPublishedPort *, string> publishedPortsToRename,
											  set<VuoPublishedPort *> publishedPortsToRemove,
											  vector<VuoPublishedPort *> publishedPortsToAdd,
											  bool beginUndoStackMacro,
											  bool endUndoStackMacro)
{
	if (beginUndoStackMacro)
		undoStack->beginMacro(tr("Protocol Port Modification"));

	// Rename requested ports.
	for (map<VuoPublishedPort *, string>::iterator i = publishedPortsToRename.begin(); i != publishedPortsToRename.end(); ++i)
	{
		VuoPublishedPort *port = i->first;
		string name = i->second;
		undoStack->push(new VuoCommandSetPublishedPortName(dynamic_cast<VuoRendererPublishedPort *>(port->getRenderer()), name, this));
	}

	// Remove the requested protocol ports.
	foreach (VuoPublishedPort *port, publishedPortsToRemove)
		undoStack->push(new VuoCommandRemoveProtocolPort(dynamic_cast<VuoRendererPublishedPort *>(port->getRenderer()), this));

	// Add the requested protocol ports.
	foreach (VuoPublishedPort *port, publishedPortsToAdd)
		undoStack->push(new VuoCommandAddPublishedPort(dynamic_cast<VuoRendererPublishedPort *>(port->getRenderer()), this));

	if (endUndoStackMacro)
		undoStack->endMacro();
}

/**
 * Ends any 'Undo' stack macro known to be in progress.
 */
void VuoEditorWindow::resetUndoStackMacros()
{
	if (itemDragMacroInProgress)
	{
		itemDragMacroInProgress = false;

		set<VuoRendererNode *> draggedNodeSet;
		set<VuoRendererComment *> draggedCommentSet;

		foreach (QGraphicsItem *item, itemsBeingDragged)
		{
			if (dynamic_cast<VuoRendererNode *>(item))
				draggedNodeSet.insert(dynamic_cast<VuoRendererNode *>(item));
			else if (dynamic_cast<VuoRendererComment *>(item))
				draggedCommentSet.insert(dynamic_cast<VuoRendererComment *>(item));
		}

		undoStack->push(new VuoCommandMove(draggedNodeSet, draggedCommentSet, itemDragDx, itemDragDy, this, true));

		itemsBeingDragged.clear();

		itemDragDx = 0;
		itemDragDy = 0;
	}

	if (commentResizeMacroInProgress)
	{
		commentResizeMacroInProgress = false;

		if (commentBeingResized && ((commentResizeDx != 0) || (commentResizeDy != 0)))
			undoStack->push(new VuoCommandResizeComment(commentBeingResized, commentResizeDx, commentResizeDy, this));

		commentBeingResized = NULL;

		commentResizeDx = 0;
		commentResizeDy = 0;
	}

	if (duplicationMacroInProgress)
	{
		undoStack->endMacro();
		duplicationMacroInProgress = false;
	}
}

/**
 * Handles events for the editor window.
 */
bool VuoEditorWindow::event(QEvent *event)
{
	if (event->type() == QEvent::Show)
	{
		if (zoomOutToFitOnNextShowEvent)
		{
			zoomOutToFit();
			zoomOutToFitOnNextShowEvent = false;
		}

		// Use a queued connection to avoid mutual recursion between VuoEditorWindow::event() and VuoEditorComposition::updateFeedbackErrors().
		QMetaObject::invokeMethod(composition, "updateFeedbackErrors", Qt::QueuedConnection);
	}

	// Workaround to force a cursor update when the cursor re-enters the window during a canvas drag.
	// See https://b33p.net/kosada/node/7718#comment-27792 .
	if (canvasDragInProgress && (event->type() == QEvent::MouseMove))
		updateCursor();

	// If the window is activated, update its UI elements (e.g., to correctly reflect
	// that it is the active document within the "Window" menu and OS X dock context menu).
	else if (event->type() == QEvent::WindowActivate)
	{
		mostRecentActiveEditorWindow = this;
		updateUI();
		emit windowActivated();
	}

	else if (event->type() == QEvent::WindowDeactivate)
		emit windowDeactivated();

	if (event->type() == QEvent::WindowStateChange)
	{
	  if (isMinimized())
	  {
		  composition->disableErrorPopovers();
		  composition->emitCompositionOnTop(false);
	  }
	  else
		  composition->updateFeedbackErrors();
	}

	else if (event->type() == QEvent::MouseButtonPress)
	{
		// Forward published cable drags initiated from the sidebar border onto the canvas.
		bool leftMouseButtonPressed = (((QMouseEvent *)(event))->button() == Qt::LeftButton);
		if (leftMouseButtonPressed)
		{
			VuoRendererPublishedPort *publishedPortNearCursor = NULL;
			if (!inputPortSidebar->isHidden())
				publishedPortNearCursor = inputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange, true);

			if (!outputPortSidebar->isHidden() && !publishedPortNearCursor)
				publishedPortNearCursor = outputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange, true);

			if (publishedPortNearCursor)
			{
				QGraphicsSceneMouseEvent mouseEvent;
				mouseEvent.setButtons(static_cast<QMouseEvent *>(event)->buttons());
				mouseEvent.setScenePos(ui->graphicsView->mapToScene(ui->graphicsView->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos())));
				composition->leftMousePressEventAtNearbyItem(static_cast<QGraphicsItem *>(publishedPortNearCursor), &mouseEvent);

				forwardingEventsToCanvas = true;
				event->accept();
				return true;
			}
		}

		// If it's possible that the user is attempting to re-size a docked widget,
		// make sure that we have re-enabled re-sizing for the widgets that
		// share the left docking area, having possibly disabled this functionality
		// within setPublishedPortSidebarVisibility(...) and/or conformToGlobalNodeLibraryVisibility(...).
		restoreDefaultLeftDockedWidgetWidths();
	}

	else if (event->type() == QEvent::MouseMove)
	{
		// Forward published cable drags initiated from the sidebar border onto the canvas.
		if (forwardingEventsToCanvas)
		{
			QMouseEvent mouseEvent(QEvent::MouseMove,
								   ui->graphicsView->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos()),
								   static_cast<QMouseEvent *>(event)->screenPos(),
								   static_cast<QMouseEvent *>(event)->button(),
								   static_cast<QMouseEvent *>(event)->buttons(),
								   static_cast<QMouseEvent *>(event)->modifiers());

			QApplication::sendEvent(ui->graphicsView->viewport(), &mouseEvent);

			event->accept();
			return true;
		}
	}

	else if (event->type() == QEvent::MouseButtonRelease)
	{
		// Forward published cable drags initiated from the sidebar border onto the canvas.
		if (forwardingEventsToCanvas)
		{
			QMouseEvent mouseEvent(QEvent::MouseButtonRelease,
								   ui->graphicsView->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos()),
								   static_cast<QMouseEvent *>(event)->screenPos(),
								   static_cast<QMouseEvent *>(event)->button(),
								   static_cast<QMouseEvent *>(event)->buttons(),
								   static_cast<QMouseEvent *>(event)->modifiers());

			QApplication::sendEvent(ui->graphicsView->viewport(), &mouseEvent);

			forwardingEventsToCanvas = false;
			event->accept();
			return true;
		}
	}

	return QMainWindow::event(event);
}

/**
 * Filters events on watched objects.
 */
bool VuoEditorWindow::eventFilter(QObject *object, QEvent *event)
{
	// If it's been a while since we received a scroll event, turn interactivity back on.
	// Workaround for Qt sometimes not sending us a Qt::ScrollEnd event.
	const double scrollTimeoutSeconds = 0.5;
	if (scrollInProgress && VuoLogGetElapsedTime() - timeOfLastScroll > scrollTimeoutSeconds)
	{
		VDebugLog("Turning canvas interactivity back on even though we didn't receive a Qt::ScrollEnd event, since there hasn't been a scroll event in %g seconds.", scrollTimeoutSeconds);
		ui->graphicsView->setInteractive(true);
		scrollInProgress = false;
	}

	// Prevent the composition from responding to certain types of events during canvas grabs/drags.
	// The canvas drag itself must be handled in response to intercepted QEvent::MouseMove
	// events rather than here in response to intercepted QEvent::GraphicsSceneMouseMove
	// events; otherwise the canvas doesn't track correctly with the cursor.
	if ((canvasDragEnabled || canvasDragInProgress) && (object == composition) && (
				event->type() == QEvent::GraphicsSceneMouseMove ||
				event->type() == QEvent::GraphicsSceneMousePress ||
				event->type() == QEvent::GraphicsSceneMouseDoubleClick ||
				event->type() == QEvent::GraphicsSceneContextMenu ||
				event->type() == QEvent::KeyPress))
		return true;

	// If an input editor is displayed at the time that the canvas is clicked, the click should be consumed
	// with the closing of the input editor rather than having additional effects on the canvas.
	if ((object == composition) &&
			(event->type() == QEvent::GraphicsSceneMousePress) &&
			inputEditorSession)
	{
		consumeNextMouseReleaseToCanvas = true;
		return true;
	}
	if ((object == composition) &&
			(event->type() == QEvent::GraphicsSceneMouseRelease) &&
			consumeNextMouseReleaseToCanvas)
	{
		consumeNextMouseReleaseToCanvas = false;
		return true;
	}

	if (event->type() == QEvent::Wheel
	 && ui->graphicsView->rubberBandRect().isNull()
	 && !ui->graphicsView->pinchZoomInProgress())
	{
		// Disable interaction while scrolling, to improve the framerate.
		QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
		if (wheelEvent->phase() == Qt::ScrollBegin)
		{
			ui->graphicsView->setInteractive(false);
			scrollInProgress = true;
		}
		else if (wheelEvent->phase() == Qt::ScrollEnd)
		{
			ui->graphicsView->setInteractive(true);
			scrollInProgress = false;
		}

		// Skip rendering if we aren't keeping up.
		double lag = VuoEditorCocoa_systemUptime() - wheelEvent->timestamp()/1000.;
		const double lagLimit = .1;
		if (lag > lagLimit)
			return true;

		// Remove Shift modifier from mouse-wheel events to prevent it from triggering page-step mode.
		QInputEvent *filteredInputEvent = (QInputEvent *)(event);
		Qt::KeyboardModifiers modifiersOtherThanShift = (filteredInputEvent->modifiers() & ~Qt::ShiftModifier);
		filteredInputEvent->setModifiers(modifiersOtherThanShift);
		object->removeEventFilter(this);
		QApplication::sendEvent(object, filteredInputEvent);
		object->installEventFilter(this);

		timeOfLastScroll = VuoLogGetElapsedTime();

		return true;
	}

	// Disable any non-detached port or node popover upon mouse press.
	else if (event->type() == QEvent::MouseButtonPress)
	{
		bool leftMouseButtonPressed = (((QMouseEvent *)(event))->button() == Qt::LeftButton);
		if (leftMouseButtonPressed)
			this->lastLeftMousePressHadOptionModifier = VuoEditorUtilities::optionKeyPressedForEvent(event);

		composition->disableNondetachedPortPopovers(NULL, true);

		// Initiate canvas drag if previously enabled by press-and-hold of the spacebar.
		if (canvasDragEnabled && leftMouseButtonPressed && (object == ui->graphicsView->viewport()))
		{
			initiateCanvasDrag();
			return true;
		}

		// If a sidebar published port is near the cursor, send the mouse press directly to that published port
		// rather than handing hover detection over to the composition's findNearbyComponent(...) algorithm.
		else if (leftMouseButtonPressed)
		{
			VuoRendererPublishedPort *publishedPortNearCursor = NULL;
			if (!inputPortSidebar->isHidden())
				publishedPortNearCursor = inputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange);

			if (!outputPortSidebar->isHidden() && !publishedPortNearCursor)
				publishedPortNearCursor = outputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange);

			if (publishedPortNearCursor)
			{
				QGraphicsSceneMouseEvent mouseEvent;
				mouseEvent.setButtons(static_cast<QMouseEvent *>(event)->buttons());
				mouseEvent.setScenePos(ui->graphicsView->mapToScene(ui->graphicsView->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos())));
				composition->leftMousePressEventAtNearbyItem(static_cast<QGraphicsItem *>(publishedPortNearCursor), &mouseEvent);
				return true;
			}
		}
	}

	// Continue any canvas drag currently in progress.
	else if ((event->type() == QEvent::MouseMove) &&
			 canvasDragInProgress &&
			 (object == ui->graphicsView->viewport()))
	{
		mouseMoveEvent((QMouseEvent *)event);
		return true;
	}

	// Conclude canvas drag.
	else if ((event->type() == QEvent::MouseButtonRelease) &&
			 canvasDragInProgress &&
			 (((QMouseEvent *)(event))->button() == Qt::LeftButton) &&
			 (object == ui->graphicsView->viewport()))
	{
		concludeCanvasDrag();
		return true;
	}

	// If a cable drag is in progress and a published port sidebar is directly under the cursor,
	// let the sidebar, rather than the composition canvas, handle hover highlighting
	// and the conclusion of the cable drag.
	else if ((((event->type() == QEvent::MouseButtonRelease) &&
			  (((QMouseEvent *)(event))->button() == Qt::LeftButton)) ||
			  ((event->type() == QEvent::MouseMove) &&
			   (((QMouseEvent *)(event))->buttons() & Qt::LeftButton)))
			 &&
			 composition->getCableInProgress())
	{
		QPoint cursorPosition = ((QMouseEvent *)(event))->globalPos();

		QRect inputPortSidebarRect = inputPortSidebar->geometry();
		inputPortSidebarRect.moveTopLeft(inputPortSidebar->parentWidget()->mapToGlobal(inputPortSidebarRect.topLeft()));

		QRect outputPortSidebarRect = outputPortSidebar->geometry();
		outputPortSidebarRect.moveTopLeft(outputPortSidebar->parentWidget()->mapToGlobal(outputPortSidebarRect.topLeft()));

		VuoRendererPublishedPort *publishedInputPortNearCursor = NULL;
		if (!inputPortSidebar->isHidden())
			publishedInputPortNearCursor = inputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange, true);

		VuoRendererPublishedPort *publishedOutputPortNearCursor = NULL;
		if (!outputPortSidebar->isHidden())
			publishedOutputPortNearCursor = outputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange, true);

		// Case: drag with left mouse button pressed
		if ((event->type() == QEvent::MouseMove) &&
			 (((QMouseEvent *)(event))->buttons() & Qt::LeftButton))
		{
			bool dragOverInputPortSidebar = ((! inputPortSidebar->isHidden()) && (inputPortSidebarRect.contains(cursorPosition) || publishedInputPortNearCursor));
			bool dragOverOutputPortSidebar = ((! outputPortSidebar->isHidden()) && (outputPortSidebarRect.contains(cursorPosition) || publishedOutputPortNearCursor));

			if (dragOverInputPortSidebar || dragOverOutputPortSidebar)
			{
				if (!previousDragMoveWasOverSidebar)
					composition->clearHoverHighlighting();

				if (dragOverInputPortSidebar)
					inputPortSidebar->updateHoverHighlighting((QMouseEvent *)event, VuoEditorComposition::componentCollisionRange);
				else if (dragOverOutputPortSidebar)
					outputPortSidebar->updateHoverHighlighting((QMouseEvent *)event, VuoEditorComposition::componentCollisionRange);

				previousDragMoveWasOverSidebar = true;
			}

			else
			{
				if (previousDragMoveWasOverSidebar)
				{
					inputPortSidebar->clearHoverHighlighting();
					outputPortSidebar->clearHoverHighlighting();
					if (composition->getCableInProgress())
						composition->getCableInProgress()->getRenderer()->setFloatingEndpointAboveEventPort(false);
				}

				previousDragMoveWasOverSidebar = false;
			}
		}

		// Case: left mouse button release
		else if ((event->type() == QEvent::MouseButtonRelease) &&
			(((QMouseEvent *)(event))->button() == Qt::LeftButton))
		{
			VuoCable *cableInProgress = composition->getCableInProgress();

			// Case: Concluding a published cable drag at a sidebar published input port.
			if (cableInProgress && !inputPortSidebar->isHidden() && (inputPortSidebarRect.contains(cursorPosition) || publishedInputPortNearCursor))
				inputPortSidebar->concludePublishedCableDrag((QMouseEvent *)event, cableInProgress, composition->getCableInProgressWasNew());

			// Case: Concluding a published cable drag at a sidebar published output port.
			else if (cableInProgress && !outputPortSidebar->isHidden() && (outputPortSidebarRect.contains(cursorPosition) || publishedOutputPortNearCursor))
				outputPortSidebar->concludePublishedCableDrag((QMouseEvent *)event, cableInProgress, composition->getCableInProgressWasNew());
		}

		object->removeEventFilter(this);
		QApplication::sendEvent(object, event);
		object->installEventFilter(this);
		return true;
	}

	// Determine whether the cursor is near a published sidebar port, and hover-highlight that port,
	// before handing hover detection over to the composition's findNearbyComponent(...) algorithm.
	else if (event->type() == QEvent::MouseMove && !composition->getCableInProgress())
	{
		VuoRendererPublishedPort *publishedPortNearCursor = NULL;
		if (!inputPortSidebar->isHidden())
		{
			publishedPortNearCursor = inputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange);
			inputPortSidebar->updateHoverHighlighting((QMouseEvent *)event, VuoEditorComposition::componentCollisionRange);
		}

		if (!outputPortSidebar->isHidden() && !publishedPortNearCursor)
		{
			publishedPortNearCursor = outputPortSidebar->getPublishedPortUnderCursorForEvent(static_cast<QMouseEvent *>(event), VuoEditorComposition::componentCollisionRange);
			outputPortSidebar->updateHoverHighlighting((QMouseEvent *)event, VuoEditorComposition::componentCollisionRange);
		}

		if (publishedPortNearCursor)
		{
			if (!publishedPortNearCursorPreviously)
				composition->clearHoverHighlighting();
		}

		else
		{
			if (publishedPortNearCursorPreviously)
			{
				inputPortSidebar->clearHoverHighlighting();
				outputPortSidebar->clearHoverHighlighting();
			}

			object->removeEventFilter(this);
			QApplication::sendEvent(object, event);
			object->installEventFilter(this);
		}

		publishedPortNearCursorPreviously = publishedPortNearCursor;

		return true;
	}

	// Customize handling of keypress events.
	else if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = (QKeyEvent *)(event);

		// The Esc key closes non-detached popovers.
		if (keyEvent->key() == Qt::Key_Escape)
			composition->disableNondetachedPortPopovers();

		// Arrow keys may be used to move the viewport when no composition components are selected.
		if ((composition->selectedItems().isEmpty()) &&
			((keyEvent->key() == Qt::Key_Up) ||
			(keyEvent->key() == Qt::Key_Down) ||
			(keyEvent->key() == Qt::Key_Left) ||
			(keyEvent->key() == Qt::Key_Right)))
		{
			keyPressEvent(keyEvent);
		}

		// The canvas may be dragged with spacebar+drag.
		else if ((object == composition) && (keyEvent->key() == Qt::Key_Space))
		{
			keyPressEvent(keyEvent);
		}

		else
		{
			object->removeEventFilter(this);
			QApplication::sendEvent(object, event);
			object->installEventFilter(this);
		}
		return true;
	}

	// Suppress ContextMenu events sent to the composition.  We generate them ourselves
	// within VuoEditorComposition::mousePressEvent and VuoEditorGraphicsView::viewportEvent
	// to customize the selection behavior that accompanies the presentation of the context menu.
	else if (event->type() == QEvent::ContextMenu)
	{
		return true;
	}

	return QMainWindow::eventFilter(object, event);
}

/**
 * Returns true if a scroll gesture is currently being performed.
 */
bool VuoEditorWindow::isScrollInProgress()
{
	return scrollInProgress;
}

/**
 * Returns true if items are currently being dragged around the canvas.
 */
bool VuoEditorWindow::isItemDragInProgress()
{
	return itemDragMacroInProgress;
}

/**
 * Call this to indicate that a drag is in progress or has just completed.
 */
void VuoEditorWindow::updateLatestDragTime()
{
	latestDragTime = VuoLogGetElapsedTime();
}

/**
 * Returns the timestamp of the most recent drag event.
 */
double VuoEditorWindow::getLatestDragTime()
{
	return latestDragTime;
}

/**
 * Enables canvas-drag mode. In this mode, mouse drags will move the canvas.
 * Prepares the canvas to be dragged by dismissing any open popovers and
 * clearing any hover highlighting.
 */
void VuoEditorWindow::enableCanvasDrag()
{
	this->canvasDragEnabled = true;
	updateUI();

	composition->clearHoverHighlighting();
	composition->disableNondetachedPortPopovers();
}

/**
 * Disables canvas-drag mode. Mouse drags will no longer move the canvas.
 */
void VuoEditorWindow::disableCanvasDrag()
{
	this->canvasDragEnabled = false;
	updateUI();
}

/**
 * Initiates a drag of the canvas.
 */
void VuoEditorWindow::initiateCanvasDrag()
{
	this->canvasDragInProgress = true;
	QPoint currentCursorPos = QCursor::pos();
	this->lastCursorLocationDuringCanvasDrag = currentCursorPos;

	int xNegativeScrollPotential = ui->graphicsView->horizontalScrollBar()->value() - ui->graphicsView->horizontalScrollBar()->minimum();
	int yNegativeScrollPotential = ui->graphicsView->verticalScrollBar()->value() - ui->graphicsView->verticalScrollBar()->minimum();

	int xPositiveScrollPotential = ui->graphicsView->horizontalScrollBar()->maximum() - ui->graphicsView->horizontalScrollBar()->value();
	int yPositiveScrollPotential = ui->graphicsView->verticalScrollBar()->maximum() - ui->graphicsView->verticalScrollBar()->value();

	// Positive changes in cursor position correspond to negative changes in scrollbar value.
	this->canvasDragMinCursorPos = QPoint(currentCursorPos.x() - xPositiveScrollPotential, currentCursorPos.y() - yPositiveScrollPotential);
	this->canvasDragMaxCursorPos = QPoint(currentCursorPos.x() + xNegativeScrollPotential, currentCursorPos.y() + yNegativeScrollPotential);

	updateUI();
}

/**
 * Concludes the current drag of the canvas.
 */
void VuoEditorWindow::concludeCanvasDrag()
{
	this->canvasDragInProgress = false;
	this->lastCursorLocationDuringCanvasDrag = QPoint();
	updateUI();
}

/**
 * Handle keypress events.
 */
void VuoEditorWindow::keyPressEvent(QKeyEvent *event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();
	qreal adjustedViewportStepRate = viewportStepRate;
	if (modifiers & Qt::ShiftModifier)
	{
		adjustedViewportStepRate *= viewportStepRateMultiplier;
	}

	if (event->key() == Qt::Key_Escape)
	{
		if (!getCurrentNodeLibrary()->isHidden())
			getCurrentNodeLibrary()->close();
		else if (arePublishedPortSidebarsVisible())
			on_showPublishedPorts_triggered();
		else if (isFullScreen())
			showNormal();
		return;
	}

	if (composition->hasFocus())
	{
		switch (event->key())
		{
			case Qt::Key_Up:
			{
				const int y = ui->graphicsView->verticalScrollBar()->value() -
						adjustedViewportStepRate*(ui->graphicsView->verticalScrollBar()->singleStep());
				ui->graphicsView->verticalScrollBar()->setValue(y);
				break;
			}
			case Qt::Key_Down:
			{
				const int y = ui->graphicsView->verticalScrollBar()->value() +
						adjustedViewportStepRate*(ui->graphicsView->verticalScrollBar()->singleStep());
				ui->graphicsView->verticalScrollBar()->setValue(y);
				break;
			}
			case Qt::Key_Left:
			{
				const int x = ui->graphicsView->horizontalScrollBar()->value() -
						adjustedViewportStepRate*(ui->graphicsView->horizontalScrollBar()->singleStep());
				ui->graphicsView->horizontalScrollBar()->setValue(x);
				break;
			}
			case Qt::Key_Right:
			{
				const int x = ui->graphicsView->horizontalScrollBar()->value() +
						adjustedViewportStepRate*(ui->graphicsView->horizontalScrollBar()->singleStep());
				ui->graphicsView->horizontalScrollBar()->setValue(x);
				break;
			}
			case Qt::Key_Space:
			{
				enableCanvasDrag();
				break;
			}
			default:
			{
				QGraphicsItem *nearbyItem = composition->findNearbyComponent(getCursorScenePos());
				VuoRendererPort *nearbyPort = (dynamic_cast<VuoRendererPort *>(nearbyItem)? (VuoRendererPort *)nearbyItem : NULL);
				if (nearbyPort)
				{
					composition->sendEvent(nearbyPort, event);
					event->accept();
				}
				else
					QMainWindow::keyPressEvent(event);

				break;
			}
		}
	}
}

/**
 * Handle key release events.
 */
void VuoEditorWindow::keyReleaseEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Space:
		{
			disableCanvasDrag();
			break;
		}
		default:
		{
			QMainWindow::keyReleaseEvent(event);
			break;
		}
	}
}

/**
 * Handle mouse move events.
 */
void VuoEditorWindow::mouseMoveEvent(QMouseEvent *event)
{
	if (canvasDragInProgress)
	{
		QPoint oldPos = lastCursorLocationDuringCanvasDrag;
		QPoint currentCursorPos = QCursor::pos();

		// Ignore cursor movements beyond the range that would have affected the scollbar values at the beginning
		// of the drag, to ensure a single continuous contact point between cursor and canvas when the cursor is within range.
		QPoint effectiveNewPos = QPoint(	fmax(canvasDragMinCursorPos.x(), fmin(canvasDragMaxCursorPos.x(), currentCursorPos.x())),
											fmax(canvasDragMinCursorPos.y(), fmin(canvasDragMaxCursorPos.y(), currentCursorPos.y())));

		// Positive changes in cursor position correspond to negative changes in scrollbar value.
		int dx = -1 * (effectiveNewPos - oldPos).x();
		int dy = -1 * (effectiveNewPos - oldPos).y();

		if (dx)
		{
			lastCursorLocationDuringCanvasDrag.setX(effectiveNewPos.x());

			int xScrollbarMin = ui->graphicsView->horizontalScrollBar()->minimum();
			int xScrollbarMax = ui->graphicsView->horizontalScrollBar()->maximum();
			int xScrollbarOldValue = ui->graphicsView->horizontalScrollBar()->value();
			int xScrollbarNewValue = fmax(xScrollbarMin, fmin(xScrollbarMax, xScrollbarOldValue + dx));

			if (xScrollbarNewValue != xScrollbarOldValue)
				ui->graphicsView->horizontalScrollBar()->setValue(xScrollbarNewValue);
		}

		if (dy)
		{
			lastCursorLocationDuringCanvasDrag.setY(effectiveNewPos.y());

			int yScrollbarMin = ui->graphicsView->verticalScrollBar()->minimum();
			int yScrollbarMax = ui->graphicsView->verticalScrollBar()->maximum();
			int yScrollbarOldValue = ui->graphicsView->verticalScrollBar()->value();
			int yScrollbarNewValue = fmax(yScrollbarMin, fmin(yScrollbarMax, yScrollbarOldValue + dy));


			if (yScrollbarNewValue != yScrollbarOldValue)
				ui->graphicsView->verticalScrollBar()->setValue(yScrollbarNewValue);
		}
	}

	QMainWindow::mouseMoveEvent(event);
}

/**
 * Initializes the Node Class Library browser for this editor window.
 */
void VuoEditorWindow::initializeNodeLibrary(VuoCompiler *nodeLibraryCompiler, VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode, VuoNodeLibrary::nodeLibraryState nodeLibraryState, VuoNodeLibrary *floater)
{
	ownedNodeLibrary = new VuoNodeLibrary(nodeLibraryCompiler, this, nodeLibraryDisplayMode);
	ownedNodeLibrary->setObjectName(composition? composition->getBase()->getMetadata()->getName().c_str() : "");

	nl = ownedNodeLibrary;
	transitionNodeLibraryConnections(NULL, nl);

	// Dock the library initially before setting it to the docking state dictated by global settings.
	// This ensures that if it is ever undocked, double-clicking on its title bar will re-dock it.
	addDockWidget(Qt::LeftDockWidgetArea, nl);
	conformToGlobalNodeLibraryVisibility(nodeLibraryState, floater, false);

	nodeLibraryMinimumWidth = nl->minimumWidth();
	nodeLibraryMaximumWidth = nl->maximumWidth();
}

/**
 * Displays a dialogue allowing the user to edit the composition metadata.
 */
void VuoEditorWindow::on_compositionInformation_triggered()
{
	metadataEditor->show();
	updateUI();
}

/**
 * De-selects all composition components and resets the documentation panel content.
 */
void VuoEditorWindow::on_selectNone_triggered()
{
	composition->deselectAllCompositionComponents();
	displayAppropriateDocumentation();
}

/**
 * Zooms in.
 */
void VuoEditorWindow::on_zoomIn_triggered()
{
	ui->graphicsView->scale(zoomRate,zoomRate);

	// Try to keep selected components visible.
	QRectF selectedItemsRect;
	foreach (QGraphicsItem *selectedComponent, composition->selectedItems())
		selectedItemsRect |= selectedComponent->sceneBoundingRect();

	if (!selectedItemsRect.isNull())
		ui->graphicsView->centerOn(selectedItemsRect.center());

	isZoomedToFit = false;
	updateToolbarElementUI();
}

/**
 * Zooms out.
 */
void VuoEditorWindow::on_zoomOut_triggered()
{
	ui->graphicsView->scale(1/zoomRate,1/zoomRate);
	isZoomedToFit = false;
	updateToolbarElementUI();
}

/**
 * Zooms to 100% (1:1).
 */
void VuoEditorWindow::on_zoom11_triggered()
{
	bool zoomingIn = (ui->graphicsView->transform().m11() <= 1.0);
	ui->graphicsView->setTransform(QTransform());

	if (zoomingIn)
	{
		// Try to keep selected components visible.
		QRectF selectedItemsRect;
		foreach (QGraphicsItem *selectedComponent, composition->selectedItems())
			selectedItemsRect |= selectedComponent->sceneBoundingRect();

		if (!selectedItemsRect.isNull())
			ui->graphicsView->centerOn(selectedItemsRect.center());
	}

	isZoomedToFit = false;
	updateToolbarElementUI();
}

/**
 * Unconditionally zooms to fit the composition.
 */
void VuoEditorWindow::on_zoomToFit_triggered()
{
	QRectF itemsTightBoundingRect = (!composition->selectedItems().isEmpty()? composition->internalSelectedItemsBoundingRect() :
																   composition->internalItemsBoundingRect());
	QRectF itemsBoundingRect = itemsTightBoundingRect.adjusted(-VuoEditorWindow::compositionMargin,
																				 -VuoEditorWindow::compositionMargin,
																				 VuoEditorWindow::compositionMargin,
																				 VuoEditorWindow::compositionMargin);
	ui->graphicsView->fitInView(itemsBoundingRect, Qt::KeepAspectRatio);
	updateSceneRect();
	isZoomedToFit = true;
	updateToolbarElementUI();
}

/**
 * Zooms to fit if the composition is too large to be displayed in full within the current viewport.
 */
void VuoEditorWindow::zoomOutToFit()
{
	QRectF itemsTightBoundingRect = (!composition->selectedItems().isEmpty()? composition->internalSelectedItemsBoundingRect() :
																   composition->internalItemsBoundingRect());

	QRectF viewportRect = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();

	// If the itemsBoundingRect is larger than the viewport, zoom out to fit.
	if ((viewportRect.width() < itemsTightBoundingRect.width()) || (viewportRect.height() < itemsTightBoundingRect.height()))
		on_zoomToFit_triggered();

	// If the viewport just needs to be shifted in order to display all composition components, do that.
	else if (!viewportRect.contains(itemsTightBoundingRect))
		ui->graphicsView->ensureVisible(itemsTightBoundingRect, VuoEditorWindow::compositionMargin, VuoEditorWindow::compositionMargin);
}

void VuoEditorWindow::viewportFitReset()
{
	isZoomedToFit = false;

	updateToolbarElementUI();
}

void VuoEditorWindow::on_saveComposition_triggered()
{
	QString savedPath = windowFilePath();
	if (! VuoFileUtilities::fileExists(savedPath.toStdString()))
		on_saveCompositionAs_triggered();
	else
	{
		VUserLog("%s:      Save", getWindowTitleWithoutPlaceholder().toUtf8().data());
		saveFile(savedPath);
	}
}

void VuoEditorWindow::on_saveCompositionAs_triggered()
{
	string savedPath = saveCompositionAs().toUtf8().constData();
	if (savedPath.empty())
		return;
}

/**
 * Prompts the user for a "Save As..." location and proceeds to save the composition
 * at that location.
 *
 * Returns the path at which the composition was saved.
 */
QString VuoEditorWindow::saveCompositionAs()
{
	// Don't use QFileDialog::getSaveFileName() here since it doesn't present the window as a Mac OS X sheet --- https://lists.qt-project.org/pipermail/qt4-feedback/2009-February/000518.html
	QFileDialog d(this, Qt::Sheet);
	//d.setWindowModality(Qt::WindowModal);  // Causes dialog to always return status Rejected.

	if (VuoFileUtilities::fileExists(windowFilePath().toStdString()))
		d.setDirectory(windowFilePath());

	else
	{
		d.selectFile(QString(composition->getBase()->getMetadata()->getName().c_str())
					 .append(".")
					 .append(VuoEditor::vuoCompositionFileExtension));
	}

	d.setAcceptMode(QFileDialog::AcceptSave);

	// Temporary for https://b33p.net/kosada/node/6762 :
	// Since the QFileDialog warns the user about identically named files *before*
	// adding the default extension instead of after, for now, don't have it append
	// a default extension at all. This way we know exactly what filename the user has
	// entered, and if there is a conflict only after we have added the default
	// extension, we can abort the save rather than overwriting the existing file.
	//d.setDefaultSuffix(VuoEditor::vuoCompositionFileExtension);

	QString savedPath = "";
	if (d.exec() == QDialog::Accepted)
	{
		savedPath = d.selectedFiles()[0];
		string dir, file, ext;
		VuoFileUtilities::splitPath(savedPath.toUtf8().constData(), dir, file, ext);
		VUserLog("%s:      Save as %s.%s", getWindowTitleWithoutPlaceholder().toUtf8().data(), file.c_str(), ext.c_str());
		saveFileAs(savedPath);
	}

	setAsActiveWindow();
	return savedPath;
}

/**
 * Saves the composition at the provided path.
 * Also prompts the user as to how to handle resourcse referenced using relative paths,
 * updating the paths and/or copying the files as the user instructs.
 *
 * @param savePath The path where the composition is to be saved.
 */
bool VuoEditorWindow::saveFileAs(const QString & savePath)
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(savePath.toUtf8().constData(), dir, file, ext);
	bool installingAsSubcomposition = (VuoFileUtilities::getUserModulesPath().c_str() == QFileInfo(dir.c_str()).canonicalFilePath());

	QDir newCompositionDir(QFileInfo(savePath).absoluteDir().canonicalPath());

	map<VuoPort *, string> modifiedPortConstantRelativePaths = composition->getPortConstantResourcePathsRelativeToDir(newCompositionDir);
	string modifiedIconPath = composition->getAppIconResourcePathRelativeToDir(newCompositionDir);
	bool iconPathChanged = (modifiedIconPath != composition->getBase()->getMetadata()->getIconURL());

	if (!installingAsSubcomposition && (!modifiedPortConstantRelativePaths.empty() || iconPathChanged))
	{
		// If the previous storage directory was the /tmp directory (as in the case of example compositions),
		// offer to copy resource files to the new directory; otherwise, offer to update resource paths referenced
		// within the composition so that they are correct when resolved relative to the new directory.
		QDir compositionDir = QDir(composition->getBase()->getDirectory().c_str());
		bool copyTmpFiles = (compositionDir.canonicalPath() == QDir(VuoFileUtilities::getTmpDir().c_str()).canonicalPath());

		map<VuoPort *, string> pathsToUpdate;
		QString copiedFileDetails = "";
		QString updatedPathDetails = "";

		// Update relative paths referenced within port constants.
		for (map <VuoPort *, string>::iterator i = modifiedPortConstantRelativePaths.begin(); i != modifiedPortConstantRelativePaths.end(); ++i)
		{
			VuoPort *port = i->first;
			string modifiedPath = i->second;

			// Check whether this is a /tmp file (so that we copy it rather than updating the path).
			VuoText origRelativePathText = VuoMakeRetainedFromString(port->getRenderer()->getConstantAsString().c_str(), VuoText);
			QString origRelativePath = origRelativePathText;
			VuoRelease(origRelativePathText);
			string origRelativeDir, file, ext;
			VuoFileUtilities::splitPath(origRelativePath.toUtf8().constData(), origRelativeDir, file, ext);
			string resourceFileName = file;
			if (!ext.empty())
			{
				resourceFileName += ".";
				resourceFileName += ext;
			}
			QString origAbsolutePath = compositionDir.filePath(QDir(origRelativeDir.c_str()).filePath(resourceFileName.c_str()));

			// Mark the file for copying or the path for updating, as appropriate.
			if (copyTmpFiles && VuoRendererComposition::isTmpFile(origAbsolutePath.toUtf8().constData()))
				copiedFileDetails.append(QString("%1\n").arg(origRelativePath));
			else
			{
				VuoText modifiedPathText = VuoMakeRetainedFromString(modifiedPath.c_str(), VuoText);
				updatedPathDetails.append(QString("%1 → %2\n").arg(origRelativePath, modifiedPathText));
				VuoRelease(modifiedPathText);
				pathsToUpdate[port] = modifiedPath;
			}
		}

		// Update relative path to custom app icon.
		if (iconPathChanged)
		{
			QString origRelativePath = composition->getBase()->getMetadata()->getIconURL().c_str();

			// Check whether this is a /tmp file (so that we copy it rather than updating the path).
			string origRelativeDir, file, ext;
			VuoFileUtilities::splitPath(origRelativePath.toUtf8().constData(), origRelativeDir, file, ext);
			string resourceFileName = file;
			if (!ext.empty())
			{
				resourceFileName += ".";
				resourceFileName += ext;
			}
			QString origAbsolutePath = compositionDir.filePath(QDir(origRelativeDir.c_str()).filePath(resourceFileName.c_str()));

			// Mark the file for copying or the path for updating, as appropriate.
			if (copyTmpFiles && VuoRendererComposition::isTmpFile(origAbsolutePath.toUtf8().constData()))
				copiedFileDetails.append(QString("%1\n").arg(origRelativePath));
			else
				updatedPathDetails.append(QString("%1 → %2\n").arg(origRelativePath, modifiedIconPath.c_str()));
		}

		bool copyFiles = (!copiedFileDetails.isEmpty());
		bool updatePaths = (!updatedPathDetails.isEmpty());
		//: Appears in a dialog after selecting File > Save As on a composition that refers to resources with relative paths.
		const QString updateSummary = "<p>" + tr("You're saving your composition to a different folder.") + "</p>"
			+ "<p>"
			+ (copyFiles
				//: Appears in a dialog after selecting File > Save As on a composition that refers to resources with relative paths.
				? tr("Do you want to copy the example files used by your composition?")
				//: Appears in a dialog after selecting File > Save As on a composition that refers to resources with relative paths.
				: tr("Do you want to update the paths to files in your composition?"))
			+ "</p>";

		QString updateDetails = "";
		if (copyFiles)
			//: Appears in a dialog after selecting File > Save As on a composition that refers to resources with relative paths.
			updateDetails += tr("The following file(s) will be copied", "", modifiedPortConstantRelativePaths.size()) + ":\n\n" + copiedFileDetails + "\n";

		if (updatePaths)
			//: Appears in a dialog after selecting File > Save As on a composition that refers to resources with relative paths.
			updateDetails += tr("The following path(s) will be updated", "", pathsToUpdate.size()) + ":\n\n" + updatedPathDetails;

		QMessageBox messageBox(this);
		messageBox.setWindowFlags(Qt::Sheet);
		messageBox.setWindowModality(Qt::WindowModal);
		messageBox.setTextFormat(Qt::RichText);
		messageBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Ok);
		messageBox.setButtonText(QMessageBox::Discard, (copyFiles? tr("Leave files in place") : tr("Leave paths unchanged")));
		messageBox.setButtonText(QMessageBox::Ok, (copyFiles? tr("Copy files") : tr("Update paths")));
		messageBox.setDefaultButton(QMessageBox::Ok);
		messageBox.setStyleSheet("#qt_msgbox_informativelabel, QMessageBoxDetailsText { font-weight: normal; font-size: 11pt; }");
		messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());
		messageBox.setText(updateSummary);
		messageBox.setDetailedText(updateDetails);

		// Give the "Leave in place" button keyboard focus (without "Default" status) so that it can be activated by spacebar.
		static_cast<QPushButton *>(messageBox.button(QMessageBox::Discard))->setAutoDefault(false);
		messageBox.button(QMessageBox::Discard)->setFocus();

		if (messageBox.exec() == QMessageBox::Ok)
		{
			// Copy example resource files to the new directory.
			if (copyFiles)
			{
				bool tmpFilesOnly = true;
				composition->bundleResourceFiles(newCompositionDir.canonicalPath().toUtf8().constData(), tmpFilesOnly);
			}
			// Update relative resource paths.
			if (updatePaths)
			{
				undoStack->beginMacro(tr("File Path Updates"));

				// Update relative URLs referenced within port constants.
				for (map <VuoPort *, string>::iterator i = pathsToUpdate.begin(); i != pathsToUpdate.end(); ++i)
				{
					VuoPort *port = i->first;
					string modifiedPath = i->second;

					setPortConstant(port->getRenderer(), modifiedPath);
				}


				// Update relative path to custom icon.
				if (iconPathChanged)
				{
					string customizedCompositionName = composition->getBase()->getMetadata()->getCustomizedName();

					VuoCompositionMetadata *metadataCopy = new VuoCompositionMetadata(*composition->getBase()->getMetadata());
					metadataCopy->setName(customizedCompositionName);
					metadataCopy->setIconURL(modifiedIconPath);

					undoStack->push(new VuoCommandSetMetadata(metadataCopy, this));

				}

				undoStack->endMacro();
			}
		}
	}

	return saveFile(savePath);
}

string VuoEditorWindow::on_installSubcomposition_triggered()
{
	QString oldTitle = getWindowTitleWithoutPlaceholder();

	string nodeClassName = installSubcomposition("");

	VUserLog("%s:      %s: %s",
		oldTitle.toUtf8().data(),
		ui->installSubcomposition->text().toUtf8().data(),
		nodeClassName.c_str());

	if (!nodeClassName.empty())
		static_cast<VuoEditor *>(qApp)->highlightNewNodeClassInAllLibraries(nodeClassName);

	return nodeClassName;
}

/**
 * Installs this window's composition as a subcomposition
 * in either the System, User, or Composition-Local Library.
 */
string VuoEditorWindow::installSubcomposition(string parentCompositionPath)
{
	string currentCompositionPath = windowFilePath().toUtf8().constData();
	bool currentCompositionExists = VuoFileUtilities::fileExists(currentCompositionPath);

	QString operationTitle;
	string installedSubcompositionDir;
	if (parentCompositionPath.empty())  // "File > Move/Save to User Library"
	{
		if (currentCompositionExists)
			operationTitle = tr("Move Subcomposition to User Library");
		else
			operationTitle = tr("Save Subcomposition to User Library");
		installedSubcompositionDir = VuoFileUtilities::getUserModulesPath();
	}
	else  // "Edit > Insert Subcomposition" or "Edit > Package as Subcomposition"
	{
		if (VuoStringUtilities::beginsWith(parentCompositionPath, VuoFileUtilities::getUserModulesPath()))
		{
			operationTitle = tr("Save Subcomposition to User Library");
			installedSubcompositionDir = VuoFileUtilities::getUserModulesPath();
		}
		else if (VuoStringUtilities::beginsWith(parentCompositionPath, VuoFileUtilities::getSystemModulesPath()))
		{
			operationTitle = tr("Save Subcomposition to System Library");
			installedSubcompositionDir = VuoFileUtilities::getSystemModulesPath();
		}
		else
		{
			operationTitle = tr("Save Subcomposition to Composition-Local Library");
			installedSubcompositionDir = VuoFileUtilities::getCompositionLocalModulesPath(parentCompositionPath);
		}
	}

	VuoFileUtilities::makeDir(installedSubcompositionDir);
	string nodeClassName;

	// Case: The composition is already installed as a module. Preserve its existing node class name.
	if (VuoFileUtilities::fileExists(currentCompositionPath) && VuoFileUtilities::isInstalledAsModule(currentCompositionPath))
		nodeClassName = VuoCompiler::getModuleKeyForPath(currentCompositionPath);

	// Case: The composition is being installed as a module for the first time. It needs a name.
	else
	{
		QString defaultNodeDisplayName = composition->formatCompositionFileNameForDisplay(composition->getBase()->getMetadata()->getDefaultName().c_str());
		QString defaultNodeCategory = static_cast<VuoEditor *>(qApp)->getDefaultSubcompositionPrefix();

		QString currentNodeDisplayName = (!composition->getBase()->getMetadata()->getCustomizedName().empty()?
												composition->getBase()->getMetadata()->getCustomizedName().c_str() :
												defaultNodeDisplayName);
		QString currentNodeCategory = static_cast<VuoEditor *>(qApp)->getSubcompositionPrefix();

		VuoSubcompositionSaveAsDialog d(this, Qt::Sheet, operationTitle,
										defaultNodeDisplayName, defaultNodeCategory,
										currentNodeDisplayName, currentNodeCategory);
		bool ok = d.exec();
		if (!ok)
			return "";

		QString nodeDisplayName = d.nodeTitle();
		QString nodeCategory = d.nodeCategory();

		if (nodeCategory != currentNodeCategory)
			static_cast<VuoEditor *>(qApp)->updateSubcompositionPrefix(nodeCategory);

		if (composition->getBase()->getMetadata()->getCustomizedName() != nodeDisplayName.toUtf8().constData())
		{
			VuoCompositionMetadata *newMetadata = new VuoCompositionMetadata(*composition->getBase()->getMetadata());
			newMetadata->setName(nodeDisplayName.toUtf8().constData());
			undoStack->push(new VuoCommandSetMetadata(newMetadata, this));
		}

		nodeClassName = getNodeClassNameForDisplayNameAndCategory(nodeDisplayName, nodeCategory, defaultNodeDisplayName, defaultNodeCategory).toStdString();
	}

	string copiedCompositionPath = installedSubcompositionDir + "/" + nodeClassName + "." + VuoEditor::vuoCompositionFileExtension.toUtf8().constData();

	// Make sure the node class doesn't already exist as a .vuo or .vuonode file in the same directory.
	int documentIdentifierInstanceNum = 1;
	while (VuoFileUtilities::fileExists(copiedCompositionPath) ||
		   VuoFileUtilities::fileExists(copiedCompositionPath + "node"))
	{
		std::ostringstream oss;
		oss << ++documentIdentifierInstanceNum;
		copiedCompositionPath = installedSubcompositionDir + "/" + nodeClassName + oss.str() + "." + VuoEditor::vuoCompositionFileExtension.toUtf8().constData();
	}

	bool saveSucceeded = saveFileAs(copiedCompositionPath.c_str());
	if (! saveSucceeded)
		return "";

	VuoFileUtilities::deleteFile(currentCompositionPath);

	return VuoCompiler::getModuleKeyForPath(windowFilePath().toStdString());
}

/**
 * Returns the node class name derived from the provided composition name
 * and category (subcomposition prefix), or defaults if either derivation is empty.
 */
QString VuoEditorWindow::getNodeClassNameForDisplayNameAndCategory(QString compositionName, QString category, QString defaultCompositionName, QString defaultCategory)
{
	compositionName = QString::fromStdString(deriveBaseNodeClassNameFromDisplayName(compositionName.toStdString()));
	if (compositionName.isEmpty())
		compositionName = QString::fromStdString(deriveBaseNodeClassNameFromDisplayName(defaultCompositionName.toStdString()));

	// Treat "vuo" as a reserved prefix.
	QString thirdPartyCategory = category.remove(QRegExp("^(vuo\\.?)+", Qt::CaseInsensitive));

	category = QString::fromStdString(deriveBaseNodeClassNameFromDisplayName(thirdPartyCategory.toStdString()));
	if (category.isEmpty())
		category = QString::fromStdString(deriveBaseNodeClassNameFromDisplayName(defaultCategory.toStdString()));

	return category + "." + compositionName;
}

/**
 * Derives a base node class name from the provided node display name, for use
 * in installing user-named subcomposition nodes.
 *
 * Similar to VuoStringUtilities::transcodeToIdentifier,
 * but we can't use that as-is since it permits non-RDNS-compatible characters (e.g., underscores),
 * removes dots, and doesn't apply camelCase.
 */
string VuoEditorWindow::deriveBaseNodeClassNameFromDisplayName(string displayName)
{
	if (displayName.empty())
		return "";

	// Transliterate Unicode to Latin ASCII, to provide some support for non-English-language names.
	{
		CFStringRef cfs = CFStringCreateWithCString(NULL, displayName.c_str(), kCFStringEncodingUTF8);
		if (!cfs)
			return "";
		CFMutableStringRef cfsm = CFStringCreateMutableCopy(NULL, 0, cfs);
		CFStringTransform(cfsm, NULL, kCFStringTransformToLatin, false);  // Converts 'ä' -> 'a', '張' -> 'zhang'.
		CFStringTransform(cfsm, NULL, kCFStringTransformStripCombiningMarks, false);
		CFStringTransform(cfsm, NULL, kCFStringTransformStripDiacritics, false);
		CFStringTransform(cfsm, NULL, CFSTR("ASCII"), false);  // Converts 'ß' -> 'ss'.
		displayName = VuoStringUtilities::makeFromCFString(cfsm);
		CFRelease(cfsm);
		CFRelease(cfs);
	}

	return VuoStringUtilities::convertToCamelCase(displayName, false, true, false, true);
}

/**
 * Updates the composition metadata to match that set by the user in this window's
 * `metadataEditor` instance.
 */
void VuoEditorWindow::editMetadata(int dialogResult)
{
	if (dialogResult == QDialog::Accepted)
		undoStack->push(new VuoCommandSetMetadata(metadataEditor->toMetadata(), this));

	updateUI();
}

/**
  * Generates the default uncustomized copyright text for this composition.
  */
string VuoEditorWindow::generateCurrentDefaultCopyright()
{
	const string user = static_cast<VuoEditor *>(qApp)->getUserName();
	const string userProfileURL = static_cast<VuoEditor *>(qApp)->getStoredUserProfileURL();
	const string userProfileLink = (userProfileURL.empty()? user : "[" + user + "](" + userProfileURL + ")");
	const string currentYear = QDate::currentDate().toString("yyyy").toUtf8().constData();

	const string copyright = "Copyright © " + currentYear + " " + userProfileLink;
	return copyright;
}

/**
 * Shows a dialog asking whether to overwrite the file or folder specified by `path`.
 */
bool VuoEditorWindow::confirmReplacingFile(string path)
{
	VuoRendererFonts *fonts = VuoRendererFonts::getSharedFonts();
	QFileInfo fileInfo(QString::fromStdString(path));
	QMessageBox d(this);
	d.setWindowFlags(Qt::Sheet);
	d.setWindowModality(Qt::WindowModal);
	d.setFont(fonts->dialogHeadingFont());
	d.setTextFormat(Qt::RichText);
	d.setText(tr("<b>“%1” already exists. Do you want to replace it?</b>").arg(fileInfo.fileName()));
	d.setInformativeText("<style>p{" + fonts->getCSS(fonts->dialogBodyFont()) + "}</style>"
					   + tr("<p>A %1 with the same name already exists in the “%2” folder.</p><p>Replacing it will overwrite its current contents.<br></p>")
						 .arg(VuoFileUtilities::dirExists(path) ? "folder" : "file")
						 .arg(fileInfo.dir().dirName()));
	d.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	d.setDefaultButton(QMessageBox::Cancel);
	d.setButtonText(QMessageBox::Yes, tr("Replace"));
	d.setIcon(QMessageBox::Warning);

	// Give the "Replace" button keyboard focus (without "Default" status) so that it can be activated by spacebar.
	static_cast<QPushButton *>(d.button(QMessageBox::Yes))->setAutoDefault(false);
	d.button(QMessageBox::Yes)->setFocus();

	return d.exec() == QMessageBox::Yes;
}

/**
 * Finds and selects nodes that contain the user-entered search term
 * within their display names or node class names.
 */
void VuoEditorWindow::on_find_triggered()
{
	addDockWidget(Qt::TopDockWidgetArea, searchBox);
	searchBox->setFocus();
	searchBox->show();
	updateUI();
}

/**
 * Advances to the next search result.
 */
void VuoEditorWindow::on_findNext_triggered()
{
	if (!searchBox->isHidden())
		searchBox->goToNextResult();
}

/**
 * Returns to the previous search result.
 */
void VuoEditorWindow::on_findPrevious_triggered()
{
	if (!searchBox->isHidden())
		searchBox->goToPreviousResult();
}

/**
 * Saves the composition, as would be done by the Save or Save As action.
 * Updates the window attributes.
 *
 * @param savePath The path where the composition is to be saved.
 */
bool VuoEditorWindow::saveFile(const QString & savePath)
{
	// Temporary for https://b33p.net/kosada/node/6762 :
	// The file dialogue does not currently append the default file suffix automatically;
	// do so here.  If the addition of the suffix results in a file-name conflict,
	// abort the save, since the user has not given permission to overwrite the existing composition.
	bool saveAborted = false;
	QString failureDetails = "";
	QString modifiedSavePath = savePath;
	QString expectedFileExtension = QString(".").append(VuoEditor::vuoCompositionFileExtension);
	if (!savePath.endsWith(expectedFileExtension))
	{
		modifiedSavePath.append(expectedFileExtension);
		if (VuoFileUtilities::fileExists(modifiedSavePath.toStdString()))
		{
			saveAborted = true;
			failureDetails = "A file or folder with the same name already exists.";
		}
	}

	QString existingPath = windowFilePath();
	bool saveSucceeded = !saveAborted && saveFile(modifiedSavePath, existingPath);
	int error = errno;

	if (saveSucceeded)
	{
		// Clear the "Untitled Composition" window title, since windowTitle overrides windowFilePath.
		if (windowTitle().startsWith(untitledComposition))
			setWindowTitle("");

		setWindowFilePath(modifiedSavePath);
#if VUO_PRO
		toolbar->updateTitle();
#endif
		compositionUpgradedSinceLastSave = false;
		undoStack->setClean();
		setCompositionModified(false);

		string oldDir = composition->getBase()->getDirectory();
		string newDir, file, ext;
		VuoFileUtilities::splitPath(modifiedSavePath.toUtf8().constData(), newDir, file, ext);
		bool compositionDirChanged = ! VuoFileUtilities::arePathsEqual(oldDir, newDir);
		if (compositionDirChanged)
		{
			// Find any composition-local modules that the composition depends on.
			QStringList compositionLocalModules;
			for (const string &dependency : compiler->getDependenciesForComposition(getComposition()->getBase()->getCompiler()))
				if (compiler->isCompositionLocalModule(dependency))
					compositionLocalModules.append(QString::fromStdString(dependency));

			if (! compositionLocalModules.empty())
			{
				compositionLocalModules.sort();

				QString summary = QString("<p>You're saving your composition to a different folder.</p>")
								  .append("<p>Where do you want to install the composition's local nodes?</p>");

				QString details = QString("The following nodes will be moved or copied:\n\n")
								  .append(compositionLocalModules.join("\n"))
								  .append("\n");

				QMessageBox messageBox(this);
				messageBox.setWindowFlags(Qt::Sheet);
				messageBox.setWindowModality(Qt::WindowModal);
				messageBox.setTextFormat(Qt::RichText);
				messageBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
				messageBox.setButtonText(QMessageBox::No, "Move to User Library");
				messageBox.setButtonText(QMessageBox::Yes, "Copy to new Composition Library");
				messageBox.setDefaultButton(QMessageBox::Yes);
				messageBox.setStyleSheet("#qt_msgbox_informativelabel, QMessageBoxDetailsText { font-weight: normal; font-size: 11pt; }");
				messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());
				messageBox.setText(summary);
				messageBox.setDetailedText(details);

				// Give the non-default button keyboard focus so that it can be activated by spacebar.
				static_cast<QPushButton *>(messageBox.button(QMessageBox::No))->setAutoDefault(false);
				messageBox.button(QMessageBox::No)->setFocus();

				int ret = messageBox.exec();

				string oldModulesDirPath = VuoFileUtilities::getCompositionLocalModulesPath(existingPath.toStdString());
				QDir oldModulesDir = QDir(QString::fromStdString(oldModulesDirPath));

				string newModulesDirPath = (ret == QMessageBox::Yes ?
												VuoFileUtilities::getCompositionLocalModulesPath(modifiedSavePath.toStdString()) :
												VuoFileUtilities::getUserModulesPath());
				VuoFileUtilities::makeDir(newModulesDirPath);
				QDir newModulesDir(QString::fromStdString(newModulesDirPath));

				// Move/copy the required composition-local modules from the old to the new Modules dir.
				foreach (QString moduleFileName, oldModulesDir.entryList(QDir::Files))
				{
					string moduleKey = VuoCompiler::getModuleKeyForPath(moduleFileName.toStdString());

					if (compositionLocalModules.contains(QString::fromStdString(moduleKey)))
					{
						string oldModulePath = oldModulesDir.filePath(moduleFileName).toStdString();
						string newModulePath = newModulesDir.filePath(moduleFileName).toStdString();

						// Skip if the file already exists in the new Modules dir.
						if (! VuoFileUtilities::fileExists(newModulePath))
						{
							if (ret == QMessageBox::Yes)
								VuoFileUtilities::copyFile(oldModulePath, newModulePath);
							else
								VuoFileUtilities::moveFile(oldModulePath, newModulePath);
						}
					}
				}
			}
		}

		// Update the compiler's list of loaded modules.
		compiler->setCompositionPath(modifiedSavePath.toUtf8().constData());

		if (includeInRecentFileMenu)
			static_cast<VuoEditor *>(qApp)->addFileToAllOpenRecentFileMenus(modifiedSavePath);
	}

	else
	{
		if (failureDetails.isEmpty())
			failureDetails = strerror(error);

		QMessageBox fileSaveFailureDialog(this);
		fileSaveFailureDialog.setWindowFlags(Qt::Sheet);
		fileSaveFailureDialog.setWindowModality(Qt::WindowModal);
		//: Appears in a dialog after selecting File > Save or Save As.
		QString errorMessage = tr("The composition could not be saved at “%1”.").arg(modifiedSavePath);
		fileSaveFailureDialog.setText(tr(errorMessage.toUtf8().constData()));
		fileSaveFailureDialog.setStyleSheet("#qt_msgbox_informativelabel { font-weight: normal; font-size: 11pt; }");
		fileSaveFailureDialog.setInformativeText(failureDetails);
		fileSaveFailureDialog.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		fileSaveFailureDialog.setButtonText(QMessageBox::Save, tr("Save As…"));
		fileSaveFailureDialog.setIcon(QMessageBox::Warning);

		switch(fileSaveFailureDialog.exec()) {
			case QMessageBox::Save:
				on_saveCompositionAs_triggered();
				break;
			case QMessageBox::Cancel:
				break;
			default:
				break;
		}
	}

	return saveSucceeded;
}

/**
 * Populates the provided menu @c m with the editor's built-in protocols.
 */
void VuoEditorWindow::populateProtocolsMenu(QMenu *m)
{
	foreach (VuoProtocol *protocol, VuoProtocol::getProtocols())
	{
		QAction *protocolAction = new QAction(this);
		protocolAction->setText(VuoEditor::tr(protocol->getName().c_str()));
		protocolAction->setData(QVariant::fromValue(protocol));
		protocolAction->setCheckable(true);
		connect(protocolAction, &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);
		m->addAction(protocolAction);
	}

	m->addSeparator();

	QAction *protocolAction = new QAction(this);
	protocolAction->setText(VuoEditor::tr("None"));
	protocolAction->setCheckable(true);
	connect(protocolAction, &QAction::triggered, this, &VuoEditorWindow::changeActiveProtocol);
	m->addAction(protocolAction);
}

/**
 * Updates the provided protocol menu to reflect the current enabled/disabled status of each protocol.
 */
void VuoEditorWindow::updateProtocolsMenu(QMenu *m)
{
	foreach (QAction *protocolAction, m->actions())
	{
		VuoProtocol *currentProtocol = protocolAction->data().value<VuoProtocol *>();
		protocolAction->setChecked(composition->getActiveProtocol() == currentProtocol);
	}
}

/**
 * Updates the composition editor window with the active
 * protocol indicated by the sender.
 */
void VuoEditorWindow::changeActiveProtocol(void)
{
	QAction *sender = (QAction *)QObject::sender();
	VuoProtocol *selectedProtocol = sender->data().value<VuoProtocol *>();

	if (!selectedProtocol)  // "None"
	{
		selectedProtocol = composition->getActiveProtocol();
		if (!selectedProtocol)
		{
			// If the user selected "None", and there is already no protocol, there's nothing to do,
			// except ensure that the menu checkmarks are up-to-date.
			updateUI();
			return;
		}
	}

	VUserLog("%s:      %s protocol %s {",
		getWindowTitleWithoutPlaceholder().toUtf8().data(),
		composition->getActiveProtocol() == selectedProtocol ? "Remove" : "Add",
		selectedProtocol->getName().c_str());

	// @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	toggleActiveStatusForProtocol(selectedProtocol);

	setPublishedPortSidebarVisibility(true);

	VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
}

/**
 * If the provided protocol is currently inactive, activates it; otherwise, deactivates it.
 */
void VuoEditorWindow::toggleActiveStatusForProtocol(VuoProtocol *protocol)
{
	if (composition->getActiveProtocol() == protocol)
		composition->removeActiveProtocol(protocol, NULL);
	else
		composition->addActiveProtocol(protocol, true);
}

/**
 * Updates the published port sidebars and the editor window UI to reflect the currently active protocol.
 */
void VuoEditorWindow::updateActiveProtocolDisplay(void)
{
	inputPortSidebar->updateActiveProtocol();
	outputPortSidebar->updateActiveProtocol();
	updateUI();
}

/**
 * If the composition currenty has no active protocol, evaluates it for compliance with available
 * protocols and promotes it to the first compliant protocol encountered, if any.
 */
void VuoEditorWindow::evaluateCompositionForProtocolPromotion()
{
	// @todo: Account for multiple simultaneous active protocols. https://b33p.net/kosada/node/9585
	if (composition->getActiveProtocol())
		return;

	string compositionAsString = composition->takeSnapshot();
	foreach (VuoProtocol *protocol, VuoProtocol::getProtocols())
	{
		if (protocol->isCompositionCompliant(compositionAsString))
		{
			composition->addActiveProtocol(protocol, false);
			return;
		}
	}

	return;
}

/**
 * Saves the composition.
 *
 * @param savePath The path where the composition is to be saved.
 * @param existingPath The path where the composition has previously been saved (or empty if this is a new composition).
 * @return true upon success; false upon failure.
 */
bool VuoEditorWindow::saveFile(const QString & savePath, QString & existingPath)
{
	string existingCompositionFooter = "";

	// Modifying an existing composition
	if (! existingPath.isEmpty())
	{
		ifstream existingFile(existingPath.toUtf8().constData());

		// Preserve the original composition footer (everything after the final '}').
		bool graphvizStatementListEndFound = false;
		for (char c = existingFile.get(); existingFile.good(); c = existingFile.get())
		{
			if (existingFile.good())
			{
				if (c == '}')
				{
					existingCompositionFooter = "";
					graphvizStatementListEndFound = true;
				}
				else if (graphvizStatementListEndFound)
					existingCompositionFooter += c;
			}
		}
		existingFile.close();
	}

	// Update the version of Vuo last used to save the composition.
	string previousLastSavedInVuoVersion = composition->getBase()->getMetadata()->getLastSavedInVuoVersion();
	composition->getBase()->getMetadata()->setLastSavedInVuoVersion(VUO_VERSION_STRING);

	// Generate the modified composition.
	string compositionHeader = composition->generateCompositionHeader();
	string outputComposition = composition->getBase()->getCompiler()->getGraphvizDeclaration(composition->getActiveProtocol(),
																							 compositionHeader,
																							 existingCompositionFooter);
	// Save the modified composition to a temporary file.
	string dir, file, ext;
	string finalSavePath = savePath.toUtf8().constData();
	VuoFileUtilities::splitPath(finalSavePath, dir, file, ext);
	string tmpSavePath = VuoFileUtilities::makeTmpFile("." + file, ext, dir);

	// The mode of temp files defaults to 0600.
	// Change it to match the process's umask.
	{
		// http://man7.org/linux/man-pages/man2/umask.2.html says:
		// "It is impossible to use umask() to fetch a process's umask without at
		// the same time changing it.  A second call to umask() would then be
		// needed to restore the umask."
		mode_t currentUMask = umask(0);
		umask(currentUMask);

		chmod(tmpSavePath.c_str(), 0666 & ~currentUMask);
	}

	ofstream savedFile(tmpSavePath.c_str(), ios::trunc);
	savedFile << outputComposition;
	savedFile.close();

	string defaultName = VuoEditorComposition::getDefaultNameForPath(finalSavePath);
	composition->getBase()->getMetadata()->setDefaultName(defaultName);

	// Move the generated temporary file to the desired save path.
	bool saveSucceeded = ((! savedFile.fail()) && (! rename(tmpSavePath.c_str(), finalSavePath.c_str())));

	if (saveSucceeded)
	{
		this->composition->getBase()->setDirectory(dir);
		this->metadataPanel->setIsUserComposition(true);
		this->metadataPanel->update();
	}
	else
		this->composition->getBase()->getMetadata()->setLastSavedInVuoVersion(previousLastSavedInVuoVersion);

	return saveSucceeded;
}

/**
 * Responds to changes in the Undo stack's 'clean' state.
 */
void VuoEditorWindow::undoStackCleanStateChanged(bool clean)
{
	// When the Undo stack leaves its clean state, mark the composition as modified.
	if (!clean)
		setCompositionModified(true);

	// Mark the composition as unmodified when the Undo stack enters
	// its clean state as long as the composition has not been modified by the
	// upgrade manager since its last save.
	else if (clean && !compositionUpgradedSinceLastSave)
		setCompositionModified(false);
}

/**
 * Updates the UI to indicate whether the composition has unsaved changes.
 */
void VuoEditorWindow::setCompositionModified(bool modified)
{
	setWindowModified(modified);
	updateUI();
}

/**
 * Implements the Run action to run the composition.
 */
void VuoEditorWindow::on_runComposition_triggered()
{
	VUserLog("%s:      Run", getWindowTitleWithoutPlaceholder().toUtf8().data());

	toolbar->changeStateToBuildPending();

	string snapshot = composition->takeSnapshot();
	composition->run(snapshot);

	updateUI();
}

/**
 * Implements the Stop action to stop the composition.
 */
void VuoEditorWindow::on_stopComposition_triggered()
{
	VUserLog("%s:      Stop", getWindowTitleWithoutPlaceholder().toUtf8().data());

	toolbar->changeStateToStopInProgress();
	updateUI();

	composition->stop();
}

/**
 * Implements the Restart action to restart the running composition.
 */
void VuoEditorWindow::on_restartComposition_triggered()
{
	on_stopComposition_triggered();
	on_runComposition_triggered();
}

/**
 * Re-fires the most recently manually fired trigger port.
 */
void VuoEditorWindow::on_refireEvent_triggered()
{
	composition->refireTriggerPortEvent();
}

/**
 * Replaces the Run button icon with an activity indicator.
 */
void VuoEditorWindow::showBuildActivityIndicator()
{
	toolbar->changeStateToBuildInProgress();
	updateUI();
}

/**
 * Removes the activity indicator from the Run button icon.
 */
void VuoEditorWindow::hideBuildActivityIndicator(QString buildError)
{
	if (! buildError.isEmpty())
	{
		toolbar->changeStateToStopped();
		updateUI();
		updateRefireAction();

		QString details = "";
		if (buildError.contains("Nodes not installed", Qt::CaseInsensitive))
			details = "This composition contains nodes that aren't installed.";

		VuoErrorDialog::show(this, tr("There was a problem running the composition."), details, buildError);
	}
	else
	{
		toolbar->changeStateToRunning();
		updateUI();
		updateRefireAction();
	}
}

/**
 * Removes the activity indicator from the Stop button icon.
 */
void VuoEditorWindow::hideStopActivityIndicator()
{
	toolbar->changeStateToStopped();
	updateUI();
	updateRefireAction();
}

void VuoEditorWindow::on_openUserModulesFolder_triggered()
{
	VuoEditorUtilities::openUserModulesFolder();
}

void VuoEditorWindow::on_openSystemModulesFolder_triggered()
{
	VuoEditorUtilities::openSystemModulesFolder();
}

void VuoEditorWindow::on_showConsole_triggered()
{
	VuoConsole::show(this);
}

/**
 * Shows the Node Class Library browser for this editor window and focuses its text filter.
 */
void VuoEditorWindow::on_showNodeLibrary_triggered(void)
{
	if (nl->isHidden())
		nl->emitNodeLibraryHiddenOrUnhidden(true);

	nl->focusTextFilter();
}

/**
 * Toggles the visibility of the Published Ports side panels for this editor window.
 */
void VuoEditorWindow::on_showPublishedPorts_triggered(void)
{
	setPublishedPortSidebarVisibility(!arePublishedPortSidebarsVisible());
	updateUI();
}

/**
 * Shows the Published Ports side panels for this editor window if they are not already visible.
 * Returns a boolean indicating whether they did in fact need to be shown (were previously hidden).
 */
void VuoEditorWindow::showPublishedPortSidebars(void)
{
	if (!arePublishedPortSidebarsVisible())
		on_showPublishedPorts_triggered();
}

/**
 * Returns true if the published port sidebars are currently visible.
 */
bool VuoEditorWindow::arePublishedPortSidebarsVisible()
{
	return !inputPortSidebar->isHidden() && !outputPortSidebar->isHidden();
}

/**
 * Toggles the visibility of hidden cables for this editor window.
 */
void VuoEditorWindow::on_showHiddenCables_triggered(void)
{
	bool previouslyShowingHiddenCables = composition->getRenderHiddenCables();
	if (!previouslyShowingHiddenCables)
	{
		if (composition->hasHiddenPublishedCables() && (inputPortSidebar->isHidden() || outputPortSidebar->isHidden()))
			on_showPublishedPorts_triggered();
	}

	composition->setRenderHiddenCables(!previouslyShowingHiddenCables);

	updateUI();
}

/**
 * Toggles 'Show Events' mode for this composition.
 */
void VuoEditorWindow::on_showEvents_triggered(void)
{
	VUserLog("%s:      %s Show Events mode",
		getWindowTitleWithoutPlaceholder().toUtf8().data(),
		composition->getShowEventsMode() ? "Disable" : "Enable");

	composition->setShowEventsMode(! composition->getShowEventsMode());
	updateUI();
}

/**
 * Closes both published port sidebars.
 */
void VuoEditorWindow::closePublishedPortSidebars()
{
	setPublishedPortSidebarVisibility(false);
}

/**
 * Displays both published port sidebars if @c visible is true.
 */
void VuoEditorWindow::conditionallyShowPublishedPortSidebars(bool visible)
{
	if (visible)
		setPublishedPortSidebarVisibility(true);
}

/**
 * Sets the visibility of the Published Ports side panels for this editor window.
 */
void VuoEditorWindow::setPublishedPortSidebarVisibility(bool visible)
{
	// Display the published input and output port sidebars as widgets docked in the left
	// and right docking areas, respectively.
	if (visible)
	{
		// If the node library is already docked in the left docking area, situate the
		// input port sidebar between the node library and the canvas.
		if (nl && (! nl->isHidden()) && (! nl->isFloating()))
			splitDockWidget(nl, inputPortSidebar, Qt::Horizontal);

		else
			addDockWidget(Qt::LeftDockWidgetArea, inputPortSidebar);

		addDockWidget(Qt::RightDockWidgetArea, outputPortSidebar);

		this->setFocus();
	}

	else
	{
		// Prevent the docked node library from expanding to fill all available space when
		//  there are changes to the input port sidebar's visibility.
		if (! nl->isFloating())
		{
			nl->setMinimumWidth(nl->width());
			nl->setMaximumWidth(nl->minimumWidth());
		}
	}

	inputPortSidebar->setVisible(visible);
	outputPortSidebar->setVisible(visible);

	updatePublishedCableGeometry();
	inputPortSidebar->updatePortList();
	outputPortSidebar->updatePortList();

	if (!nl->isHidden() && !nl->isFloating())
		nl->fixWidth(visible);

	// Ensure port bounding boxes are large enough to include the antennae for hidden cables.
	composition->updateGeometryForAllComponents();

	updateUI();
}

/**
 * Prepares all published cables in the composition for geometry changes
 * and, if the published port sidebars are currently visible, updates the viewport.
 */
void VuoEditorWindow::updatePublishedCableGeometry()
{
	QGraphicsItem::CacheMode defaultCacheMode = composition->getCurrentDefaultCacheMode();

	set<VuoCable *> cables = composition->getBase()->getCables();
	foreach (VuoCable *cable, cables)
	{
		if (cable->isPublished())
		{
			cable->getRenderer()->setCacheMode(QGraphicsItem::NoCache);
			cable->getRenderer()->updateGeometry();
			cable->getRenderer()->setCacheMode(defaultCacheMode);
		}
	}

	// Updating the entire viewport here prevents published cable artifacts from
	// remaining behind, e.g., after scrolling, but (@todo:) handle this with
	// more precision.
	if (!inputPortSidebar->isHidden() || !outputPortSidebar->isHidden())
		ui->graphicsView->viewport()->update();

	composition->repaintFeedbackErrorMarks();
}

/**
 * Updates the composition's published input or output port list to match the provided list.
 */
void VuoEditorWindow::updatePublishedPortOrder(vector<VuoPublishedPort *> ports, bool isInput)
{
	// If the requested ordering is identical to the current ordering, do nothing.
	if (isInput && (ports == composition->getBase()->getPublishedInputPorts()))
		return;

	if (!isInput && (ports == composition->getBase()->getPublishedOutputPorts()))
		return;

	// If the requested change would place non-protocol ports above protocol ports, ignore the request.
	bool foundNonProtocolPort = false;
	foreach (VuoPublishedPort *port, ports)
	{
		if (!port->isProtocolPort())
			foundNonProtocolPort = true;
		else
		{
			if (foundNonProtocolPort)
				return;
		}
	}

	undoStack->push(new VuoCommandReorderPublishedPorts(ports, isInput, this));
}

/**
 * Displays the node library in the left docking area, taking care not to situate
 * it between the published input port sidebar and the canvas.
 */
void VuoEditorWindow::displayDockedNodeLibrary()
{
	addDockWidget(Qt::LeftDockWidgetArea, nl);

	bool publishedPortsDisplayed = (inputPortSidebar && (! inputPortSidebar->isHidden()));
	if (publishedPortsDisplayed)
		splitDockWidget(nl, inputPortSidebar, Qt::Horizontal);

	nl->setFloating(false);
	nl->fixWidth(publishedPortsDisplayed);
	nl->prepareAndMakeVisible();
	nl->setFocus();
}

/**
 * Update the display mode of the node class library to conform with global settings.
 */
void VuoEditorWindow::conformToGlobalNodeLibraryVisibility(VuoNodeLibrary::nodeLibraryState visibility, VuoNodeLibrary *floater, bool previousFloaterDestroyed)
{
	// Prevent the input port sidebar from expanding to fill all available space when there are changes
	// to the node library's docking status and/or visibility.
	inputPortSidebar->setMinimumWidth(inputPortSidebar->width());
	inputPortSidebar->setMaximumWidth(inputPortSidebar->minimumWidth());

	if (visibility == VuoNodeLibrary::nodeLibraryDocked)
	{
		releaseSurrogateNodeLibrary(previousFloaterDestroyed);
		displayDockedNodeLibrary();
		displayAppropriateDocumentation();
	}

	else if (visibility == VuoNodeLibrary::nodeLibraryFloating)
	{
		nl->fixWidth(false);

		// If our own node library was the one that initiated global
		// floating-node-library mode by being undocked, let it float.
		// It is now the single application-wide floating library.
		if (nl == floater)
		{
			if (ownedNodeLibrary == floater)
			{
				if (! nl->isFloating())
				{
					nl->setFloating(true);
				}

				nl->prepareAndMakeVisible();
				nl->setFocus();
				displayAppropriateDocumentation();
			}
		}

		// Otherwise, hide it and adopt the global floater as our own.
		else
		{
			ownedNodeLibrary->releaseDocumentationWidget();
			ownedNodeLibrary->setVisible(false);
			releaseSurrogateNodeLibrary(previousFloaterDestroyed);
			assignSurrogateNodeLibrary(floater);
		}
	}

	else if (visibility == VuoNodeLibrary::nodeLibraryHidden)
	{
		releaseSurrogateNodeLibrary(previousFloaterDestroyed);
		nl->setVisible(false);
	}

	updateUI();
}

/**
 * Displays the composition metadata in the node library documentation panel, if appropriate.
 * Otherwise, displays the documentation for the node class currently selected within the node library.
 */
void VuoEditorWindow::displayAppropriateDocumentation()
{
	// Only display the composition information in the metadata pane if the relevant composition metadata has been changed from the default,
	// or if the composition has been saved.
	bool compositionHasCustomizedDescription = (!composition->getBase()->getMetadata()->getDescription().empty() && (composition->getBase()->getMetadata()->getDescription() != VuoRendererComposition::deprecatedDefaultDescription));
	bool compositionHasCustomizedCopyright = (!composition->getBase()->getMetadata()->getCopyright().empty() && (composition->getBase()->getMetadata()->getCopyright() != generateCurrentDefaultCopyright()));
	bool compositionHasOtherCustomizedMetadata = (
													 !composition->getBase()->getMetadata()->getCustomizedName().empty() ||
													 !composition->getBase()->getMetadata()->getHomepageURL().empty() ||
													 !composition->getBase()->getMetadata()->getDocumentationURL().empty());
	bool compositionHasBeenSaved = !windowFilePath().isEmpty();
	if (compositionHasCustomizedDescription || compositionHasCustomizedCopyright || compositionHasOtherCustomizedMetadata || compositionHasBeenSaved)
	{
		dispatch_async(((VuoEditor *)qApp)->getDocumentationQueue(), ^{
						   QMetaObject::invokeMethod(nl, "displayPopoverInPane", Qt::QueuedConnection, Q_ARG(QWidget *, metadataPanel));
					   });
	}
	else
		nl->displayPopoverForCurrentNodeClass();
}

/**
 * Implement the Close action to close the composition.
 */
void VuoEditorWindow::closeEvent(QCloseEvent *event)
{
	// If already in the process of closing, don't redo work.
	if (closing)
	{
		event->accept();
		return;
	}

	// If an input editor or popup menu is open, ignore the user's click on the composition window's red X, since it leads to a crash.
	// https://b33p.net/kosada/node/15831
	if (inputEditorSession
	 || metadataEditor->isVisible()
	 || composition->getMenuSelectionInProgress()
	 || inputPortSidebar->getMenuSelectionInProgress()
	 || outputPortSidebar->getMenuSelectionInProgress())
	{
		event->ignore();
		return;
	}

	if (isWindowModified())
	{
		auto mb = new QMessageBox(this);
		mb->setWindowFlags(Qt::Sheet);
		mb->setWindowModality(Qt::WindowModal);

		//: Appears in the File > Close dialog.
		QString message = tr("Do you want to save the changes made to the document “%1”?").arg(getWindowTitleWithoutPlaceholder());
		//: Appears in the File > Close dialog.
		QString details = tr("Your changes will be lost if you don’t save them.");
		mb->setText(tr(message.toUtf8().constData()));
		mb->setStyleSheet("#qt_msgbox_informativelabel { font-weight: normal; font-size: 11pt; }");
		mb->setInformativeText(tr(details.toUtf8().constData()));
		mb->setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());

		mb->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		mb->setDefaultButton(QMessageBox::Save);

		// Give the "Discard" button keyboard focus (without "Default" status) so that it can be activated by spacebar.
		static_cast<QPushButton *>(mb->button(QMessageBox::Discard))->setAutoDefault(false);
		mb->button(QMessageBox::Discard)->setFocus();

		if (windowFilePath().isEmpty())
			mb->setButtonText(QMessageBox::Save, tr("Save As…"));

		connect(mb, &QMessageBox::buttonClicked, this, [=](QAbstractButton *button){
			auto editor = static_cast<VuoEditor *>(qApp);
			auto role = mb->buttonRole(button);
			if (role == QMessageBox::AcceptRole)
			{
				on_saveComposition_triggered();
				if (isWindowModified())
					editor->cancelQuit();
				else
				{
					acceptCloseEvent();
					editor->continueQuit(this);
				}
			}
			else if (role == QMessageBox::DestructiveRole)
			{
				acceptCloseEvent();
				editor->continueQuit(this);
			}
			else // if (role == QMessageBox::RejectRole)
				editor->cancelQuit();
		});

		mb->open();

		event->ignore();
	}
	else // if (! isWindowModified())
	{
		acceptCloseEvent();
		event->accept();
		static_cast<VuoEditor *>(qApp)->continueQuit(this);
	}
}

/**
 * Helper for @ref closeEvent.
 */
void VuoEditorWindow::acceptCloseEvent()
{
	if (composition->isRunning())
	{
		connect(composition, &VuoEditorComposition::stopFinished, this, &VuoEditorWindow::deleteLater);
		on_stopComposition_triggered();
	}
	else
		VuoEditorWindow::deleteLater();

	composition->disablePopovers();
	searchBox->close();

	closing = true;

	if (VuoFileUtilities::fileExists(windowFilePath().toStdString()))
		static_cast<VuoEditor *>(qApp)->addFileToRecentlyClosedList(windowFilePath());
}

/**
 * Enables or disables the 'Cut', 'Copy', and 'Delete' menu items and updates their text depending
 * what composition components from the canvas are currently selected.
 */
void VuoEditorWindow::updateSelectedComponentMenuItems()
{
	string cutCommandText = tr("Cut").toStdString();
	string copyCommandText = tr("Copy").toStdString();
	string duplicateCommandText = tr("Duplicate").toStdString();
	string deleteCommandText = tr("Delete").toStdString();
	string resetCommandText = tr("Reset").toStdString();

	QList<QGraphicsItem *> selectedCompositionComponents = composition->selectedItems();

	int selectedNodesFound = 0;
	int selectedCablesFound = 0;
	int selectedListsFound = 0;
	int selectedCommentsFound = 0;

	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin();
						(! (selectedNodesFound && selectedCablesFound)) &&
						(i != selectedCompositionComponents.end());
						++i)
	{
		QGraphicsItem *compositionComponent = *i;

		VuoRendererInputDrawer *rl = dynamic_cast<VuoRendererInputDrawer *>(compositionComponent);
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
		VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(compositionComponent);
		VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(compositionComponent);

		if (rl && !rl->paintingDisabled())
			selectedListsFound++;

		else if (rn && !rn->paintingDisabled())
			selectedNodesFound++;

		else if (rc && !rc->paintingDisabled())
			selectedCablesFound++;

		else if (rcomment)
			selectedCommentsFound++;
	}

	int distinctComponentTypesFound = 0;
	if (selectedListsFound)
		distinctComponentTypesFound++;
	if (selectedNodesFound)
		distinctComponentTypesFound++;
	if (selectedCablesFound)
		distinctComponentTypesFound++;
	if (selectedCommentsFound)
		distinctComponentTypesFound++;

	if (selectedListsFound && (distinctComponentTypesFound == 1))
	{
		string selectedListText = "Selected List";
		string selectedListsText = "Selected Lists";

		ui->cutCompositionComponents->setText(tr(cutCommandText.c_str()));
		ui->copyCompositionComponents->setText(tr(copyCommandText.c_str()));
		ui->duplicateCompositionComponents->setText(tr(duplicateCommandText.c_str()));

		if (selectedListsFound > 1)
		{
			ui->deleteCompositionComponents->setText(tr((resetCommandText + " " + selectedListsText).c_str()));
			composition->getContextMenuDeleteSelectedAction()->setText(tr((resetCommandText + " " + selectedListsText).c_str()));
		}
		else
		{
			ui->deleteCompositionComponents->setText(tr((resetCommandText + " " + selectedListText).c_str()));
			composition->getContextMenuDeleteSelectedAction()->setText(tr((resetCommandText + " " + selectedListText).c_str()));
		}
	}
	else
	{
		ui->cutCompositionComponents->setText(tr(cutCommandText.c_str()));
		ui->copyCompositionComponents->setText(tr(copyCommandText.c_str()));
		ui->duplicateCompositionComponents->setText(tr(duplicateCommandText.c_str()));
		ui->deleteCompositionComponents->setText(tr(deleteCommandText.c_str()));
		composition->getContextMenuDeleteSelectedAction()->setText(tr(deleteCommandText.c_str()));
	}

	bool enableSelectedComponentDeleteMenuItem = (!selectedCompositionComponents.isEmpty());
	bool enableSelectedComponentCutMenuItem = (!selectedCompositionComponents.isEmpty() &&
													!(selectedListsFound && (distinctComponentTypesFound == 1)));
	bool enableSelectedComponentDuplicateMenuItem = ((selectedNodesFound || selectedCommentsFound) &&
													!(selectedListsFound && (distinctComponentTypesFound == 1)));

	bool copyAppliesToNodeLibraryDocumentation = (nl && !nl->getSelectedDocumentationText().isEmpty());
	bool enableCopyMenuItem = (copyAppliesToNodeLibraryDocumentation || enableSelectedComponentDuplicateMenuItem);

	ui->cutCompositionComponents->setEnabled(enableSelectedComponentCutMenuItem);
	ui->copyCompositionComponents->setEnabled(enableCopyMenuItem);
	ui->duplicateCompositionComponents->setEnabled(enableSelectedComponentDuplicateMenuItem);
	ui->deleteCompositionComponents->setEnabled(enableSelectedComponentDeleteMenuItem);

	ui->renameNodes->setEnabled(selectedNodesFound);
	ui->refactor->setEnabled(selectedNodesFound || selectedCommentsFound);
	contextMenuTints->setEnabled(selectedNodesFound || selectedCommentsFound);
}

/**
 * Enables or disables the "Edit > Change (Node) To" menu item and updates its contents depending
 * what composition components from the canvas are currently selected.
 */
void VuoEditorWindow::updateChangeNodeMenu()
{
	QList<QGraphicsItem *> selectedCompositionComponents = composition->selectedItems();

	int selectedNodesFound = 0;
	int selectedCommentsFound = 0;

	VuoRendererNode *foundNode = NULL;

	for (QList<QGraphicsItem *>::iterator i = selectedCompositionComponents.begin();
						(selectedNodesFound <= 1) && !selectedCommentsFound &&
						(i != selectedCompositionComponents.end());
						++i)
	{
		QGraphicsItem *compositionComponent = *i;

		VuoRendererInputDrawer *rl = dynamic_cast<VuoRendererInputDrawer *>(compositionComponent);
		VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
		VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(compositionComponent);

		if (!rl && rn && !rn->paintingDisabled())
		{
			foundNode = rn;
			selectedNodesFound++;
		}

		else if (rcomment)
			selectedCommentsFound++;
	}

	// Update "Change (Node) To" submenu
	ui->menuEdit->removeAction(menuChangeNode->menuAction());

	bool menuChangeNodeDisplayed = false;
	if ((selectedNodesFound == 1) && !selectedCommentsFound)
	{
		composition->populateChangeNodeMenu(menuChangeNode, foundNode);
		if (!menuChangeNode->actions().isEmpty())
		{
			ui->menuEdit->insertMenu(ui->refactor, menuChangeNode);
			menuChangeNode->setEnabled(true);
			menuChangeNodeDisplayed = true;
		}
	}
	// Rather than including an empty submenu to indicate that this option is disabled,
	// display some disabled (gray) placeholder text in the parent menu.
	ui->changeNodePlaceholder->setVisible(!menuChangeNodeDisplayed);
}

/**
 * Performs a quick check to determine whether the clipboard
 * contains the text of a likely .vuo composition.
 */
bool VuoEditorWindow::containsLikelyVuoComposition(QString text)
{
	const QString compositionIndicatorText = "digraph";
	return (text.toCaseFolded().contains(compositionIndicatorText.toCaseFolded()));
}

/**
 * Ensures that the composition's current sceneRect includes the provided @c region.
 * If it doesn't already, updates it to do so.
 */
void VuoEditorWindow::ensureSceneRectContainsRegion(const QList<QRectF> &region)
{
	// Autoscroll during cable drags.
	if (composition->getCableInProgress())
	{
		updateSceneRect();
		return;
	}


	bool regionContained = true;
	QRectF sceneRect = composition->sceneRect();
	foreach (QRectF rect, region)
	{
		if (!sceneRect.contains(rect))
		{
			regionContained = false;
			break;
		}
	}

	if (!regionContained)
			updateSceneRect();

	return;
}

/**
 * Resizes the composition's sceneRect to accommodate the current scene items,
 * while never shrinking below the size of the viewport.
 */
void VuoEditorWindow::updateSceneRect()
{
	// Disable sceneRect updates while menu option selection is in progress so that
	// the scene doesn't shift underneath the menu (e.g., after a
	// cable drag autoscrolled the canvas).
	if (composition->getMenuSelectionInProgress())
		return;

	// Enforce a margin around each edge of the composition when possible.
	const int horizontalMargin = 0.2 * ui->graphicsView->geometry().width();
	const int verticalMargin = 0.2 * ui->graphicsView->geometry().height();

	int horizontalScrollBarHeight = ui->graphicsView->horizontalScrollBar()->sizeHint().height();
	int verticalScrollBarWidth = ui->graphicsView->verticalScrollBar()->sizeHint().width();

	QRectF viewportRect = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();
	QRectF itemsBoundingRect = composition->internalItemsBoundingRect().adjusted(-horizontalMargin, -verticalMargin, horizontalMargin, verticalMargin);
	QRectF viewportItemsUnionRect;

	// If the itemsBoundingRect is fully contained within the viewport (horizontally),
	// do not alter the sceneRect in this dimension.
	if (viewportRect.left() <= itemsBoundingRect.left() && viewportRect.right() >= itemsBoundingRect.right())
	{
		viewportItemsUnionRect.setLeft(viewportRect.left());
		viewportItemsUnionRect.setRight(viewportRect.right());
	}

	// Otherwise, if the discrepancy is greater at the left, align the sceneRect with the right edge
	// of the itemsBoundingRect to maximize stability.
	else if (abs(viewportRect.left() - itemsBoundingRect.left()) > abs(itemsBoundingRect.right() - viewportRect.right()))
	{
		viewportItemsUnionRect.setRight(itemsBoundingRect.right());
		viewportItemsUnionRect.setLeft(itemsBoundingRect.right() - max(itemsBoundingRect.width(), viewportRect.width()));
	}

	// If the discrepancy is greater at the right, align the sceneRect with the left edge
	// of the itemsBoundingRect to maximize stability.
	else
	{
		viewportItemsUnionRect.setLeft(itemsBoundingRect.left());
		viewportItemsUnionRect.setRight(itemsBoundingRect.left() + max(itemsBoundingRect.width(), viewportRect.width()));
	}

	// If the itemsBoundingRect is fully contained within the viewport (vertically),
	// do not alter the sceneRect in this dimension.
	if (viewportRect.top() <= itemsBoundingRect.top() && viewportRect.bottom() >= itemsBoundingRect.bottom())
	{
		viewportItemsUnionRect.setTop(viewportRect.top());
		viewportItemsUnionRect.setBottom(viewportRect.bottom());
	}

	// Otherwise, if the discrepancy is greater at the top, align the sceneRect with the bottom edge
	// of the itemsBoundingRect to maximize stability.
	else if (abs(viewportRect.top() - itemsBoundingRect.top()) > abs(itemsBoundingRect.bottom() - viewportRect.bottom()))
	{
		viewportItemsUnionRect.setBottom(itemsBoundingRect.bottom());
		viewportItemsUnionRect.setTop(itemsBoundingRect.bottom() - max(itemsBoundingRect.height(), viewportRect.height()));
	}

	// If the discrepancy is greater at the bottom, align the sceneRect with the top edge
	// of the itemsBoundingRect to maximize stability.
	else
	{
		viewportItemsUnionRect.setTop(itemsBoundingRect.top());
		viewportItemsUnionRect.setBottom(itemsBoundingRect.top() + max(itemsBoundingRect.height(), viewportRect.height()));
	}

	int viewportItemsUnionRectAdjustedWidth = viewportItemsUnionRect.width();
	int viewportItemsUnionRectAdjustedHeight = viewportItemsUnionRect.height();

	if ((viewportItemsUnionRect.width() > viewportRect.width()) &&
		(viewportItemsUnionRect.width() <= viewportRect.width() + verticalScrollBarWidth))
	{
		// Prevent the horizontal scrollbar from toggling into and out of existence as the scene is updated.
		viewportItemsUnionRectAdjustedWidth = viewportRect.width() + verticalScrollBarWidth + 1;

		// Prevent the introduction of the horizontal scrollbar from triggering the vertical scrollbar.
		viewportItemsUnionRectAdjustedHeight -= horizontalScrollBarHeight;
	}

	if ((viewportItemsUnionRect.height() > viewportRect.height()) &&
		(viewportItemsUnionRect.height() <= viewportRect.height() + horizontalScrollBarHeight))
	{
		// Prevent the vertical scrollbar from toggling into and out of existence as the scene is updated.
		viewportItemsUnionRectAdjustedHeight = viewportRect.height() + horizontalScrollBarHeight + 1;

		// Prevent the introduction of the vertical scrollbar from triggering the horizontal scrollbar.
		viewportItemsUnionRectAdjustedWidth -= verticalScrollBarWidth;
	}

	QRectF viewportItemsUnionRectAdjusted(viewportItemsUnionRect.topLeft().x(),
										  viewportItemsUnionRect.topLeft().y(),
										  viewportItemsUnionRectAdjustedWidth,
										  viewportItemsUnionRectAdjustedHeight);

	// Do not allow the scene to shrink while the left mouse button is held down
	// (e.g., while a node is being dragged).
	if ((QApplication::mouseButtons() & Qt::LeftButton))
		viewportItemsUnionRectAdjusted = viewportItemsUnionRectAdjusted.united(composition->sceneRect());
	else
		viewportItemsUnionRectAdjusted = viewportItemsUnionRectAdjusted.united(viewportRect);

	VuoCable *cableBeingDragged = composition->getCableInProgress();

	// Cable drags should not cause the scene to grow.
	if (!cableBeingDragged)
		composition->setSceneRect(viewportItemsUnionRectAdjusted);

	// Cable drags should, however, auto-scroll the viewport under certain circumstances.
	if (cableBeingDragged)
	{
		QPointF cableFloatingEndpointSceneLoc = cableBeingDragged->getRenderer()->getFloatingEndpointLoc();
		VuoRendererPort *cableFixedPort = (cableBeingDragged->getFromPort()?
											   cableBeingDragged->getFromPort()->getRenderer() :
											   cableBeingDragged->getToPort()?
												   cableBeingDragged->getToPort()->getRenderer() :
												   NULL);

		// Autoscroll should normally be disabled if the cursor is positioned above the
		// canvas or one of the published port sidebars.
		QRect inputPortSidebarRect = inputPortSidebar->geometry();
		inputPortSidebarRect.moveTopLeft(inputPortSidebar->parentWidget()->mapToGlobal(inputPortSidebarRect.topLeft()+QPoint(1,0)));

		QRect outputPortSidebarRect = outputPortSidebar->geometry();
		outputPortSidebarRect.moveTopLeft(outputPortSidebar->parentWidget()->mapToGlobal(outputPortSidebarRect.topLeft()));

		QRect viewportRectGlobal = ui->graphicsView->viewport()->geometry();
		viewportRectGlobal.moveTopLeft(ui->graphicsView->viewport()->parentWidget()->mapToGlobal(viewportRectGlobal.topLeft()));

		QRect noAutoscrollZone = QRect();
		if (!inputPortSidebar->isHidden())
			noAutoscrollZone |= inputPortSidebarRect;
		if (!outputPortSidebar->isHidden())
			noAutoscrollZone |= outputPortSidebarRect;
		if (!inputPortSidebar->isHidden() || !outputPortSidebar->isHidden())
			noAutoscrollZone |= viewportRectGlobal;

		QPoint cursorPosition = QCursor::pos();
		bool cursorWithinNoAutoscrollZone = noAutoscrollZone.contains(cursorPosition);
		bool cursorWithinNoAutoscrollZoneLeft = (cursorWithinNoAutoscrollZone &&
												 ((cursorPosition.x() - noAutoscrollZone.left()) < (noAutoscrollZone.right() - cursorPosition.x())));

		// Make an exception and allow the autoscroll if it would bring the cable's
		// connected node closer to being back within the viewport.
		bool autoscrollWouldBringSourceNodeCloser = false;
		if (cableFixedPort && !dynamic_cast<VuoRendererPublishedPort *>(cableFixedPort))
		{
			VuoRendererNode *fixedNode = cableFixedPort->getUnderlyingParentNode();
			QRectF fixedNodeBoundingRect = fixedNode->boundingRect().adjusted(-horizontalMargin, -verticalMargin, horizontalMargin, verticalMargin);
			bool fixedNodeVisible = ((fixedNodeBoundingRect.left()+fixedNode->scenePos().x() >= viewportRect.left()) &&
								(fixedNodeBoundingRect.right()+fixedNode->scenePos().x() <= viewportRect.right()));

			autoscrollWouldBringSourceNodeCloser = (!fixedNodeVisible && (cursorWithinNoAutoscrollZoneLeft ==
												   (fixedNodeBoundingRect.left()+fixedNode->scenePos().x() < viewportRect.left())));
		}

		bool enableCableDragAutoscroll = (!cursorWithinNoAutoscrollZone || autoscrollWouldBringSourceNodeCloser);
		if (enableCableDragAutoscroll)
		{
			ui->graphicsView->ensureVisible(cableFloatingEndpointSceneLoc.x(),
											cableFloatingEndpointSceneLoc.y(),
											0,
											VuoEditorWindow::compositionMargin);
		}
	}
}

/**
 * Enables and disables selection of the various types of composition items in response
 * to the beginning and end of rubberband drags.
 */
void VuoEditorWindow::updateRubberBandSelectionMode(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint)
{
	bool rubberBandSelectionPreviouslyInProgress = this->rubberBandSelectionInProgress;
	bool rubberBandSelectionNowInProgress = !(rubberBandRect.isNull() && fromScenePoint.isNull() && toScenePoint.isNull());

	bool beginningRubberBandSelection = (!rubberBandSelectionPreviouslyInProgress && rubberBandSelectionNowInProgress);
	bool endingRubberBandSelection = (rubberBandSelectionPreviouslyInProgress && !rubberBandSelectionNowInProgress);

	if (beginningRubberBandSelection)
	{
		// Disable selection of comment bodies (non-title-handle areas) during rubberband drags.
		foreach (VuoComment *comment, composition->getBase()->getComments())
			if (comment->hasRenderer())
				comment->getRenderer()->setBodySelectable(false);

		// Disable selection of cables during rubberband drags, unless the relevant keyboard modifier was pressed.
		if (!lastLeftMousePressHadOptionModifier)
		{
			foreach (VuoCable *cable, composition->getBase()->getCables())
				if (cable->hasRenderer())
					cable->getRenderer()->setSelectable(false);
		}
	}
	else if (endingRubberBandSelection)
	{
		foreach (VuoComment *comment, composition->getBase()->getComments())
			if (comment->hasRenderer())
				comment->getRenderer()->setBodySelectable(true);

		if (!lastLeftMousePressHadOptionModifier)
		{
			foreach (VuoCable *cable, composition->getBase()->getCables())
				if (cable->hasRenderer())
					cable->getRenderer()->setSelectable(true);
		}
	}

	this->rubberBandSelectionInProgress = rubberBandSelectionNowInProgress;
}

/**
 * Sets the constant for the provided @c port to the provided @c value.
 */
void VuoEditorWindow::setPortConstant(VuoRendererPort *port, string value)
{
	if (value != port->getConstantAsString())
	{
		VuoCompilerInputEventPort *eventPort = dynamic_cast<VuoCompilerInputEventPort *>(port->getBase()->getCompiler());
		if (eventPort)
			undoStack->push(new VuoCommandSetPortConstant(eventPort, value, this));
	}
}

/**
 * Displays an input editor for an input port's constant value.
 */
void VuoEditorWindow::showInputEditor(VuoRendererPort *port)
{
	showInputEditor(port, true);
}

/**
 * Displays an input editor for an input port's constant value.
 */
void VuoEditorWindow::showInputEditor(VuoRendererPort *port, bool forwardTabTraversal)
{
	VuoPublishedPortSidebar *sidebar = nullptr;
	VuoRendererPublishedPort *publishedPort = dynamic_cast<VuoRendererPublishedPort *>(port);
	if (publishedPort)
		sidebar = (publishedPort->getInput() ? outputPortSidebar : inputPortSidebar);

	inputEditorSession = new VuoInputEditorSession(inputEditorManager, composition, sidebar, this);
	map<VuoRendererPort *, pair<string, string> > originalAndFinalValueForPort = inputEditorSession->execute(port, forwardTabTraversal);

	delete inputEditorSession;
	inputEditorSession = nullptr;

	bool startedUndoStackMacro = false;
	for (auto i : originalAndFinalValueForPort)
	{
		VuoRendererPort *port = i.first;
		string originalEditingSessionValue = i.second.first;
		string finalEditingSessionValue = i.second.second;

		// Now that the editing session is over, check whether the port constant ended up with
		// a different value than it had initially. If so, push the change onto the Undo stack.
		if (finalEditingSessionValue != originalEditingSessionValue)
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getBase()->getCompiler());

			// Briefly set the constant back to its original value so that the Undo stack
			// has the information it needs to revert it properly.  An alternative would be to
			// modify the VuoCommandSetPortConstant constructor to accept a revertedValue argument.
			composition->updatePortConstant(compilerPort, originalEditingSessionValue, false);

			if (!startedUndoStackMacro)
			{
				undoStack->beginMacro(tr("Set Port Constant"));
				startedUndoStackMacro = true;
			}

			undoStack->push(new VuoCommandSetPortConstant(compilerPort, finalEditingSessionValue, this));

			composition->performStructuralChangesAfterValueChangeAtPort(this, undoStack, port, originalEditingSessionValue, finalEditingSessionValue);
		}

		// If the port constant ended up with the same value as it had initially
		// (either because the user dismissed the editor by pressing 'Esc' or
		// re-committed the initial value), do not push the change onto the Undo
		// stack, but do revert to the original value if there were intermediate
		// changes while the input editor was open.
		else if (port->getConstantAsString() != originalEditingSessionValue)
		{
			VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getBase()->getCompiler());
			composition->updatePortConstant(compilerPort, originalEditingSessionValue);
		}
	}

	if (startedUndoStackMacro)
		undoStack->endMacro();
}

/**
 * Displays an input editor for a node's title.
 */
void VuoEditorWindow::showNodeTitleEditor(VuoRendererNode *node)
{
	if (! node->getBase()->hasCompiler())
		return;

	composition->setPopoverEventsEnabled(false);

	VuoTitleEditor *titleEditor = new VuoTitleEditor();
	string originalValue = node->getBase()->getTitle();

	// Position the input editor overtop the node's title.
	QRectF nodeBoundingRect = node->boundingRect();
	QPointF nodeTitleRightCenterInScene = node->scenePos() + nodeBoundingRect.topRight() + QPointF(0, VuoRendererNode::nodeTitleHeight/2.);
	QPoint nodeTitleRightCenterInView = ui->graphicsView->mapFromScene(nodeTitleRightCenterInScene);
	QPoint nodeTitleRightCenterGlobal = ui->graphicsView->mapToGlobal(nodeTitleRightCenterInView);

	// Set the input editor's width to match the node's.
	int nodeWidth = node->boundingRect().width();
	int scaledWidth = ui->graphicsView->transform().m11() * nodeWidth;
	titleEditor->setWidth(scaledWidth);

	json_object *details = json_object_new_object();
	VuoEditor *editor = (VuoEditor *)qApp;
	json_object_object_add(details, "isDark", json_object_new_boolean(editor->isInterfaceDark()));

	json_object *originalValueAsJson = json_object_new_string(originalValue.c_str());
	json_object *newValueAsJson = titleEditor->show(nodeTitleRightCenterGlobal, originalValueAsJson, details);
	string newValue = json_object_get_string(newValueAsJson);
	json_object_put(originalValueAsJson);
	json_object_put(newValueAsJson);

	// If the user has deleted the title, revert to the node class default title.
	if (newValue == "")
		newValue = node->getBase()->getNodeClass()->getDefaultTitleWithoutSuffix();

	if (newValue != originalValue)
		undoStack->push(new VuoCommandSetNodeTitle(node->getBase()->getCompiler(), newValue, this));

	composition->setPopoverEventsEnabled(true);
}

/**
 * Displays an input editor for a comment's text content.
 */
void VuoEditorWindow::showCommentEditor(VuoRendererComment *comment)
{
	string originalText = comment->getBase()->getContent();

	json_object *details = json_object_new_object();
	VuoEditor *editor = (VuoEditor *)qApp;
	json_object_object_add(details, "isDark", json_object_new_boolean(editor->isInterfaceDark()));

	json_object *originalTextAsJson = json_tokener_parse(originalText.c_str());

	VuoCommentEditor *commentEditor = new VuoCommentEditor();

	// Set the text editor's width to match the comment's.
	int commentWidth = comment->boundingRect().width();
	int scaledWidth = ui->graphicsView->transform().m11() * commentWidth;
	commentEditor->setWidth(scaledWidth);

	// Set the text editor's height to match the comment's.
	int commentHeight= comment->boundingRect().height();
	int scaledHeight = ui->graphicsView->transform().m11() * commentHeight;
	commentEditor->setHeight(scaledHeight);

	// Position the text editor overtop the comment.
	QRectF commentBoundingRect = comment->boundingRect();
	QPointF commentTextRightCenterInScene = comment->scenePos() + commentBoundingRect.topRight() + QPointF(0, commentBoundingRect.height()/2.0);
	QPoint commentTextRightCenterInView = ui->graphicsView->mapFromScene(commentTextRightCenterInScene);
	QPoint commentTextRightCenterGlobal = ui->graphicsView->mapToGlobal(commentTextRightCenterInView);

	json_object *newTextAsJson = commentEditor->show(commentTextRightCenterGlobal, originalTextAsJson, details);

	string newText = json_object_to_json_string_ext(newTextAsJson, JSON_C_TO_STRING_PLAIN);
	json_object_put(originalTextAsJson);
	json_object_put(newTextAsJson);

	if (newText != originalText)
		undoStack->push(new VuoCommandSetCommentText(comment, newText, this));
}

/**
 * Zooms to fit the viewport to the provided comment.
 */
void VuoEditorWindow::zoomToFitComment(VuoRendererComment *comment)
{
	comment->setSelected(true);
	on_zoomToFit_triggered();
}

/**
 * Renames a published port.
 */
void VuoEditorWindow::changePublishedPortName(VuoRendererPublishedPort *port, string newName, bool useUndoStack)
{
	if (useUndoStack)
		undoStack->push(new VuoCommandSetPublishedPortName(port, newName, this));
	else
		composition->setPublishedPortName(port, newName);
}

/**
 * Changes the details (suggested min/max, etc.) associated with a published port.
 */
void VuoEditorWindow::changePublishedPortDetails(VuoRendererPublishedPort *port, json_object *newDetails)
{
	undoStack->push(new VuoCommandSetPublishedPortDetails(dynamic_cast<VuoRendererPublishedPort *>(port), newDetails, this));
}

/**
 * Opens an editing session for the source code associated with the provided editable @c node.
 */
void VuoEditorWindow::openEditableSourceForNode(VuoRendererNode *node)
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
	if (!VuoEditorUtilities::isNodeClassEditable(nodeClass, actionText, sourcePath))
		return;

	// If the node is an installed subcomposition or shader, open its source for editing within Vuo.
	if (nodeClass->getCompiler()->isSubcomposition() || nodeClass->getCompiler()->isIsf())
	{
		QMainWindow *window = static_cast<VuoEditor *>(qApp)->openFileWithName(sourcePath, false);
		if (nodeClass->getCompiler()->isSubcomposition())
		{
			VuoEditorWindow *compositionWindow = dynamic_cast<VuoEditorWindow *>(window);
			if (compositionWindow)
				static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->linkSubcompositionToNodeInSupercomposition(compositionWindow->getComposition(), getComposition(), node);
		}
	}
	// If the node has other editable source code, open it in the appropriate application.
	else
		static_cast<VuoEditor *>(qApp)->openUrl("file://" + sourcePath);
}

/**
 * Sets the boolean indicating whether this file should be added to the
 * "File > Open Recent" list when saved.
 */
void VuoEditorWindow::setIncludeInRecentFileMenu(bool include)
{
	this->includeInRecentFileMenu = include;
}

/**
 * Inserts an empty comment node near the current (fitted) location of the cursor and
 * opens it for editing.
 */
void VuoEditorWindow::on_insertComment_triggered()
{
	insertCommentAtPos(getFittedScenePos(getCursorScenePos()));
}

/**
 * Inserts an empty comment at the provided scene position.
 */
void VuoEditorWindow::insertCommentAtPos(QPointF targetScenePos)
{
	// Create an empty comment.
	VuoRendererComment *comment = composition->createRendererComment(
									  (new VuoCompilerComment(new VuoComment("",
																			 targetScenePos.x(),
																			 targetScenePos.y(),
																			 4*VuoRendererComposition::majorGridLineSpacing,
																			 2*VuoRendererComposition::majorGridLineSpacing
																			 )))->getBase());

	// Add the comment to the composition.
	QList<QGraphicsItem *> componentsToAdd = QList<QGraphicsItem *>();
	componentsToAdd.append(comment);
	undoStack->push(new VuoCommandAdd(componentsToAdd, this, "Insert Comment"));

	// Open the comment immediately for editing.
	showCommentEditor(comment);
}

/**
 * Inserts an empty subcomposition node near the current (fitted) location of the cursor.
 */
void VuoEditorWindow::on_insertSubcomposition_triggered()
{
	insertSubcompositionAtPos(getFittedScenePos(getCursorScenePos()-
												QPointF(0,VuoRendererNode::nodeHeaderYOffset)));
}

/**
 * Checks whether this composition has been saved to the filesystem.
 * If not, prompts the user to save the composition before it can have local subcompositions installed,
 * and if the user chooses to proceed, presents the "Save As" dialogue.
 * Returns a boolean indicating whether the composition has been saved to the filesystem
 * when all is said and done.
 */
bool VuoEditorWindow::ensureThisParentCompositionSaved()
{
	if (!VuoFileUtilities::fileExists(windowFilePath().toUtf8().constData()))
	{
		QMessageBox messageBox(this);
		messageBox.setWindowFlags(Qt::Sheet);
		messageBox.setWindowModality(Qt::WindowModal);
		messageBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Save);
		messageBox.setDefaultButton(QMessageBox::Save);
		messageBox.setStyleSheet("#qt_msgbox_informativelabel, QMessageBoxDetailsText { font-weight: normal; font-size: 11pt; }");
		messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());
		messageBox.setText(tr("Save before packaging"));
		messageBox.setInformativeText(tr("Please save this composition so Vuo knows where to save your new subcomposition."));
		messageBox.setButtonText(QMessageBox::Save, tr("Save As…"));

		// Give the non-default button keyboard focus so that it can be activated by spacebar.
		static_cast<QPushButton *>(messageBox.button(QMessageBox::Cancel))->setAutoDefault(false);
		messageBox.button(QMessageBox::Cancel)->setFocus();

		// Give the user a chance to cancel subcomposition installation without saving.
		if (messageBox.exec() != QMessageBox::Save)
			return false;

		// Present the "Save As" dialogue for the user to save the parent composition.
		on_saveCompositionAs_triggered();
	}

	return VuoFileUtilities::fileExists(windowFilePath().toUtf8().constData());
}

/**
 * Inserts an empty subcomposition node at the provided scene position.
 */
void VuoEditorWindow::insertSubcompositionAtPos(QPointF targetScenePos)
{
	VUserLog("%s:      Insert subcomposition {", getWindowTitleWithoutPlaceholder().toUtf8().data());

	// If the parent composition has not been saved to the filesystem, prompt the user to do so now
	// so that the subcomposition can be installed in a composition-local Modules directory.
	if (!ensureThisParentCompositionSaved())
	{
		VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
		return;
	}

	string subcompositionContent = "digraph G {}";
	VuoEditorWindow *subcompositionWindow = static_cast<VuoEditor *>(qApp)->newCompositionWithContent(subcompositionContent);
	if (!subcompositionWindow)
	{
		VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
		return;
	}
	subcompositionWindow->setIncludeInRecentFileMenu(false);
	subcompositionWindow->showPublishedPortSidebars();
	subcompositionWindow->setAsActiveWindow();

	string nodeClassName = subcompositionWindow->installSubcomposition(windowFilePath().toUtf8().constData());
	if (!nodeClassName.empty())
	{
		VuoModuleManager::CallbackType subcompositionAdded = ^void (void) {

			VuoRendererNode *subcompositionNode = composition->createNode(nodeClassName.c_str(), "",
																		  targetScenePos.x(),
																		  targetScenePos.y());
			if (!subcompositionNode)
			{
				VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
				return;
			}

			static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->linkSubcompositionToNodeInSupercomposition(subcompositionWindow->getComposition(),
																												  getComposition(),
																												  subcompositionNode);

			// Add the new node to the composition.
			QList<QGraphicsItem *> componentsToAdd = QList<QGraphicsItem *>();
			componentsToAdd.append(subcompositionNode);
			undoStack->push(new VuoCommandAdd(componentsToAdd, this, "Insert Subcomposition"));

			VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());

			// Highlight the new node class in the node library.
			VuoNodeLibrary *nl = composition->getModuleManager()->getNodeLibrary();
			if (nl)
				nl->highlightNodeClass(nodeClassName);
		};

		// The order of the following two lines matters -- the second callback registered
		// for a given module manager and node class replaces the first.
		static_cast<VuoEditor *>(qApp)->highlightNewNodeClassInAllLibraries(nodeClassName);
		composition->getModuleManager()->doNextTimeNodeClassIsLoaded(nodeClassName, subcompositionAdded);
	}
	else
		VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
}

/**
 * Refactors the selected comments, nodes, and the nodes' internal cables into a subcomposition.
 */
void VuoEditorWindow::refactorSelectedItems()
{
	VUserLog("%s:      Package as subcomposition {", getWindowTitleWithoutPlaceholder().toUtf8().data());

	// If the parent composition has not been saved to the filesystem, prompt the user to do so now
	// so that the subcomposition can be installed in a composition-local Modules directory.
	if (!ensureThisParentCompositionSaved())
	{
		VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
		return;
	}

	string subcompositionContent = getMaximumSubcompositionFromSelection(false, false);

	QPoint selectedItemsAvgPos = QPoint(0,0);
	int selectedItemCount = 0;

	set<string> portsToPublish;
	__block map<int, string> inputCablesToRestoreFromPort;
	__block map<int, string> inputCablesToRestoreToPort;
	__block map<int, bool> inputCablesToRestoreEventOnly;
	__block map<int, string> outputCablesToRestoreFromPort;
	__block map<int, string> outputCablesToRestoreToPort;
	__block map<int, bool> outputCablesToRestoreEventOnly;

	QList<QGraphicsItem *> selectedItems = composition->selectedItems();
	foreach (QGraphicsItem *item, selectedItems)
	{
		VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(item);
		if (node)
		{
			// Determine which input ports to publish.
			foreach (VuoPort *port, node->getBase()->getInputPorts())
			{
				if (!dynamic_cast<VuoRendererTypecastPort *>(port->getRenderer()))
				{
					vector<VuoCable *> incomingCables = port->getConnectedCables(true);
					foreach (VuoCable *cable, incomingCables)
					{
						bool fromNodeIsExternal = (cable->getFromNode()->hasRenderer() &&
												   !cable->getFromNode()->getRenderer()->isSelected());
						if (fromNodeIsExternal || cable->isPublishedInputCable())
						{
							portsToPublish.insert(composition->getIdentifierForStaticPort(cable->getToPort()));

							int cableNum = inputCablesToRestoreEventOnly.size();
							inputCablesToRestoreFromPort[cableNum] = composition->getIdentifierForStaticPort(cable->getFromPort());
							inputCablesToRestoreToPort[cableNum] = composition->getIdentifierForStaticPort(cable->getToPort());
							inputCablesToRestoreEventOnly[cableNum] = cable->getCompiler()->getAlwaysEventOnly();
						}
					}
				} // end if not an attached typecast
				else // if attached typecast
				{
					VuoRendererNode *typecastNode = dynamic_cast<VuoRendererTypecastPort *>(port->getRenderer())->getUncollapsedTypecastNode();
					VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
					vector<VuoCable *> incomingCables = typecastInPort->getConnectedCables(true);

					// If the input port has an attached typecast, determine whether the typecast's incoming cables will be
					// internal or external to the refactored subcomposition, or some combination of internal and external.
					bool typecastHasInternalInput = false;
					bool typecastHasExternalInput = false;
					foreach (VuoCable *typecastInCable, incomingCables)
					{
						if ((typecastInCable->getFromNode() && typecastInCable->getFromNode()->hasRenderer() &&
							  typecastInCable->getFromNode()->getRenderer()->isSelected()))
							typecastHasInternalInput = true;
						else
							typecastHasExternalInput = true;
					}

					// Case: All of the typecast’s incoming cables will be external.
					// The typecast’s host port needs to be published and reconnected to the soon-to-be-external typecast’s output port.
					// The soon-to-be-external typecast needs to be uncollapsed.
					if (!typecastHasInternalInput)
					{
						VuoPort *typecastOutPort = typecastNode->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
						VuoPort *typecastHostPort = port;

						portsToPublish.insert(composition->getIdentifierForStaticPort(typecastHostPort));

						int cableNum = inputCablesToRestoreEventOnly.size();
						inputCablesToRestoreFromPort[cableNum] = composition->getIdentifierForStaticPort(typecastOutPort);
						inputCablesToRestoreToPort[cableNum] = composition->getIdentifierForStaticPort(typecastHostPort);
						inputCablesToRestoreEventOnly[cableNum] = false;

						composition->uncollapseTypecastNode(typecastNode);
					}

					// Case: Some of the typecast's incoming cables will be external, some internal.
					// The soon-to-be internal typecast's child port needs to be published and reconnected to its soon-to-be external input sources.
					else if (typecastHasExternalInput && typecastHasInternalInput)
					{
						portsToPublish.insert(composition->getIdentifierForStaticPort(typecastInPort));

						foreach (VuoCable *typecastInCable, incomingCables)
						{
							bool externalInput = !(typecastInCable->getFromNode() && typecastInCable->getFromNode()->hasRenderer() &&
								  typecastInCable->getFromNode()->getRenderer()->isSelected());
							if (externalInput)
							{
								int cableNum = inputCablesToRestoreEventOnly.size();
								inputCablesToRestoreFromPort[cableNum] = composition->getIdentifierForStaticPort(typecastInCable->getFromPort());
								inputCablesToRestoreToPort[cableNum] = composition->getIdentifierForStaticPort(typecastInPort);
								inputCablesToRestoreEventOnly[cableNum] = false;
							}
						}
					}

					// Case: All of the typecast's incoming cables will be internal.
					// The soon-to-be-internal typecast need not have either its child port or its host port published, nor any
					// external connections restored.
					//else if (!typecastHasExternalInput)
					//{}
				}
			}

			// Determine which output ports to publish.
			foreach (VuoPort *port, node->getBase()->getOutputPorts())
			{
				foreach (VuoCable *cable, port->getConnectedCables(true))
				{
					bool toNodeIsExternal = (cable->getToNode()->hasRenderer() &&
											  !dynamic_cast<VuoRendererTypecastPort *>(cable->getToPort()->getRenderer()->getTypecastParentPort()) &&
											  !cable->getToNode()->getRenderer()->isSelected());
					bool connectedTypecastIsExternal = false;
					if (dynamic_cast<VuoRendererTypecastPort *>(cable->getToPort()->getRenderer()->getTypecastParentPort()))
					{
						VuoRendererNode *typecastNode = dynamic_cast<VuoRendererTypecastPort *>(cable->getToPort()->getRenderer()->getTypecastParentPort())->getUncollapsedTypecastNode();
						VuoPort *typecastOutPort = typecastNode->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
						vector<VuoCable *> outCables = typecastOutPort->getConnectedCables(true);

						connectedTypecastIsExternal = ((outCables.size() < 1) || !outCables[0]->getToNode() ||
															 !outCables[0]->getToNode()->hasRenderer() ||
															 !outCables[0]->getToNode()->getRenderer()->isSelected());
						if (connectedTypecastIsExternal)
							composition->uncollapseTypecastNode(typecastNode);
					}

					if (toNodeIsExternal || connectedTypecastIsExternal || cable->isPublishedOutputCable())
					{
						portsToPublish.insert(composition->getIdentifierForStaticPort(cable->getFromPort()));

						int cableNum = outputCablesToRestoreEventOnly.size();
						outputCablesToRestoreFromPort[cableNum] = composition->getIdentifierForStaticPort(cable->getFromPort());
						outputCablesToRestoreToPort[cableNum] = composition->getIdentifierForStaticPort(cable->getToPort());
						outputCablesToRestoreEventOnly[cableNum] = cable->getCompiler()->getAlwaysEventOnly();
					}
				}
			}

			selectedItemsAvgPos += item->scenePos().toPoint();
			selectedItemCount++;
		}

		else if (dynamic_cast<VuoRendererComment *>(item))
		{
			selectedItemsAvgPos += item->scenePos().toPoint();
			selectedItemCount++;
		}
	}

	selectedItemsAvgPos /= selectedItemCount;

	string parentCompositionPath = windowFilePath().toStdString();
	string subcompositionDir = VuoFileUtilities::getCompositionLocalModulesPath(parentCompositionPath);

	VuoEditorWindow *subcompositionWindow = static_cast<VuoEditor *>(qApp)->newCompositionWithContent(subcompositionContent, subcompositionDir);
	subcompositionWindow->setIncludeInRecentFileMenu(false);
	__block map<string, string> publishedPortNames = subcompositionWindow->getComposition()->publishPorts(portsToPublish);
	subcompositionWindow->showPublishedPortSidebars();
	subcompositionWindow->setAsActiveWindow();

	string nodeClassName = subcompositionWindow->installSubcomposition(parentCompositionPath);
	if (!nodeClassName.empty())
	{
		VuoModuleManager::CallbackType subcompositionAdded = ^void (void) {

		VuoRendererNode *subcompositionNode = composition->createNode(nodeClassName.c_str(), "",
																	  selectedItemsAvgPos.x(),
																	  selectedItemsAvgPos.y());
		if (!subcompositionNode)
		{
			composition->collapseTypecastNodes();
			VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
			return;
		}

		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->linkSubcompositionToNodeInSupercomposition(subcompositionWindow->getComposition(),
																												getComposition(),
																												subcompositionNode);

		undoStack->beginMacro(tr("Package as Subcomposition"));

		// Remove the selected comments, nodes, and their internal connections.
		QList<QGraphicsItem *> itemsToRemove;
		foreach (VuoRendererNode *node, composition->getSelectedNodes())
			itemsToRemove.append(node);
		foreach (VuoRendererComment *comment, composition->getSelectedComments())
			itemsToRemove.append(comment);

		undoStack->push(new VuoCommandRemove(itemsToRemove, this, inputEditorManager, "Package as Subcomposition", true));

		// Add the refactored subcomposition node.
		QList<QGraphicsItem *> componentsToAdd = QList<QGraphicsItem *>();
		componentsToAdd.append(subcompositionNode);
		undoStack->push(new VuoCommandAdd(componentsToAdd, this, "", true));

		// Restore external connections.
		QList<QGraphicsItem *> inputCablesToRestore;
		for (int i = 0; i < inputCablesToRestoreEventOnly.size(); ++i)
		{
			VuoPort *fromPort = composition->getPortWithStaticIdentifier(inputCablesToRestoreFromPort[i]);
			VuoNode *fromNode = (fromPort? composition->getUnderlyingParentNodeForPort(fromPort, composition) : NULL);
			VuoPort *toPort = subcompositionNode->getBase()->getInputPortWithName(publishedPortNames[inputCablesToRestoreToPort[i] ]);
			VuoNode *toNode = subcompositionNode->getBase();

			if (fromNode && fromPort && toNode && toPort &&
					toNode->hasRenderer() && (toNode->getRenderer()->scene() == composition))
			{
				// Restore internal input cables.
				if (fromNode->hasRenderer() && (fromNode->getRenderer()->scene() == composition) &&
						(fromNode != composition->getPublishedInputNode()))
				{
					VuoRendererCable *cable = new VuoRendererCable((new VuoCompilerCable(fromNode->getCompiler(),
																						 static_cast<VuoCompilerPort *>(fromPort->getCompiler()),
																						 toNode->getCompiler(),
																						 static_cast<VuoCompilerPort *>(toPort->getCompiler())))->getBase());
					cable->getBase()->getCompiler()->setAlwaysEventOnly(inputCablesToRestoreEventOnly[i]);
					inputCablesToRestore.append(cable);
				}
				// Restore published input cables.
				else if (fromNode == composition->getPublishedInputNode())
				{
					undoStack->push(new VuoCommandPublishPort(toPort,
															  NULL,
															  this,
															  inputCablesToRestoreEventOnly[i],
															  inputCablesToRestoreFromPort[i],
															  true
															  ));
				}
			}
		}

		QList<QGraphicsItem *> outputCablesToRestore;
		for (int i = 0; i < outputCablesToRestoreEventOnly.size(); ++i)
		{
			VuoPort *fromPort = subcompositionNode->getBase()->getOutputPortWithName(publishedPortNames[outputCablesToRestoreFromPort[i] ]);
			VuoNode *fromNode = subcompositionNode->getBase();
			VuoPort *toPort = composition->getPortWithStaticIdentifier(outputCablesToRestoreToPort[i]);
			VuoNode *toNode = (toPort? composition->getUnderlyingParentNodeForPort(toPort, composition) : NULL);

			if (fromNode && fromPort && toNode && toPort &&
					fromNode->hasRenderer() && (fromNode->getRenderer()->scene() == composition))
			{
				// Restore internal output cables.
				if (toNode->hasRenderer() && (toNode->getRenderer()->scene() == composition) &&
						(toNode != composition->getPublishedOutputNode()))
				{
					VuoRendererCable *cable = new VuoRendererCable((new VuoCompilerCable(fromNode->getCompiler(),
																						 static_cast<VuoCompilerPort *>(fromPort->getCompiler()),
																						 toNode->getCompiler(),
																						 static_cast<VuoCompilerPort *>(toPort->getCompiler())))->getBase());
					cable->getBase()->getCompiler()->setAlwaysEventOnly(outputCablesToRestoreEventOnly[i]);
					outputCablesToRestore.append(cable);
				}
				// Restore published output cables.
				else if (toNode == composition->getPublishedOutputNode())
				{
					undoStack->push(new VuoCommandPublishPort(fromPort,
															  NULL,
															  this,
															  outputCablesToRestoreEventOnly[i],
															  outputCablesToRestoreToPort[i],
															  true
															  ));
				}
			}
		}

		undoStack->push(new VuoCommandAdd(inputCablesToRestore, this));
		undoStack->push(new VuoCommandAdd(outputCablesToRestore, this));

		// Note the refactoring in the composition diff.
		{
			string compositionIdentifier = static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->getCompositionIdentifier(composition);

			set<VuoCompilerNode *> nodesMoved;
			for (QGraphicsItem *item : itemsToRemove)
			{
				VuoRendererNode *node = dynamic_cast<VuoRendererNode *>(item);
				if (node)
					nodesMoved.insert(node->getBase()->getCompiler());
			}

			VuoCompilerNode *subcompositionMovedTo = subcompositionNode->getBase()->getCompiler();

			string compositionSnapshot = composition->takeSnapshot();

			VuoCompilerCompositionDiff *diffInfo = new VuoCompilerCompositionDiff();
			diffInfo->addRefactoring(compositionIdentifier, nodesMoved, subcompositionMovedTo);
			coalesceSnapshots(compositionSnapshot, compositionSnapshot, diffInfo);
		}

		undoStack->endMacro();
		composition->collapseTypecastNodes();

			VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());

			// Highlight the new node class in the node library.
			VuoNodeLibrary *nl = composition->getModuleManager()->getNodeLibrary();
			if (nl)
				nl->highlightNodeClass(nodeClassName);
		};

		// The order of the following two lines matters -- the second callback registered
		// for a given module manager and node class replaces the first.
		static_cast<VuoEditor *>(qApp)->highlightNewNodeClassInAllLibraries(nodeClassName);
		composition->getModuleManager()->doNextTimeNodeClassIsLoaded(nodeClassName, subcompositionAdded);
	}

	// Case: Subcomposition installation failed or was cancelled.
	else // if (nodeClassName.empty())
		{
			// Re-collapse any typecasts that we uncollapsed during attempted refactoring.
			composition->collapseTypecastNodes();

			VUserLog("%s:      }", getWindowTitleWithoutPlaceholder().toUtf8().data());
		}
}

/**
* Returns the action associated with raising the current editor window to the foreground.
*/
QAction * VuoEditorWindow::getRaiseDocumentAction() const
{
	return raiseDocumentAction;
}

/**
 * Returns the Show Events action.
 */
QAction *VuoEditorWindow::getShowEventsAction() const
{
	return ui->showEvents;
}

/**
 * Returns a zoom action.
 */
QAction *VuoEditorWindow::getZoomOutAction() const
{
	return ui->zoomOut;
}

/**
 * Returns a zoom action.
 */
QAction *VuoEditorWindow::getZoom11Action() const
{
	return ui->zoom11;
}

/**
 * Returns a zoom action.
 */
QAction *VuoEditorWindow::getZoomToFitAction() const
{
	return ui->zoomToFit;
}

/**
 * Returns a zoom action.
 */
QAction *VuoEditorWindow::getZoomInAction() const
{
	return ui->zoomIn;
}

/**
* Raises the current editor window to the foreground and activates it.
*/
void VuoEditorWindow::setAsActiveWindow()
{
	if (isMinimized())
		showNormal();
	else
		show();

	raise();
	activateWindow();
}

/**
 * Returns the most recent active editor window.
 */
VuoEditorWindow * VuoEditorWindow::getMostRecentActiveEditorWindow()
{
	return (VuoEditorUtilities::getOpenCompositionEditingWindows().contains(mostRecentActiveEditorWindow)? mostRecentActiveEditorWindow : NULL);
}

/**
 * Returns a pointer to the node class library owned by this editor window.
 */
VuoNodeLibrary * VuoEditorWindow::getOwnedNodeLibrary()
{
	return ownedNodeLibrary;
}

/**
 * Returns a pointer to the node class library currently in use by this editor window.
 * This may be the library docked into the window or the single floating library.
 */
VuoNodeLibrary * VuoEditorWindow::getCurrentNodeLibrary()
{
	return nl;
}

/**
 * Assigns a surrogate node class library to be used instead of the
 * library owned by this window, e.g., when in global floating-node-library
 * mode and the library owned by this window is hidden.
 */
void VuoEditorWindow::assignSurrogateNodeLibrary(VuoNodeLibrary *library)
{
	transitionNodeLibraryConnections(this->nl, library);
	this->nl = library;
}

/**
 * Releases any surrogate node class library currently assigned to
 * this window.
 */
void VuoEditorWindow::releaseSurrogateNodeLibrary(bool previousFloaterDestroyed)
{
	transitionNodeLibraryConnections((previousFloaterDestroyed? NULL : this->nl), this->ownedNodeLibrary);
	this->nl = this->ownedNodeLibrary;
}

/**
 * Returns the "File" menu associated with this editor window.
 */
QMenu * VuoEditorWindow::getFileMenu()
{
	return ui->menuFile;
}

/**
 * Returns the "File > Open Recent" menu associated with this editor window.
 */
VuoRecentFileMenu * VuoEditorWindow::getRecentFileMenu()
{
	return menuOpenRecent;
}

/**
 * Returns the composition associated with this editor window.
 */
VuoEditorComposition * VuoEditorWindow::getComposition()
{
	return composition;
}

/**
 * Remove connections to/from node library @c oldLibrary and initialize them for node library @c newLibrary.
 */
void VuoEditorWindow::transitionNodeLibraryConnections(VuoNodeLibrary *oldLibrary, VuoNodeLibrary *newLibrary)
{
	if (oldLibrary)
	{
		disconnect(oldLibrary, &VuoNodeLibrary::nodeLibraryReceivedPasteCommand, this, &VuoEditorWindow::disambiguatePasteRequest);
		disconnect(oldLibrary, &VuoNodeLibrary::componentsAdded, this, &VuoEditorWindow::componentsAdded);
		disconnect(ui->actionShowNodeNames, &QAction::triggered, oldLibrary, &VuoNodeLibrary::clickedNamesButton);
		disconnect(ui->actionShowNodeClassNames, &QAction::triggered, oldLibrary, &VuoNodeLibrary::clickedFlatClassButton);
		disconnect(oldLibrary, &VuoNodeLibrary::changedIsHumanReadable, this, &VuoEditorWindow::updateUI);
		disconnect(oldLibrary, &VuoNodeLibrary::documentationSelectionChanged, this, &VuoEditorWindow::updateUI);

		disconnect(oldLibrary, &VuoNodeLibrary::visibilityChanged, this, &VuoEditorWindow::updateSceneRect);
		disconnect(oldLibrary, &VuoNodeLibrary::dockLocationChanged, this, &VuoEditorWindow::updateSceneRect);
		disconnect(oldLibrary, &VuoNodeLibrary::nodeDocumentationPanelWidthChanged, metadataPanel, &VuoCompositionMetadataPanel::setTextWidth);

		disconnect(composition, SIGNAL(nodePopoverRequestedForClass(VuoNodeClass *)), oldLibrary, SLOT(prepareAndDisplayNodePopoverForClass(VuoNodeClass *)));

		// Update the "Show/Hide Node Library" menu item when the node library visibility is changed.
		disconnect(oldLibrary, &VuoNodeLibrary::visibilityChanged, this, &VuoEditorWindow::updateUI);
	}

	if (newLibrary)
	{
		connect(newLibrary, &VuoNodeLibrary::nodeLibraryReceivedPasteCommand, this, &VuoEditorWindow::disambiguatePasteRequest);
		connect(newLibrary, &VuoNodeLibrary::componentsAdded, this, &VuoEditorWindow::componentsAdded);
		connect(ui->actionShowNodeNames, &QAction::triggered, newLibrary, &VuoNodeLibrary::clickedNamesButton);
		connect(ui->actionShowNodeClassNames, &QAction::triggered, newLibrary, &VuoNodeLibrary::clickedFlatClassButton);
		connect(newLibrary, &VuoNodeLibrary::changedIsHumanReadable, this, &VuoEditorWindow::updateUI);
		connect(newLibrary, &VuoNodeLibrary::documentationSelectionChanged, this, &VuoEditorWindow::updateUI);

		connect(newLibrary, &VuoNodeLibrary::visibilityChanged, this, &VuoEditorWindow::updateSceneRect);
		connect(newLibrary, &VuoNodeLibrary::dockLocationChanged, this, &VuoEditorWindow::updateSceneRect);
		connect(newLibrary, &VuoNodeLibrary::nodeDocumentationPanelWidthChanged, metadataPanel, &VuoCompositionMetadataPanel::setTextWidth);

		connect(composition, SIGNAL(nodePopoverRequestedForClass(VuoNodeClass *)), newLibrary, SLOT(prepareAndDisplayNodePopoverForClass(VuoNodeClass *)));


		// Update the "Show/Hide Node Library" menu item when the node library visibility is changed.
		connect(newLibrary, &VuoNodeLibrary::visibilityChanged, this, &VuoEditorWindow::updateUI);
	}
}

/**
 * Restores the default minimum and maximum widths for the node library and
 * published input port sidebar, which may share the left docking area.
 * When one or the other is removed from the docking area, the other has
 * horizontal re-sizing temporarily disabled to prevent it from expanding
 * to fill the previous combined space of the two widgets.
 * Here we re-enable its ability to be re-sized.
 */
void VuoEditorWindow::restoreDefaultLeftDockedWidgetWidths()
{
	if (nl)
	{
		nl->setMinimumWidth(nodeLibraryMinimumWidth);
		nl->setMaximumWidth(nodeLibraryMaximumWidth);
	}

	if (inputPortSidebar && outputPortSidebar)
	{
		inputPortSidebar->setMinimumWidth(outputPortSidebar->minimumWidth());
		inputPortSidebar->setMaximumWidth(outputPortSidebar->maximumWidth());
	}
}

/**
 * Receive widget move events passed in the @c event parameter.  When the widget receives
 * this event, it is already at the new position.
 */
void VuoEditorWindow::moveEvent(QMoveEvent *event)
{
	// If a screen other than the main screen is focused,
	// `ui->setupUI` will invoke `QMainWindow::setUnifiedTitleAndToolBarOnMac`,
	// which will invoke `moveEvent` before the VuoEditorComposition has been initialized.
	if (composition)
	{
		QPoint positionChange = event->pos() - event->oldPos();
		composition->movePopoversBy(positionChange.x(), positionChange.y());
	}

	QMainWindow::moveEvent(event);
}

/**
 * Receive widget resize events passed in the @c event parameter.  When the widget receives
 * this event, it already has its new geometry.
 */
void VuoEditorWindow::resizeEvent(QResizeEvent *event)
{
#if VUO_PRO
	if (toolbar)
		toolbar->updateTitle();
#endif

	QMainWindow::resizeEvent(event);
}

/**
 * Checks whether the provided @c port is an input port that takes a list input; if so, creates
 * and returns a new "Make List" node and connecting cable to serve as the port's new
 * input.  Otherwise, returns an empty component list.
 */
QList<QGraphicsItem *> VuoEditorWindow::createAnyNecessaryMakeListComponents(VuoPort *port)
{
	QList<QGraphicsItem *> makeListComponents;

	VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler());
	if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()))
	{
		VuoNode *parentNode = port->getRenderer()->getUnderlyingParentNode()->getBase();
		VuoRendererCable *makeListCable = NULL;
		VuoRendererNode *makeListNode = composition->createAndConnectMakeListNode(parentNode, port, makeListCable);

		makeListComponents.append(makeListNode);
		makeListComponents.append(makeListCable);
	}

	return makeListComponents;
}

/**
  * Updates the canvas background grid rendering.
  */
void VuoEditorWindow::updateGrid()
{
	QGraphicsView::CacheMode defaultViewportCacheMode = ui->graphicsView->cacheMode();

	ui->graphicsView->setCacheMode(QGraphicsView::CacheNone);
	ui->graphicsView->viewport()->update();

	ui->graphicsView->setCacheMode(defaultViewportCacheMode);
}

/**
  * Updates the canvas opacity level in accordance with the current global setting.
  */
void VuoEditorWindow::updateCanvasOpacity()
{
	int opacity = static_cast<VuoEditor *>(qApp)->getCanvasOpacity();
	VuoEditorUtilities::setWindowOpacity(this, opacity);
}

/**
 * Updates the node library, canvas scrollbar, and comment colors to match the specified dark interface mode.
 */
void VuoEditorWindow::updateColor(bool isDark)
{
	VuoRendererColors *colors = VuoRendererColors::getSharedColors();
	QString backgroundColor                = colors->canvasFill().name();
	QString scrollBarColor                 = isDark ? "#505050" : "#dfdfdf";
	QString dockwidgetTitleBackgroundColor = isDark ? "#919191" : "#efefef";

	QString styles = VuoRendererCommon::getStyleSheet(isDark);

	if (doneInitializing)
		setStyleSheet(VUO_QSTRINGIFY(
						  QMainWindow {
							  background-color: %2;
						  }
						  QMainWindow::separator {
							  background-color: %1;
							  width: 1px;
							  height: 0px;
							  margin: -1px;
							  padding: 0px;
						  }
					  )
					  .arg(dockwidgetTitleBackgroundColor)
					  .arg(backgroundColor)
					  + styles);

	if (VuoEditorWindowToolbar::usingOverlayScrollers())
		ui->graphicsView->setStyleSheet(styles);
	else
		ui->graphicsView->setStyleSheet(VUO_QSTRINGIFY(
											 QScrollBar {
												 background: %1;
												 height: 14px;
												 width: 14px;
											 }
											 QScrollBar::handle {
												 background: %2;
												 border-radius: 5px;
												 min-width: 20px;
												 min-height: 20px;
												 margin: 2px;
											 }
											 QAbstractScrollArea::corner,
											 QScrollBar::add-line,
											 QScrollBar::sub-line,
											 QScrollBar::add-page,
											 QScrollBar::sub-page {
												 background: %1;
												 border: none;
											 }
											 )
										 .arg(backgroundColor)
										 .arg(scrollBarColor)
										 + styles);


	if (doneInitializing)
		foreach (VuoComment *comment, composition->getBase()->getComments())
		{
			if (comment->hasRenderer())
				comment->getRenderer()->updateColor();
		}
}

/**
 * Applies a coalesced set of composition changes to the running composition,
 * then clears the record of coalesced changes to be applied.
 */
void VuoEditorWindow::coalescedUpdateRunningComposition()
{
	if (coalescedOldCompositionSnapshot.empty() != coalescedNewCompositionSnapshot.empty())
			return;

	foreach (string nodeID, coalescedNodesToUnlink)
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->unlinkNodeInSupercompositionFromSubcomposition(composition, nodeID);

	if (!coalescedOldCompositionSnapshot.empty() && !coalescedNewCompositionSnapshot.empty())
		composition->updateRunningComposition(coalescedOldCompositionSnapshot, coalescedNewCompositionSnapshot, coalescedDiffInfo);

	foreach (string portID, coalescedInternalPortConstantsToSync)
		composition->syncInternalPortConstantInRunningComposition(portID);

	foreach (string portID, coalescedPublishedPortConstantsToSync)
		composition->syncPublishedPortConstantInRunningComposition(portID);

	foreach (string nodeID, coalescedNodesToRelink)
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->relinkNodeInSupercompositionToSubcomposition(composition, nodeID);

	coalescedOldCompositionSnapshot = "";
	coalescedNewCompositionSnapshot = "";
	coalescedInternalPortConstantsToSync.clear();
	coalescedPublishedPortConstantsToSync.clear();
	coalescedNodesToUnlink.clear();
	coalescedNodesToRelink.clear();
	coalescedDiffInfo = nullptr;
}

/**
 * Coalesces the provided composition modifications with previously recorded modifications.
 */
void VuoEditorWindow::coalesceSnapshots(string oldCompositionSnapshot, string newCompositionSnapshot, VuoCompilerCompositionDiff *diffInfo)
{
	// @todo For now, don't attempt to merge VuoCompilerCompositionDiffs.
	if (diffInfo && coalescedDiffInfo)
		coalescedUpdateRunningComposition();

	if (coalescedOldCompositionSnapshot.empty())
		coalescedOldCompositionSnapshot = oldCompositionSnapshot;

	coalescedNewCompositionSnapshot = newCompositionSnapshot;
	coalescedDiffInfo = diffInfo;
}

/**
 * Adds the provided port to the previously recorded list of internal ports to be synced in the running composition.
 */
void VuoEditorWindow::coalesceInternalPortConstantsToSync(string portID)
{
	coalescedInternalPortConstantsToSync.push_back(portID);
}

/**
 * Adds the provided port to the previously recorded list of published ports to be synced in the running composition.
 */
void VuoEditorWindow::coalescePublishedPortConstantsToSync(string portID)
{
	coalescedPublishedPortConstantsToSync.push_back(portID);
}

/**
 * Adds the provided node to the previously recorded list of nodes to unlink.
 */
void VuoEditorWindow::coalesceNodesToUnlink(string nodeID)
{
	coalescedNodesToUnlink.push_back(nodeID);
}

/**
 * Adds the provided node to the previously recorded list of nodes to relink.
 */
void VuoEditorWindow::coalesceNodesToRelink(string nodeID)
{
	coalescedNodesToRelink.push_back(nodeID);
}

/**
 * Checks whether the `protocolComplianceReevaluationPending` flag has been set since
 * the last time this function was called; if so, re-evaluates the composition
 * for compliance with available protocols.
 */
void VuoEditorWindow::handlePendingProtocolComplianceReevaluationRequests()
{
	if (protocolComplianceReevaluationPending)
	{
		evaluateCompositionForProtocolPromotion();
		protocolComplianceReevaluationPending = false;
	}

	return;
}

/**
 * Sets the `protocolComplianceReevaluationPending` flag for this composition,
 * meaning that the composition will be re-evaluated for compliance with available
 * protocols the next time the `handlePendingProtocolComplianceReevaluationRequests()`
 * function is called.
 */
void VuoEditorWindow::registerProtocolComplianceEvaluationRequest()
{
	this->protocolComplianceReevaluationPending = true;
}

/**
 * Returns the current position of the cursor in scene coordinates.
 */
QPointF VuoEditorWindow::getCursorScenePos()
{
	return ui->graphicsView->mapToScene(ui->graphicsView->mapFromGlobal(QCursor::pos()));
}

/**
  * Returns the metadata panel for this composition.
  */
VuoCompositionMetadataPanel * VuoEditorWindow::getMetadataPanel()
{
	return metadataPanel;
}

/**
 * Clamps the provided scene coordinates to the viewport bounds and avoids
 * returning coordinates that exactly match the position of any existing node,
 * for use, e.g., in positioning new nodes near the cursor but within the viewport.
 *
 * Enforces the provided margins around the edges of the viewport.
 * Current defaults are approximations in an effort to make nodes fully visible
 * within the viewport.
 */
QPointF VuoEditorWindow::getFittedScenePos(QPointF origPos, int leftMargin, int topMargin, int rightMargin, int bottomMargin)
{
		QRectF viewportRect = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();
		double targetX = min(max(origPos.x(), viewportRect.left()+leftMargin), viewportRect.right()-rightMargin);
		double targetY = min(max(origPos.y(), viewportRect.top()+topMargin), viewportRect.bottom()-bottomMargin);

		// Don't return the position of any pre-existing node.
		bool targetPositionClearOfCoincidentNodes = false;
		while (!targetPositionClearOfCoincidentNodes)
		{
			QGraphicsItem *preexistingItem = composition->itemAt(QPoint(targetX, targetY), composition->views()[0]->transform());
			if (dynamic_cast<VuoRendererNode *>(preexistingItem) && (preexistingItem->scenePos() == QPoint(targetX, targetY)))
			{
					targetX += pastedComponentOffset;
					targetY += pastedComponentOffset;
			}
			else
				targetPositionClearOfCoincidentNodes = true;
		}

	return QPointF(targetX, targetY);
}

/**
 * Toggles the docked state of the window-owned node library.
 */
void VuoEditorWindow::toggleNodeLibraryDockedState()
{
	if (nl)
	{
		bool floatLibrary = !nl->isFloating();
		nl->setFloating(floatLibrary);
		nl->fixWidth(!floatLibrary && !inputPortSidebar->isHidden());
		updateUI();
	}
}

/**
 * Initiates an Undo stack macro with the provided @c commandName.
 */
void VuoEditorWindow::beginUndoStackMacro(QString commandName)
{
	undoStack->beginMacro(tr(commandName.toUtf8().constData()));
}

/**
 * Concludes the Undo stack macro currently being composed.
 */
void VuoEditorWindow::endUndoStackMacro()
{
	undoStack->endMacro();
}
