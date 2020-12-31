/**
 * @file
 * VuoEditor implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditor.hh"

#include "VuoCodeWindow.hh"
#include "VuoCompilerDriver.hh"
#include "VuoCompilerIssue.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoCompilerComposition.hh"
#include "VuoEditorAboutBox.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorUtilities.hh"
#include "VuoEditorWindow.hh"
#include "VuoErrorDialog.hh"
#include "VuoException.hh"
#include "VuoModuleManager.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"
#include "VuoStringUtilities.hh"
#include "VuoRendererCommon.hh"
#include "VuoRendererFonts.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoExampleMenu.hh"
#include "VuoNodePopover.hh"
#include "VuoOsStatus.h"
#include "VuoProtocol.hh"
#include "VuoRecentFileMenu.hh"
#include "VuoRendererComposition.hh"
#include "VuoSubcompositionMessageRouter.hh"

#include <langinfo.h>
#include <sstream>
#include <fstream>

#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <dlfcn.h>
#include <objc/objc-runtime.h>

const QString VuoEditor::recentFileListSettingsKey = "recentFileList";
const QString VuoEditor::subcompositionPrefixSettingsKey = "subcompositionPrefix";
const QString VuoEditor::nodeLibraryDisplayModeSettingsKey = "nodeLibraryDisplayMode";
const QString VuoEditor::nodeLibraryVisibilityStateSettingsKey = "nodeLibraryVisible";
const QString VuoEditor::nodeLibraryDockingStateSettingsKey = "nodeLibraryDocked";
const QString VuoEditor::nodeLibraryFloatingPositionSettingsKey = "nodeLibraryFloatingPosition";
const QString VuoEditor::nodeLibraryWidthSettingsKey = "nodeLibraryWidth";
const QString VuoEditor::nodeLibraryHeightSettingsKey = "nodeLibraryHeight";
const QString VuoEditor::nodeDocumentationPanelHeightSettingsKey = "nodeDocumentationPanelHeight";
const QString VuoEditor::shaderDocumentationVisibilitySettingsKey = "shaderDocumentationVisible";
const QString VuoEditor::gridTypeSettingsKey = "canvasGridType";
const QString VuoEditor::gridOpacitySettingsKey = "canvasGridOpacity";
const qreal   VuoEditor::defaultGridOpacity = 1;
const QString VuoEditor::snapToGridSettingsKey = "canvasGridSnap";
const QString VuoEditor::darkInterfaceSettingsKey = "darkInterface";
const QString VuoEditor::canvasOpacitySettingsKey = "canvasOpacity";
const QString VuoEditor::movieExportWidthSettingsKey = "movieExportWidth";
const QString VuoEditor::movieExportHeightSettingsKey = "movieExportHeight";
const QString VuoEditor::movieExportTimeSettingsKey = "movieExportTime";
const QString VuoEditor::movieExportDurationSettingsKey = "movieExportDuration";
const QString VuoEditor::movieExportFramerateSettingsKey = "movieExportFramerate";
const QString VuoEditor::movieExportSpatialSupersampleSettingsKey = "movieExportSpatialSupersample";
const QString VuoEditor::movieExportTemporalSupersampleSettingsKey = "movieExportTemporalSupersample";
const QString VuoEditor::movieExportShutterAngleSettingsKey = "movieExportShutterAngle";
const QString VuoEditor::movieExportImageFormatSettingsKey = "movieExportImageFormat";
const QString VuoEditor::movieExportQualitySettingsKey = "movieExportQuality";

const QString VuoEditor::vuoHelpBookScheme = "vuo-help";  ///< The URL scheme used for opening a section of the user manual.
const QString VuoEditor::vuoNodeSetDocumentationScheme = "vuo-nodeset";	///< The URL scheme used for opening node set documentation.
const QString VuoEditor::vuoNodeDocumentationScheme = "vuo-node"; ///< The URL scheme used for opening node documentation.
const QString VuoEditor::vuoExampleCompositionScheme = "vuo-example";	///< The URL scheme used for opening example compositions.
const QString VuoEditor::vuoExampleHighlightedNodeClassQueryItem = "nodeClass";	///< The query item expected to indicate what node class to highlight within an example composition.
const QString VuoEditor::vuoCompositionFileExtension = "vuo";	///< The file extension for Vuo compositions.
const QString VuoEditor::vuoNodeClassFileExtension = "vuonode";	///< The file extension for Vuo node classes.
const QString VuoEditor::vuoTutorialURL = "https://vuo.org/tutorials"; ///< The URL for Vuo tutorial videos.

string VuoEditor::documentationGenerationDirectory = "";

/**
 * Sets up the Vuo Editor at launch time.
 */
VuoEditor::VuoEditor(int &argc, char *argv[])
	: QApplication(argc,argv)
{
	compilerQueue = dispatch_queue_create("org.vuo.editor.compiler", DISPATCH_QUEUE_SERIAL);

	// Load stored application settings.
	QCoreApplication::setOrganizationName("Vuo");
	QCoreApplication::setOrganizationDomain("vuo.org");
	QCoreApplication::setApplicationName("Editor");
	settings = new QSettings();

	loadTranslations();

	VuoEditorWindow::untitledComposition = QObject::tr("Untitled Composition");


	// Controls the 2-finger-scroll speed.
	// Defaults to 3 which (after https://b33p.net/kosada/node/13245) feels a little too fast.
	// 2 seems to better match Preview.app.
	setWheelScrollLines(2);


	uiInitialized = false;
	VuoRendererComposition::createAutoreleasePool();

	setQuitOnLastWindowClosed(false);
	aboutBox = nullptr;
	ownedNodeLibrary = NULL;
	documentationQueue = dispatch_queue_create("org.vuo.editor.documentation", NULL);
	userSubscriptionLevel = VuoEditor::CommunityUser;
	reportAbsenceOfUpdates = false;
	networkManager = NULL;
	compiler = nullptr;
	moduleManager = nullptr;
	subcompositionRouter = new VuoSubcompositionMessageRouter();

	// Initialize default settings.
	const VuoNodeLibrary::nodeLibraryDisplayMode defaultNodeLibraryDisplayMode = VuoNodeLibrary::displayByName;
	const bool defaultNodeLibraryVisibilityState = true;	// Node library is visible by default.
	const bool defaultNodeLibraryDockingState = true;		// When visible, node library is docked by default.
	const int defaultNodeLibraryDocumentationPanelHeight = 280;

	qApp->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);
	qApp->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

	qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
	QToolTip::setFont(VuoRendererFonts::getSharedFonts()->portPopoverFont());

	// Disable macOS's restore-open-documents-on-launch feature, since it doesn't work with Qt anyway,
	// and since it pops up a meaningless "Do you want to try to reopen its windows again?" window after a crash.
	settings->setValue("ApplePersistenceIgnoreStateQuietly", true);

	QStringList storedRecentFileList = settings->value(recentFileListSettingsKey).toStringList();

	menuOpenRecent = new VuoRecentFileMenu();
	menuOpenRecent->setRecentFiles(storedRecentFileList);


#if VUO_PRO
	VuoEditor_Pro0();
#endif

	subcompositionPrefix = settings->contains(subcompositionPrefixSettingsKey)?
							   settings->value(subcompositionPrefixSettingsKey).toString() :
							   "";

	canvasOpacity = settings->contains(canvasOpacitySettingsKey)?
						settings->value(canvasOpacitySettingsKey).toInt() :
						255;

	nodeLibraryDisplayMode = settings->contains(nodeLibraryDisplayModeSettingsKey)?
									  (VuoNodeLibrary::nodeLibraryDisplayMode)settings->value(nodeLibraryDisplayModeSettingsKey).toInt() :
									  defaultNodeLibraryDisplayMode;

	nodeLibraryCurrentlyVisible = settings->contains(nodeLibraryVisibilityStateSettingsKey)?
									  settings->value(nodeLibraryVisibilityStateSettingsKey).toBool() :
									  defaultNodeLibraryVisibilityState;

	nodeLibraryCurrentlyDocked = settings->contains(nodeLibraryDockingStateSettingsKey)?
									 settings->value(nodeLibraryDockingStateSettingsKey).toBool() :
									 defaultNodeLibraryDockingState;

	if (settings->contains(nodeLibraryFloatingPositionSettingsKey))
	{
		settingsContainedNodeLibraryFloatingPosition = true;
		nodeLibraryFloatingPosition = settings->value(nodeLibraryFloatingPositionSettingsKey).toPoint();
	}
	else
	{
		settingsContainedNodeLibraryFloatingPosition = false;
		nodeLibraryFloatingPosition = QPoint();
	}

	if (settings->contains(nodeLibraryWidthSettingsKey))
		nodeLibraryWidth = settings->value(nodeLibraryWidthSettingsKey).toInt();
	else
		nodeLibraryWidth = -1;

	if (settings->contains(nodeLibraryHeightSettingsKey))
		nodeLibraryHeight = settings->value(nodeLibraryHeightSettingsKey).toInt();
	else
		nodeLibraryHeight = -1;

	if (settings->contains(nodeDocumentationPanelHeightSettingsKey))
		nodeDocumentationPanelHeight = settings->value(nodeDocumentationPanelHeightSettingsKey).toInt();
	else
		nodeDocumentationPanelHeight = defaultNodeLibraryDocumentationPanelHeight;

	previousVisibleNodeLibraryStateWasDocked = nodeLibraryCurrentlyDocked;
	currentFloatingNodeLibrary = NULL;

	shaderDocumentationVisible = settings->contains(shaderDocumentationVisibilitySettingsKey)?
									 settings->value(shaderDocumentationVisibilitySettingsKey).toBool() :
									 true;

	applyStoredMovieExportSettings();

	// Disable the Mac OS 10.7+ restore-open-documents-on-launch feature, since it doesn't work with Qt anyway,
	// and since it pops up a meaningless "Do you want to try to reopen its windows again?" window after a crash.
	// (Reverted for now, since it it causes "ApplePersistenceIgnoreState: Existing state will not be touched. New state will be written to…" console messages.
//	settings->setValue("ApplePersistenceIgnoreState", true);

	compiler = new VuoCompiler();
	moduleManager = new VuoModuleManager(compiler);

#if VUO_PRO
	VuoEditor_Pro1();
#endif

	builtInDriversQueue = dispatch_queue_create("org.vuo.editor.drivers", NULL);
	dispatch_async(builtInDriversQueue, ^{
		initializeBuiltInDrivers();
	});

	compiler->prepareForFastBuild();

	// If operating in documentation-generation mode, generate the node documentation and exit.
	if (!documentationGenerationDirectory.empty())
	{
		menuBar = nullptr;
		dispatch_async(dispatch_get_main_queue(), ^{
						   generateAllNodeSetHtmlDocumentation(documentationGenerationDirectory);
						   quitCleanly();
					   });
		return;
	}

	// Register the URL handler for the "vuo-help" scheme, used for displaying a section of the user manual.
	QDesktopServices::setUrlHandler(vuoHelpBookScheme, this, "openHelpBookPageFromUrl");

	// Register the URL handler for the "vuo-nodeset" scheme, used for displaying node set documentation.
	QDesktopServices::setUrlHandler(vuoNodeSetDocumentationScheme, this, "showNodeSetDocumentationFromUrl");

	// Register the URL handler for the "vuo-node" scheme, used for displaying node documentation.
	QDesktopServices::setUrlHandler(vuoNodeDocumentationScheme, this, "showNodeDocumentationFromUrl");

	// Register the URL handler for the "vuo-example" scheme, used for opening example compositions.
	QDesktopServices::setUrlHandler(vuoExampleCompositionScheme, this, "openExampleCompositionFromUrl");

	// Construct the basic menu bar here, so that it's visible when no document windows are open.
	menuBar = new QMenuBar();
	{
		menuFile = new QMenu(menuBar);
		menuFile->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		menuFile->setTitle(tr("&File"));

		menuFile->addAction(tr("&New Composition"), this, SLOT(newComposition()), QKeySequence("Ctrl+N"));

		menuNewCompositionWithTemplate = new QMenu(tr("New Composition from Template"));
		menuNewCompositionWithTemplate->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		populateNewCompositionWithTemplateMenu(menuNewCompositionWithTemplate);
		menuFile->addMenu(menuNewCompositionWithTemplate);

		QMenu *menuNewShader = new QMenu(tr("New Shader"));
		menuNewShader->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		populateNewShaderMenu(menuNewShader);
		menuFile->addMenu(menuNewShader);

		menuFile->addSeparator();
		menuFile->addAction(tr("&Open…"), this, SLOT(openFile()), QKeySequence("Ctrl+O"));

		// "Open Recent" menu
		connect(menuOpenRecent, &VuoRecentFileMenu::recentFileSelected, this, &VuoEditor::openUrl);
		connect(menuOpenRecent, &VuoRecentFileMenu::recentFileListCleared, this, &VuoEditor::synchronizeOpenRecentFileMenus);
		menuFile->addMenu(menuOpenRecent);
		connect(menuFile, &QMenu::aboutToShow, this, &VuoEditor::pruneAllOpenRecentFileMenus);

		// "Open Example" menu
		menuOpenExample = new VuoExampleMenu(menuFile, compiler);
		connect(menuOpenExample, &VuoExampleMenu::exampleSelected, this, &VuoEditor::openUrl);
		menuFile->addMenu(menuOpenExample);

		// Connect the "Quit" menu item action to our customized quit method.  On Mac OS X, this menu
		// item will automatically be moved from the "File" menu to the Application menu.
		menuFile->addAction(tr("Quit"), this, SLOT(quitCleanly()), QKeySequence("Ctrl+Q"));

		// "About" menu item
		menuFile->addAction(tr("About Vuo…"), this, SLOT(about()));

		// Workaround for bug preventing the "Quit" and "About" menu items from appearing within the Application menu:
		// Add them again to the "File" menu, this time with Qt's automatic menu merging behavior disabled;
		// this at least gains us a functional keyboard shortcut for the "Quit" item. (@todo: Fix properly; see
		// https://b33p.net/kosada/node/5260 ).
		QAction *aboutAction = new QAction(NULL);
		aboutAction->setText(tr("About Vuo…"));
		connect(aboutAction, &QAction::triggered, this, &VuoEditor::about);
		aboutAction->setMenuRole(QAction::NoRole); // Disable automatic menu merging for this item.
		menuFile->addSeparator();
		menuFile->addAction(aboutAction);

		QAction *quitAction = new QAction(NULL);
		quitAction->setText(tr("&Quit Vuo"));
		quitAction->setShortcut(QKeySequence("Ctrl+Q"));
		connect(quitAction, &QAction::triggered, this, &VuoEditor::quitCleanly);
		quitAction->setMenuRole(QAction::NoRole); // Disable automatic menu merging for this item.
		menuFile->addSeparator();
		menuFile->addAction(quitAction);

		menuBar->addAction(menuFile->menuAction());

#if VUO_PRO
		enableMenuItems(menuFile, canCloseWelcomeWindow());
#endif
	}

	{
		menuView = new QMenu(menuBar);
		menuView->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		menuView->setTitle(tr("&View"));

		// "Show Node Library"
		showNodeLibraryAction = new QAction(NULL);
		showNodeLibraryAction->setText(tr("Show Node &Library"));
		showNodeLibraryAction->setShortcut(QKeySequence("Ctrl+Return"));
		connect(showNodeLibraryAction, &QAction::triggered, this, &VuoEditor::showNodeLibrary);
		menuView->addAction(showNodeLibraryAction);


		// "Grid" submenu
		{
			// "Snap" menu item
			snapToGridAction = new QAction(NULL);
			snapToGridAction->setText(tr("Snap"));
			snapToGridAction->setCheckable(true);
			bool snapToGrid = (settings->contains(snapToGridSettingsKey)?
								   settings->value(snapToGridSettingsKey).toBool() :
								   true);
			VuoRendererItem::setSnapToGrid(snapToGrid);
			snapToGridAction->setChecked(snapToGrid);
			connect(snapToGridAction, &QAction::toggled, this, &VuoEditor::updateSnapToGrid);

			// "Line"/"Points" menu items
			int gridOpacity;
			if (settings->contains(gridOpacitySettingsKey))
				gridOpacity = settings->value(gridOpacitySettingsKey).toInt();
			else
			{
				gridOpacity = VuoEditor::defaultGridOpacity;
				settings->setValue(gridOpacitySettingsKey, gridOpacity);
			}

			VuoRendererComposition::setGridOpacity(gridOpacity);

			int gridType = (settings->contains(gridTypeSettingsKey)
							? settings->value(gridTypeSettingsKey).toInt()
							: VuoRendererComposition::LineGrid);
			VuoRendererComposition::setGridType((VuoRendererComposition::GridType)gridType);

			showGridLinesAction = new QAction(NULL);
			showGridLinesAction->setText(tr("Lines"));
			showGridLinesAction->setCheckable(true);
			showGridLinesAction->setChecked(gridType == VuoRendererComposition::LineGrid);
			connect(showGridLinesAction,  &QAction::triggered, this, &VuoEditor::showGridLinesToggled);

			showGridPointsAction = new QAction(NULL);
			showGridPointsAction->setText(tr("Points"));
			showGridPointsAction->setCheckable(true);
			showGridPointsAction->setChecked(gridType == VuoRendererComposition::PointGrid);
			connect(showGridPointsAction, &QAction::triggered, this, &VuoEditor::showGridPointsToggled);
		}


		// "Canvas Transparency"
		canvasTransparencyNoneAction = new QAction(NULL);
		canvasTransparencyNoneAction->setText(tr("None"));
		canvasTransparencyNoneAction->setData(255);
		canvasTransparencyNoneAction->setCheckable(true);
		canvasTransparencyNoneAction->setShortcut(QKeySequence("Ctrl+1"));

		canvasTransparencySlightAction = new QAction(NULL);
		canvasTransparencySlightAction->setText(tr("Slightly Transparent"));
		canvasTransparencySlightAction->setData(230);
		canvasTransparencySlightAction->setCheckable(true);
		canvasTransparencySlightAction->setShortcut(QKeySequence("Ctrl+2"));

		canvasTransparencyHighAction = new QAction(NULL);
		canvasTransparencyHighAction->setText(tr("Very Transparent"));
		canvasTransparencyHighAction->setData(191);
		canvasTransparencyHighAction->setCheckable(true);
		canvasTransparencyHighAction->setShortcut(QKeySequence("Ctrl+3"));

		canvasTransparencyOptions = new QActionGroup(this);
		canvasTransparencyOptions->addAction(canvasTransparencyNoneAction);
		canvasTransparencyOptions->addAction(canvasTransparencySlightAction);
		canvasTransparencyOptions->addAction(canvasTransparencyHighAction);

		foreach (QAction *action, canvasTransparencyOptions->actions())
		{
			if (action->data().toInt() == canvasOpacity)
			{
				action->setChecked(true);
				action->setEnabled(false);
				break;
			}
		}

		// updateCanvasOpacity() needs to happen before canvasOpacityChanged(),
		// since slots attached to the latter rely on VuoEditor::getCanvasOpacity() which is updated by the former.
		connect(canvasTransparencyOptions, &QActionGroup::triggered, this, &VuoEditor::updateCanvasOpacity);
		connect(canvasTransparencyOptions, &QActionGroup::triggered, this, &VuoEditor::canvasOpacityChanged);

		menuBar->addAction(menuView->menuAction());
	}

	{
		// @todo: For some reason the following doesn't work; no "Window" menu item is displayed.
		menuWindow = new QMenu(menuBar);
		menuWindow->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		menuWindow->setTitle(tr("&Window"));

		populateWindowMenu(menuWindow, nullptr);

		connect(menuWindow, &QMenu::aboutToShow, this, &VuoEditor::updateUI);

		menuBar->addAction(menuWindow->menuAction());
	}

	{
		menuHelp = new QMenu(menuBar);
		menuHelp->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		menuHelp->setTitle(tr("&Help"));
		populateHelpMenu(menuHelp);

		// Prevent "Help > Search" from triggering lookup of all customized example composition titles at once.
		connect(menuHelp, &QMenu::aboutToShow, menuOpenExample, &VuoExampleMenu::disableExampleTitleLookup);
		connect(menuHelp, &QMenu::aboutToHide, menuOpenExample, &VuoExampleMenu::enableExampleTitleLookup);

		connect(menuHelp, &QMenu::aboutToShow, [this] { populateHelpMenu(menuHelp); });

		menuBar->addAction(menuHelp->menuAction());
	}

