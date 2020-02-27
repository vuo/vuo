/**
 * @file
 * VuoCodeWindow implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeWindow.hh"

#include "VuoCodeEditor.hh"
#include "VuoCodeEditorStages.hh"
#include "VuoCodeGutter.hh"
#include "VuoCodeIssueList.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoDocumentationSidebar.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindowToolbar.hh"
#include "VuoErrorDialog.hh"
#include "VuoException.hh"
#include "VuoInputEditorManager.hh"
#include "VuoInputEditorSession.hh"
#include "VuoMetadataEditor.hh"
#include "VuoModuleManager.hh"
#include "VuoNodeClass.hh"
#include "VuoProtocol.hh"
#include "VuoPublishedPortSidebar.hh"
#include "VuoShaderIssues.hh"
#include "VuoStringUtilities.hh"
#include "VuoRecentFileMenu.hh"
#include "VuoSubcompositionMessageRouter.hh"

/**
 * Creates a window for editing the fragment shader at @a sourcePath.
 * When this function returns, the window is ready to be shown.
 */
VuoCodeWindow::VuoCodeWindow(const string &sourcePath)
{
	stages = nullptr;
	issueList = nullptr;
	issues = new VuoShaderIssues();  // VuoCodeGutter::paintEvent wants this to be non-null.
	saveAction = nullptr;
	toggleInputPortSidebarAction = nullptr;
	toggleDocumentationSidebarAction = nullptr;
	zoom11Action = nullptr;
	zoomInAction = nullptr;
	zoomOutAction = nullptr;
	runAction = nullptr;
	stopAction = nullptr;
	restartAction = nullptr;
	reloadAction = nullptr;
	fileMenu = nullptr;
	windowMenu = nullptr;
	recentFileMenu = nullptr;
	shaderFile = nullptr;
	includeInRecentFileMenu = true;
	publishedInputsModified = false;
	metadataModified = false;
	closing = false;

	compiler = new VuoCompiler();

	VuoCompilerComposition *compilerComposition = new VuoCompilerComposition(new VuoComposition(), nullptr);
	wrapperComposition = new VuoEditorComposition(this, compilerComposition->getBase());
	wrapperComposition->setCompiler(compiler);

	VuoModuleManager *moduleManager = new VuoModuleManager(compiler);
	moduleManager->setComposition(wrapperComposition);
	moduleManager->setCodeWindow(this);
	wrapperComposition->setModuleManager(moduleManager);

	raiseDocumentAction = new QAction(this);
	raiseDocumentAction->setCheckable(true);
	connect(raiseDocumentAction, &QAction::triggered, this, &VuoCodeWindow::setAsActiveWindow);

	toolbar = nullptr;  // VuoEditorWindowToolbar constructor indirectly calls resizeEvent, which uses toolbar if non-null
	toolbar = new VuoEditorWindowToolbar(this, true);
	setUnifiedTitleAndToolBarOnMac(true);

	inputPortSidebar = new VuoPublishedPortSidebar(this, wrapperComposition, true, false);
	addDockWidget(Qt::LeftDockWidgetArea, inputPortSidebar);
	inputPortSidebar->limitAllowedPortTypes(VuoShaderFile::supportedVuoTypes());

	setSourcePath(sourcePath);

	if (!isNewUnsavedDocument())
		VUserLog("%s:      Open", getWindowTitleWithoutPlaceholder().toUtf8().data());

	stages = new VuoCodeEditorStages(this, shaderFile);
	connect(stages, &VuoCodeEditorStages::modificationMayHaveChanged, this, &VuoCodeWindow::updateModifiedIndicator);
	setCentralWidget(stages);

	bringNodeClassInSyncWithSourceCode();

	documentationSidebar = new VuoDocumentationSidebar(this);
	documentationSidebar->setVisible(static_cast<VuoEditor *>(qApp)->getGlobalShaderDocumentationVisibility());
	connect(documentationSidebar, &VuoDocumentationSidebar::visibilityChanged, this, &VuoCodeWindow::updateDocumentationSidebarMenuItem);
	connect(documentationSidebar, &VuoDocumentationSidebar::visibilityChanged, static_cast<VuoEditor *>(qApp), &VuoEditor::updateGlobalShaderDocumentationVisibility);
	addDockWidget(Qt::RightDockWidgetArea, documentationSidebar);

	resize(700,700);
	setMinimumWidth(650);  // Wide enough for published input ports + gutter + 3 tabs + shader type label.
	stages->setMinimumWidth(200);  // Wide enough for 1 tab.
	resizeDocks({documentationSidebar}, {250}, Qt::Horizontal);

	this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

	inputEditorManager = new VuoInputEditorManager();
	wrapperComposition->setInputEditorManager(inputEditorManager);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::visibilityChanged, this, &VuoCodeWindow::updateInputPortSidebarMenuItem);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::inputEditorRequested, this, &VuoCodeWindow::showPublishedInputEditor);
	connect(inputPortSidebar, SIGNAL(publishedPortDetailsChangeRequested(VuoRendererPublishedPort *, json_object *)), this, SLOT(changePublishedPortDetails(VuoRendererPublishedPort *, json_object *)));
	connect(inputPortSidebar, &VuoPublishedPortSidebar::publishedPortNameEditorRequested, this, &VuoCodeWindow::showPublishedPortNameEditor);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::newPublishedPortRequested, this, &VuoCodeWindow::addPublishedPort);
	connect(inputPortSidebar, &VuoPublishedPortSidebar::externalPortUnpublicationRequested, this, &VuoCodeWindow::deletePublishedPort);

	metadataEditor = new VuoMetadataEditor(wrapperComposition, this, Qt::Sheet, true);
	metadataEditor->setWindowModality(Qt::WindowModal);
	connect(metadataEditor, &VuoMetadataEditor::finished, this, &VuoCodeWindow::changeMetadata);

	issueList = new VuoCodeIssueList(this);
	addDockWidget(Qt::BottomDockWidgetArea, issueList);

	populateMenus();

	zoom11();

	connect(wrapperComposition, &VuoEditorComposition::buildStarted, this, &VuoCodeWindow::showBuildActivityIndicator);
	connect(wrapperComposition, &VuoEditorComposition::buildFinished, this, &VuoCodeWindow::hideBuildActivityIndicator);
	connect(wrapperComposition, &VuoEditorComposition::stopFinished, this, &VuoCodeWindow::hideStopActivityIndicator);
	connect(wrapperComposition, &VuoEditorComposition::compositionStoppedItself, this, &VuoCodeWindow::on_stopComposition_triggered);

	connect(static_cast<VuoEditor *>(qApp), &VuoEditor::darkInterfaceToggled, this, &VuoCodeWindow::updateColor);
	updateColor();
	updateCanvasOpacity();
	updateModifiedIndicator();

	static_cast<VuoEditor *>(qApp)->registerOpenDocument(this);
}

