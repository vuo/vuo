/**
 * @file
 * VuoEditor interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoHeap.h"
#include "VuoNodeLibrary.hh"

class VuoCompiler;
class VuoCompilerDriver;
class VuoEditorAboutBox;
class VuoEditorWelcome;
class VuoEditorWindow;
class VuoExampleMenu;
class VuoModuleManager;
class VuoNodeSet;
class VuoProtocol;
class VuoRecentFileMenu;
class VuoSubcompositionMessageRouter;

/**
 * The Vuo Editor's QApplication subclass.  Singleton.  Provides the standard menu bar.
 */
class VuoEditor : public QApplication
{
	Q_OBJECT
public:
	/// Possible user subscription levels:
	enum subscriptionLevel
	{
		CommunityUser,
		ProSubscriber
	};
	Q_ENUMS(subscriptionLevel)

	explicit VuoEditor(int &argc, char *argv[]);
	~VuoEditor();
	Q_INVOKABLE void processQueuedCompositionsToOpen();
	Q_INVOKABLE void reevaluateWelcomePrecedence();
	static void setDocumentationGenerationDirectory(string dir);
	static QString getDefaultCompositionStorageDirectory();
	static string removeVuoLinks(string markdownText);
	string getUserName();
	string getStoredUserProfileURL();
	QString getSubcompositionPrefix();
	QString getDefaultSubcompositionPrefix();
	map<QString, QString> getUberExampleCompositions();
	map<QString, QString> getExampleCompositionsForProtocol(VuoProtocol *protocol);
	static QString getURLForExampleComposition(QString compositionName, QString nodeSetName);
	static QString getVuoManualURL();
	static QString getClipboardText();
	int getPreferredNodeDocumentationPanelHeight();
	int getPreferredNodeLibraryWidth();
	void highlightNewNodeClassInAllLibraries(string nodeClassName);
	void updateSubcompositionPrefix(QString prefix);
	void updateGlobalShaderDocumentationVisibility(bool isVisible);
	bool getGlobalShaderDocumentationVisibility();
	void getGlobalMovieExportSettings(int &width, int &height, double &time, double &duration, double &framerate, int &spatialSupersample, int &temporalSupersample, float &shutterAngle, QString &imageFormat, double &quality);
	static QString generateHtmlDocumentationStyles(bool forBrowser=true, bool isDark=false);
	string getResourceDirectoryForNodeSet(string nodeSetName);
	void setResourceDirectoryForNodeSet(string nodeSetName, string directory);
	void generateMainHtmlPageForNodeSet(VuoNodeSet *nodeSet, string saveFileName, bool publishInternalVuoLinks);
	void generateNodeClassHtmlPagesForNodeSet(VuoNodeSet *nodeSet, string saveDir, bool publishInternalVuoLinks);
	VuoSubcompositionMessageRouter * getSubcompositionRouter(void);
	VuoCompilerDriver * getBuiltInDriverForProtocol(VuoProtocol *protocol);
	dispatch_queue_t getDocumentationQueue();
	bool isNodeLibraryCurrentlyDocked();
	void registerOpenDocument(QMainWindow *window);
	VuoEditorWindow * newCompositionWithContent(string content, string compositionDir="");
	QMainWindow * openFileWithName(QString filename, bool addToRecentFileMenu=true);
	void populateNewCompositionWithTemplateMenu(QMenu *m);
	void populateNewShaderMenu(QMenu *m);
	void populateCanvasTransparencyMenu(QMenu *m);
	void populateWindowMenu(QMenu *m, QMainWindow *currentWindow);
	void populateHelpMenu(QMenu *m);
	QAction *showGridLinesAction;   ///< Toggles grid line rendering.
	QAction *showGridPointsAction;  ///< Toggles grid point rendering.
	QAction *snapToGridAction;      ///< Toggles grid snapping.
	bool isInterfaceDark(void);
	int getCanvasOpacity(void);
	Q_INVOKABLE QStringList recentFiles();
	QActionGroup *canvasTransparencyOptions;  ///< Actions for changing canvas transparency.
	QAction *canvasTransparencyNoneAction;    ///< Makes the canvas opaque.
	QAction *canvasTransparencySlightAction;  ///< Makes the canvas slightly transparent.
	QAction *canvasTransparencyHighAction;    ///< Makes the canvas highly transparent.
	static const QString vuoHelpBookScheme;
	static const QString vuoNodeSetDocumentationScheme;
	static const QString vuoNodeDocumentationScheme;
	static const QString vuoExampleCompositionScheme;
	static const QString vuoExampleHighlightedNodeClassQueryItem;
	static const QString vuoCompositionFileExtension;
	static const QString vuoNodeClassFileExtension;
	static const QString vuoTutorialURL;

signals:
	void globalNodeLibraryStateChanged(VuoNodeLibrary::nodeLibraryState newState, VuoNodeLibrary *floater, bool previousFloaterDestroyed); ///< Emitted after @c updateGlobalNodeLibraryState() is called.
	void activeApplicationStateChanged(bool active); ///< Emitted when the application gains or loses focus.
	void darkInterfaceToggled(bool isDark);	///< Emitted when the user changes the "Dark Interface" menu item.
	void showGridToggled(); ///< Emitted when the user toggles the "Grid > Lines" or "Grid > Points" menu item.
	void canvasOpacityChanged(); ///< Emitted when the user selects a new "Canvas Transparency" menu item.
	void applicationWillHide();    ///< Emitted when macOS hides the application.
	void applicationDidUnhide();  ///< Emitted when macOS unhides the application.

public slots:
	void about(void);
	void newComposition(void);
	void newCompositionWithProtocol(void);
	void newCompositionWithTemplate(void);
	void openUrl(const QString &url);
	void openFile(void);
	void openExampleComposition(QString filename, VuoNodeSet *nodeSet, string nodeClassToHighlight);
	void showNodeLibrary(void);
	void addFileToAllOpenRecentFileMenus(QString filePath);
	void addFileToRecentlyClosedList(QString filePath);
	void clearAllOpenRecentFileMenus();
	void pruneAllOpenRecentFileMenus();
	void synchronizeOpenRecentFileMenus();
	void showNodeSetDocumentationFromUrl(const QUrl &url);
	void showNodeDocumentationFromUrl(const QUrl &url);
	void openFileFromSenderData();
	void removeFileFromSenderData();
	void openExternalUrlFromSenderData();
	void openExampleCompositionFromUrl(const QUrl &url);
	void openMostRecentFile();
	void openRandomExample();