#ifdef __APPLE__
	{
		// Construct an OS X dock context menu.
		// Items included in this menu will supplement the default OS X dock context menu items.
		dockContextMenu = new QMenu();
		dockContextMenu->setSeparatorsCollapsible(false); /// @todo https://b33p.net/kosada/node/8133
		dockContextMenu->setAsDockMenu();

		connect(dockContextMenu, &QMenu::aboutToShow, this, &VuoEditor::updateUI);
	}
#endif

	initializeTopLevelNodeLibrary(compiler,
								  nodeLibraryDisplayMode,
								  settingsContainedNodeLibraryFloatingPosition,
								  nodeLibraryFloatingPosition,
								  nodeLibraryWidth,
								  nodeLibraryHeight);
	if (!nodeLibraryCurrentlyDocked)
		designateNewFloatingNodeLibrary(ownedNodeLibrary);

	moduleManager->setNodeLibrary(ownedNodeLibrary);


	// @todo https://b33p.net/kosada/node/14299 The following line causes nodes to be listed in the
	// floating node library immediately on launch (good), but then to be duplicated moments later (bad).
	//moduleManager->updateWithAlreadyLoadedModules();
	conformToGlobalNodeLibraryVisibility(
		getGlobalNodeLibraryStateForAttributes(nodeLibraryCurrentlyVisible, nodeLibraryCurrentlyDocked),
		currentFloatingNodeLibrary);
	ownedNodeLibrary->displayPopoverForCurrentNodeClass();

	// Update the global node library display mode whenever it is changed for the top-level node library.
	connect(ownedNodeLibrary, &VuoNodeLibrary::changedIsHumanReadable, static_cast<VuoEditor *>(qApp), &VuoEditor::updateNodeLibraryDisplayMode);

	// Update the "Show/Hide Node Library" menu item when the node library visibility or display mode is changed.
	connect(ownedNodeLibrary, &VuoNodeLibrary::visibilityChanged, this, &VuoEditor::updateUI);
	enableGlobalStateConformanceToLibrary(ownedNodeLibrary);
	connect(this, &VuoEditor::globalNodeLibraryStateChanged, this, &VuoEditor::conformToGlobalNodeLibraryVisibility);

#ifndef VUO_PRO
	QApplication::processEvents(); // Force processing of any incoming QEvent::FileOpen event, which may populate queuedCompositionsToOpen.
	if (queuedCompositionsToOpen.empty())
		newComposition();
#endif

	// Initialize a file dialogue with its default directory set to something reasonable, so that
	// file dialogues initialized subsequently within the same editor session will default to
	// this location rather than to the executable directory.
	// See https://b33p.net/kosada/node/7302 .
	QFileDialog d;
	if (d.history().isEmpty())
		d.setDirectory(getDefaultCompositionStorageDirectory());

	uiInitialized = true;
	processQueuedCompositionsToOpen();
	updateUI();
}

/**
 * Destructor.
 */
VuoEditor::~VuoEditor()
{
	dispatch_sync(builtInDriversQueue, ^{});

	if (menuBar)
		menuBar->deleteLater();
	delete subcompositionRouter;
	moduleManager->deleteWhenReady();  // deletes compiler
}

/**
 * This is a copy of private method QTranslator::find_translation(), for debugging.
 */
QString VuoEditor::qtFindTranslation(const QLocale &locale, const QString &filename, const QString &prefix, const QString &directory, const QString &suffix)
{
	QString path;
	if (QFileInfo(filename).isRelative()) {
		path = directory;
		if (!path.isEmpty() && !path.endsWith(QLatin1Char('/')))
			path += QLatin1Char('/');
	}

	QFileInfo fi;
	QString realname;
	QStringList fuzzyLocales;

	// see http://www.unicode.org/reports/tr35/#LanguageMatching for inspiration

	QStringList languages = locale.uiLanguages();
	for (int i = languages.size()-1; i >= 0; --i) {
		QString lang = languages.at(i);
		QString lowerLang = lang.toLower();
		if (lang != lowerLang)
			languages.insert(i+1, lowerLang);
	}

	// try explicit locales names first
	foreach (QString localeName, languages) {
		localeName.replace(QLatin1Char('-'), QLatin1Char('_'));

		realname = path + filename + prefix + localeName + (suffix.isNull() ? QLatin1String(".qm") : suffix);
		fi.setFile(realname);
		if (fi.isReadable() && fi.isFile())
			return realname;

		realname = path + filename + prefix + localeName;
		fi.setFile(realname);
		if (fi.isReadable() && fi.isFile())
			return realname;

		fuzzyLocales.append(localeName);
	}

	// start guessing
	foreach (QString localeName, fuzzyLocales) {
		for (;;) {
			int rightmost = localeName.lastIndexOf(QLatin1Char('_'));
			// no truncations? fail
			if (rightmost <= 0)
				break;
			localeName.truncate(rightmost);

			realname = path + filename + prefix + localeName + (suffix.isNull() ? QLatin1String(".qm") : suffix);
			fi.setFile(realname);
			if (fi.isReadable() && fi.isFile())
				return realname;

			realname = path + filename + prefix + localeName;
			fi.setFile(realname);
			if (fi.isReadable() && fi.isFile())
				return realname;
		}
	}

	if (!suffix.isNull()) {
		realname = path + filename + suffix;
		fi.setFile(realname);
		if (fi.isReadable() && fi.isFile())
			return realname;
	}

	realname = path + filename + prefix;
	fi.setFile(realname);
	if (fi.isReadable() && fi.isFile())
		return realname;

	realname = path + filename;
	fi.setFile(realname);
	if (fi.isReadable() && fi.isFile())
		return realname;

	return QString();
}

/**
 * Determines which UI language translation set to load, and loads and installs it.
 */
void VuoEditor::loadTranslations()
{
	if (settings->value("translation/enable", true).toBool())
	{
		QLocale locale = QLocale::system();
		if (VuoIsDebugEnabled())
		{
			VUserLog("C locale:");
			VUserLog("    NL codeset         : %s", nl_langinfo(CODESET));
			VUserLog("    LANG               : %s", getenv("LANG"));
			VUserLog("    LC_ALL             : %s", getenv("LC_ALL"));

			VUserLog("Apple locale:");
			CFStringRef appleLocale = (CFStringRef)CFPreferencesCopyAppValue(CFSTR("AppleLocale"), kCFPreferencesCurrentApplication);
			VUserLog("    ISO (pref)         : %s", VuoStringUtilities::makeFromCFString(appleLocale).c_str());
			CFRelease(appleLocale);

			CFLocaleRef localeCF = CFLocaleCopyCurrent();
			VUserLog("    ISO (CF)           : %s", VuoStringUtilities::makeFromCFString(CFLocaleGetIdentifier(localeCF)).c_str());
			CFRelease(localeCF);

			auto logCFArray = ^(const char *label, CFArrayRef array){
				CFIndex count = CFArrayGetCount(array);
				vector<string> s;
				for (CFIndex i = 0; i < count; ++i)
					s.push_back(VuoStringUtilities::makeFromCFString(CFArrayGetValueAtIndex(array, i)));
				VUserLog("    %s: %s", label, VuoStringUtilities::join(s, ',').c_str());
				CFRelease(array);
			};

			logCFArray("UI languages (pref)", (CFArrayRef)CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), kCFPreferencesCurrentApplication));

			logCFArray("UI languages (CF)  ", CFLocaleCopyPreferredLanguages());

			VUserLog("Qt locale:");
			VUserLog("    ISO                : %s", locale.name().toUtf8().data());
			VUserLog("    BCP47              : %s", locale.bcp47Name().toUtf8().data());
			VUserLog("    country            : %s (%s)", QLocale::countryToString(locale.country()).toUtf8().data(), locale.nativeCountryName().toUtf8().data());
			VUserLog("    language           : %s (%s)", QLocale::languageToString(locale.language()).toUtf8().data(), locale.nativeLanguageName().toUtf8().data());
			VUserLog("    UI languages       : %s", locale.uiLanguages().join(',').toUtf8().data());
			VUserLog("    script             : %s", QLocale::scriptToString(locale.script()).toUtf8().data());
		}

		QString translationsPath = QDir(QString::fromStdString(VuoFileUtilities::getVuoFrameworkPath() + "/../../Resources/Translations")).canonicalPath();
		QList<QString> contexts;
		contexts << "qtbase" << "vuo";
		QString prefix("_");
		QString suffix(".qm");
		foreach (QString context, contexts)
		{
			VDebugLog("Context %s:", context.toUtf8().data());

			QTranslator *t = new QTranslator();
			QString translationFilename = qtFindTranslation(locale, context, prefix, translationsPath, suffix);
			if (translationFilename.isEmpty())
			{
				VDebugLog("    No translation file found.");
				continue;
			}

			bool loaded = t->load(locale, context, prefix, translationsPath, suffix);
			VDebugLog("    Loading    '%s': %s", translationFilename.toUtf8().data(), loaded ? "ok" : "error");
			if (!loaded)
				continue;

			bool installed = installTranslator(t);
			VDebugLog("    Installing '%s': %s", translationFilename.toUtf8().data(), installed ? "ok" : "error");
		}
	}
	else
		VDebugLog("Disabling translations since preference `translation.enable` is false.");
}

/**
 * Iterates through the queue of pending files to open and tries to open each one.
 */
 void VuoEditor::processQueuedCompositionsToOpen()
 {
	 vector<QString> queuedCompositionsToOpenSnapshot(queuedCompositionsToOpen);
	 queuedCompositionsToOpen.clear();

	 // Iterate through a copy of the queue rather than the original queue,
	 // because calls to openUrl() might add the file back into the original queue,
	 // so we need the clear() call to have happened before that.
 	foreach (QString queuedUrl, queuedCompositionsToOpenSnapshot)
		openUrl(queuedUrl);
 }

 /**
 * Allow pending file-open events to override the welcome window display
 * if the welcome window is in a state where it is allowed to be closed.
 */
 void VuoEditor::reevaluateWelcomePrecedence()
 {
#if VUO_PRO
	 enableMenuItems(menuFile, canCloseWelcomeWindow());

	 if (!queuedCompositionsToOpen.empty())
		 closeWelcomeWindow();
#endif
 }

 /**
 * Sets the enabled/disabled state of all items within the provided @c menu,
 * with the exception of certain items (e.g., "Quit") that should never be disabled
 * and certain other items (e.g., headers) that should never be enabled.
 */
 void VuoEditor::enableMenuItems(QMenu *menu, bool enable)
 {
	 if (!menu)
		return;

	 foreach (QAction *action, menu->actions())
	 {
		 // Apply recursively to submenus.
		 if (action->menu())
		 {
			 VuoExampleMenu *exampleMenu = dynamic_cast<VuoExampleMenu *>(action->menu());
			 if (exampleMenu)
				 exampleMenu->enableMenuItems(enable);
			 else
				 enableMenuItems(action->menu(), enable);
		 }
		 else
		 {
			 bool isQuitOrHelp = action->text().contains("Quit") || action->text().contains("About");
			 bool isTemplateHeader = (menu == menuNewCompositionWithTemplate) &&
									 action->data().value<QString>().isEmpty() &&
									 !action->data().value<void *>();
			 if (!(isQuitOrHelp || isTemplateHeader))
				 action->setEnabled(enable);
		 }
	 }
 }

/**
 * Returns the list of recently-opened files.
 */
QStringList VuoEditor::recentFiles()
{
	menuOpenRecent->pruneNonexistentFiles();
	return menuOpenRecent->getRecentFiles();
}

/**
 * Initiates final cleanup, and attempts to close all windows.
 */
void VuoEditor::quitCleanly()
{
	VUserLog("Conditionally quit");

	// Write any unsaved settings changes to permanent storage.
	settings->sync();

	// Prevent node library visibility changes from impacting application-global settings
	// from this point forward, since we are about to forcibly close them.
	if (ownedNodeLibrary)
		disableGlobalStateConformanceToLibrary(ownedNodeLibrary);

	foreach (VuoEditorWindow *window, VuoEditorUtilities::getOpenCompositionEditingWindows())
		disableGlobalStateConformanceToLibrary(window->getOwnedNodeLibrary());

	// Try to close each window sequentially, in the order they're stacked.
	windowsRemainingAfterQuitRequested = VuoEditorUtilities::getOpenEditingWindowsStacked();
	if (windowsRemainingAfterQuitRequested.empty())
		reallyQuit();
	else
		// Start the chain reaction.
		windowsRemainingAfterQuitRequested[0]->close();
}

/**
 * Called by VuoEditorWindow after the user has _cancelled_ a window-close request.
 *
 * If a quit is in progress, it is cancelled (by clearing the list of windows awaiting user action),
 * and the node library is reenabled.
 */
void VuoEditor::cancelQuit()
{
	if (windowsRemainingAfterQuitRequested.empty())
		// The user hasn't requested to quit, so do nothing.
		return;

	// Restore node library connections.
	foreach (QMainWindow *window, windowsRemainingAfterQuitRequested)
	{
		VuoEditorWindow *compositionWindow = dynamic_cast<VuoEditorWindow *>(window);
		if (compositionWindow)
			enableGlobalStateConformanceToLibrary(compositionWindow->getOwnedNodeLibrary());
	}

	if (ownedNodeLibrary)
		enableGlobalStateConformanceToLibrary(ownedNodeLibrary);

	windowsRemainingAfterQuitRequested.clear();
}