VuoCodeWindow::~VuoCodeWindow(void)
{
	relinquishSourcePath();

	delete toolbar;
	delete issues;
	delete wrapperComposition;  // deletes compiler
}

/**
 * Creates a menu bar for the window (if it doesn't already exist) and adds menus.
 */
void VuoCodeWindow::populateMenus(void)
{
	{
		// "File" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&File"));
		fileMenu = m;

		m->addAction(tr("&New Composition"), static_cast<VuoEditor *>(qApp), &VuoEditor::newComposition, QKeySequence("Ctrl+N"));

		{
			// "New Composition from Template" menu
			QMenu *mm = new QMenu(tr("New Composition from Template"));
			mm->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

			static_cast<VuoEditor *>(qApp)->populateNewCompositionWithTemplateMenu(mm);

			m->addMenu(mm);
		}

		{
			// "New Shader" menu
			QMenu *mm = new QMenu(tr("New Shader"));
			mm->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133

			static_cast<VuoEditor *>(qApp)->populateNewShaderMenu(mm);

			m->addMenu(mm);
		}

		m->addSeparator();
		m->addAction(tr("&Open…"), static_cast<VuoEditor *>(qApp), &VuoEditor::openFile, QKeySequence("Ctrl+O"));

		{
			// "Open Recent" menu
			recentFileMenu = new VuoRecentFileMenu();

			connect(recentFileMenu, &VuoRecentFileMenu::recentFileSelected, static_cast<VuoEditor *>(qApp), &VuoEditor::openUrl);

			m->addMenu(recentFileMenu);
		}

		m->addSeparator();
		saveAction = m->addAction(tr("&Save"), this, &VuoCodeWindow::save, QKeySequence("Ctrl+S"));
		m->addAction(tr("Save As…"), this, &VuoCodeWindow::saveAs, QKeySequence("Ctrl+Shift+S"));

		m->addSeparator();
		m->addAction(tr("Close"), this, &VuoCodeWindow::close, QKeySequence("Ctrl+W"));

		m->addAction(tr("Quit"), static_cast<VuoEditor *>(qApp), &VuoEditor::quitCleanly, QKeySequence("Ctrl+Q"));

		m->addAction(tr("About Vuo…"), static_cast<VuoEditor *>(qApp), &VuoEditor::about);

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Edit" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("Edit"));

		m->addSeparator();
		m->addAction(tr("Undo"), stages->currentEditor(), &VuoCodeEditor::undo, QKeySequence("Ctrl+Z"));
		m->addAction(tr("Redo"), stages->currentEditor(), &VuoCodeEditor::redo, QKeySequence("Ctrl+Shift+Z"));

		m->addSeparator();
		m->addAction(tr("Cut"), stages->currentEditor(), &VuoCodeEditor::cut, QKeySequence("Ctrl+X"));
		m->addAction(tr("Copy"), this, &VuoCodeWindow::copy, QKeySequence("Ctrl+C"));
		m->addAction(tr("Paste"), stages->currentEditor(), &VuoCodeEditor::paste, QKeySequence("Ctrl+V"));
		m->addAction(tr("Delete"), [this](){ stages->currentEditor()->textCursor().removeSelectedText(); });

		m->addSeparator();
		m->addAction(tr("Select All"), stages->currentEditor(), &VuoCodeEditor::selectAll, QKeySequence("Ctrl+A"));

		m->addSeparator();
		m->addAction(tr("Composition Information…"), [this](){ metadataEditor->show(); }, QKeySequence("Ctrl+I"));

		menuBar()->addAction(m->menuAction());
	}

	{
		// "View" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&View"));

		toggleInputPortSidebarAction = m->addAction(tr("Hide Published Ports"), this, &VuoCodeWindow::toggleInputPortSidebarVisibility, QKeySequence("Ctrl+4"));
		toggleDocumentationSidebarAction = m->addAction("", this, &VuoCodeWindow::toggleDocumentationSidebarVisibility, QKeySequence("Ctrl+5"));
		updateDocumentationSidebarMenuItem();

		m->addSeparator();
		zoom11Action = m->addAction(tr("Actual Size"), this, &VuoCodeWindow::zoom11, QKeySequence("Ctrl+0"));
		zoomInAction = m->addAction(tr("Zoom In"), this, &VuoCodeWindow::zoomIn, QKeySequence("Ctrl+="));
		zoomOutAction = m->addAction(tr("Zoom Out"), this,  &VuoCodeWindow::zoomOut, QKeySequence("Ctrl+-"));

		m->addSeparator();

		{
			// "Canvas Transparency" menu
			QMenu *mm = new QMenu(menuBar());
			mm->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
			mm->setTitle(tr("&Canvas Transparency"));

			static_cast<VuoEditor *>(qApp)->populateCanvasTransparencyMenu(mm);
			connect(static_cast<VuoEditor *>(qApp), &VuoEditor::canvasOpacityChanged, this, &VuoCodeWindow::updateCanvasOpacity);

			m->addMenu(mm);
		}

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Run" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("Run"));

		runAction = m->addAction(tr("Run"), this, &VuoCodeWindow::on_runComposition_triggered, QKeySequence("Ctrl+R"));
		stopAction = m->addAction(tr("Stop"), this, &VuoCodeWindow::on_stopComposition_triggered, QKeySequence("Ctrl+."));
		restartAction = m->addAction(tr("Restart"), this, &VuoCodeWindow::on_restartComposition_triggered, QKeySequence("Ctrl+Shift+R"));

		m->addSeparator();
		reloadAction = m->addAction(tr("Reload"), [=]{
			VUserLog("%s:      Reload", getWindowTitleWithoutPlaceholder().toUtf8().data());
			bringNodeClassInSyncWithSourceCode();
		});
		reloadAction->setShortcuts({ QKeySequence("Ctrl+Return"), QKeySequence("Alt+Return") });

		stopAction->setEnabled(false);
		restartAction->setEnabled(false);
		updateReloadAction();

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Tools" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("Tools"));

		m->addAction(tr("Open User Library in Finder"), &VuoEditorUtilities::openUserModulesFolder);
		m->addAction(tr("Open System Library in Finder"), &VuoEditorUtilities::openSystemModulesFolder);

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Window" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&Window"));

		static_cast<VuoEditor *>(qApp)->populateWindowMenu(m, this);
		windowMenu = m;
		connect(windowMenu, &QMenu::aboutToShow, this, &VuoCodeWindow::updateWindowMenu);

		menuBar()->addAction(m->menuAction());
	}

	{
		// "Help" menu
		QMenu *m = new QMenu(menuBar());
		m->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		m->setTitle(tr("&Help"));

		static_cast<VuoEditor *>(qApp)->populateHelpMenu(m);

		menuBar()->addAction(m->menuAction());
	}
}