	void quitCleanly();
	void continueQuit(QMainWindow *window);
	void cancelQuit();

	Q_INVOKABLE void openHelpBook();

private slots:
	void openHelpBookPageFromUrl(const QUrl &url);
	void updateUI();
	void conformToGlobalNodeLibraryVisibility(VuoNodeLibrary::nodeLibraryState visibility, VuoNodeLibrary *floater);
	void showGridLinesToggled(bool show);
	void showGridPointsToggled(bool show);
	void updateSnapToGrid(bool snap);
	void updateColor(bool isDark);
	void updateCanvasOpacity(QAction *setOpacityAction);
	void updateGlobalMovieExportSettings(int width, int height, double time, double duration, double framerate, int spatialSupersample, int temporalSupersample, float shutterAngle, QString imageFormat, double quality);
	void updateNodeLibraryDisplayMode(bool humanReadable);
	void updateGlobalNodeLibraryFloatingPosition(QPoint newPos);
	void updateGlobalNodeLibraryWidth(int newWidth);
	void updateGlobalNodeLibraryHeight(int newHeight);
	void updateGlobalNodeDocumentationPanelHeight(int newSize);
	void updateGlobalNodeLibraryVisibilityState(bool visible);
	void updateGlobalNodeLibraryDockedState(bool floating);
	void updateFloatingNodeLibraryModules();
	void assignTopLevelLibraryAsReplacementFloater();

protected:
	bool event(QEvent * e) VuoWarnUnusedResult;

private:
	friend class TestVuoEditor;

	static const QString recentFileListSettingsKey;
	static const QString subcompositionPrefixSettingsKey;
	static const QString nodeLibraryDisplayModeSettingsKey;
	static const QString nodeLibraryVisibilityStateSettingsKey;
	static const QString nodeLibraryDockingStateSettingsKey;
	static const QString nodeLibraryFloatingPositionSettingsKey;
	static const QString nodeLibraryWidthSettingsKey;
	static const QString nodeLibraryHeightSettingsKey;
	static const QString nodeDocumentationPanelHeightSettingsKey;
	static const QString shaderDocumentationVisibilitySettingsKey;
	static const QString gridTypeSettingsKey;
	static const QString gridOpacitySettingsKey;
	static const qreal   defaultGridOpacity;
	static const QString snapToGridSettingsKey;
	static const QString darkInterfaceSettingsKey;
	static const QString canvasOpacitySettingsKey;
	static const QString movieExportWidthSettingsKey;
	static const QString movieExportHeightSettingsKey;
	static const QString movieExportTimeSettingsKey;
	static const QString movieExportDurationSettingsKey;
	static const QString movieExportFramerateSettingsKey;
	static const QString movieExportSpatialSupersampleSettingsKey;
	static const QString movieExportTemporalSupersampleSettingsKey;
	static const QString movieExportShutterAngleSettingsKey;
	static const QString movieExportImageFormatSettingsKey;
	static const QString movieExportQualitySettingsKey;

