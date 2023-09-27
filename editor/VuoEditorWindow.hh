/**
 * @file
 * VuoEditorWindow interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoEditorComposition.hh"
#include "VuoMainWindow.hh"
#include "VuoNode.hh"
#include "VuoNodeLibrary.hh"
#include "VuoPortClass.hh"
class VuoCompiler;
class VuoCompositionMetadataPanel;
class VuoEditorWindowToolbar;
class VuoExampleMenu;
class VuoExportMovieDialog;
class VuoInputEditorManager;
class VuoInputEditorSession;
class VuoMetadataEditor;
class VuoPort;
class VuoProtocol;
class VuoPublishedPort;
class VuoPublishedPortSidebar;
class VuoRecentFileMenu;
class VuoRendererCable;
class VuoRendererComment;
class VuoRendererInputAttachment;
class VuoRendererNode;
class VuoRendererPort;
class VuoRendererPublishedPort;
class VuoSearchBox;

namespace Ui
{
	class VuoEditorWindow;
}

/**
 * Represents a window for editing a Vuo Composition.  One instance per composition.
 */
class VuoEditorWindow : public VuoMainWindow
{
	Q_OBJECT

public:
	explicit VuoEditorWindow(QString documentIdentifier="", QString compositionPath="",
							 const string &compositionAsString="",
							 VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode=VuoNodeLibrary::displayByClass,
							 VuoNodeLibrary::nodeLibraryState nodeLibraryState=VuoNodeLibrary::nodeLibraryHidden,
							 VuoNodeLibrary *floater=NULL,
							 VuoProtocol *activeProtocol=NULL,
							 string nodeClassToHighlight="");
	~VuoEditorWindow();

	static VuoEditorWindow * existingWindowWithNewFile();
	bool isScrollInProgress();
	bool isItemDragInProgress();
	double getLatestDragTime();
	void highlightNodeClass(string nodeClass);
	static bool containsLikelyVuoComposition(QString text);
	static VuoEditorWindow * getMostRecentActiveEditorWindow();
	static QString getNodeClassNameForDisplayNameAndCategory(QString compositionName, QString category, QString defaultCompositionName, QString defaultCategory);
	VuoNodeLibrary * getOwnedNodeLibrary();
	VuoNodeLibrary * getCurrentNodeLibrary();
	void assignSurrogateNodeLibrary(VuoNodeLibrary *library);
	void releaseSurrogateNodeLibrary(bool previousFloaterDestroyed);
	QMenu * getFileMenu();
	VuoRecentFileMenu * getRecentFileMenu();
	void setIncludeInRecentFileMenu(bool include);
	VuoEditorComposition * getComposition();
	QPointF getCursorScenePos();
	VuoCompositionMetadataPanel * getMetadataPanel();
	QPointF getFittedScenePos(QPointF origPos, int leftMargin=50, int topMargin=50, int rightMargin=175, int bottomMargin=75);
	void showPublishedPortSidebars();
	void resetCompositionWithSnapshot(string snapshot);
	void zoomOutToFit();