/**
 * Called by VuoEditorWindow or VuoCodeWindow after the user has _confirmed_ a window-close request.
 *
 * If a quit is in progress, the specified window is removed from the list of windows awaiting user action.
 * Then we try closing the next window (or if the list is now empty, we really quit).
 */
void VuoEditor::continueQuit(QMainWindow *window)
{
	if (windowsRemainingAfterQuitRequested.empty())
		// The user hasn't requested to quit, so do nothing.
		return;

	windowsRemainingAfterQuitRequested.removeOne(window);

	if (windowsRemainingAfterQuitRequested.empty())
		reallyQuit();
	else
		// Continue the chain reaction.
		windowsRemainingAfterQuitRequested[0]->close();
}

/**
 * Cleans up, then unconditionally and immediately quits the app.
 */
void VuoEditor::reallyQuit()
{
	VUserLog("Quit");

	try
	{
		VuoCompiler::deleteOldModuleCaches();
	}
	catch (...)
	{
		// Do nothing; it doesn't matter if this sometimes fails.
	}

	QApplication::quit();
}

/**
 * Returns true if the node library is docked, or false if it is floating.
 */
bool VuoEditor::isNodeLibraryCurrentlyDocked()
{
	return nodeLibraryCurrentlyDocked;
}

/**
 * Connects all signals from the provided node library necessary for changes in the
 * node library's visibility to impact the application-global node library visibility state.
 * See also VuoEditorWindow::disableGlobalStateConformanceToLibrary(VuoNodeLibrary *library).
 */
void VuoEditor::enableGlobalStateConformanceToLibrary(VuoNodeLibrary *library)
{
	connect(library, &VuoNodeLibrary::nodeLibraryHiddenOrUnhidden, this, &VuoEditor::updateGlobalNodeLibraryVisibilityState);
	connect(library, &VuoNodeLibrary::topLevelChanged, this, &VuoEditor::updateGlobalNodeLibraryDockedState);
	connect(library, &VuoNodeLibrary::nodeLibraryMoved, this, &VuoEditor::updateGlobalNodeLibraryFloatingPosition);
	connect(library, &VuoNodeLibrary::nodeLibraryWidthChanged, this, &VuoEditor::updateGlobalNodeLibraryWidth);
	connect(library, &VuoNodeLibrary::nodeLibraryHeightChanged, this, &VuoEditor::updateGlobalNodeLibraryHeight);
	connect(library, &VuoNodeLibrary::nodeDocumentationPanelHeightChanged, this, &VuoEditor::updateGlobalNodeDocumentationPanelHeight);
}

/**
 * Disconnects all signals from the provided node library necessary for changes in the
 * node library's visibility to impact the application-global node library visibility state.
 * See also VuoEditorWindow::enableGlobalStateConformanceToLibrary(VuoNodeLibrary *library).
 */
void VuoEditor::disableGlobalStateConformanceToLibrary(VuoNodeLibrary *library)
{
	disconnect(library, &VuoNodeLibrary::nodeLibraryHiddenOrUnhidden, this, &VuoEditor::updateGlobalNodeLibraryVisibilityState);
	disconnect(library, &VuoNodeLibrary::topLevelChanged, this, &VuoEditor::updateGlobalNodeLibraryDockedState);
	disconnect(library, &VuoNodeLibrary::nodeLibraryMoved, this, &VuoEditor::updateGlobalNodeLibraryFloatingPosition);
	disconnect(library, &VuoNodeLibrary::nodeLibraryWidthChanged, this, &VuoEditor::updateGlobalNodeLibraryWidth);
	disconnect(library, &VuoNodeLibrary::nodeLibraryHeightChanged, this, &VuoEditor::updateGlobalNodeLibraryHeight);
	disconnect(library, &VuoNodeLibrary::nodeDocumentationPanelHeightChanged, this, &VuoEditor::updateGlobalNodeDocumentationPanelHeight);
}

/**
 * Initializes a parentless Node Class Library browser.
 */
void VuoEditor::initializeTopLevelNodeLibrary(VuoCompiler *nodeLibraryCompiler,
											  VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode,
											  bool setFloatingPosition,
											  QPoint floatingPosition,
											  int nodeLibraryWidth,
											  int nodeLibraryHeight)
{
	ownedNodeLibrary = new VuoNodeLibrary(nodeLibraryCompiler, NULL, nodeLibraryDisplayMode);
	ownedNodeLibrary->setObjectName("Top-level node library");

	ownedNodeLibrary->setFloating(true);

	if (setFloatingPosition)
		ownedNodeLibrary->move(floatingPosition);

	if (nodeLibraryHeight >= 0)
		ownedNodeLibrary->resize(ownedNodeLibrary->rect().width(), nodeLibraryHeight);
}

/**
 * Displays an "About" box.
 */
void VuoEditor::about()
{
	if (!aboutBox)
		aboutBox = new VuoEditorAboutBox();

	aboutBox->showNormal();
	aboutBox->raise();
	aboutBox->activateWindow();
}

/**
 * Populates the provided menu @c m with items related to canvas transparency.
 */
void VuoEditor::populateCanvasTransparencyMenu(QMenu *m)
{
	m->addAction(canvasTransparencyNoneAction);
	m->addAction(canvasTransparencySlightAction);
	m->addAction(canvasTransparencyHighAction);
}

/**
 * Populates the provided menu with window-related items, including the list of open documents.
 */
void VuoEditor::populateWindowMenu(QMenu *m, QMainWindow *currentWindow)
{
	if (currentWindow)
	{
		m->addAction(tr("Minimize"), currentWindow, &QMainWindow::showMinimized, QKeySequence("Ctrl+M"));
		m->addAction(tr("Zoom"), currentWindow, &QMainWindow::showMaximized);

		m->addSeparator();
	}

#if VUO_PRO
	populateWindowMenu_Pro(m);
#endif

	QList<QMainWindow *> openWindows = VuoEditorUtilities::getOpenEditingWindows();
	if (! openWindows.empty())
	{
		m->addSeparator();

		for (QMainWindow *openWindow : openWindows)
		{
			QAction *raiseDocumentAction = VuoEditorUtilities::getRaiseDocumentActionForWindow(openWindow);
			m->addAction(raiseDocumentAction);

			if (currentWindow)
				raiseDocumentAction->setChecked(openWindow == currentWindow);
			else if (openWindow->isMinimized())
				raiseDocumentAction->setChecked(false);
		}
	}
}

/**
 * Populates the provided menu @c m with support-related items.
 */
