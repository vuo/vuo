/**
 * @file
 * VuoCodeWindow interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCodeEditorStages;
class VuoCodeGutter;
class VuoCodeIssueList;
class VuoCompiler;
class VuoCompilerIssues;
class VuoCompilerNodeClass;
class VuoCompilerPublishedPort;
class VuoDocumentationSidebar;
class VuoEditorComposition;
class VuoEditorWindowToolbar;
class VuoInputEditorManager;
class VuoInputEditorSession;
class VuoMetadataEditor;
class VuoPublishedPortSidebar;
class VuoRecentFileMenu;
class VuoRendererPort;
class VuoRendererPublishedPort;

#include "VuoMainWindow.hh"
#include "VuoShaderFile.hh"

/**
 * A window for editing text code.
 */
class VuoCodeWindow : public VuoMainWindow
{
	Q_OBJECT

public:
	explicit VuoCodeWindow(const string &sourcePath);
	~VuoCodeWindow(void);

	void setIssues(VuoCompilerIssues *compilerIssues);
	string getNodeClassName(void);

	void setAsActiveWindow();
	void updateReloadAction();
	QMenu * getFileMenu();
	VuoRecentFileMenu * getRecentFileMenu();
	void setIncludeInRecentFileMenu(bool include);

	QAction * getRaiseDocumentAction() const;
	QAction * getZoom11Action() const;
	QAction * getZoomInAction() const;
	QAction * getZoomOutAction() const;

	static void newShaderWithTemplate(QAction *sender);

	VuoCodeEditorStages *stages;  ///< The code editor widgets for this window.
	VuoCodeIssueList *issueList;  ///< The widget that renders this window's issues.
	VuoShaderIssues *issues;      ///< The issues for this window's shader.

public slots:
	void on_runComposition_triggered();
	void on_stopComposition_triggered();
	void on_restartComposition_triggered();

private:
	void populateMenus(void);
	void save();
	void saveAs();
	void saveToPath(QString savePath);
	bool isNewUnsavedDocument();
	void closeEvent(QCloseEvent *event);

	void relinquishSourcePath(void);
	void setSourcePath(const string &sourcePath);
	void updateWrapperComposition(void);
	void bringNodeClassInSyncWithSourceCode(void);
	void bringStoredShaderInSyncWithSourceCode(void);
	void bringStoredShaderInSyncWithPublishedInputPorts(VuoCompilerPublishedPort *publishedInputAdded = nullptr, const pair<string, string> &publishedInputRenamed = {}, const string &publishedInputRemoved = "");
	void bringStoredShaderInSyncWithCompositionMetadata(void);

	void updateModifiedIndicator();
	void updateColor();
	void updateCanvasOpacity();
	void updateToolbar();
	void resizeEvent(QResizeEvent *event);
	void updateWindowMenu();
	void toggleInputPortSidebarVisibility();
	void updateInputPortSidebarMenuItem();
	void toggleDocumentationSidebarVisibility();
	void updateDocumentationSidebarMenuItem();
	void showPublishedInputEditor(VuoRendererPort *port);
private slots:
	void showPublishedPortNameEditor(VuoRendererPublishedPort *port);
	void changePublishedPortDetails(VuoRendererPublishedPort *port, json_object *newDetails);
private:
	void changePublishedPortName(VuoRendererPublishedPort *port, string newName);
	void addPublishedPort(string typeName, bool isInput);
	void deletePublishedPort(VuoRendererPublishedPort *port);
	void changeMetadata(int dialogResult);

	void showBuildActivityIndicator();
	void hideBuildActivityIndicator(QString buildError);
	void showStopActivityIndicator();
	void hideStopActivityIndicator();

	void zoom11();
	void zoomIn();
	void zoomOut();

	void copy();

	VuoPublishedPortSidebar *inputPortSidebar;
	VuoDocumentationSidebar *documentationSidebar;
	VuoEditorWindowToolbar *toolbar;
	QAction *saveAction;
	QAction *toggleInputPortSidebarAction;
	QAction *toggleDocumentationSidebarAction;
	QAction *raiseDocumentAction;
	QAction *zoom11Action;
	QAction *zoomInAction;
	QAction *zoomOutAction;
	QAction *runAction;
	QAction *stopAction;
	QAction *restartAction;
	QAction *reloadAction;
	QMenu *fileMenu;
	QMenu *windowMenu;
	VuoRecentFileMenu *recentFileMenu;

	VuoEditorComposition *wrapperComposition;  ///< Wraps the node class in a composition. Run to preview the node class.
	VuoCompiler *compiler;
	VuoShaderFile *shaderFile;
	VuoInputEditorManager *inputEditorManager;
	VuoInputEditorSession *inputEditorSession;
	VuoMetadataEditor *metadataEditor;

	bool includeInRecentFileMenu;  ///< True if this file should be added to the "File > Open Recent" list when saved.
	bool publishedInputsModified;  ///< True if any published inputs have unsaved changes.
	bool metadataModified;  ///< True if the composition metadata has been edited and thus may have unsaved changes.
	bool closing;

	friend VuoCodeGutter;
	friend class TestVuoEditor;
};