	static QString untitledComposition; ///< The display name to use for an untitled composition.
	static VuoEditorWindow *mostRecentActiveEditorWindow; ///< The most recent active @c VuoEditorWindow.

public slots:
	void updateLatestDragTime();
	void updateUI();
	void itemsMoved(set<VuoRendererNode *> nodes, set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging);
	void commentResized(VuoRendererComment *comment, qreal dx, qreal dy);
	void connectionCompleted(VuoRendererCable *cableInProgress, VuoRendererPort *targetPort, pair<VuoRendererCable *, VuoRendererCable *> cableArgs, VuoRendererNode *typecastNodeToDelete, pair<string, string> typeArgs, pair<VuoRendererPort *, VuoRendererPort *> portArgs);
	void setTriggerThrottling(VuoPort *triggerPort, enum VuoPortClass::EventThrottling eventThrottling);
	void adjustInputPortCountForNode(VuoRendererNode *node, int inputPortCountDelta, bool adjustmentRequestedByDragging);
	void swapNodes(VuoRendererNode *node, string newNodeClass);
	VuoRendererNode * specializePortNetwork(VuoRendererPort *port, string specializedTypeName);
	VuoRendererNode * specializePortNetwork(VuoRendererPort *port, string specializedTypeName, bool encapsulateInMacro);
	VuoRendererNode * unspecializePortNetwork(VuoRendererPort *port);
	VuoRendererNode * unspecializePortNetwork(VuoRendererPort *port, bool encapsulateInMacro);
	VuoRendererNode * respecializePortNetwork(VuoRendererPort *port, string specializedTypeName);
	VuoRendererNode * respecializePortNetwork(VuoRendererPort *port, string specializedTypeName, bool encapsulateInMacro);
	void tintSelectedItems(VuoNode::TintColor tintColor);
	void internalExternalPortPairPublished(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoPort *portToSpecialize=NULL, string specializedTypeName="", string typecastToInsert="", bool useUndoStackMacro=true);
	void internalPortPublishedViaDropBox(VuoPort *port, bool forceEventOnlyPublication, bool useUndoStackMacro);
	void internalPortPublished(VuoPort *port, bool forceEventOnlyPublication, string name="", bool merge=false, VuoPort *portToSpecialize=NULL, string specializedTypeName="", string typecastToInsert="", bool useUndoStackMacro=true);
	void externalPortUnpublished(VuoRendererPublishedPort *port);
	void internalPortUnpublished(VuoPort *port);
	void makeProtocolPortChanges(map<VuoPublishedPort *, string> publishedPortsToRename, set<VuoPublishedPort *> publishedPortsToRemove, vector<VuoPublishedPort *> publishedPortsToAdd, bool beginUndoStackMacro, bool endUndoStackMacro);
	void componentsRemoved(QList<QGraphicsItem *> components, string commandDescription);
	void componentsAdded(QList<QGraphicsItem *> components, VuoEditorComposition *target);
	void cutSelectedCompositionComponents();
	void copySelectedCompositionComponents();
	void disambiguatePasteRequest();
	void duplicateSelectedCompositionComponentsByDrag();
	void cleanUpCancelledDuplication();
	void conformToGlobalNodeLibraryVisibility(VuoNodeLibrary::nodeLibraryState visibility, VuoNodeLibrary *floater, bool previousFloaterDestroyed);
	void displayAppropriateDocumentation();
	QAction *getRaiseDocumentAction() const;
	QAction *getShowEventsAction() const;
	QAction *getZoomOutAction() const;
	QAction *getZoom11Action() const;
	QAction *getZoomToFitAction() const;
	QAction *getZoomInAction() const;
	void on_zoomToFit_triggered();
	void setAsActiveWindow();
	void on_runComposition_triggered();
	void on_stopComposition_triggered();
	void on_restartComposition_triggered();
	void on_refireEvent_triggered();
	void on_showNodeLibrary_triggered();
	void on_selectNone_triggered();
	void on_find_triggered();
	void on_findNext_triggered();
	void on_findPrevious_triggered();
	void coalesceSnapshots(string oldCompositionSnapshot, string newCompositionSnapshot, VuoCompilerCompositionDiff *diffInfo=nullptr);
	void coalesceInternalPortConstantsToSync(string portID);
	void coalescePublishedPortConstantsToSync(string portID);
	void coalesceNodesToUnlink(string nodeID);
	void coalesceNodesToRelink(string nodeID);
	void beginUndoStackMacro(QString commandName);
	void endUndoStackMacro();
	static string deriveBaseNodeClassNameFromDisplayName(string displayName);

signals:
	void windowActivated();  ///< Emitted when the window has been activated.
	void windowDeactivated();  ///< Emitted when the window has been de-activated.

protected:
	bool event(QEvent *event) VuoWarnUnusedResult;
	bool eventFilter(QObject *object, QEvent *event) VuoWarnUnusedResult;
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void closeEvent(QCloseEvent *event);
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);