void VuoEditor::populateHelpMenu(QMenu *m)
{
	m->clear();

	// Vuo Manual
	QAction *vuoManualAction = new QAction(m);
	vuoManualAction->setText(tr("Vuo Manual"));
	vuoManualAction->setData(QUrl(getVuoManualURL()));
	connect(vuoManualAction, &QAction::triggered, this, &VuoEditor::openHelpBook);
	m->addAction(vuoManualAction);

	// Vuo Manual (PDF)
	QAction *vuoManualPDFAction = new QAction(m);
	vuoManualPDFAction->setText(tr("Vuo Manual (PDF)"));
	vuoManualPDFAction->setData(QUrl(getVuoManualURL()));
	connect(vuoManualPDFAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(vuoManualPDFAction);

	// Video Tutorials
	QAction *videoTutorialsAction = new QAction(m);
	videoTutorialsAction->setText(tr("Video Tutorials"));
	videoTutorialsAction->setData(QUrl(vuoTutorialURL));
	connect(videoTutorialsAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(videoTutorialsAction);

	m->addSeparator();

	// Search vuo.org
	QAction *searchVuoOrgAction = new QAction(m);
	searchVuoOrgAction->setText(tr("Search vuo.org"));
	searchVuoOrgAction->setData(QUrl("https://vuo.org/search"));
	connect(searchVuoOrgAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(searchVuoOrgAction);

	m->addSeparator();

	// View Community Activity
	QAction *communityActivityAction = new QAction(m);
	communityActivityAction->setText(tr("View Community Activity"));
	communityActivityAction->setData(QUrl("https://vuo.org/community"));
	connect(communityActivityAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(communityActivityAction);

	// Share a Composition
	QAction *shareCompositionAction = new QAction(m);
	shareCompositionAction->setText(tr("Share a Composition"));
	shareCompositionAction->setData(QUrl("https://vuo.org/composition"));
	connect(shareCompositionAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(shareCompositionAction);

	// Start a Discussion
	QAction *startDiscussionAction = new QAction(m);
	startDiscussionAction->setText(tr("Start a Discussion"));
	startDiscussionAction->setData(QUrl("https://vuo.org/community/discussion"));
	connect(startDiscussionAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(startDiscussionAction);

	// Report a Bug
	QAction *reportBugAction = new QAction(m);
	reportBugAction->setText(tr("Report a Bug"));
	reportBugAction->setData(QUrl("https://vuo.org/bug"));
	connect(reportBugAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(reportBugAction);

	// Request a Feature
	QAction *requestFeatureAction = new QAction(m);
	requestFeatureAction->setText(tr("Request a Feature"));
	requestFeatureAction->setData(QUrl("https://vuo.org/feature-request"));
	connect(requestFeatureAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(requestFeatureAction);

	m->addSeparator();

	// Help Us Improve Vuo
	QAction *improveVuoAction = new QAction(m);
	improveVuoAction->setText(tr("Help Us Improve Vuo"));
	improveVuoAction->setData(QUrl("https://vuo.org/community-edition"));
	connect(improveVuoAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(improveVuoAction);

	// Contact Team Vuo
	QAction *contactTeamVuoAction = new QAction(m);
	contactTeamVuoAction->setText(tr("Contact Team Vuo"));
	contactTeamVuoAction->setData(QUrl("https://vuo.org/contact"));
	connect(contactTeamVuoAction, &QAction::triggered, this, &VuoEditor::openExternalUrlFromSenderData);
	m->addAction(contactTeamVuoAction);

#if VUO_PRO
	VuoEditor::populateHelpMenu_Pro(m);
#endif
}

/**
 * Launches `HelpViewer.app` and opens the Vuo Manual to the introduction / table of contents.
 */
void VuoEditor::openHelpBook()
{
	OSStatus ret = AHGotoPage(CFSTR("org.vuo.Editor.help"), NULL, NULL);
	if (ret)
	{
		char *description = VuoOsStatus_getText(ret);
		VUserLog("Error: Couldn't open Vuo Manual in HelpViewer.app: %s", description);
		free(description);
	}
}

/**
 * Launches `HelpViewer.app` and opens the Vuo Manual to the relative path in @a url.
 *
 * @param url Example: `vuo-help:how-events-travel-through-a-subcomposition.html`. No `//` after the scheme.
 */
void VuoEditor::openHelpBookPageFromUrl(const QUrl &url)
{
	CFStringRef relativePath = CFStringCreateWithCString(NULL, url.url(QUrl::RemoveScheme).toUtf8().constData(), kCFStringEncodingUTF8);
	OSStatus ret = AHGotoPage(CFSTR("org.vuo.Editor.help"), relativePath, NULL);
	if (ret)
	{
		char *description = VuoOsStatus_getText(ret);
		VUserLog("Error: Couldn't open Vuo Manual in HelpViewer.app: %s", description);
		free(description);
	}
	CFRelease(relativePath);
}

/**
 * Sets up and displays a composition window.
 *
 * @param filename A file name or path for the composition.
 * @param existingComposition If true, opens the composition in @a filename. Otherwise, creates a new, empty composition.
 *
 * @return The created editor window.
 */
VuoEditorWindow * VuoEditor::createEditorWindow(QString filename, bool existingComposition, VuoProtocol *activeProtocol)
{
	string compositionAsString = (existingComposition ? VuoFileUtilities::readFileToString(filename.toStdString()) : "");
	VuoEditorWindow *w = createEditorWindow(filename, filename, compositionAsString, activeProtocol);
	return w;
}

/**
 * Sets up and displays a composition window.
 *
 * @param documentIdentifier A file name or "Untitled Composition" string.
 * @param filename A file name for the composition. Doesn't need to refer to an existing file.
 * @param compositionAsString The serialized composition.
 *
 * @return The created editor window.
 */
VuoEditorWindow * VuoEditor::createEditorWindow(QString documentIdentifier, QString filename, const string &compositionAsString,
												VuoProtocol *activeProtocol, string nodeClassToHighlight)
{
	VuoEditorWindow *w;
	try
	{
		// @todo Take into account the current node library visibility state in deciding
		// whether to dock a node library to the newly opened window.
		// See: https://b33p.net/kosada/node/3464, https://b33p.net/kosada/node/3002,
		// https://b33p.net/kosada/node/3854, https://b33p.net/kosada/node/3087.
		w = new VuoEditorWindow(documentIdentifier, filename,
								compositionAsString,
								nodeLibraryDisplayMode,
								getGlobalNodeLibraryStateForAttributes(nodeLibraryCurrentlyVisible, nodeLibraryCurrentlyDocked),
								currentFloatingNodeLibrary,
								activeProtocol,
								nodeClassToHighlight);
	}
	catch (const VuoCompilerException &e)
	{
		VuoErrorDialog::show(NULL, tr("%1 can't be opened").arg(QFileInfo(filename).fileName()), e.what());
		return NULL;
	}

	documentIdentifierAssigned[filename.isEmpty() ? documentIdentifier : filename] = w;

	const int defaultWindowXOffset = 40;
	const int defaultWindowYOffset = 40;

	VuoEditorWindow *activeWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (!activeWindow)
	{
		// If this will be the only window…

		if (ownedNodeLibrary && nodeLibraryCurrentlyVisible && !nodeLibraryCurrentlyDocked)
		{
			// If the node library is visible and floating, make sure the new window doesn't cover it up.
			int initialWindowXOffset = defaultWindowXOffset;
			int initialWindowYOffset = defaultWindowYOffset;

			int nodeLibraryLeftPos = ownedNodeLibrary->mapToGlobal(ownedNodeLibrary->rect().topLeft()).x();
			int nodeLibraryRightPos = ownedNodeLibrary->mapToGlobal(ownedNodeLibrary->rect().topRight()).x();
			int nodeLibraryCenterPos = 0.5*(nodeLibraryLeftPos + nodeLibraryRightPos);

			int availableSpaceLeftBoundary = QApplication::desktop()->availableGeometry(w).left();
			int availableSpaceRightBoundary = QApplication::desktop()->availableGeometry(w).right();

			bool nodeLibraryCloserToLeft = (nodeLibraryCenterPos - availableSpaceLeftBoundary) <
										   (availableSpaceRightBoundary - nodeLibraryCenterPos);

			const int horizontalBuffer = 10;
			bool spaceForWindowToRightOfNodeLibrary = (availableSpaceRightBoundary - nodeLibraryRightPos) >=
													  (w->geometry().width() + horizontalBuffer);

			// Leave enough space to the left of the initial window to display the floating library, if applicable
			// and it makes sense to do so.
			if ((nodeLibraryCloserToLeft && spaceForWindowToRightOfNodeLibrary))
				initialWindowXOffset = nodeLibraryRightPos + horizontalBuffer;

			// If the floating node library's top edge is higher than the default top edge position
			// for composition windows, vertically align the first composition window with the top edge
			// of the node library instead of using the default.
			// ownedNodeLibrary->titleBarWidget() doesn't reliably return non-NULL; hard-code title bar height instead of
			// actually retrieving it.
			const int titleBarHeight = 16;
			int nodeLibraryTopPos = ownedNodeLibrary->mapToGlobal(ownedNodeLibrary->rect().topLeft()).y() - titleBarHeight;

			if (nodeLibraryTopPos < defaultWindowYOffset)
				initialWindowYOffset = nodeLibraryTopPos;

			w->move(initialWindowXOffset, initialWindowYOffset);
		}
		else
		{
			// If the node library is hidden or docked, just let the window system position the new window.
		}

		yScreenSpaceShortage = qMax(0, (w->geometry().bottom()) - QApplication::desktop()->availableGeometry(w).bottom() + 5);
	}
	else
	{
		// If there's already a window open, offset the new window relative to the active window,
		// to make it obvious that there are multiple windows.
		w->move(activeWindow->pos() + QPoint(defaultWindowXOffset, defaultWindowYOffset));
	}

	// Workaround for bug that meant that maximizing or unmaximizing a window required a subsequent click to
	// the window before further interaction with it was possible, if the window height was limited by
	// available screen real estate. https://b33p.net/kosada/node/6914
	w->resize(w->width(), w->height()-yScreenSpaceShortage);

	// Keep the floating node library's node class list synchronized with the currently active composition window.
	// Make sure to set this up before the window's show() call.
	connect(w, &VuoEditorWindow::windowActivated, this, &VuoEditor::updateFloatingNodeLibraryModules);
	connect(w, &VuoEditorWindow::windowDeactivated, this, &VuoEditor::updateFloatingNodeLibraryModules);

	w->show();

#if VUO_PRO
	createEditorWindow_Pro(w);
#endif

	// Update the application UI (Open Recent menu, Window menu, macOS dock context menu)
	// whenever an editor window is created, destroyed, or modifies its "Document" list.
	registerOpenDocument(w);

	// Update the global node library display mode whenever it is changed for any node library instance.
	connect(w->getOwnedNodeLibrary(), &VuoNodeLibrary::changedIsHumanReadable, static_cast<VuoEditor *>(qApp), &VuoEditor::updateNodeLibraryDisplayMode);

	// Keep node library visibility state synchronized among editor windows.
	enableGlobalStateConformanceToLibrary(w->getOwnedNodeLibrary());

	try
	{
		VuoCompilerIssues *issues = new VuoCompilerIssues();
		w->getComposition()->getBase()->getCompiler()->checkForMissingNodeClasses(issues);
		delete issues;
	}
	catch (const VuoCompilerException &e)
	{
		QString summary = tr("This composition contains nodes that aren't installed.");

		QString details = tr("<p>If you save the composition while it contains these nodes, some information will be lost.</p>"
						  "<p>Try searching for these nodes in the <a href=\"%1\">Node Gallery</a>. "
						  "Or if you don't need them, just delete them from the composition.</p>")
						  .arg("https://vuo.org/nodes")
			+ QString::fromStdString(e.getIssues()->getHint(true));

		QString disclosureDetails = QString::fromStdString(e.getIssues()->getShortDescription(false));

		VuoErrorDialog::show(w, summary, details, disclosureDetails);
	}

	return w;
}

/**
 * Updates the application UI immediately (on creation of @a window) and when @a window is destroyed.
 */
void VuoEditor::registerOpenDocument(QMainWindow *window)
{
	// Not sure if this is necessary, since menus that display currently-open documents already get updated on QMenu::aboutToShow.
	updateUI();

	// Ensure that the global menu is restored after closing the last window.
	connect(window, &QMainWindow::destroyed, this, &VuoEditor::updateUI);

	// Keep the list of recent documents synchronized among the "File > Open Recent" menu of each editor window.
	connect(VuoEditorUtilities::getRecentFileMenuForWindow(window), &VuoRecentFileMenu::recentFileListCleared, this, &VuoEditor::clearAllOpenRecentFileMenus);
	connect(VuoEditorUtilities::getFileMenuForWindow(window), &QMenu::aboutToShow, this, &VuoEditor::pruneAllOpenRecentFileMenus);
	synchronizeOpenRecentFileMenus();
}

/**
 * Creates an empty composition editor window.
 */
void VuoEditor::newComposition(void)
{
	if (!uiInitialized)
	{
		dispatch_async(compilerQueue, ^{
			dispatch_sync(dispatch_get_main_queue(), ^{

			// Don't bother opening the standard new untitled composition during startup
			// if another composition has opened in the meantime
			// (by dropping a composition file on the Vuo.app icon,
			// or double-clicking a composition file in Finder after double-clicking Vuo.app,
			// or by passing a filename as a command-line argument).
			if (VuoEditorUtilities::getOpenEditingWindows().size())
				return;
#if VUO_PRO
			if (!closeWelcomeWindow())
				return;
#endif
			QString identifier = assignUntitledDocumentIdentifier();
			VUserLog("%s:      New empty composition", identifier.toUtf8().data());
			createEditorWindow(identifier, "", "", NULL);
			});
		});
		return;
	}

#if VUO_PRO
	if (!closeWelcomeWindow())
		return;
#endif

	QString identifier = assignUntitledDocumentIdentifier();
	VUserLog("%s:      New empty composition", identifier.toUtf8().data());
	createEditorWindow(identifier, "", "", NULL);
}

/**
 * Creates a composition editor window with the provided content.
 * Returns a pointer to the new window.
 */
VuoEditorWindow * VuoEditor::newCompositionWithContent(string content, string compositionDir)
{
	QString identifier = assignUntitledDocumentIdentifier();
	VUserLog("%s:      New Composition with content", identifier.toUtf8().data());
	return createEditorWindow(identifier, QString::fromStdString(compositionDir), content, NULL);
}

/**
 * Creates an empty composition editor window with the active
 * protocol template indicated by the sender.
 */
void VuoEditor::newCompositionWithProtocol(void)
{
	QAction *sender = (QAction *)QObject::sender();
	VuoProtocol *selectedProtocol = static_cast<VuoProtocol *>(sender->data().value<void *>());

	closeUnmodifiedUntitledComposition();
#if VUO_PRO
	if (!closeWelcomeWindow())
		return;
#endif

	QString identifier = assignUntitledDocumentIdentifier();
	VUserLog("%s:      New Composition with %s protocol", identifier.toUtf8().data(), selectedProtocol->getName().c_str());
	VuoEditorWindow *w = createEditorWindow(identifier, false, selectedProtocol);

	if (selectedProtocol && w && w->getCurrentNodeLibrary())
	{
		string nodeLibraryFilterText = getFilterTextForTemplate(selectedProtocol->getId());
		if (!nodeLibraryFilterText.empty())
		{
			w->getCurrentNodeLibrary()->searchForText(nodeLibraryFilterText.c_str());
			w->getCurrentNodeLibrary()->focusTextFilter();
		}
	}
}

/**
 * Creates an empty composition editor window with the active
 * window template indicated by the sender.
 */
void VuoEditor::newCompositionWithTemplate(void)
{
	QAction *sender = (QAction *)QObject::sender();
	QString selectedTemplate = static_cast<QString>(sender->data().value<QString>());
	string templatePath = VuoFileUtilities::getVuoFrameworkPath() + "/Resources/" + selectedTemplate.toUtf8().constData() + ".vuo";

	closeUnmodifiedUntitledComposition();
#if VUO_PRO
	if (!closeWelcomeWindow())
		return;
#endif

	string compositionAsString = VuoFileUtilities::readFileToString(templatePath);
	QString identifier = assignUntitledDocumentIdentifier();
	VUserLog("%s:      New Composition with %s template", identifier.toUtf8().data(), selectedTemplate.toUtf8().data());
	VuoEditorWindow *w = createEditorWindow(identifier, "", compositionAsString, NULL);

	if (w && w->getCurrentNodeLibrary())
	{
		string nodeLibraryFilterText = getFilterTextForTemplate(selectedTemplate.toUtf8().constData());
		if (!nodeLibraryFilterText.empty())
		{
			w->getCurrentNodeLibrary()->searchForText(nodeLibraryFilterText.c_str());
			w->getCurrentNodeLibrary()->focusTextFilter();
		}
	}
}

/**
 * Returns suggested node library filter text to display upon creating a composition with
 * the provided protocol or window template.
 *
 * For protocol templates, the input string should be the protocol ID (VuoProtocol->getId()).
 * For window templates, the input string should be the base filename of the template (e.g., "imageTemplate").
 */
string VuoEditor::getFilterTextForTemplate(string templateID)
{
	map<string, string> filterTextForTemplate;

	// Protocols
	filterTextForTemplate[VuoProtocol::imageFilter] = "image";
	filterTextForTemplate[VuoProtocol::imageGenerator] = "image";
	filterTextForTemplate[VuoProtocol::imageTransition] = "image";

	// Window templates
	filterTextForTemplate["imageTemplate"] = "vuo.image";
	filterTextForTemplate["layersTemplate"] = "vuo.layer shape";
	filterTextForTemplate["sceneTemplate"] = "vuo.scene shape";

	// Export templates
	filterTextForTemplate["movie"] = filterTextForTemplate[VuoProtocol::imageGenerator];
	filterTextForTemplate["screensaver"] = filterTextForTemplate[VuoProtocol::imageGenerator];

	filterTextForTemplate["FFGLSource"] = filterTextForTemplate[VuoProtocol::imageGenerator];
	filterTextForTemplate["FFGLEffect"] = filterTextForTemplate[VuoProtocol::imageFilter];
	filterTextForTemplate["FFGLBlendMode"] = filterTextForTemplate[VuoProtocol::imageTransition];

	filterTextForTemplate["FxPlugGenerator"] = filterTextForTemplate[VuoProtocol::imageGenerator];
	filterTextForTemplate["FxPlugEffect"] = filterTextForTemplate[VuoProtocol::imageFilter];
	filterTextForTemplate["FxPlugTransition"] = filterTextForTemplate[VuoProtocol::imageTransition];

	map<string, string>::iterator i = filterTextForTemplate.find(templateID);
	if (i != filterTextForTemplate.end())
		return i->second;
	else
		return "";
}

/**
 * Assigns a unique "Untitled Document" identifier.
 */
QString VuoEditor::assignUntitledDocumentIdentifier(void)
{
	QString uniqueDocumentIdentifier = VuoEditorWindow::untitledComposition;
	int documentIdentifierInstanceNum = 1;

	while(documentIdentifierAssigned[uniqueDocumentIdentifier])
	{
		std::ostringstream oss;
		oss << " " << ++documentIdentifierInstanceNum;
		uniqueDocumentIdentifier = VuoEditorWindow::untitledComposition + QString::fromStdString(oss.str());
	}

	return uniqueDocumentIdentifier;
}

/**
 * Displays an "Open File" dialog, and if a composition or shader is selected, opens it in an editor window.
 */
void VuoEditor::openFile(void)
{
	QFileDialog d(NULL, "", "", "Vuo Composition (*.vuo);;Fragment Shader (*.fs)");
	d.setFileMode(QFileDialog::ExistingFiles);

	// At Qt 5.2.1, the file dialogue's history() always seems to be empty.
	// See https://b33p.net/kosada/node/7302 .
	//if (d.history().isEmpty())
	//	d.setDirectory(getDefaultCompositionStorageDirectory());

	QStringList fileNames;
	if (d.exec() == QDialog::Accepted)
		fileNames = d.selectedFiles();

	foreach (QString fileName, fileNames)
		openFileWithName(fileName);
}

/**
 * Opens a composition editor window with the specified @c filename.
 */
QMainWindow * VuoEditor::openFileWithName(QString filename, bool addToRecentFileMenu)
{
	closeUnmodifiedUntitledComposition();
#if VUO_PRO
	if (!closeWelcomeWindow())
	{
		QString fileURL = QString("file://").append(filename);
		queuedCompositionsToOpen.push_back(QString("file://").append(filename));
		return NULL;
	}
#endif

	QMainWindow *existing = VuoEditorUtilities::existingWindowWithFile(filename);
	if (existing)
	{
		VuoEditorUtilities::setWindowAsActiveWindow(existing);
		return existing;
	}

	if (! VuoFileUtilities::fileIsReadable(filename.toUtf8().constData()))
	{
		VuoErrorDialog::show(NULL, tr("You do not have permission to open the document \"%1\".").arg(QFileInfo(filename).fileName()), "");
		return NULL;
	}

	string dir, file, ext;
	VuoFileUtilities::splitPath(filename.toStdString(), dir, file, ext);
	bool isComposition = VuoFileUtilities::isCompositionExtension(ext);

	QMainWindow *window = nullptr;
	if (isComposition)
	{
		VUserLog("%s.%s:      Open", file.c_str(), ext.c_str());
		window = createEditorWindow(filename, true);
		if (!window)
			return nullptr;

		dynamic_cast<VuoEditorWindow *>(window)->setIncludeInRecentFileMenu(addToRecentFileMenu);
	}
	else
	{
		try
		{
			window = new VuoCodeWindow(filename.toStdString());
			if (!window)
				return nullptr;

			dynamic_cast<VuoCodeWindow *>(window)->setIncludeInRecentFileMenu(addToRecentFileMenu);
		}
		catch (VuoException &e)
		{
			VuoErrorDialog::show(NULL, tr("Couldn't open the shader."), e.what());
			return nullptr;
		}

		window->show();
	}

	if (addToRecentFileMenu)
		addFileToAllOpenRecentFileMenus(filename);

	return window;
}

/**
 * Opens a composition editor window for an example composition.
 *
 * @param filename The name of the example composition.
 * @param nodeSet The node set that containst the example composition.
 * @param nodeClassToHighlight The node class name to initially select.
 */
void VuoEditor::openExampleComposition(QString filename, VuoNodeSet *nodeSet, string nodeClassToHighlight)
{
	VUserLog("%s:      Open", filename.toUtf8().data());

	string filenameStr = filename.toStdString();
	string workingDir = VuoFileUtilities::getTmpDir();
	string compositionContents = nodeSet->getExampleCompositionContents(filenameStr);
	QString compositionPath = QString::fromStdString(workingDir + "/" + filenameStr);

	nodeSet->extractExampleCompositionResources(workingDir);
	closeUnmodifiedUntitledComposition();
#if VUO_PRO
	if (!closeWelcomeWindow())
		return;
#endif
	createEditorWindow(compositionPath, compositionPath, compositionContents, NULL, nodeClassToHighlight);

	QString exampleCompositionUrl = QString(VuoEditor::vuoExampleCompositionScheme)
			.append("://")
			.append(nodeSet->getName().c_str())
			.append("/")
			.append(filename);

	addFileToAllOpenRecentFileMenus(exampleCompositionUrl);
}

/**
 * Closes the unmodified untitled composition, if currently open.
 * Has no effect on compositions created from protocol or window composition templates,
 * even if untitled and unmodified by the user.
 */
void VuoEditor::closeUnmodifiedUntitledComposition()
{
	VuoEditorWindow *untitled = VuoEditorWindow::existingWindowWithNewFile();
	if (untitled)
		untitled->close();
}

/**
 * Shows the Node Class Library browser and focuses its text filter.
 */
void VuoEditor::showNodeLibrary(void)
{
	// If there is an editor window open and the user has somehow accessed the
	// application-global menu anyway, trigger the equivalent menu item of the
	// topmost editor window instead.
	VuoEditorWindow *topmostWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (topmostWindow)
		topmostWindow->on_showNodeLibrary_triggered();

	// Unhiding the top-level node class library triggers node-library-floating mode globally.
	else
	{
		if ((ownedNodeLibrary->isHidden()))
		{
			VUserLog("Show node library");

			designateNewFloatingNodeLibrary(ownedNodeLibrary);
			updateGlobalNodeLibraryState(true, false);
		}

		ownedNodeLibrary->focusTextFilter();
	}
}

/**
 * Update the display mode of the top-level node class library to conform with global settings.
 */
void VuoEditor::conformToGlobalNodeLibraryVisibility(VuoNodeLibrary::nodeLibraryState visibility,
													 VuoNodeLibrary *floater)
{
	if ((visibility == VuoNodeLibrary::nodeLibraryHidden) ||
		(visibility == VuoNodeLibrary::nodeLibraryDocked))
	{
		ownedNodeLibrary->releaseDocumentationWidget();
		ownedNodeLibrary->setVisible(false);
	}

	else if (visibility == VuoNodeLibrary::nodeLibraryFloating)
	{
		// If our own node library was the one that initiated global
		// floating-node-library mode by being undocked, let it float.
		// It is now the single application-wide floating library.
		if (ownedNodeLibrary == floater)
		{
			if (! ownedNodeLibrary->isFloating())
				ownedNodeLibrary->setFloating(true);

			//ownedNodeLibrary->setVisible(true);
			ownedNodeLibrary->prepareAndMakeVisible();
			ownedNodeLibrary->setFocus();
			ownedNodeLibrary->displayPopoverForCurrentNodeClass();
		}
	}

	updateUI();
}

/**
 * Updates the UI elements (e.g., enables/disables buttons) based on the application's state.
 */
void VuoEditor::updateUI()
{
	if (!uiInitialized)
		return;

	QList<QMainWindow *> openWindows = VuoEditorUtilities::getOpenEditingWindowsStacked();
	QMainWindow *frontWindow = (openWindows.empty() ? nullptr : openWindows.front());

	// Update the list of open documents in the "Window" menu, including the checkmarks for the actions.
	menuWindow->clear();
	populateWindowMenu(menuWindow, frontWindow);

#ifdef __APPLE__
	// Update the list of open documents in the OS X dock context menu, using the same actions.
	dockContextMenu->clear();
	for (QMainWindow *openWindow : VuoEditorUtilities::getOpenEditingWindows())
		dockContextMenu->addAction(VuoEditorUtilities::getRaiseDocumentActionForWindow(openWindow));

	// Ensure that the global menu is restored after closing the last window.
	if (openWindows.empty())
	{
		id *nsAppGlobal = (id *)dlsym(RTLD_DEFAULT, "NSApp");
		id currentMainMenu = objc_msgSend(*nsAppGlobal, sel_getUid("mainMenu"), nil);
		if (menuBar->toNSMenu() != currentMainMenu)
		{
			// No windows are open, and the currently-visible main menu isn't the global menu, so we need to explicitly restore the global menu.
			// QMenuBar::changeEvent calls QMenuBarPrivate::handleReparent, which calls QCocoaMenuBar::handleReparent, which calls QCocoaMenuBar::updateMenuBarImmediately, which calls NSApp::setMainMenu.
			QEvent e(QEvent::ParentChange);
			sendEvent(menuBar, &e);
		}
	}
#endif

	// Update the "enabled" status of the canvas transparency menu items.
	foreach (QAction *action, canvasTransparencyOptions->actions())
		action->setEnabled(!action->isChecked());
}

/**
 * Reimplements QApplication::event(), to handle application-wide events.
 */
bool VuoEditor::event(QEvent * e)
{
	switch (e->type())
	{
		case QEvent::FileOpen:
			openUrl(static_cast<QFileOpenEvent *>(e)->url().toString());
			return true;

		case QEvent::ApplicationActivate:
			emit activeApplicationStateChanged(true);
			return true;

		case QEvent::ApplicationDeactivate:
			emit activeApplicationStateChanged(false);
			return true;

		// macOS Dock context menu > Quit
		case QEvent::Close:
			e->ignore();
			quitCleanly();
			return true;

		default:
			return QApplication::event(e);
	}
}

/**
 * Uses an external browser to open the URL associated with the sender
 * action that triggered the call to this slot.
 */
void VuoEditor::openExternalUrlFromSenderData()
{
	QAction *sender = (QAction *)QObject::sender();
	QDesktopServices::openUrl(sender->data().toUrl());
}

/**
 * Opens the local file at the path associated with the sender
 * action that triggered the call to this slot.
 */
void VuoEditor::openFileFromSenderData()
{
	QAction *sender = (QAction *)QObject::sender();
	QString filePath = sender->data().value<QString>();

	if (! filePath.startsWith("file://"))
		filePath = QString("file://").append(filePath);

	openUrl(filePath);
}

/**
 * Removes (by moving to the Trash) the file at the path associated with the sender
 * action that triggered the call to this slot.
 */
void VuoEditor::removeFileFromSenderData()
{
	QAction *sender = (QAction *)QObject::sender();
	QString filePath = sender->data().value<QString>();
	moveFileToTrash(filePath);
}

/**
 * Removes the file at the provided path by moving it to the Trash.
 */
void VuoEditor::moveFileToTrash(QString filePath)
{
	try
	{
		VuoFileUtilities::moveFileToTrash(filePath.toUtf8().constData());
	}
	catch (const VuoException &e)
	{
		if (VuoFileUtilities::fileExists(filePath.toUtf8().constData()))
			VuoErrorDialog::show(NULL, VuoEditor::tr("Couldn't move file to the Trash"), e.what());
	}
}

/**
 * Determines what type of @c url has been provided:
 * - A Vuo example composition (prefixed with VuoEditor::vuoExampleCompositionScheme), or
 * - A URL with the "file://" scheme, assumed to contain the path of a local composition, node class, or shader.
 * Opens the @c url using the appropriate handler.
 */
void VuoEditor::openUrl(const QString &url)
{
	QUrl fileUrl(url);

	if (fileUrl.isValid() && fileUrl.scheme() == VuoEditor::vuoExampleCompositionScheme)
	{
		if (!uiInitialized)
			queuedCompositionsToOpen.push_back(url);
		else
			openExampleCompositionFromUrl(fileUrl);
	}
	else
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(fileUrl.toLocalFile().toUtf8().constData(), dir, file, ext);

		if (QString(ext.c_str()) == VuoEditor::vuoNodeClassFileExtension)
		{
			string installedNodeDir = VuoFileUtilities::getUserModulesPath();
			string sourceNodePath = dir + "/" + file + "." + ext;
			string targetNodePath = installedNodeDir + "/" + file + "." + ext;

			VuoFileUtilities::makeDir(installedNodeDir);

			if (VuoFileUtilities::fileExists(targetNodePath))
			{
				QMessageBox messageBox;
				string errorSummary = "A node named \"" + file + "\" already exists. Do you want to replace it with the one you're installing?";

				// On OS X, this combination of flags displays the minimize, maximimize, and close buttons, but all in a disabled state.
				messageBox.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint);

				messageBox.setTextFormat(Qt::RichText);
				messageBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
				messageBox.setButtonText(QMessageBox::Discard, tr("Replace"));
				messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
				messageBox.setDefaultButton(QMessageBox::Discard);
				messageBox.setText(errorSummary.c_str());
				messageBox.setStyleSheet("#qt_msgbox_informativelabel, QMessageBoxDetailsText { font-weight: normal; font-size: 11pt; }");

				// Give the "Cancel" button keyboard focus (without "Default" status) so that it can be activated by spacebar.
				static_cast<QPushButton *>(messageBox.button(QMessageBox::Cancel))->setAutoDefault(false);
				messageBox.button(QMessageBox::Cancel)->setFocus();

				messageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());

				if (messageBox.exec() == QMessageBox::Discard)
				{
					// Move pre-existing module to the trash.
					try
					{
						VuoFileUtilities::moveFileToTrash(targetNodePath);
					}
					catch (VuoException &e)
					{
						if (VuoFileUtilities::fileExists(targetNodePath))
						{
							VuoErrorDialog::show(NULL, tr("There was a problem installing the node."), e.what());
							return;
						}
					}

				}
				else
					return;
			}

			// Install the node class in the "User Modules" directory.
			try
			{
				VUserLog("Install node %s.%s", file.c_str(), ext.c_str());

				VuoFileUtilities::copyFile(sourceNodePath, targetNodePath);
			}
			catch (VuoException &e)
			{
				VuoErrorDialog::show(NULL, tr("There was a problem installing the node."), e.what());
				return;
			}

			if (VuoFileUtilities::fileExists(targetNodePath))
			{
				QMessageBox installationSuccessMessageBox;
				installationSuccessMessageBox.setText(tr("Your node has been installed!"));
				installationSuccessMessageBox.setIconPixmap(VuoEditorUtilities::vuoLogoForDialogs());
				installationSuccessMessageBox.exec();
			}
		}

		else if (VuoFileUtilities::isCompositionExtension(ext) || VuoFileUtilities::isIsfSourceExtension(ext))
		{
			if (!uiInitialized)
				queuedCompositionsToOpen.push_back(url);
			else
				openFileWithName(fileUrl.toLocalFile());
		}

		else
			QDesktopServices::openUrl(url);
	}
}

/**
 * Updates the global shader documentation setting to match @a isVisible.
 */
void VuoEditor::updateGlobalShaderDocumentationVisibility(bool isVisible)
{
	shaderDocumentationVisible = isVisible;
	settings->setValue(shaderDocumentationVisibilitySettingsKey, shaderDocumentationVisible);
}

/**
 * Returns the current global shader documentation setting.
 */
bool VuoEditor::getGlobalShaderDocumentationVisibility()
{
	return shaderDocumentationVisible;
}

/**
 * Applies the movie export settings specified in the user's stored settings file.
 */
void VuoEditor::applyStoredMovieExportSettings()
{
	movieExportWidth = (settings->contains(movieExportWidthSettingsKey)? settings->value(movieExportWidthSettingsKey).toInt() : 1024);
	movieExportHeight = (settings->contains(movieExportHeightSettingsKey)? settings->value(movieExportHeightSettingsKey).toInt() : 768);
	movieExportTime = (settings->contains(movieExportTimeSettingsKey)? settings->value(movieExportTimeSettingsKey).toDouble() : 0.);
	movieExportDuration = (settings->contains(movieExportDurationSettingsKey)? settings->value(movieExportDurationSettingsKey).toDouble() : 10.);
	movieExportFramerate = (settings->contains(movieExportFramerateSettingsKey)? settings->value(movieExportFramerateSettingsKey).toDouble() : 30.);
	movieExportSpatialSupersample = (settings->contains(movieExportSpatialSupersampleSettingsKey)? settings->value(movieExportSpatialSupersampleSettingsKey).toInt() : 1.);
	movieExportTemporalSupersample = (settings->contains(movieExportTemporalSupersampleSettingsKey)? settings->value(movieExportTemporalSupersampleSettingsKey).toInt() : 1.);
	movieExportShutterAngle = (settings->contains(movieExportShutterAngleSettingsKey)? static_cast<float>(settings->value(movieExportShutterAngleSettingsKey).toDouble()) : 360.);
	movieExportImageFormat = (settings->contains(movieExportImageFormatSettingsKey)? settings->value(movieExportImageFormatSettingsKey).toString() : "H.264");
	movieExportQuality = (settings->contains(movieExportQualitySettingsKey)? settings->value(movieExportQualitySettingsKey).toDouble() : 1.);
}

/**
 * Updates the global movie export settings in accordance with the provided parameter values.
 */
void VuoEditor::updateGlobalMovieExportSettings(int width,
												int height,
												double time,
												double duration,
												double framerate,
												int spatialSupersample,
												int temporalSupersample,
												float shutterAngle,
												QString imageFormat,
												double quality)
{
	if (width != movieExportWidth)
	{
		movieExportWidth = width;
		settings->setValue(movieExportWidthSettingsKey, movieExportWidth);
	}

	if (height != movieExportHeight)
	{
		movieExportHeight = height;
		settings->setValue(movieExportHeightSettingsKey, movieExportHeight);
	}

	if (time != movieExportTime)
	{
		movieExportTime = time;
		settings->setValue(movieExportTimeSettingsKey, movieExportTime);
	}

	if (duration != movieExportDuration)
	{
		movieExportDuration = duration;
		settings->setValue(movieExportDurationSettingsKey, movieExportDuration);
	}

	if (framerate != movieExportFramerate)
	{
		movieExportFramerate = framerate;
		settings->setValue(movieExportFramerateSettingsKey, movieExportFramerate);
	}

	if (spatialSupersample != movieExportSpatialSupersample)
	{
		movieExportSpatialSupersample = spatialSupersample;
		settings->setValue(movieExportSpatialSupersampleSettingsKey, movieExportSpatialSupersample);
	}

	if (temporalSupersample != movieExportTemporalSupersample)
	{
		movieExportTemporalSupersample = temporalSupersample;
		settings->setValue(movieExportTemporalSupersampleSettingsKey, movieExportTemporalSupersample);
	}

	if (shutterAngle != movieExportShutterAngle)
	{
		movieExportShutterAngle = shutterAngle;
		settings->setValue(movieExportShutterAngleSettingsKey, static_cast<double>(movieExportShutterAngle));
	}

	if (imageFormat != movieExportImageFormat)
	{
		movieExportImageFormat = imageFormat;
		settings->setValue(movieExportImageFormatSettingsKey, movieExportImageFormat);
	}

	if (quality != movieExportQuality)
	{
		movieExportQuality = quality;
		settings->setValue(movieExportQualitySettingsKey, movieExportQuality);
	}
}

/**
 * Returns the current global movie export settings.
 */
void VuoEditor::getGlobalMovieExportSettings(int &width,
												int &height,
												double &time,
												double &duration,
												double &framerate,
												int &spatialSupersample,
												int &temporalSupersample,
												float &shutterAngle,
												QString &imageFormat,
												double &quality)
{
	width = movieExportWidth;
	height = movieExportHeight;
	time = movieExportTime;
	duration = movieExportDuration;
	framerate = movieExportFramerate;
	spatialSupersample = movieExportSpatialSupersample;
	temporalSupersample = movieExportTemporalSupersample;
	shutterAngle = movieExportShutterAngle;
	imageFormat = movieExportImageFormat;
	quality = movieExportQuality;
}


/**
 * Returns the node library state corresponding to the input combination of
 * @c visible and @c docked states.
 */
VuoNodeLibrary::nodeLibraryState VuoEditor::getGlobalNodeLibraryStateForAttributes(bool visible, bool docked)
{
	return ((! visible)? VuoNodeLibrary::nodeLibraryHidden :
						(docked? VuoNodeLibrary::nodeLibraryDocked :
								 VuoNodeLibrary::nodeLibraryFloating));
}

/**
 * Updates the node library display mode in accordance with the input @c humanReadable boolean.
 * This change will impact the stored application settings and any new node libraries created from this
 * point on, but will not affect existing node libraries.
 */
void VuoEditor::updateNodeLibraryDisplayMode(bool humanReadable)
{
	nodeLibraryDisplayMode = (humanReadable? VuoNodeLibrary::displayByName : VuoNodeLibrary::displayByClass);
	settings->setValue(nodeLibraryDisplayModeSettingsKey, nodeLibraryDisplayMode);

	// Exception to the non-propagation policy for this setting:
	// If the top-level node library is not currently visible, do synchronize its display mode so that
	// it is up-to-date if/when it next appears.
	if ((ownedNodeLibrary->isHidden()) && (ownedNodeLibrary->getHumanReadable() != humanReadable))
	{
		ownedNodeLibrary->setHumanReadable(humanReadable);
		ownedNodeLibrary->updateUI();
		updateUI();
	}
}

/**
 * Updates the global node library floating position in accordance with the input @c newPos.
 */
void VuoEditor::updateGlobalNodeLibraryFloatingPosition(QPoint newPos)
{
	nodeLibraryFloatingPosition = newPos;
	settings->setValue(nodeLibraryFloatingPositionSettingsKey, nodeLibraryFloatingPosition);
}

/**
 * Updates the global node library width in accordance with the input @c newWidth.
 */
void VuoEditor::updateGlobalNodeLibraryWidth(int newWidth)
{
	nodeLibraryWidth = newWidth;
	settings->setValue(nodeLibraryWidthSettingsKey, nodeLibraryWidth);
}

/**
 * Updates the global node library height in accordance with the input @c newHeight.
 */
void VuoEditor::updateGlobalNodeLibraryHeight(int newHeight)
{
	nodeLibraryHeight = newHeight;
	settings->setValue(nodeLibraryHeightSettingsKey, nodeLibraryHeight);
}

/**
 * Updates the global node documentation panel height in accordance with the input @c newHeight.
 */
void VuoEditor::updateGlobalNodeDocumentationPanelHeight(int newSize)
{
	nodeDocumentationPanelHeight = newSize;
	settings->setValue(nodeDocumentationPanelHeightSettingsKey, nodeDocumentationPanelHeight);
}

/**
 * Updates the global node library state in accordance with the input @c visible state.
 */
void VuoEditor::updateGlobalNodeLibraryVisibilityState(bool visible)
{
	VUserLog("%s node library", visible ? "Show" : "Hide");

	// If transitioning to a visible state, revert to the docking state from the last time the node library was visible.
	bool updatedDockingState = ((visible && (! nodeLibraryCurrentlyVisible))? previousVisibleNodeLibraryStateWasDocked : nodeLibraryCurrentlyDocked);
	updateGlobalNodeLibraryState(visible, updatedDockingState);
}

/**
 * Updates the global node library state in accordance with the input @c floating state.
 */
void VuoEditor::updateGlobalNodeLibraryDockedState(bool floating)
{
	VUserLog("%s node library", floating ? "Detach" : "Attach");

	VuoNodeLibrary *sender = (VuoNodeLibrary *)QObject::sender();
	VuoNodeLibrary *floater = (floating? sender : NULL);
	designateNewFloatingNodeLibrary(floater);
	updateGlobalNodeLibraryState(this->nodeLibraryCurrentlyVisible, (! floating));

	if (floating)
		updateFloatingNodeLibraryModules();
	else
		updateDockedNodeLibraryModules();
}

/**
 * Updates the global node library state and saves the updated state to the stored application settings.
 * Helper function for updateGlobalNodeLibraryVisibilityState(bool visible) and
 * updateGlobalNodeLibraryDockedState(bool floating).
 */
void VuoEditor::updateGlobalNodeLibraryState(bool visible, bool docked)
{
	VuoNodeLibrary::nodeLibraryState currentNodeLibraryState = getGlobalNodeLibraryStateForAttributes(nodeLibraryCurrentlyVisible, nodeLibraryCurrentlyDocked);
	VuoNodeLibrary::nodeLibraryState updatedNodeLibraryState = getGlobalNodeLibraryStateForAttributes(visible, docked);

	if (currentNodeLibraryState != VuoNodeLibrary::nodeLibraryHidden)
		previousVisibleNodeLibraryStateWasDocked = nodeLibraryCurrentlyDocked;

	nodeLibraryCurrentlyVisible = visible;
	nodeLibraryCurrentlyDocked = docked;
	if (docked)
		currentFloatingNodeLibrary = NULL;

	settings->setValue(nodeLibraryVisibilityStateSettingsKey, nodeLibraryCurrentlyVisible);
	settings->setValue(nodeLibraryDockingStateSettingsKey, nodeLibraryCurrentlyDocked);

	emit globalNodeLibraryStateChanged(updatedNodeLibraryState, currentFloatingNodeLibrary, false);
}

/**
 * Updates the content of the current floating node library, if any, to list exactly the modules
 * available to the most recently activated composition window.
 */
void VuoEditor::updateFloatingNodeLibraryModules()
{
	if (!currentFloatingNodeLibrary)
		return;

	VuoEditorWindow *activeWindow = VuoEditorWindow::getMostRecentActiveEditorWindow();
	if (activeWindow)
	{
		VuoModuleManager *activeCompositionModuleManager = activeWindow->getComposition()->getModuleManager();
		if (activeCompositionModuleManager->getNodeLibrary() != currentFloatingNodeLibrary)
		{
			// Tell other open editor windows to stop sending updates to the floating node library, if they were doing so.
			QList<VuoEditorWindow *> openWindows = VuoEditorUtilities::getOpenCompositionEditingWindows();
			foreach (VuoEditorWindow *window, openWindows)
			{
				if (window != activeWindow)
				{
					VuoModuleManager *windowModuleManager = window->getComposition()->getModuleManager();
					if (windowModuleManager->getNodeLibrary() == currentFloatingNodeLibrary)
						windowModuleManager->setNodeLibrary(NULL);
				}
			}

			// Tell the top-level app to stop sending updates to the floating node library, if it was doing so.
			if (moduleManager->getNodeLibrary() == currentFloatingNodeLibrary)
				moduleManager->setNodeLibrary(NULL);
		}
	}

	VuoModuleManager *updatedModuleManager = (activeWindow? activeWindow->getComposition()->getModuleManager() : this->moduleManager);
	if (updatedModuleManager->getNodeLibrary() != currentFloatingNodeLibrary)
	{
		// Document the node library's current state (selected and documented items, filter text) before resetting it.
		QString origFilterText;
		set<string> origSelectedNodeClasses;
		string origDocumentedNodeClass;
		currentFloatingNodeLibrary->getState(origFilterText, origSelectedNodeClasses, origDocumentedNodeClass);

		// Populate the node library with its new content.
		currentFloatingNodeLibrary->clearNodeClassList();
		updatedModuleManager->setNodeLibrary(currentFloatingNodeLibrary);
		updatedModuleManager->updateWithAlreadyLoadedModules();

		// Now restore the node library's original state.
		currentFloatingNodeLibrary->setState(origFilterText, origSelectedNodeClasses, origDocumentedNodeClass);

		// If the documentation pane displayed previously is no longer relevant, display
		// whatever documentation is most appropriate now.
		QString newFilterText;
		set<string> newSelectedNodeClasses;
		string newDocumentedNodeClass;
		currentFloatingNodeLibrary->getState(newFilterText, newSelectedNodeClasses, newDocumentedNodeClass);
		if (newDocumentedNodeClass.empty())
		{
			if (activeWindow)
				activeWindow->displayAppropriateDocumentation();
			else
				currentFloatingNodeLibrary->displayPopoverForCurrentNodeClass();
		}
	}
}

/**
 * Highlights the requested new node class in all visible node libraries where it is listed,
 * via callback made by each node library's module manager once installation has completed.
 */
void VuoEditor::highlightNewNodeClassInAllLibraries(string nodeClassName)
{
	if (moduleManager->getNodeLibrary())
	{
		VuoModuleManager::CallbackType subcompositionCreated = ^void (void) {
			moduleManager->getNodeLibrary()->highlightNodeClass(nodeClassName);
		};
		moduleManager->doNextTimeNodeClassIsLoaded(nodeClassName, subcompositionCreated);
	}

	QList<VuoEditorWindow *> openWindows = VuoEditorUtilities::getOpenCompositionEditingWindows();
	foreach (VuoEditorWindow *window, openWindows)
	{
		VuoModuleManager *windowModuleManager = window->getComposition()->getModuleManager();
		if (windowModuleManager->getNodeLibrary())
		{
			VuoModuleManager::CallbackType subcompositionCreated = ^void (void) {
				windowModuleManager->getNodeLibrary()->highlightNodeClass(nodeClassName);
			};
			windowModuleManager->doNextTimeNodeClassIsLoaded(nodeClassName, subcompositionCreated);
		}
	}
}

/**
 * Updates the contents of each currently docked node library to list exactly the modules
 * available to the composition associated with the window in which the library is docked.
 */
void VuoEditor::updateDockedNodeLibraryModules()
{
	if (currentFloatingNodeLibrary)
		return;

	// Tell each open editor window to send updates to the node library that it owns and not to any others.
	QList<VuoEditorWindow *> openWindows = VuoEditorUtilities::getOpenCompositionEditingWindows();
	foreach (VuoEditorWindow *window, openWindows)
	{
		VuoModuleManager *windowModuleManager = window->getComposition()->getModuleManager();
		VuoNodeLibrary *windowOwnedNodeLibrary = window->getOwnedNodeLibrary();

		if (windowModuleManager->getNodeLibrary() != windowOwnedNodeLibrary)
		{
			windowOwnedNodeLibrary->clearNodeClassList();
			windowModuleManager->setNodeLibrary(windowOwnedNodeLibrary);
			windowModuleManager->updateWithAlreadyLoadedModules();
		}
	}

	// Tell the top-level app to send updates to the node library that it owns and not to any others.
	if (moduleManager->getNodeLibrary() != ownedNodeLibrary)
	{
		ownedNodeLibrary->clearNodeClassList();
		moduleManager->setNodeLibrary(ownedNodeLibrary);
		moduleManager->updateWithAlreadyLoadedModules();
	}
}

/**
 * Designates the input @c library as the current floating node class library used by
 * any/all open composition windows.
 */
void VuoEditor::designateNewFloatingNodeLibrary(VuoNodeLibrary *library)
{
	if (currentFloatingNodeLibrary && (currentFloatingNodeLibrary != ownedNodeLibrary))
		disconnect(currentFloatingNodeLibrary, &VuoNodeLibrary::aboutToBeDestroyed, this, &VuoEditor::assignTopLevelLibraryAsReplacementFloater);

	if (library && (library != ownedNodeLibrary))
		connect(library, &VuoNodeLibrary::aboutToBeDestroyed, this, &VuoEditor::assignTopLevelLibraryAsReplacementFloater);

	currentFloatingNodeLibrary = library;
}

/**
 * Designates the application-owned (top-level) node class library as the current floating library,
 * to replace a node library that has been destroyed.
 */
void VuoEditor::assignTopLevelLibraryAsReplacementFloater()
{
	currentFloatingNodeLibrary = ownedNodeLibrary;

	emit globalNodeLibraryStateChanged(getGlobalNodeLibraryStateForAttributes(nodeLibraryCurrentlyVisible, nodeLibraryCurrentlyDocked),
									   currentFloatingNodeLibrary, true);
}

/**
 * Adds the file at @c filePath to the application-global "File > Open Recent"
 * menu list, and synchronizes each editor window's "File > Open Recent" menu
 * with the global version.
 */
void VuoEditor::addFileToAllOpenRecentFileMenus(QString filePath)
{
	menuOpenRecent->addFile(filePath);
	synchronizeOpenRecentFileMenus();
}

/**
 * Adds the provided @c filePath to the list of composition files that have been closed
 * during this editor session since the list was last cleared.
 */
void VuoEditor::addFileToRecentlyClosedList(QString filePath)
{
	this->closedFiles.push_back(filePath.toUtf8().constData());
}

/**
 * Clears the list of recently opened files from the application-global "File > Open Recent"
 * menu list, and synchronizes each editor window's "File > Open Recent" menu
 * with the global version.
 */
void VuoEditor::clearAllOpenRecentFileMenus()
{
	menuOpenRecent->clearRecentFileListActionTriggered();
	synchronizeOpenRecentFileMenus();
}

/**
 * Prunes the list of recently opened files from the application-global "File > Open Recent"
 * menu list, removing any that no longer exist, and synchronizes each editor window's "File > Open Recent" menu
 * with the global version.
 */
void VuoEditor::pruneAllOpenRecentFileMenus()
{
	menuOpenRecent->pruneNonexistentFiles();
	synchronizeOpenRecentFileMenus();
}

/**
 * Synchronizes each editor window's "File > Open Recent" menu with the global version,
 * and saves the updated file list to the stored application settings.
 */
void VuoEditor::synchronizeOpenRecentFileMenus()
{
	for (QMainWindow *openWindow : VuoEditorUtilities::getOpenEditingWindows())
		VuoEditorUtilities::getRecentFileMenuForWindow(openWindow)->setRecentFiles(menuOpenRecent->getRecentFiles());

	settings->setValue(recentFileListSettingsKey, menuOpenRecent->getRecentFiles());
}

/**
 * Returns the path of the locally bundled Vuo manual if it exists, or the URL of the
 * manual hosted on vuo.org otherwise.
 */
QString VuoEditor::getVuoManualURL()
{
	QString bundledManual = QDir::cleanPath(QApplication::applicationDirPath().append("/../Resources"))
							.append(QDir::separator())
							.append("vuo-")
							.append(VUO_VERSION_AND_BUILD_STRING)
							.append("--manual.pdf");

	if (QFile(bundledManual).exists())
		return QString("file://").append(bundledManual);
	else
		return "https://vuo.org/manual.pdf";
}

/**
 * Returns the default directory to be used for opening and saving Vuo compositions.
 */
QString VuoEditor::getDefaultCompositionStorageDirectory()
{
	return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

/**
 * Generates the full collection of node set and node class documentation and
 * saves it within the provided `saveDirectory`. If the directory does not already
 * exist, creates it.
 */
void VuoEditor::generateAllNodeSetHtmlDocumentation(string saveDirectory)
{
	const bool publishInternalVuoLinks = true;
	VuoFileUtilities::makeDir(saveDirectory);


	QString indexFilename(QString::fromStdString(saveDirectory) + "/index.html");
	QFile indexFile(indexFilename);
	if (!indexFile.open(QFile::WriteOnly | QFile::Truncate))
		throw VuoException(("Couldn't open " + indexFilename).toUtf8().data());

	QTextStream indexWriter(&indexFile);
	indexWriter << VUO_QSTRINGIFY(
		<html>
			<head>
				<title>Vuo %1 node documentation</title>
				<style>
					* { font-size: 13pt; font-family: 'PT Sans',Avenir,'Trebuchet MS',Tahoma,sans-serif; }
					body { padding: 5em; }
					h1 { font-size: 24pt; color: #aaa; font-weight: normal; }
					a, a:visited { font-weight: bold; color: #69a; }
					p { margin-top: 0; color: #aaa; }
					p a, p a:visited { font-weight: normal; color: #aaa; }
					li { list-style: none; }
				</style>
			</head>
			<body>
				<h1>Vuo %1 node documentation</h1>
				<ul>
		)
		.arg(VUO_VERSION_STRING);

	map<string, VuoCompilerNodeClass *> nodeClassesMap = compiler->getNodeClasses();
	vector<VuoCompilerNodeClass *> nodeClasses;
	for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClassesMap.begin(); i != nodeClassesMap.end(); ++i)
		nodeClasses.push_back(i->second);
	VuoNodeLibrary::cullHiddenNodeClasses(nodeClasses);

	set<string> nodeSetNames;
	foreach (VuoCompilerNodeClass *nodeClass, nodeClasses)
		if (nodeClass->getBase()->getNodeSet())
			nodeSetNames.insert(nodeClass->getBase()->getNodeSet()->getName());

	foreach (string nodeSetName, nodeSetNames)
	{
		string nodeSetSaveDirectory = saveDirectory + "/" + nodeSetName;
		VuoFileUtilities::makeDir(nodeSetSaveDirectory);

		string saveNodeSetHtml = nodeSetSaveDirectory + "/index.html";
		VuoNodeSet *nodeSet = compiler->getNodeSetForName(nodeSetName);

		generateMainHtmlPageForNodeSet(nodeSet, saveNodeSetHtml, publishInternalVuoLinks);
		nodeSet->extractDocumentationResources(nodeSetSaveDirectory);
		generateNodeClassHtmlPagesForNodeSet(nodeSet, nodeSetSaveDirectory, publishInternalVuoLinks);

		string description = nodeSet->getDescription();
		string firstLineOfDescription = description.substr(0, description.find('\n') - 1);
		string filteredDescription = VuoStringUtilities::generateHtmlFromMarkdownLine(VuoRendererCommon::externalizeVuoNodeLinks(compiler, QString::fromStdString(firstLineOfDescription), false).toStdString());
		indexWriter << VUO_QSTRINGIFY(
			<li><a href="%1/">%2</a><p>%3</p></li>
			)
			.arg(QString::fromStdString(nodeSetName))
			.arg(VuoEditorComposition::formatNodeSetNameForDisplay(QString::fromStdString(nodeSetName)))
			.arg(QString::fromStdString(filteredDescription));
	}

	indexWriter << VUO_QSTRINGIFY(
				</ul>
			</body>
		</html>
		);
}

/**
 * Activates the "vuo-nodeset" URL handler to handle the input @c url.
 * Extracts the node set name from the URL and displays its documentation
 * in an external browser.
 *
 * Sample @c url format: "vuo-nodeset://vuo.math"
 */
void VuoEditor::showNodeSetDocumentationFromUrl(const QUrl &url)
{
	const bool publishInternalVuoLinks = false;

	string nodeSetName = url.host().toUtf8().constData();
	VuoNodeSet *nodeSet = compiler->getNodeSetForName(nodeSetName);

	if (!nodeSet)
		return;

	// If the node set resource directory did not already exist, create and populate it now.
	string preexistingResourceDir = getResourceDirectoryForNodeSet(nodeSetName);
	string tmpSaveDir = (!preexistingResourceDir.empty()? preexistingResourceDir : VuoFileUtilities::makeTmpDir(nodeSetName));
	string tmpSaveNodeSetHtml = tmpSaveDir + "/index.html";

	if (tmpSaveDir != preexistingResourceDir)
	{
		generateMainHtmlPageForNodeSet(nodeSet, tmpSaveNodeSetHtml, publishInternalVuoLinks);

		// Extract resources referenced by the documentation.
		nodeSet->extractDocumentationResources(tmpSaveDir);
	}

	// Open the node set html file using an external browser.
	if (QDesktopServices::openUrl(QString("file://").append(tmpSaveNodeSetHtml.c_str())))
		emit activeApplicationStateChanged(false);

	// Save documentation for each member node class to its own html file.
	if (tmpSaveDir != preexistingResourceDir)
	{
		generateNodeClassHtmlPagesForNodeSet(nodeSet, tmpSaveDir, publishInternalVuoLinks);
		setResourceDirectoryForNodeSet(nodeSet->getName().c_str(), tmpSaveDir);
	}
}

/**
 * Activates the "vuo-node" URL handler to handle the input @c url.
 * Extracts the node class name from the URL and displays its documentation
 * in the node library documentation pane.
 *
 * Sample @c url format: "vuo-node://vuo.math.calculate"
 */
void VuoEditor::showNodeDocumentationFromUrl(const QUrl &url)
{
	string nodeClassName = url.host().toUtf8().constData();
	VuoNodeLibrary *topmostNodeLibrary = (VuoEditorWindow::getMostRecentActiveEditorWindow()?
											  VuoEditorWindow::getMostRecentActiveEditorWindow()->getCurrentNodeLibrary() :
											  NULL);

	if (topmostNodeLibrary)
		topmostNodeLibrary->prepareAndDisplayNodePopoverForClass(nodeClassName);
}

/**
 * Returns the HTML `<style>` block for use in node set and node documentation.
 *
 * When `forBrowser` is true, the styles are adjusted for viewing in a large web browser.
 * When false, the styles are adjusted for viewing in the small node documentation panel
 */
QString VuoEditor::generateHtmlDocumentationStyles(bool forBrowser, bool isDark)
{
	return VUO_QSTRINGIFY(<style>
		table, th, td {
			border: 1px solid #ccc;
			border-collapse: collapse;
			padding: %1;
		}
		code, kbd, pre {
			font-family: 'Monaco';
			font-size: 12px;
			background-color: %2;
			padding: 0 0.4em;
		}
		pre {
			padding: 1em;
			white-space: pre-wrap;
		}
	</style>)
	.arg(forBrowser ? "0.4em" : "0")
	.arg(isDark ? "#383838" : "#ececec");
}

/**
 * Generates the main HTML documentation page for the provided @c nodeSet,
 * and saves it at the provided @c saveFileName.
 */
void VuoEditor::generateMainHtmlPageForNodeSet(VuoNodeSet *nodeSet, string saveFileName, bool publishInternalVuoLinks)
{
	string nodeSetName = nodeSet->getName();
	vector<string> nodeSetClassNames = nodeSet->getNodeClassNames();

	// Metadata and node set description
	//: Appears in the webpage title on node set documentation pages.
	QString htmlHeader = "<html><head><meta charset=\"utf-8\"><title>" + tr("Vuo Node Set Documentation") + ": "
			+ QString::fromStdString(nodeSetName)
			+ "</title>"
			+ generateHtmlDocumentationStyles()
			+ "</head><body>";

	QString nodeSetDisplayName = VuoEditorComposition::formatNodeSetNameForDisplay(QString::fromStdString(nodeSetName));
	QString title = QString("<h2>").append(nodeSetDisplayName);
	if (nodeSetDisplayName != nodeSetName.c_str())
		title.append(" (").append(nodeSetName.c_str()).append(")");
	title.append("</h2>");

	string nodeSetDocumentationContent = nodeSet->getDescription();
	QString filteredNodeSetDocumentationContent = VuoStringUtilities::generateHtmlFromMarkdown(publishInternalVuoLinks?
																													   VuoRendererCommon::externalizeVuoNodeLinks(compiler, QString::fromStdString(nodeSetDocumentationContent), false).toStdString() :
																								  removeVuoLinks(nodeSetDocumentationContent)).c_str();


	QString htmlFooter = "</body></html>";

	// Example compositions
	vector<string> nodeSetExampleCompositionFileNames = nodeSet->getExampleCompositionFileNames();
	QString nodeSetExampleCompositionText = "";

	foreach (string compositionFileName, nodeSetExampleCompositionFileNames)
	{
		string compositionAsString = nodeSet->getExampleCompositionContents(compositionFileName);
		VuoCompositionMetadata metadata(compositionAsString);

		string name = metadata.getCustomizedName();
		if (name.empty())
			name = VuoEditorComposition::formatCompositionFileNameForDisplay(compositionFileName.c_str()).toUtf8().constData();

		string description = metadata.getDescription();
		string filteredDescription = (publishInternalVuoLinks? VuoRendererCommon::externalizeVuoNodeLinks(compiler, QString::fromStdString(description), false).toStdString() : removeVuoLinks(description));
		QString compositionDescription = VuoStringUtilities::generateHtmlFromMarkdownLine(filteredDescription).c_str();

		nodeSetExampleCompositionText.append("<li>")
				.append("<a href=\"")
				.append(VuoEditor::vuoExampleCompositionScheme)
				.append("://")
				.append(nodeSetName.c_str())
				.append("/")
				.append(compositionFileName.c_str())
				.append("\"><font size=+1>")
				.append(name.c_str())
				.append("</font></a>");

		if (!compositionDescription.isEmpty())
			nodeSetExampleCompositionText.append(": ").append(compositionDescription);

		nodeSetExampleCompositionText.append("</li>\n");
	}

	if (nodeSetExampleCompositionText.size() > 0)
		//: Appears on node set documentation webpages.
		nodeSetExampleCompositionText = "<BR><HR><h3>" + tr("Example composition(s)", "", nodeSetExampleCompositionFileNames.size()) + ":</h3>\n<ul>\n" + nodeSetExampleCompositionText + "</ul>";

	// Node classes
	QString nodeSetClassesText = "";

	// Sort by default node title.
	std::sort(nodeSetClassNames.begin(), nodeSetClassNames.end(), [=](const string nodeClassName1, const string nodeClassName2) {
		VuoCompilerNodeClass *nodeClass1 = compiler->getNodeClass(nodeClassName1);
		VuoCompilerNodeClass *nodeClass2 = compiler->getNodeClass(nodeClassName2);
		string nodeClass1Title = nodeClass1? nodeClass1->getBase()->getDefaultTitle() : "";
		string nodeClass2Title = nodeClass2? nodeClass2->getBase()->getDefaultTitle() : "";

		return nodeClass1Title < nodeClass2Title;
	});

	foreach (string nodeClassName, nodeSetClassNames)
	{
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName);
		if (!nodeClass || nodeClass->getBase()->getDeprecated())
			continue;

		QString nodeClassTitle = nodeClass->getBase()->getDefaultTitle().c_str();
		QString nodeClassDescription = VuoStringUtilities::generateHtmlFromMarkdown(nodeClass->getBase()->getDescription()).c_str();
		QString nodeClassProNodeIndicator;
#if VUO_PRO
		if (nodeClass->getBase()->isPro())
			//: Appears on node set documentation webpages.
			nodeClassProNodeIndicator = " <b>[<a href=\"https://vuo.org/pro-nodes\">" + tr("Pro node") + "</a>]</b>";
#endif

		QString nodeClassDocumentationLink = QString((nodeClassName + ".html").c_str());

		// Strip HTML and extract the first sentence of the node class description for display.
		nodeClassDescription.remove(QRegExp("<[^>]*>"));
		nodeClassDescription.replace(QRegExp("\\.\\s.*"), ".");

		nodeSetClassesText.append("<li>")
				.append("<a href=\"")
				.append(nodeClassDocumentationLink)
				.append("\">")
				.append("<font size=+1>")
				.append(nodeClassTitle)
				.append("</font>")
				.append("</a>")
				.append(" (")
				.append(nodeClassName.c_str())
				.append(")");

		nodeSetClassesText.append(nodeClassProNodeIndicator);

		if (!nodeClassDescription.isEmpty())
			nodeSetClassesText.append(": ").append(nodeClassDescription);

		nodeSetClassesText.append("</li>\n");
	}

	if (nodeSetClassesText.size() > 0)
		//: Appears on node set documentation webpages.
		nodeSetClassesText = "<BR><HR><h3>" + tr("Node(s)", "", nodeSetClassNames.size()) + ":</h3>\n<ul>\n" + nodeSetClassesText + "</ul>";

	// Save the node set documentation to an html file.
	ofstream savedNodeSetFile(saveFileName.c_str(), ios::trunc);

	savedNodeSetFile << htmlHeader.append("\n\n").toUtf8().constData();
	savedNodeSetFile << title.append("\n\n").toUtf8().constData();
	savedNodeSetFile << filteredNodeSetDocumentationContent.append("\n\n").toUtf8().constData();
	savedNodeSetFile << nodeSetExampleCompositionText.append("\n\n").toUtf8().constData();
	savedNodeSetFile << nodeSetClassesText.append("\n\n").toUtf8().constData();
	savedNodeSetFile << htmlFooter.append("\n\n").toUtf8().constData();

	savedNodeSetFile.close();
}

/**
 * Generates the HTML documentation pages for non-deprecated node classes belonging to the
 * provided @c nodeSet, and saves them in provided @c saveDir.
 *
 * If @c publishInternalVuoLinks is true, maps internal vuo-node:// and vuo-nodeset:// links to
 * the appropriate relative links for use within web documentation. This function is not
 * responsible for ensuring that the target files referenced by those links actually exist.
 *
 * If @c publishInternalVuoLinks is false, filters out internal links instead of mapping them
 * to external links.
 */
void VuoEditor::generateNodeClassHtmlPagesForNodeSet(VuoNodeSet *nodeSet, string saveDir, bool publishInternalVuoLinks)
{
	string nodeSetName = nodeSet->getName();
	vector<string> nodeSetClassNames = nodeSet->getNodeClassNames();
	foreach (string nodeClassName, nodeSetClassNames)
	{
		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName);
		if (!nodeClass || nodeClass->getBase()->getDeprecated())
			continue;

		string nodeClassTitle = nodeClass->getBase()->getDefaultTitle();
		string nodeClassDescription = nodeClass->getBase()->getDescription();
		string filteredNodeClassDescription = (publishInternalVuoLinks? VuoRendererCommon::externalizeVuoNodeLinks(compiler, QString::fromStdString(nodeClassDescription), false).toStdString() :
																		removeVuoLinks(nodeClassDescription));

		vector<string> manualKeywords = nodeClass->getBase()->getKeywords();
		vector<string> automaticKeywords = nodeClass->getAutomaticKeywords();

		set<string> sortedUniqueKeywords;
		foreach (string keyword, manualKeywords)
			sortedUniqueKeywords.insert(keyword);

		foreach (string keyword, automaticKeywords)
			sortedUniqueKeywords.insert(keyword);

		// Metadata and node class description
		//: Appears in the webpage title on node documentation pages.
		QString nodeClassHtmlHeader = "<html><head><meta charset=\"utf-8\"><title>" + tr("Vuo Node Documentation") + ": "
			+ QString::fromStdString(nodeClassTitle)
			+ " (" + QString::fromStdString(nodeClassName) + ")"
			+ "</title>"
			+ generateHtmlDocumentationStyles()
			+ "</head><body>";
		QString nodeClassDocumentationTitle = QString("<h2>")
											  .append(nodeClassTitle.c_str())
											  .append(" (").append(nodeClassName.c_str()).append(")")
											  .append("</h2>");
		QString nodeClassDocumentationContent = VuoStringUtilities::generateHtmlFromMarkdown(filteredNodeClassDescription).c_str();
		//: Appears on node documentation webpages.
		QString nodeClassKeywordsIntro = "<p><b>" + tr("Keyword(s)", "", sortedUniqueKeywords.size()) + "</b>: ";
		QString nodeClassKeywordsText = nodeClassKeywordsIntro;
		foreach (string keyword, sortedUniqueKeywords)
		{
			if (nodeClassKeywordsText != nodeClassKeywordsIntro)
				nodeClassKeywordsText.append(", ");

			nodeClassKeywordsText.append("<i>").append(keyword.c_str()).append("</i>");
		}
		nodeClassKeywordsText.append("</p>");

		// Rendering of model node
		VuoRendererComposition *composition = new VuoRendererComposition(new VuoComposition());
		VuoRendererNode *modelNode = new VuoRendererNode(nodeClass->newNode(), NULL);
		modelNode->setAlwaysDisplayPortNames(true);
		composition->addNode(modelNode->getBase());
		composition->createAndConnectInputAttachments(modelNode, compiler);

		QSizeF size = composition->itemsBoundingRect().size().toSize();
		QSizeF retinaSize(size.width()*2, size.height()*2);
		QPixmap pixmap(retinaSize.toSize());
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
		painter.setRenderHint(QPainter::TextAntialiasing, true);

		composition->render(&painter);

		string nodeClassRenderedPreviewFileName = nodeClassName + ".png";
		string tmpSaveNodeClassImage = saveDir + "/" + nodeClassRenderedPreviewFileName;
		QFile file(tmpSaveNodeClassImage.c_str());
		file.open(QIODevice::WriteOnly);
		pixmap.save(&file, "PNG");

		delete modelNode;
		delete composition;

		QString nodeClassRenderedPreview = QString("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
										   .arg(nodeClassRenderedPreviewFileName.c_str())
										   .arg(size.width())
										   .arg(size.height());

		// Node class example compositions
		vector<string> nodeClassExampleCompositionFileNames = nodeClass->getBase()->getExampleCompositionFileNames();
		QString nodeClassExampleCompositionText = "";

		foreach (string compositionFileName, nodeClassExampleCompositionFileNames)
		{
			string compositionAsString = nodeSet->getExampleCompositionContents(compositionFileName);
			VuoCompositionMetadata metadata(compositionAsString);

			string name = metadata.getCustomizedName();
			if (name.empty())
				name = VuoEditorComposition::formatCompositionFileNameForDisplay(compositionFileName.c_str()).toUtf8().constData();

			// Override @c publishInternalVuoLinks here, and always filter out links to node classes from
			// example composition descriptions, since those example descriptions will almost always reference the same
			// node class whose documentation references the example.
			string description = metadata.getDescription();
			string filteredDescription = removeVuoLinks(description);
			QString compositionDescription = VuoStringUtilities::generateHtmlFromMarkdownLine(filteredDescription).c_str();

			nodeClassExampleCompositionText.append("<li>")
					.append("<a href=\"")
					.append(VuoEditor::vuoExampleCompositionScheme)
					.append("://")
					.append(nodeSetName.c_str())
					.append("/")
					.append(compositionFileName.c_str())
					.append("?")
					.append(VuoEditor::vuoExampleHighlightedNodeClassQueryItem)
					.append("=")
					.append(nodeClass->getBase()->getClassName().c_str())
					.append("\"><font size=+1>")
					.append(name.c_str())
					.append("</font></a>");

			if (!compositionDescription.isEmpty())
				nodeClassExampleCompositionText.append(": ").append(compositionDescription);

			nodeClassExampleCompositionText.append("</li>\n");
		}

		if (nodeClassExampleCompositionFileNames.size() > 0)
			//: Appears on node documentation webpages.
			nodeClassExampleCompositionText = "<HR><h3>" + tr("Example composition(s)", "", nodeClassExampleCompositionFileNames.size()) + ":</h3>\n<ul>\n" + nodeClassExampleCompositionText + "</ul>";

		// Footer
		QString nodeClassProNodeIndicator;
#if VUO_PRO
		if (nodeClass->getBase()->isPro())
			nodeClassProNodeIndicator = QString(VuoNodePopover::installedProNodeText).append("<br><br>");
#endif
		//: Appears on node documentation webpages.
		QString nodeClassSetReference = "<HR>" + nodeClassProNodeIndicator + tr("Back to %1 node set documentation.")
			.arg("<a href=\"index.html\">" + QString::fromStdString(nodeSetName) + "</a>");
		QString nodeClassHtmlFooter = "</body></html>";

		string saveNodeClassHtml = saveDir + "/" + nodeClassName + ".html";
		ofstream savedNodeClassFile(saveNodeClassHtml.c_str(), ios::trunc);

		savedNodeClassFile << nodeClassHtmlHeader.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassDocumentationTitle.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassRenderedPreview.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassDocumentationContent.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassKeywordsText.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassExampleCompositionText.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassSetReference.append("\n\n").toUtf8().constData();
		savedNodeClassFile << nodeClassHtmlFooter.append("\n\n").toUtf8().constData();

		savedNodeClassFile.close();
	}
}


/**
 * Filters out internal vuo-node:// and vuo-nodeset:// links within the provided @c markdownText,
 * encapsulating the previously linked text in backticks.
 */
string VuoEditor::removeVuoLinks(string markdownText)
{
	QString filteredText(markdownText.c_str());
	QRegularExpression vuoNodeLink("\\[([^\\]]+)\\](\\(vuo-node://([^)]+)\\))");
	filteredText.replace(vuoNodeLink, "`\\1`");

	QRegularExpression vuoNodeSetLink("\\[([^\\]]+)\\](\\(vuo-nodeset://([^)]+)\\))");
	filteredText.replace(vuoNodeSetLink, "`\\1`");

	return filteredText.toUtf8().constData();
}

/**
 * Activates the "vuo-example" URL handler to handle the input @c url.
 * Extracts the node set name and example composition name from the URL and
 * opens the example composition.
 *
 * Sample @c url format: "vuo-example://vuo.text/RevealWord.vuo"
 */
void VuoEditor::openExampleCompositionFromUrl(const QUrl &url)
{
	string nodeSetName = url.host().toUtf8().constData();
	string nodeClassToHighlight = "";

	QUrlQuery query(url.query());
	// Check whether the url contains the expected query parameters.
	if (query.hasQueryItem(vuoExampleHighlightedNodeClassQueryItem))
		nodeClassToHighlight = query.queryItemValue(vuoExampleHighlightedNodeClassQueryItem, QUrl::FullyDecoded).toUtf8().constData();

	string compositionFileName = VuoStringUtilities::substrAfter(url.path().toUtf8().constData(), "/");

	VuoNodeSet *nodeSet = compiler->getNodeSetForName(nodeSetName);
	if (!nodeSet)
		return;

	// Open the composition.
	openExampleComposition(compositionFileName.c_str(), nodeSet, nodeClassToHighlight);
}

/**
 * Returns the text content of the clipboard, or the empty string if the clipboard
 * does not contain text content.
 */
QString VuoEditor::getClipboardText()
{
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();

	if (!mimeData->hasFormat("text/plain") || mimeData->text().isNull())
		return "";

	return mimeData->text();
}

/**
 * Returns the preferred node documentation panel height as indicated in stored user
 * preferences and/or actions performed since the stored preferences were last applied.
 */
int VuoEditor::getPreferredNodeDocumentationPanelHeight()
{
	return this->nodeDocumentationPanelHeight;
}

/**
 * Returns the preferred node library width as indicated in stored user
 * preferences and/or actions performed since the stored preferences were last applied.
 */
int VuoEditor::getPreferredNodeLibraryWidth()
{
	return this->nodeLibraryWidth;
}

/**
 * Returns the resource directory associated with the provided @c nodeSetName.
 * If no such resource directory exists, returns the empty string.
 * The existence of the resource directory implies that all resources (e.g., images)
 * associated with the node set have already been extracted, and all HTML pages
 * for the node set and its node classes have already been generated and saved within this directory.
 */
string VuoEditor::getResourceDirectoryForNodeSet(string nodeSetName)
{
	if (resourceDirectoryForNodeSet.find(nodeSetName) != resourceDirectoryForNodeSet.end())
		return resourceDirectoryForNodeSet[nodeSetName];
	else
		return "";
}

/**
 * Sets the resource directory associated with the provided @c nodeSetName to the
 * provided @c directory.
 */
void VuoEditor::setResourceDirectoryForNodeSet(string nodeSetName, string directory)
{
	resourceDirectoryForNodeSet[nodeSetName] = directory;
}

/**
 * Returns the subcomposition message router shared by all open compositions.
 */
VuoSubcompositionMessageRouter * VuoEditor::getSubcompositionRouter(void)
{
	return subcompositionRouter;
}

/**
 * Initialize the editor's built-in drivers.
 */
void VuoEditor::initializeBuiltInDrivers()
{
	// Image filter driver
	VuoProtocol *imageFilterProtocol = VuoProtocol::getProtocol(VuoProtocol::imageFilter);
	string imageFilterDriverPath = VuoFileUtilities::getVuoFrameworkPath() + "/Resources/" + "imageFilterDriver.vuo";
	string imageFilterDriverAsString = (VuoFileUtilities::fileExists(imageFilterDriverPath)?
											VuoFileUtilities::readFileToString(imageFilterDriverPath) :
											"");

	if (!imageFilterDriverAsString.empty())
	{
		VuoCompilerDriver *imageFilterDriver = new VuoCompilerDriver(compiler, imageFilterDriverAsString);
		if (imageFilterDriver->isValidDriverForProtocol(imageFilterProtocol))
			builtInDriverForProtocol[imageFilterProtocol] = imageFilterDriver;
	}

	// Image generator driver
	VuoProtocol *imageGeneratorProtocol = VuoProtocol::getProtocol(VuoProtocol::imageGenerator);
	string imageGeneratorDriverPath = VuoFileUtilities::getVuoFrameworkPath() + "/Resources/" + "imageGeneratorDriver.vuo";
	string imageGeneratorDriverAsString = (VuoFileUtilities::fileExists(imageGeneratorDriverPath)?
											   VuoFileUtilities::readFileToString(imageGeneratorDriverPath) :
											   "");

	if (!imageGeneratorDriverAsString.empty())
	{
		VuoCompilerDriver *imageGeneratorDriver = new VuoCompilerDriver(compiler, imageGeneratorDriverAsString);
		if (imageGeneratorDriver->isValidDriverForProtocol(imageGeneratorProtocol))
			builtInDriverForProtocol[imageGeneratorProtocol] = imageGeneratorDriver;
	}

	// Image transition driver
	VuoProtocol *imageTransitionProtocol = VuoProtocol::getProtocol(VuoProtocol::imageTransition);
	string imageTransitionDriverPath = VuoFileUtilities::getVuoFrameworkPath() + "/Resources/" + "imageTransitionDriver.vuo";
	string imageTransitionDriverAsString = (VuoFileUtilities::fileExists(imageTransitionDriverPath)?
											   VuoFileUtilities::readFileToString(imageTransitionDriverPath) :
											   "");

	if (!imageTransitionDriverAsString.empty())
	{
		VuoCompilerDriver *imageTransitionDriver = new VuoCompilerDriver(compiler, imageTransitionDriverAsString);
		if (imageTransitionDriver->isValidDriverForProtocol(imageTransitionProtocol))
			builtInDriverForProtocol[imageTransitionProtocol] = imageTransitionDriver;
	}

	// Extract built-in images to be used by protocol drivers.
	VuoNodeSet *imageNodeSet = compiler->getNodeSetForName("vuo.image");
	if (imageNodeSet)
		imageNodeSet->extractExampleCompositionResources(VuoFileUtilities::getTmpDir());
}

/**
 * Returns the built-in driver for the provided @c protocol, or NULL if none.
 */
VuoCompilerDriver * VuoEditor::getBuiltInDriverForProtocol(VuoProtocol *protocol)
{
	dispatch_sync(builtInDriversQueue, ^{});

	map<VuoProtocol *, VuoCompilerDriver *>::iterator driver = builtInDriverForProtocol.find(protocol);
	if (driver != builtInDriverForProtocol.end())
		return driver->second;
	else
		return NULL;
}

/**
 * Populates the provided menu @c m with the editor's built-in templates.
 */
void VuoEditor::populateNewCompositionWithTemplateMenu(QMenu *m)
{
	// Sub-item indentation.
	// Use transparent icons instead of whitespace padding,
	// so that users can more easily customize the menu item shortcuts via System Preferences.
	QPixmap emptyPixmap(32, 32);
	emptyPixmap.fill(Qt::transparent);
	QIcon indentIcon(emptyPixmap);

	// "Window" subcategories
	QAction *windowHeading = new QAction(this);
	windowHeading->setText(tr("Window"));
	windowHeading->setEnabled(false);
	m->addAction(windowHeading);

	// Image template
	{
		QAction *templateAction = new QAction(this);
		templateAction->setText(tr("Image"));
		templateAction->setData("imageTemplate");
		templateAction->setIcon(indentIcon);
		connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
		m->addAction(templateAction);
	}

	// Layers template
	{
		QAction *templateAction = new QAction(this);
		templateAction->setText(tr("Layers"));
		templateAction->setData("layersTemplate");
		templateAction->setIcon(indentIcon);
		connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
		m->addAction(templateAction);
	}

	// Scene template
	{
		QAction *templateAction = new QAction(this);
		templateAction->setText(tr("Scene"));
		templateAction->setData("sceneTemplate");
		templateAction->setIcon(indentIcon);
		connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
		m->addAction(templateAction);
	}

	// "Protocol" subcategories
	QAction *protocolHeading = new QAction(this);
	protocolHeading->setText(tr("Protocol"));
	protocolHeading->setEnabled(false);
	m->addSeparator();
	m->addAction(protocolHeading);

	// Protocols
	foreach (VuoProtocol *protocol, VuoProtocol::getProtocols())
	{
		QAction *protocolAction = new QAction(this);
		protocolAction->setText(tr(protocol->getName().c_str()));
		protocolAction->setData(qVariantFromValue(static_cast<void *>(protocol)));
		protocolAction->setIcon(indentIcon);
		connect(protocolAction, &QAction::triggered, this, &VuoEditor::newCompositionWithProtocol);
		m->addAction(protocolAction);
	}

	// "Export" subcategories
	{
		m->addSeparator();

		QAction *heading = new QAction(this);
		heading->setText(tr("Export"));
		heading->setEnabled(false);
		m->addAction(heading);

		{
			QAction *templateAction = new QAction(this);
			templateAction->setText(tr("Movie"));
			templateAction->setData("movie");
			templateAction->setIcon(indentIcon);
			connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
			m->addAction(templateAction);
		}

		{
			QAction *templateAction = new QAction(this);
			templateAction->setText(tr("Screen Saver"));
			templateAction->setData("screensaver");
			templateAction->setIcon(indentIcon);
			connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
			m->addAction(templateAction);
		}

		{
			QAction *heading = new QAction(this);
			heading->setText(tr("FFGL"));
			heading->setIcon(indentIcon);
			heading->setEnabled(false);
			m->addAction(heading);

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Source"));
				templateAction->setData("FFGLSource");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Effect"));
				templateAction->setData("FFGLEffect");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Blend Mode"));
				templateAction->setData("FFGLBlendMode");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}
		}

		{
			QAction *heading = new QAction(this);
			heading->setText(tr("FxPlug"));
			heading->setIcon(indentIcon);
			heading->setEnabled(false);
			m->addAction(heading);

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Generator"));
				templateAction->setData("FxPlugGenerator");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Effect"));
				templateAction->setData("FxPlugEffect");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}

			{
				QAction *templateAction = new QAction(this);
				templateAction->setText("     " + tr("Transition"));
				templateAction->setData("FxPlugTransition");
				templateAction->setIcon(indentIcon);
				connect(templateAction, &QAction::triggered, this, &VuoEditor::newCompositionWithTemplate);
				m->addAction(templateAction);
			}
		}
	}
}

/**
 * Populates the provided menu with the templates for shaders.
 */
void VuoEditor::populateNewShaderMenu(QMenu *m)
{
	{
		QAction *action = new QAction(m);
		action->setText(tr("Image Filter"));
		action->setData("GLSLImageFilter");
		connect(action, &QAction::triggered, [=] {
			closeUnmodifiedUntitledComposition();
#if VUO_PRO
			if (!closeWelcomeWindow())
				return;
#endif
			VuoCodeWindow::newShaderWithTemplate(action);
		});
		m->addAction(action);
	}

	{
		QAction *action = new QAction(m);
		action->setText(tr("Image Generator"));
		action->setData("GLSLImageGenerator");
		connect(action, &QAction::triggered, [=] {
			closeUnmodifiedUntitledComposition();
#if VUO_PRO
			if (!closeWelcomeWindow())
				return;
#endif
			VuoCodeWindow::newShaderWithTemplate(action);
		});
		m->addAction(action);
	}

	{
		QAction *action = new QAction(m);
		action->setText(tr("Image Transition"));
		action->setData("GLSLImageTransition");
		connect(action, &QAction::triggered, [=] {
			closeUnmodifiedUntitledComposition();
#if VUO_PRO
			if (!closeWelcomeWindow())
				return;
#endif
			VuoCodeWindow::newShaderWithTemplate(action);
		});
		m->addAction(action);
	}
}

/**
 * Returns the dispatch queue to be used for documentation-related tasks, such as
 * extracting resources from node set archives.
 */
dispatch_queue_t VuoEditor::getDocumentationQueue()
{
	return documentationQueue;
}

/**
 * Returns true if the user interface is in dark mode.
 */
bool VuoEditor::isInterfaceDark(void)
{
#if VUO_PRO
	return darkInterfaceAction->isChecked();
#else
	return false;
#endif
}

/**
 * Returns the current opacity level of the canvas, where 0
 * is fully transparent and 255 is fully opaque.
 */
int VuoEditor::getCanvasOpacity(void)
{
	return this->canvasOpacity;
}

/**
 * Invoked when the user toggles the "Grid > Lines" menu item.
 */
void VuoEditor::showGridLinesToggled(bool show)
{
	VuoRendererComposition::GridType type;
	if (show)
	{
		showGridPointsAction->setChecked(false);
		type = VuoRendererComposition::LineGrid;
	}
	else
		type = VuoRendererComposition::NoGrid;

	settings->setValue(gridTypeSettingsKey, type);
	VuoRendererComposition::setGridType(type);

	emit showGridToggled();
}

/**
 * Invoked when the user toggles the "Grid > Points" menu item.
 */
void VuoEditor::showGridPointsToggled(bool show)
{
	VuoRendererComposition::GridType type;
	if (show)
	{
		showGridLinesAction->setChecked(false);
		type = VuoRendererComposition::PointGrid;
	}
	else
		type = VuoRendererComposition::NoGrid;

	settings->setValue(gridTypeSettingsKey, type);
	VuoRendererComposition::setGridType(type);

	emit showGridToggled();
}

/**
 * Stores the snap-to-grid preference, and updates the shared setting.
 */
void VuoEditor::updateSnapToGrid(bool snap)
{
	settings->setValue(snapToGridSettingsKey, snap);

	VuoRendererItem::setSnapToGrid(snap);
}

/**
 * Stores the dark interface preference, and updates the shared colors.
 */
void VuoEditor::updateColor(bool isDark)
{
	settings->setValue(darkInterfaceSettingsKey, isDark);

	VuoRendererColors::setDark(isDark);

	{
		QFile f(":/Vuo.qss");
		f.open(QFile::ReadOnly | QFile::Text);
		QTextStream ts(&f);
		QString styles = ts.readAll();

		if (isDark)
		{
			QFile f(":/pro/VuoDark.qss");
			f.open(QFile::ReadOnly | QFile::Text);
			QTextStream ts(&f);
			styles += ts.readAll();
		}

		setStyleSheet(styles);
	}
}

/**
 * Sets the global canvas opacity level to the value associated with the sender action;
 * stores the value as a preference in the settings.
 */
void VuoEditor::updateCanvasOpacity(QAction *setOpacityAction)
{
	updateCanvasOpacityTo(setOpacityAction->data().toInt());
	updateUI();
}

/**
 * Sets the global canvas opacity level to the `opacity` value provided; stores the
 * provided value as a preference in the settings.
 */
void VuoEditor::updateCanvasOpacityTo(int opacity)
{
	settings->setValue(canvasOpacitySettingsKey, opacity);
	canvasOpacity = opacity;
}

/**
 * Sets the directory in which to save generated node documentation.
 *
 * If this value is non-empty when the VuoEditor is instantiated, the editor
 * will generate the full collection of node class and node set documentation,
 * save it within the provided directory, and exit.
 */
void VuoEditor::setDocumentationGenerationDirectory(string dir)
{
	VuoEditor::documentationGenerationDirectory = dir;
}

/**
 * Returns a map containing the uberexamplecompositions to be
 * displayed at the top level within the "File > Open Example" menu, and the
 * node sets to which they belong.
 */
map<QString, QString> VuoEditor::getUberExampleCompositions()
{
	map<QString, QString> modelExampleCompositionsAndNodeSets;
	modelExampleCompositionsAndNodeSets["LaserDiscoball.vuo"] = "vuo.audio";
	modelExampleCompositionsAndNodeSets["Tschuri.vuo"] = "vuo.image";
	modelExampleCompositionsAndNodeSets["SlitscanMixingInk.vuo"] = "vuo.layer";
	modelExampleCompositionsAndNodeSets["DrawInSpace.vuo"] = "vuo.scene";
	return modelExampleCompositionsAndNodeSets;
}

/**
 * Returns a map containing the example compositions to be listed in association with the provided
 * `protocol` in the "File > Open Example" menu, and the node sets to which they belong.
 */
map<QString, QString> VuoEditor::getExampleCompositionsForProtocol(VuoProtocol *protocol)
{
	map<QString, QString> modelExampleCompositionsAndNodeSets;

	if (protocol->getId() == VuoProtocol::imageFilter)
	{
		modelExampleCompositionsAndNodeSets["PixellateImageRadially.vuo"] = "vuo.image";
	}

	else if (protocol->getId() == VuoProtocol::imageGenerator)
	{
		modelExampleCompositionsAndNodeSets["GenerateCheckerboardImage.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["MakeDriftingClouds.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["MakeOvalPatterns.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["RippleImageGradients.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["SpinKaleidoscope.vuo"] = "vuo.event";
	}

	else if (protocol->getId() == VuoProtocol::imageTransition)
	{
		modelExampleCompositionsAndNodeSets["BlendImages.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["CrossfadeAndZoom.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["CrossfadeWithBlobMask.vuo"] = "vuo.image";
		modelExampleCompositionsAndNodeSets["CrossfadeWithMovingTiles.vuo"] = "vuo.image";
	}

	return modelExampleCompositionsAndNodeSets;
}

/**
 * Returns the example composition URL corresponding to the example composition with the provided name and node set.
 */
QString VuoEditor::getURLForExampleComposition(QString compositionName, QString nodeSetName)
{
	QString compositionURL = QString(VuoEditor::vuoExampleCompositionScheme)
									.append("://")
									.append(nodeSetName)
									.append("/")
									.append(compositionName);
	return compositionURL;
}

/**
  * Returns the username associated with this editor session.
  */
string VuoEditor::getUserName()
{
#if VUO_PRO
	return getStoredUserName_Pro();
#else
	return getenv("USER");
#endif
}

/**
 * Returns the user profile URL associated with this editor session.
 */
string VuoEditor::getStoredUserProfileURL()
{
#if VUO_PRO
	return getStoredUserProfileURL_Pro();
#else
	return "";
#endif
}

/**
 * Sets the global subcomposition prefix to the value provided; stores the
 * provided value as a preference in the settings.
 */
void VuoEditor::updateSubcompositionPrefix(QString prefix)
{
	settings->setValue(subcompositionPrefixSettingsKey, prefix);
	subcompositionPrefix = prefix;
}


/**
  * Returns the string to be used as the subcomposition prefix for this user.
  */
QString VuoEditor::getSubcompositionPrefix()
{
	return (!subcompositionPrefix.isEmpty()? subcompositionPrefix : getDefaultSubcompositionPrefix());
}

/**
  * Returns the default string to be used as the subcomposition prefix for this user
  * if it has not been customized by the user.
  */
QString VuoEditor::getDefaultSubcompositionPrefix()
{
	return getUserName().c_str();
}

/**
 * Opens the most recent file.
 */
void VuoEditor::openMostRecentFile()
{
	menuOpenRecent->openMostRecentFile();
}

/**
 * Opens a random example composition.
 */
void VuoEditor::openRandomExample()
{
	menuOpenExample->openRandomExample();
}