/**
 * Creates a code editor window with the code template indicated by the sender.
 */
void VuoCodeWindow::newShaderWithTemplate(QAction *sender)
{
	QString selectedTemplate = static_cast<QString>(sender->data().value<QString>());
	string templatePath = VuoFileUtilities::getVuoFrameworkPath() + "/Resources/" + selectedTemplate.toUtf8().constData() + ".fs";

	string tmpPath = VuoFileUtilities::makeTmpFile("VuoCodeEditor-untitled", "fs");
	VuoFileUtilities::copyFile(templatePath, tmpPath);

	VuoCodeWindow *window = new VuoCodeWindow(tmpPath);
	VUserLog("%s:      New Shader with %s template", window->getWindowTitleWithoutPlaceholder().toUtf8().data(), selectedTemplate.toUtf8().data());
	window->show();
}

/**
 * Saves the code in the editor to the file previously chosen by the user.
 * If this is the first time saving, shows a file chooser and saves to the chosen file.
 */
void VuoCodeWindow::save()
{
	if (isNewUnsavedDocument())
		saveAs();
	else
	{
		VUserLog("%s:      Save", getWindowTitleWithoutPlaceholder().toUtf8().data());
		saveToPath(windowFilePath());
	}
}

/**
 * Shows a file chooser and saves the code in the editor to the chosen file.
 */
void VuoCodeWindow::saveAs()
{
	QFileDialog d(this, Qt::Sheet);
	d.setAcceptMode(QFileDialog::AcceptSave);

	if (isNewUnsavedDocument())
		d.selectFile(static_cast<VuoEditor *>(qApp)->getSubcompositionPrefix() + ".shader.fs");
	else
		d.setDirectory(windowFilePath());

	if (d.exec() == QDialog::Accepted)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(d.selectedFiles()[0].toStdString(), dir, file, ext);
		VUserLog("%s:      Save as %s.%s", getWindowTitleWithoutPlaceholder().toUtf8().data(), file.c_str(), ext.c_str());
		saveToPath(d.selectedFiles()[0]);
	}
}

/**
 * Saves the code in the editor to @a savePath, appending the appropriate file extension if needed.
 * If the attempt to save fails, shows an error dialog.
 */
void VuoCodeWindow::saveToPath(QString savePath)
{
	bool saveAborted = false;
	QString failureDetails = "";
	QString expectedFileExtension = ".fs";
	if (! savePath.endsWith(expectedFileExtension))
	{
		savePath.append(expectedFileExtension);
		if (VuoFileUtilities::fileExists(savePath.toStdString()))
		{
			saveAborted = true;
			failureDetails = "A file or folder with the same name already exists.";
		}
	}

	bool saveSucceeded = false;
	if (! saveAborted)
	{
		try
		{
			bringStoredShaderInSyncWithSourceCode();
			shaderFile->save(savePath.toStdString());
			saveSucceeded = true;
		}
		catch (VuoException &e)
		{
			failureDetails = e.what();
		}
	}

	if (saveSucceeded)
	{
		if (! VuoFileUtilities::arePathsEqual(windowFilePath().toStdString(), savePath.toStdString()))
			setSourcePath(savePath.toStdString());

		stages->setUnmodified();
		publishedInputsModified = false;
		metadataModified = false;
		updateModifiedIndicator();

		if (includeInRecentFileMenu)
			static_cast<VuoEditor *>(qApp)->addFileToAllOpenRecentFileMenus(savePath);
	}
	else
	{
		QMessageBox fileSaveFailureDialog(this);
		fileSaveFailureDialog.setWindowFlags(Qt::Sheet);
		fileSaveFailureDialog.setWindowModality(Qt::WindowModal);
		fileSaveFailureDialog.setText(tr("The shader could not be saved at “%1”.").arg(savePath));
		fileSaveFailureDialog.setStyleSheet("#qt_msgbox_informativelabel { font-weight: normal; font-size: 11pt; }");
		fileSaveFailureDialog.setInformativeText(failureDetails);
		fileSaveFailureDialog.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		fileSaveFailureDialog.setButtonText(QMessageBox::Save, tr("Save As…"));
		fileSaveFailureDialog.setIcon(QMessageBox::Warning);

		switch(fileSaveFailureDialog.exec()) {
			case QMessageBox::Save:
				saveAs();
				break;
			case QMessageBox::Cancel:
				break;
			default:
				break;
		}
	}
}

/**
 * Returns true if the code in this code editor has never been saved to file.
 */
bool VuoCodeWindow::isNewUnsavedDocument()
{
	string dir, file, ext;
	VuoFileUtilities::splitPath(windowFilePath().toStdString(), dir, file, ext);

	return VuoFileUtilities::arePathsEqual(dir, VuoFileUtilities::getTmpDir()) && VuoStringUtilities::beginsWith(file, "VuoCodeEditor-untitled");
}

/**
 * Sets the boolean indicating whether this file should be added to the
 * "File > Open Recent" list when saved.
 */
void VuoCodeWindow::setIncludeInRecentFileMenu(bool include)
{
	this->includeInRecentFileMenu = include;
}