private slots:
	void showUpdateHelpDialog();

	void toggleNodeLibraryDockedState();
	void on_zoom11_triggered();
	void on_zoomIn_triggered();
	void on_zoomOut_triggered();
	void viewportFitReset();

	void on_saveComposition_triggered();
	bool saveFile(const QString &file);
	void changeActiveProtocol(void);
	void updateActiveProtocolDisplay(void);
	void evaluateCompositionForProtocolPromotion(void);
	void on_saveCompositionAs_triggered();
	void on_insertComment_triggered();
	void on_insertSubcomposition_triggered();
	string on_installSubcomposition_triggered();
	void on_compositionInformation_triggered();
	void editMetadata(int dialogResult);
	void undoStackCleanStateChanged(bool clean);

	void duplicateSelectedCompositionComponentsByMenuItem();

	void showBuildActivityIndicator();
	void hideBuildActivityIndicator(QString buildError);
	void hideStopActivityIndicator();

	void on_openUserModulesFolder_triggered();
	void on_openSystemModulesFolder_triggered();
	void on_showConsole_triggered();

	void on_showPublishedPorts_triggered();
	void on_showHiddenCables_triggered();
	void on_showEvents_triggered();
	void closePublishedPortSidebars();
	void conditionallyShowPublishedPortSidebars(bool visible);
	void updatePublishedCableGeometry();
	void updatePublishedPortOrder(vector<VuoPublishedPort *> ports, bool isInput);
	void updateWindowForNewCableDrag();
	void displayDockedNodeLibrary();

	void resetUndoStackMacros();
	void updateSelectedComponentMenuItems();
	void updateChangeNodeMenu();
	void updateSceneRect();
	void ensureSceneRectContainsRegion(const QList<QRectF> &region);
	void updateRubberBandSelectionMode(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint);

	void setPortConstant(VuoRendererPort *port, string value);
	void showInputEditor(VuoRendererPort *port);
	void showInputEditor(VuoRendererPort *port, bool forwardTabTraversal);
	void showNodeTitleEditor(VuoRendererNode *node);
	void showCommentEditor(VuoRendererComment *comment);
	void showPublishedPortNameEditor(VuoRendererPublishedPort *port, bool useUndoStack);
	void zoomToFitComment(VuoRendererComment *comment);
	void createIsolatedExternalPublishedPort(string typeName, bool isInput);
	void changePublishedPortName(VuoRendererPublishedPort *port, string newName, bool useUndoStack);
	void changePublishedPortDetails(VuoRendererPublishedPort *port, json_object *newDetails);
	void openEditableSourceForNode(VuoRendererNode *node);
	void insertCommentAtPos(QPointF targetScenePos);
	void insertSubcompositionAtPos(QPointF targetScenePos);
	void refactorSelectedItems();
	void restoreDefaultLeftDockedWidgetWidths();
	void hideSelectedInternalCables();
	void hideCables(set<VuoRendererCable *> cables);
	void unhideCables(set<VuoRendererCable *> cables);
	void updateGrid();
	void updateCanvasOpacity();
	void updateColor(bool isDark);
	void coalescedUpdateRunningComposition();
	void handlePendingProtocolComplianceReevaluationRequests();