	static string documentationGenerationDirectory;

	QNetworkAccessManager *networkManager;
	VuoEditor::subscriptionLevel userSubscriptionLevel;
	VuoEditorAboutBox *aboutBox;
	QSettings* settings;
	VuoCompiler *compiler;
	VuoModuleManager *moduleManager;
	VuoSubcompositionMessageRouter *subcompositionRouter;
	map<VuoProtocol *, VuoCompilerDriver *> builtInDriverForProtocol;
	dispatch_queue_t builtInDriversQueue;
	bool checkForUpdateOnStartup;

	// Global node library settings
	VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode;
	bool nodeLibraryCurrentlyVisible;
	bool nodeLibraryCurrentlyDocked;
	bool settingsContainedNodeLibraryFloatingPosition;
	QPoint nodeLibraryFloatingPosition;
	int nodeLibraryWidth;
	int nodeLibraryHeight;
	int nodeDocumentationPanelHeight;
	bool previousVisibleNodeLibraryStateWasDocked;
	VuoNodeLibrary *currentFloatingNodeLibrary; // The current floating node library, which may or may not be the top-level (application-owned) library.

	// Global subcomposition settings
	QString subcompositionPrefix;

	// Global code window settings
	bool shaderDocumentationVisible;

	// Global movie export settings
	int movieExportWidth;
	int movieExportHeight;
	double movieExportTime;
	double movieExportDuration;
	double movieExportFramerate;
	int movieExportSpatialSupersample;
	int movieExportTemporalSupersample;
	float movieExportShutterAngle;
	QString movieExportImageFormat;
	double movieExportQuality;

	// Global view settings
	int canvasOpacity;

	QMenuBar *menuBar;
	QMenu *menuFile;
	QMenu *menuNewCompositionWithTemplate;
	VuoRecentFileMenu *menuOpenRecent;
	VuoExampleMenu *menuOpenExample;
	QMenu *menuWindow;
	QMenu *menuView;
	QMenu *menuHelp;
	QMenu *dockContextMenu;
	void enableMenuItems(QMenu *menu, bool enable);
	QString assignUntitledDocumentIdentifier(void);
	int yScreenSpaceShortage;
	VuoEditorWindow * createEditorWindow(QString filename="", bool existingComposition=false, VuoProtocol *activeProtocol=NULL);
	VuoEditorWindow * createEditorWindow(QString documentIdentifier, QString filename, const string &compositionAsString, VuoProtocol *activeProtocol=NULL, string nodeClassToHighlight="");
	string getFilterTextForTemplate(string templateID);
	void closeUnmodifiedUntitledComposition();
	void initializeBuiltInDrivers();
	void initializeTopLevelNodeLibrary(VuoCompiler *nodeLibraryCompiler,
									   VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode,
									   bool setFloatingPosition,
									   QPoint floatingPosition=QPoint(),
									   int nodeLibraryWidth=-1,
									   int nodeLibraryHeight=-1);
	VuoNodeLibrary::nodeLibraryState getGlobalNodeLibraryStateForAttributes(bool visible, bool docked);
	void updateGlobalNodeLibraryState(bool visible, bool docked);
	void designateNewFloatingNodeLibrary(VuoNodeLibrary *library);
	void enableGlobalStateConformanceToLibrary(VuoNodeLibrary *library);
	void disableGlobalStateConformanceToLibrary(VuoNodeLibrary *library);
	void updateDockedNodeLibraryModules(void);
	void setNewManagerForNodeLibrary(VuoNodeLibrary *library, VuoModuleManager *manager);
	void updateCanvasOpacityTo(int opacity);
	void applyStoredMovieExportSettings();
	void generateAllNodeSetHtmlDocumentation(string directory);
	void reportBrokenModules();
	void moveFileToTrash(QString filePath);
	dispatch_queue_t documentationQueue;
	VuoNodeLibrary *ownedNodeLibrary; // The top-level (application-owned) node library, which may at times function as the floating node library.
	QAction *showNodeLibraryAction;
	map<QString, VuoEditorWindow *> documentIdentifierAssigned;
	vector<string> closedFiles;
	map<string, string> resourceDirectoryForNodeSet;
	vector<QString> queuedCompositionsToOpen;
	bool uiInitialized;
	bool reportAbsenceOfUpdates;

	QString qtFindTranslation(const QLocale &locale, const QString &filename, const QString &prefix, const QString &directory, const QString &suffix);
	void loadTranslations();

	QList<QMainWindow *> windowsRemainingAfterQuitRequested;
	void reallyQuit();

#ifdef VUO_PRO
#include "pro/VuoEditor_Pro.hh"
#endif
};