/**
 * Before closing the window, gives the user a chance to save any unsaved changes.
 */
void VuoCodeWindow::closeEvent(QCloseEvent *event)
{
	if (closing)
	{
		event->accept();
		return;
	}

	auto closeAndContinueQuit = [this](){
		if (isNewUnsavedDocument())
			VuoFileUtilities::deleteFile(windowFilePath().toStdString());
		else
			static_cast<VuoEditor *>(qApp)->addFileToRecentlyClosedList(windowFilePath());

		if (wrapperComposition->isRunning())
		{
			connect(wrapperComposition, &VuoEditorComposition::stopFinished, this, &VuoCodeWindow::deleteLater);
			on_stopComposition_triggered();
		}
		else
			deleteLater();

		// Don't update the documentation sidebar visibility preference when the sidebar is hidden as a side effect of closing the window.
		disconnect(documentationSidebar, &VuoDocumentationSidebar::visibilityChanged, static_cast<VuoEditor *>(qApp), &VuoEditor::updateGlobalShaderDocumentationVisibility);

		closing = true;
		static_cast<VuoEditor *>(qApp)->continueQuit(this);
	};

	if (isWindowModified() || stages->modified())
	{
		auto mb = new QMessageBox(this);
		mb->setWindowFlags(Qt::Sheet);
		mb->setWindowModality(Qt::WindowModal);

		mb->setText(tr("Do you want to save the changes made to the document “%1”?").arg(windowTitle()));
		mb->setStyleSheet("#qt_msgbox_informativelabel { font-weight: normal; font-size: 11pt; }");
		mb->setInformativeText(tr("Your changes will be lost if you don’t save them."));
		mb->setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());

		mb->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		mb->setDefaultButton(QMessageBox::Save);

		static_cast<QPushButton *>(mb->button(QMessageBox::Discard))->setAutoDefault(false);
		mb->button(QMessageBox::Discard)->setFocus();

		connect(mb, &QMessageBox::buttonClicked, this, [=](QAbstractButton *button){
			auto role = mb->buttonRole(button);
			if (role == QMessageBox::AcceptRole)
			{
				save();

				if (stages->modified())
					static_cast<VuoEditor *>(qApp)->cancelQuit();
				else
					closeAndContinueQuit();
			}
			else if (role == QMessageBox::DestructiveRole)
				closeAndContinueQuit();
			else // if (role == QMessageBox::RejectRole)
				static_cast<VuoEditor *>(qApp)->cancelQuit();
		});

		mb->open();

		event->ignore();
	}
	else
	{
		event->accept();
		closeAndContinueQuit();
	}

	VUserLog("%s:      Close", getWindowTitleWithoutPlaceholder().toUtf8().data());
}

/**
 * Runs the wrapper composition.
 */
void VuoCodeWindow::on_runComposition_triggered()
{
	VUserLog("%s:      Run", getWindowTitleWithoutPlaceholder().toUtf8().data());

	toolbar->changeStateToBuildPending();
	updateToolbar();

	bringNodeClassInSyncWithSourceCode();

	string snapshot = wrapperComposition->takeSnapshot();
	wrapperComposition->run(snapshot);
}

/**
 * Stops the wrapper composition.
 */
void VuoCodeWindow::on_stopComposition_triggered()
{
	VUserLog("%s:      Stop", getWindowTitleWithoutPlaceholder().toUtf8().data());

	showStopActivityIndicator();

	wrapperComposition->stop();
}

/**
 * Restarts the wrapper composition.
 */
void VuoCodeWindow::on_restartComposition_triggered()
{
	on_stopComposition_triggered();
	on_runComposition_triggered();
}

/**
 * Dissociates this window from the file it was editing, in preparation for switching
 * to a different file or closing the window.
 */
void VuoCodeWindow::relinquishSourcePath(void)
{
	delete shaderFile;
	shaderFile = nullptr;

	string oldSourcePath = windowFilePath().toStdString();
	if (! oldSourcePath.empty())
	{
		compiler->uninstallNodeClassAtCompositionLocalScope(oldSourcePath);

		if (isNewUnsavedDocument())
			VuoFileUtilities::deleteFile(oldSourcePath);
	}

	wrapperComposition->getModuleManager()->cancelCallbackForNodeClass(getNodeClassName());
}

/**
 * Sets the path of the file being edited by this window. Used to set the path initially
 * or change it after saving the source code to a different path.
 */
void VuoCodeWindow::setSourcePath(const string &sourcePath)
{
	relinquishSourcePath();

	string dir, file, ext;
	VuoFileUtilities::splitPath(sourcePath, dir, file, ext);
	try
	{
		shaderFile = new VuoShaderFile(VuoFileUtilities::File(dir, file + "." + ext));
	}
	catch (VuoException &e)
	{
		delete toolbar;
		delete issues;
		delete wrapperComposition;  // deletes compiler
		throw;
	}

	setWindowFilePath(QString::fromStdString(sourcePath));

	string title;
	if (isNewUnsavedDocument())
	{
		set<string> takenTitles;
		for (QMainWindow *openWindow : VuoEditorUtilities::getOpenEditingWindows())
			takenTitles.insert(openWindow->windowTitle().toStdString());
		string preferredTitle = "Untitled " + shaderFile->typeName();
		title = VuoStringUtilities::formUniqueIdentifier(takenTitles, preferredTitle, preferredTitle + " ");
	}
	else
	{
		title = file + "." + ext;
	}
	setWindowTitle(QString::fromStdString(title));
#if VUO_PRO
	toolbar->updateTitle();
#endif

	raiseDocumentAction->setText(windowTitle());

	compiler->setCompositionPath(sourcePath);

	wrapperComposition->getModuleManager()->doEveryTimeNodeClassIsLoaded(getNodeClassName(), ^{
		updateWrapperComposition();
	});

	wrapperComposition->getModuleManager()->updateWithAlreadyLoadedModules();

	VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(getNodeClassName());
	if (! nodeClass)
	{
		compiler->installNodeClassAtCompositionLocalScope(sourcePath);
		nodeClass = compiler->getNodeClass(getNodeClassName());
	}
}

/**
 * Wraps the node class implemented by the source code into a composition — one instance of the node class
 * with all of its ports published.
 */