private:
	friend class TestVuoEditor;
	friend class TestEditorCommands;

	static const qreal viewportStepRate;
	static const qreal viewportStepRateMultiplier;
	static const qreal zoomRate;
	static const qreal pastedComponentOffset;
	static const qreal compositionMargin;
	VuoCompiler *compiler;

	// The node library owned by this window, which will be the library docked into this window
	// when in docked-library mode, and which may also at times function as the floating node library.
	VuoNodeLibrary *ownedNodeLibrary;

	// The node library currently active in association with this window, which may be the library owned
	// by this window or a floating library associated with a different window or the top-level application.
	VuoNodeLibrary *nl;

	string coalescedOldCompositionSnapshot;
	string coalescedNewCompositionSnapshot;
	vector<string> coalescedInternalPortConstantsToSync;
	vector<string> coalescedPublishedPortConstantsToSync;
	vector<string> coalescedNodesToUnlink;
	vector<string> coalescedNodesToRelink;
	VuoCompilerCompositionDiff *coalescedDiffInfo;

	VuoCompositionMetadataPanel *metadataPanel;
	VuoPublishedPortSidebar *inputPortSidebar;
	VuoPublishedPortSidebar *outputPortSidebar;
	VuoInputEditorManager *inputEditorManager;
	VuoInputEditorSession *inputEditorSession;
	QMenu *menuNewCompositionWithTemplate;
	QMenu *menuProtocols;
	VuoRecentFileMenu *menuOpenRecent;
	VuoExampleMenu *menuOpenExample;
	QMenu *contextMenuTints;
	QMenu *menuChangeNode;
	QAction *raiseDocumentAction;
	QAction *undoAction;
	QAction *redoAction;
	QAction *quitAction;
	QUndoStack *undoStack;
	QUndoView *undoView;
	VuoMetadataEditor *metadataEditor;
	VuoSearchBox *searchBox;
	bool zoomOutToFitOnNextShowEvent;
	bool compositionUpgradedSinceLastSave;
	bool canvasDragEnabled;
	bool canvasDragInProgress;
	bool scrollInProgress;
	double timeOfLastScroll;
	bool consumeNextMouseReleaseToCanvas;
	bool lastLeftMousePressHadOptionModifier;
	bool rubberBandSelectionInProgress;
	QPointF cursorPosAtLastComponentCopy;
	QPoint canvasDragMinCursorPos;
	QPoint canvasDragMaxCursorPos;
	QPoint lastCursorLocationDuringCanvasDrag;
	bool previousDragMoveWasOverSidebar;
	bool itemDragMacroInProgress;
	vector<QGraphicsItem *> itemsBeingDragged;
	VuoRendererComment *commentBeingResized;
	int itemDragDx;
	int itemDragDy;
	double latestDragTime;
	bool commentResizeMacroInProgress;
	int commentResizeDx;
	int commentResizeDy;
	bool duplicationMacroInProgress;
	VuoEditorWindowToolbar *toolbar;
	bool isZoomedToFit;
	bool doneInitializing;
	bool containedPrepopulatedContent;
	bool includeInRecentFileMenu;
	bool publishedPortNearCursorPreviously;

	bool forwardingEventsToCanvas;

	Ui::VuoEditorWindow *ui;
	VuoEditorComposition *composition;

	int nodeLibraryMinimumWidth;
	int nodeLibraryMaximumWidth;

	bool protocolComplianceReevaluationPending;
	bool ignoreItemMoveSignals;
	bool closing;

	bool saveFile(const QString & savePath, QString & existingPath);
	bool saveFileAs(const QString & savePath);
	QString saveCompositionAs();
	void acceptCloseEvent();
	string installSubcomposition(string parentCompositionPath);
	bool ensureThisParentCompositionSaved();
	string sanitizeSubcompositionName(string name);
	void updateToolbarElementUI();
	void populateProtocolsMenu(QMenu *m);
	void updateProtocolsMenu(QMenu *m);
	void toggleActiveStatusForProtocol(VuoProtocol *protocol);
	void setPublishedPortSidebarVisibility(bool visible);
	void registerProtocolComplianceEvaluationRequest();
	void setCompositionModified(bool modified);
	void initializeNodeLibrary(VuoCompiler *nodeLibraryCompiler, VuoNodeLibrary::nodeLibraryDisplayMode nodeLibraryDisplayMode, VuoNodeLibrary::nodeLibraryState nodeLibraryState, VuoNodeLibrary *floater=NULL);
	void pasteCompositionComponents();
	void componentsPasted(QList<QGraphicsItem *> components, string commandDescription="Paste");
	string getMaximumSubcompositionFromSelection(bool includePublishedPorts, bool includeHeader);
	bool isStrandedAttachment(VuoRendererInputAttachment *attachment, QList<QGraphicsItem *> selectedItems);
	void instantiateNewCompositionComponentsFromString(string compositionText);
	QList<QGraphicsItem *> mergeCompositionComponentsFromString(string compositionText, bool pasteAtCursorLoc, bool pasteWithOffset, string commandDescription="Paste");
	void transitionNodeLibraryConnections(VuoNodeLibrary *oldLibrary, VuoNodeLibrary *newLibrary);
	QList<QGraphicsItem *> createAnyNecessaryMakeListComponents(VuoPort *port);
	VuoRendererNode * specializeSinglePort(VuoRendererPort *port, string specializedTypeName);
	VuoRendererNode * unspecializeSingleNode(VuoRendererNode *node, string revertedNodeClassName);
	void enableCanvasDrag();
	void disableCanvasDrag();
	void initiateCanvasDrag();
	void concludeCanvasDrag();
	void updateRefireAction();
	void updateCursor();
	string generateCurrentDefaultCopyright();
	bool confirmReplacingFile(string path);
	bool arePublishedPortSidebarsVisible();

#ifdef VUO_PRO
#include "pro/VuoEditorWindow_Pro.hh"
#endif
};