void VuoCodeWindow::updateWrapperComposition(void)
{
	string oldSnapshot = wrapperComposition->takeSnapshot();
	wrapperComposition->clear();

	VuoNode *node = wrapperComposition->createNode(QString::fromStdString(getNodeClassName()))->getBase();
	wrapperComposition->addNode(node);

	vector<VuoPort *> inputPorts = node->getInputPorts();
	for (size_t i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
	{
		VuoPort *port = node->getInputPorts().at(i);
		VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());

		string publishedName = port->getClass()->getName();

		// Rename published ports that happen to have the same name as protocol ports (but aren't actually protocol ports).
		publishedName = VuoStringUtilities::formUniqueIdentifier([=](const string &name){
			bool ok = !wrapperComposition->getBase()->getPublishedInputPortWithName(name);

			if (shaderFile->type() == VuoShaderFile::GLSLImageFilter)
				ok &= name != "image";
			else if (shaderFile->type() == VuoShaderFile::GLSLImageGenerator)
				ok &= name != "width"
				   && name != "height";

			ok &= name != "time";

			return ok;
		}, publishedName);

		// Rename the `vuo*` published ports to make the composition conform to the protocol.
		if (shaderFile->type() == VuoShaderFile::GLSLImageFilter)
		{
			if (publishedName == "inputImage")
				publishedName = "image";
		}
		else if (shaderFile->type() == VuoShaderFile::GLSLImageGenerator)
		{
			if (publishedName == "vuoWidth")
				publishedName = "width";
			else if (publishedName == "vuoHeight")
				publishedName = "height";
		}
		if (publishedName == "vuoTime")
			publishedName = "time";

		VuoType *dataType = compilerPort->getDataVuoType();

		VuoCompilerPublishedPort *publishedPort = VuoCompilerPublishedPort::newPort(publishedName, dataType);
		wrapperComposition->addPublishedPort(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);
		wrapperComposition->createRendererForPublishedPortInComposition(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);

		VuoCompilerCable *publishedCable = new VuoCompilerCable(nullptr, publishedPort, node->getCompiler(), compilerPort);
		wrapperComposition->addCable(publishedCable->getBase());

		if (dataType)
		{
			string constantValue = static_cast<VuoCompilerInputEventPort *>(compilerPort)->getData()->getInitialValue();
			publishedPort->setInitialValue(constantValue);
		}

		// Disallow renaming or deleting the published port if it may have been auto-generated by VuoIsfModuleCompiler.
		string portName = port->getClass()->getName();
		if (VuoStringUtilities::beginsWith(portName, "inputImage") ||
				VuoStringUtilities::beginsWith(portName, "vuoWidth") ||
				VuoStringUtilities::beginsWith(portName, "vuoHeight") ||
				VuoStringUtilities::beginsWith(portName, "vuoTime") ||
				VuoStringUtilities::beginsWith(portName, "vuoColorDepth"))
			static_cast<VuoRendererPublishedPort *>(publishedPort->getBase()->getRenderer())->setPermanent(true);
	}
	if (! node->getInputPortWithName("vuoTime"))
	{
		VuoCompilerPublishedPort *publishedPort = VuoCompilerPublishedPort::newPort("time", compiler->getType("VuoReal")->getBase());
		wrapperComposition->addPublishedPort(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);
		wrapperComposition->createRendererForPublishedPortInComposition(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);
	}

	vector<VuoPort *> outputPorts = node->getOutputPorts();
	for (size_t i = VuoNodeClass::unreservedOutputPortStartIndex; i < outputPorts.size(); ++i)
	{
		VuoPort *port = node->getOutputPorts().at(i);
		VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());

		VuoType *dataType = compilerPort->getDataVuoType();

		string publishedName = port->getClass()->getName();
		if (dataType->getModuleKey() == "VuoImage")
			publishedName = "outputImage";

		VuoCompilerPublishedPort *publishedPort = VuoCompilerPublishedPort::newPort(publishedName, dataType);
		wrapperComposition->addPublishedPort(static_cast<VuoPublishedPort *>(publishedPort->getBase()), false);
		wrapperComposition->createRendererForPublishedPortInComposition(static_cast<VuoPublishedPort *>(publishedPort->getBase()), false);

		VuoCompilerCable *publishedCable = new VuoCompilerCable(node->getCompiler(), compilerPort, nullptr, publishedPort);
		wrapperComposition->addCable(publishedCable->getBase());
	}

	VuoProtocol *protocol = nullptr;
	if (shaderFile->type() == VuoShaderFile::GLSLImageFilter)
		protocol = VuoProtocol::getProtocol("VuoImageFilter");
	else if (shaderFile->type() == VuoShaderFile::GLSLImageGenerator)
		protocol = VuoProtocol::getProtocol("VuoImageGenerator");
	else if (shaderFile->type() == VuoShaderFile::GLSLImageTransition)
		protocol = VuoProtocol::getProtocol("VuoImageTransition");

	if (protocol)
		wrapperComposition->addActiveProtocol(protocol, false);

	VuoCompositionMetadata *metadata = shaderFile->metadata();
	wrapperComposition->getBase()->setMetadata(metadata, true);

	string newSnapshot = wrapperComposition->takeSnapshot();
	if (oldSnapshot != newSnapshot)
		wrapperComposition->updateRunningComposition(oldSnapshot, newSnapshot);

	inputPortSidebar->updatePortList();
	inputPortSidebar->updateActiveProtocol();
}

/**
 * Tells the compiler that the node class's source code is now what is in the editor,
 * prompting the compiler to reload the node class if needed.
 */
void VuoCodeWindow::bringNodeClassInSyncWithSourceCode(void)
{
	bringStoredShaderInSyncWithSourceCode();
	string sourceCode = shaderFile->fragmentFileContents();
	compiler->overrideInstalledNodeClass(windowFilePath().toStdString(), sourceCode);
}

/**
 * Updates @ref shaderFile to match what is in the code window's text area.
 */
void VuoCodeWindow::bringStoredShaderInSyncWithSourceCode(void)
{
	shaderFile->setFragmentSource(stages->fragment->toPlainText().toStdString());
}

/**
 * Updates @ref shaderFile to match what is in the code window's published inputs.
 */
void VuoCodeWindow::bringStoredShaderInSyncWithPublishedInputPorts(VuoCompilerPublishedPort *publishedInputAdded,
																   const pair<string, string> &publishedInputRenamed,
																   const string &publishedInputRemoved)
{
	vector<VuoShaderFile::Port> shaderInputs = shaderFile->inputPorts();

	for (auto i = shaderInputs.begin(); i != shaderInputs.end(); )
	{
		if (i->key == publishedInputRemoved)
			i = shaderInputs.erase(i);
		else
			++i;
	}

	for (VuoShaderFile::Port &shaderInput : shaderInputs)
	{
		if (shaderInput.key == publishedInputRenamed.first)
			shaderInput.key = publishedInputRenamed.second;

		VuoPublishedPort *publishedInput = wrapperComposition->getBase()->getPublishedInputPortWithName(shaderInput.key);
		if (publishedInput)
		{
			VuoCompilerPublishedPort *compilerPublishedInput = static_cast<VuoCompilerPublishedPort *>(publishedInput->getCompiler());
			json_object *oldDetails = shaderInput.vuoPortDetails;
			shaderInput.vuoPortDetails = compilerPublishedInput->getDetails(true);
			json_object *scaleToSamplerRectValue = NULL;
			if (json_object_object_get_ex(oldDetails, "scaleToSamplerRect", &scaleToSamplerRectValue))
				json_object_object_add(shaderInput.vuoPortDetails, "scaleToSamplerRect", scaleToSamplerRectValue);
		}
	}

	if (publishedInputAdded)
	{
		VuoShaderFile::Port addedInput;
		addedInput.key = publishedInputAdded->getBase()->getClass()->getName();
		addedInput.vuoTypeName = (publishedInputAdded->getDataVuoType() ? publishedInputAdded->getDataVuoType()->getModuleKey() : "event");
		addedInput.vuoPortDetails = publishedInputAdded->getDetails(true);
		shaderInputs.push_back(addedInput);
	}

	shaderFile->setInputPorts(shaderInputs);
}

/**
 * Shows a list of warnings/errors (line number and text description).
 */
void VuoCodeWindow::setIssues(VuoCompilerIssues *compilerIssues)
{
	delete issues;

	VuoShaderIssues *issues = new VuoShaderIssues();
	for (VuoCompilerIssue compilerIssue : compilerIssues->getList())
		issues->addIssue(VuoShaderFile::Fragment, compilerIssue.getLineNumber(), compilerIssue.getDetails(false));

	this->issues = issues;

	// Render the error icons in the gutter.
	if (stages)
		static_cast<VuoCodeEditor *>(stages->currentWidget())->gutter->update();

	if (issueList)
		issueList->updateIssues();
}

/**
 * Returns the node class name being edited by this window.
 */
string VuoCodeWindow::getNodeClassName(void)
{
	return VuoCompiler::getModuleKeyForPath(windowFilePath().toStdString());
}

/**
 * Updates the window's indication of unsaved changes to match the state of the window contents.
 */
void VuoCodeWindow::updateModifiedIndicator()
{
	bool modified = stages->modified() || publishedInputsModified || metadataModified;
	setWindowModified(modified);

	bool saveEnabled = modified || isNewUnsavedDocument();
	saveAction->setEnabled(saveEnabled);
}

void VuoCodeWindow::updateColor()
{
	bool isDark = static_cast<VuoEditor *>(qApp)->isInterfaceDark();
	VuoCodeEditor *e = stages->someEditor();

	QString menuStyle = VUO_QSTRINGIFY(
		// Sync with VuoEditorWindow::updateColor.
		// Should parallel VuoDialogForInputEditor::getStyleSheet()'s QComboBox popup menu styles.
		QMenu {
			background-color: #404040;
		}
		QMenu::item {
			color: #cfcfcf;
			padding-left: 22px;
			padding-right: 36px;
			height: 21px;
		}
		QMenu::item:disabled {
			color: #707070;
		}
		QMenu::item:selected {
			background-color: #1060d0;
			color: #ffffff;
		}
		QMenu::right-arrow {
			left: -14px;
		}
		QMenu::indicator:checked {
			image: url(:/Icons/checkmark.svg);
			width: 11px;
		}
		QMenu::indicator:checked,
		QMenu::icon {
			margin-left: 6px;
		}
		QMenu::icon:checked,
		QMenu::icon:unchecked {
			margin-left: 0;
		}
	);

	setStyleSheet(VUO_QSTRINGIFY(
		// Hide the 1px bright line between VuoPublishedPortSidebar and VuoCodeGutter.
		QMainWindow::separator {
			width: 1px;
			height: 0px;
			margin: -1px;
			padding: 0px;
			background: transparent;
		}

		QMainWindow {
			background: %1;
		}
	).arg(e->gutterColor.name())
	+ (isDark ? menuStyle : ""));
}

/**
 * Updates the canvas opacity level in accordance with the current global setting.
 */
void VuoCodeWindow::updateCanvasOpacity()
{
	int opacity = static_cast<VuoEditor *>(qApp)->getCanvasOpacity();
	VuoEditorUtilities::setWindowOpacity(this, opacity);
}

void VuoCodeWindow::updateToolbar()
{
	toolbar->update(false, stages->currentEditor()->isZoomedToActualSize(), false);
}

void VuoCodeWindow::resizeEvent(QResizeEvent *event)
{
#if VUO_PRO
	if (toolbar)
		toolbar->updateTitle();
#endif

	QMainWindow::resizeEvent(event);
}

/**
 * Returns the action associated with raising the current code window to the foreground.
 */
QAction * VuoCodeWindow::getRaiseDocumentAction() const
{
	return raiseDocumentAction;
}

/**
 * Returns the action associated with zooming to actual size.
 */
QAction * VuoCodeWindow::getZoom11Action() const
{
	return zoom11Action;
}

/**
 * Returns the action associated with zooming in.
 */
QAction * VuoCodeWindow::getZoomInAction() const
{
	return zoomInAction;
}

/**
 * Returns the action associated with zooming out.
 */
QAction * VuoCodeWindow::getZoomOutAction() const
{
	return zoomOutAction;
}

/**
 * Returns the "File" menu associated with this code window.
 */
QMenu * VuoCodeWindow::getFileMenu()
{
	return fileMenu;
}

/**
 * Returns the "File > Open Recent" menu associated with this code window.
 */
VuoRecentFileMenu * VuoCodeWindow::getRecentFileMenu()
{
	return recentFileMenu;
}

/**
 * Raises the current code window to the foreground and activates it.
 */
void VuoCodeWindow::setAsActiveWindow()
{
	if (isMinimized())
		showNormal();
	else
		show();

	raise();
	activateWindow();
}

/**
 * Repopulates the Window menu with the current list of open windows.
 */
void VuoCodeWindow::updateWindowMenu()
{
	windowMenu->clear();
	static_cast<VuoEditor *>(qApp)->populateWindowMenu(windowMenu, this);
}

/**
 * Enables the Reload action if this shader or a composition containing it is running; disables it otherwise.
 */
void VuoCodeWindow::updateReloadAction()
{
	__block bool isRunning = wrapperComposition->isRunning();
	if (! isRunning)
	{
		string sourcePath = windowFilePath().toStdString();
		static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->applyToAllOtherTopLevelCompositions(nullptr, ^(VuoEditorComposition *topLevelComposition)
		{
			if (topLevelComposition->isRunning() && ! topLevelComposition->getModuleManager()->findInstancesOfNodeClass(sourcePath).empty())
				isRunning = true;
		});
	}

	reloadAction->setEnabled(isRunning);
}

/**
 * Toggles whether the published input ports sidebar is visible or hidden. Updates the Hide/Show menu item accordingly.
 */
void VuoCodeWindow::toggleInputPortSidebarVisibility()
{
	bool becomingVisible = ! inputPortSidebar->isVisible();
	inputPortSidebar->setVisible(becomingVisible);
	updateInputPortSidebarMenuItem();
}

/**
 * Updates the published input port sidebar's Hide/Show menu item to match the sidebar's current visibility.
 */
void VuoCodeWindow::updateInputPortSidebarMenuItem()
{
	toggleInputPortSidebarAction->setText(inputPortSidebar->isVisible() ? tr("Hide Published Ports") : tr("Show Published Ports"));
}

/**
 * Toggles whether the documentation sidebar is visible or hidden.
 */
void VuoCodeWindow::toggleDocumentationSidebarVisibility()
{
	bool becomingVisible = ! documentationSidebar->isVisible();
	documentationSidebar->setVisible(becomingVisible);
	updateDocumentationSidebarMenuItem();
}

/**
 * Updates the documentation sidebar's Hide/Show menu item to match the sidebar's current visibility.
 */
void VuoCodeWindow::updateDocumentationSidebarMenuItem()
{
	toggleDocumentationSidebarAction->setText(documentationSidebar->isVisible() ? tr("Hide GLSL/ISF Quick Reference") : tr("Show GLSL/ISF Quick Reference"));
}

/**
 * Displays an input editor for a published port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the undo stack.
 */
void VuoCodeWindow::showPublishedInputEditor(VuoRendererPort *port)
{
	VuoInputEditorSession *inputEditorSession = new VuoInputEditorSession(inputEditorManager, wrapperComposition, inputPortSidebar, this);
	map<VuoRendererPort *, pair<string, string> > originalAndFinalValueForPort = inputEditorSession->execute(port, true);

	delete inputEditorSession;
	inputEditorSession = nullptr;

	bool valueChanged = false;
	for (auto i : originalAndFinalValueForPort)
	{
		VuoRendererPort *port = i.first;
		string originalEditingSessionValue = i.second.first;
		string finalEditingSessionValue = i.second.second;

		if (finalEditingSessionValue != originalEditingSessionValue)
		{
			valueChanged = true;
			wrapperComposition->updatePortConstant(static_cast<VuoCompilerPort *>(port->getBase()->getCompiler()), finalEditingSessionValue);

			VUserLog("%s:      Set port %s to %s",
				getWindowTitleWithoutPlaceholder().toUtf8().data(),
				port->getBase()->getClass()->getName().c_str(),
				finalEditingSessionValue.c_str());
		}
		else if (port->getConstantAsString() != originalEditingSessionValue)
		{
			// Edit was canceled. Revert the published input value.
			wrapperComposition->updatePortConstant(static_cast<VuoCompilerPort *>(port->getBase()->getCompiler()), originalEditingSessionValue);
		}
	}

	if (valueChanged)
	{
		bringStoredShaderInSyncWithPublishedInputPorts();
		publishedInputsModified = true;
		updateModifiedIndicator();
	}
}

/**
 * Changes the details associated with a published input port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the undo stack.
 */
void VuoCodeWindow::changePublishedPortDetails(VuoRendererPublishedPort *port, json_object *newDetails)
{
	VUserLog("%s:      Set published port '%s' details to %s",
		getWindowTitleWithoutPlaceholder().toUtf8().data(),
		port->getBase()->getClass()->getName().c_str(),
		json_object_to_json_string(newDetails));

	static_cast<VuoCompilerPublishedPortClass *>(port->getBase()->getClass()->getCompiler())->updateDetails(newDetails);

	bringStoredShaderInSyncWithPublishedInputPorts();
	publishedInputsModified = true;
	updateModifiedIndicator();
}

/**
 * Displays a name editor for a published port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the undo stack.
 */
void VuoCodeWindow::showPublishedPortNameEditor(VuoRendererPublishedPort *port)
{
	string originalName = port->getBase()->getClass()->getName();
	inputPortSidebar->updatePortList();
	string newName = inputPortSidebar->showPublishedPortNameEditor(port);

	if (originalName != newName)
		changePublishedPortName(port, newName);
}

/**
 * Renames a published input port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the GLSL code or the undo stack.
 */
void VuoCodeWindow::changePublishedPortName(VuoRendererPublishedPort *port, string newName)
{
	string oldSnapshot = wrapperComposition->takeSnapshot();

	string oldName = port->getBase()->getClass()->getName();
	wrapperComposition->setPublishedPortName(port, newName);

	// setPublishedPortName may have changed the name to ensure it's unique.
	newName = port->getBase()->getClass()->getName();

	VUserLog("%s:      Rename published port %s to %s",
			 getWindowTitleWithoutPlaceholder().toUtf8().data(),
			 oldName.c_str(),
			 newName.c_str());

	bringStoredShaderInSyncWithPublishedInputPorts(nullptr, {oldName, newName});
	publishedInputsModified = true;
	updateModifiedIndicator();

	string newSnapshot = wrapperComposition->takeSnapshot();
	wrapperComposition->updateRunningComposition(oldSnapshot, newSnapshot);
}

/**
 * Adds a published input port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the undo stack.
 */
void VuoCodeWindow::addPublishedPort(string typeName, bool isInput)
{
	string oldSnapshot = wrapperComposition->takeSnapshot();

	VuoType *type = (! typeName.empty() ? compiler->getType(typeName)->getBase() : nullptr);
	string portName = wrapperComposition->getUniquePublishedPortName(wrapperComposition->getDefaultPublishedPortNameForType(type));

	VUserLog("%s:      Add published %s port %s %s",
		getWindowTitleWithoutPlaceholder().toUtf8().data(),
		isInput ? "input" : "output",
		typeName.empty() ? "event" : typeName.c_str(),
		portName.c_str());

	VuoCompilerPublishedPort *publishedPort = VuoCompilerPublishedPort::newPort(portName, type);
	wrapperComposition->addPublishedPort(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);
	VuoRendererPublishedPort *rpp = wrapperComposition->createRendererForPublishedPortInComposition(static_cast<VuoPublishedPort *>(publishedPort->getBase()), true);

	bringStoredShaderInSyncWithPublishedInputPorts(publishedPort);
	publishedInputsModified = true;
	updateModifiedIndicator();

	string newSnapshot = wrapperComposition->takeSnapshot();
	wrapperComposition->updateRunningComposition(oldSnapshot, newSnapshot);

	inputPortSidebar->updatePortList();
	string newName = inputPortSidebar->showPublishedPortNameEditor(static_cast<VuoRendererPublishedPort *>(publishedPort->getBase()->getRenderer()));

	if (portName != newName)
		changePublishedPortName(rpp, newName);
}

/**
 * Deletes a published input port.
 *
 * Edits affect the wrapper composition and the shader's inputs, but not the GLSL code or the undo stack.
 */
void VuoCodeWindow::deletePublishedPort(VuoRendererPublishedPort *port)
{
	VUserLog("%s:      Remove published input port %s",
		getWindowTitleWithoutPlaceholder().toUtf8().data(),
		port->getBase()->getClass()->getName().c_str());

	string oldSnapshot = wrapperComposition->takeSnapshot();

	for (VuoCable *cable : port->getBase()->getConnectedCables(true))
		wrapperComposition->removeCable(cable->getRenderer());

	string portName = port->getBase()->getClass()->getName();
	wrapperComposition->removePublishedPort(static_cast<VuoPublishedPort *>(port->getBase()), true);

	bringStoredShaderInSyncWithPublishedInputPorts(nullptr, {}, portName);
	publishedInputsModified = true;
	updateModifiedIndicator();

	string newSnapshot = wrapperComposition->takeSnapshot();
	wrapperComposition->updateRunningComposition(oldSnapshot, newSnapshot);
}

/**
 * Updates the metadata associated with the shader to match what the user entered in the Composition Information dialog.
 *
 * Edits affect the wrapper composition and the shader's metadata, but not the undo stack.
 */
void VuoCodeWindow::changeMetadata(int dialogResult)
{
	if (dialogResult == QDialog::Accepted)
	{
		VuoCompositionMetadata *metadata = metadataEditor->toMetadata();
		wrapperComposition->getBase()->setMetadata(metadata, true);

		shaderFile->setMetadata(metadata);

		metadataModified = true;
		updateModifiedIndicator();

		VUserLog("%s:      Set metadata to:\n%s",
			getWindowTitleWithoutPlaceholder().toUtf8().data(),
			metadata->toCompositionHeader().c_str());
	}
}

/**
 * Replaces the Run button icon with an activity indicator.
 */
void VuoCodeWindow::showBuildActivityIndicator()
{
	toolbar->changeStateToBuildInProgress();
	updateToolbar();

	runAction->setEnabled(false);
	stopAction->setEnabled(true);
	restartAction->setEnabled(true);
}

/**
 * Removes the activity indicator from the Run button icon.
 */
void VuoCodeWindow::hideBuildActivityIndicator(QString buildError)
{
	toolbar->changeStateToRunning();
	updateToolbar();

	runAction->setEnabled(false);
	stopAction->setEnabled(true);
	restartAction->setEnabled(true);
	updateReloadAction();
}

/**
 * Replaces the Stop button icon with an activity indicator.
 */
void VuoCodeWindow::showStopActivityIndicator()
{
	toolbar->changeStateToStopInProgress();
	updateToolbar();

	runAction->setEnabled(!toolbar->isBuildPending());
	stopAction->setEnabled(false);
	restartAction->setEnabled(false);
}

/**
 * Removes the activity indicator from the Stop button icon.
 */
void VuoCodeWindow::hideStopActivityIndicator()
{
	toolbar->changeStateToStopped();
	updateToolbar();

	runAction->setEnabled(true);
	stopAction->setEnabled(false);
	restartAction->setEnabled(false);
	updateReloadAction();
}

/**
 * Restores the default text size (100% aka 1:1).
 */
void VuoCodeWindow::zoom11()
{
	stages->zoom11();

	zoom11Action->setEnabled(false);
	toolbar->update(false, true, false);
}

/**
 * Makes the text larger.
 */
void VuoCodeWindow::zoomIn()
{
	stages->zoomIn();

	bool isActualSize = stages->currentEditor()->isZoomedToActualSize();
	zoom11Action->setEnabled(! isActualSize);
	toolbar->update(false, isActualSize, false);
}

/**
 * Makes the text smaller.
 */
void VuoCodeWindow::zoomOut()
{
	stages->zoomOut();

	bool isActualSize = stages->currentEditor()->isZoomedToActualSize();
	zoom11Action->setEnabled(! isActualSize);
	toolbar->update(false, isActualSize, false);
}

/**
 * Copies the selected text in the code editor or documentation sidebar to the clipboard.
 */
void VuoCodeWindow::copy()
{
	if (stages->currentEditor() == qApp->focusWidget())
		stages->currentEditor()->copy();
	else if (documentationSidebar->isAncestorOf(qApp->focusWidget()))
		QGuiApplication::clipboard()->setText(documentationSidebar->getSelectedText());
}
