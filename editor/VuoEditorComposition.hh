/**
 * @file
 * VuoEditorComposition interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerCompositionDiff.hh"
#include "VuoRendererComposition.hh"
#include "VuoRunner.hh"
#ifdef VUO_PRO
#include "VuoMovieExporter.hh"
#endif

#include <dispatch/dispatch.h>

class VuoCompilerInputEventPort;
class VuoCompilerPort;
class VuoEditorWindow;
class VuoErrorMark;
class VuoErrorPopover;
class VuoInputEditorManager;
class VuoMainWindow;
class VuoModuleManager;
class VuoNodeAndPortIdentifierCache;
class VuoPortPopover;
class VuoProtocol;

/**
 * Handles drags-and-drops and performs node instantiation for the editor window.
 */
class VuoEditorComposition : public VuoRendererComposition, public VuoRunnerDelegateAdapter
{
	Q_OBJECT

public:
	/**
	 * Specifies the desired type of composition component to be returned from a search.
	 */
	enum targetComponentType
	{
		targetTypePort,
		targetTypeNodeHeader,
		targetTypeAny
	};

	static const qreal componentCollisionRange;  ///<  The search range used in locating composition components near the cursor.

	explicit VuoEditorComposition(VuoMainWindow *window, VuoComposition *baseComposition);
	~VuoEditorComposition();
	void setCompiler(VuoCompiler *compiler);
	VuoCompiler *getCompiler();
	void setModuleManager(VuoModuleManager *moduleManager);
	VuoModuleManager * getModuleManager(void);
	void setColor(bool isDark);
	void setInputEditorManager(VuoInputEditorManager *inputEditorManager);
	VuoInputEditorManager * getInputEditorManager();
	VuoRendererNode * createNode(QString nodeClassName, string title="", double x=0, double y=0);
	VuoNode * createBaseNode(VuoCompilerNodeClass *nodeClass, VuoNode *modelNode, string title="", double x=0, double y=0);
	VuoNode * createNodeWithMissingImplementation(VuoNodeClass *modelNodeClass, VuoNode *modelNode, string title="", double x=0, double y=0);
	void updateNodeImplementationInPlace(VuoRendererNode *oldNode, VuoNode *newNode);
	void addNode(VuoNode *node, bool nodeShouldBeRendered=true, bool nodeShouldBeGivenUniqueIdentifier=true);
	void removeNode(VuoRendererNode *rn, bool resetState=true);
	void replaceNode(VuoRendererNode *oldNode, VuoNode *newNode);
	void addCable(VuoCable *cable, bool emitHiddenCableNotification=true);
	void removeCable(VuoRendererCable *rc, bool emitHiddenCableNotification=true);
	VuoRendererNode * createAndConnectMakeListNode(VuoNode *toNode, VuoPort *toPort, VuoRendererCable *&rendererCable);
	void createAndConnectDictionaryAttachmentsForNode(VuoNode *node, set<VuoRendererNode *> &createdRendererNodes, set<VuoRendererCable *> &createdRendererCables);
	QList<QGraphicsItem *>  createAndConnectInputAttachments(VuoRendererNode *node, bool createButDoNotAdd=false);
	void modifyComponents(void (^modify)(void));
	bool requiresStructuralChangesAfterValueChangeAtPort(VuoRendererPort *port);
	QAction * getContextMenuDeleteSelectedAction(void);
	QMenu * getContextMenuTints(QMenu *parent = 0);
	void populateChangeNodeMenu(QMenu *menu, VuoRendererNode *node, int matchLimit=0);
	set<VuoRendererCable *> getCablesInternalToSubcomposition(QList<QGraphicsItem *> subcompositionComponents);
	VuoCable * getCableInProgress();
	bool getCableInProgressWasNew();
	bool getMenuSelectionInProgress();
	VuoRendererNode * findNearbyNodeHeader(QPointF scenePos);
	QGraphicsItem * findNearbyComponent(QPointF scenePos, targetComponentType componentType=VuoEditorComposition::targetTypeAny, bool limitPortCollisionRange=true);
	VuoRendererPort * findTargetPortForCableDroppedOnNodeHeader(VuoRendererNode *node);
	VuoRendererPort * findDefaultPortForEventOnlyConnection(VuoRendererNode *node, bool inputPort);
	QRectF internalItemsBoundingRect() const;
	QRectF internalSelectedItemsBoundingRect() const;
	QRectF internalSelectedItemsChildrenBoundingRect() const;
	set<VuoRendererNode *> getSelectedNodes();
	set<VuoRendererComment *> getSelectedComments();
	set<VuoRendererCable *> getSelectedCables(bool includePublishedCables);
	void cancelCableDrag(void);
	void revertCableDrag(void);
	void clearCableEndpointEligibilityHighlighting();
	void clearHoverHighlighting();
	void repaintFeedbackErrorMarks();
	void setIgnoreApplicationStateChangeEvents(bool ignore);
	json_object * getPortValueInRunningComposition(VuoPort *port);
	static string getIdentifierForStaticPort(VuoPort *staticPort, VuoNode *parentNode=NULL);
	VuoPort * getPortWithStaticIdentifier(string portID);
	void updateInternalPortConstant(string portID, string newValue, bool updateInRunningComposition);
	void updatePublishedPortConstant(string portName, string newValue, bool updateInRunningComposition);
	void updatePortConstant(VuoCompilerPort *port, string newValue, bool updateInRunningComposition=true);
	void updateGenericPortTypes(void);
	void createReplacementsToUnspecializePort(VuoPort *port, bool shouldOutputNodesToReplace, map<VuoNode *, string> &nodesToReplace, set<VuoCable *> &cablesToDelete);
	void run(string compositionSnapshot);
	void stop(void);
	bool isRunning(void);
	void updateRunningComposition(string oldCompositionSnapshot, string newCompositionSnapshot, VuoCompilerCompositionDiff *diffInfo = nullptr, set<string> dependenciesUninstalled = set<string>());
	void updateCompositionsThatContainThisSubcomposition(string newCompositionSnapshot);
	void syncInternalPortConstantInRunningComposition(string runningPortID);
	void syncPublishedPortConstantInRunningComposition(string portName);
	void updateInternalPortConstantInRunningComposition(VuoCompilerInputEventPort *port, string constant);
	void updateInternalPortConstantInSubcompositionInstances(string subcompositionPath, string portIdentifier, string constant);
	void updatePublishedInputPortConstantInRunningComposition(VuoPublishedPort *port, string constant);
	VuoRendererPublishedPort * publishInternalPort(VuoPort *port, bool forceEventOnlyPublication, string name="", VuoType *type=NULL, bool attemptMerge=false, bool *mergePerformed=NULL);
	VuoCable * createPublishedCable(VuoPort *externalPort, VuoPort *internalPort, bool forceEventOnlyPublication);
	void addActiveProtocol(VuoProtocol *protocol, bool useUndoStack);
	bool removeActiveProtocol(VuoProtocol *protocol, VuoProtocol *replacementProtocol);
	void removeActiveProtocolWithoutModifyingPorts(VuoProtocol *protocol);
	map<string, string> publishPorts(set<string> portsToPublish);
	string generateSpecialPublishedNameForPort(VuoPort *port);
	VuoProtocol * getActiveProtocol();
	VuoCompilerDriver * getDriverForActiveProtocol();
	bool validateProtocol(VuoEditorWindow *window, bool isExportingMovie);
	void addPublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, bool shouldUpdateUi=true);
	int removePublishedPort(VuoPublishedPort *publishedPort, bool isPublishedInput, bool shouldUpdateUi=true);
	void setPublishedPortName(VuoRendererPublishedPort *publishedPort, string name);
	void leftMousePressEventAtNearbyItem(QGraphicsItem *nearbyItem, QGraphicsSceneMouseEvent *event);
	static VuoNode * getUnderlyingParentNodeForPort(VuoPort *runningPort, VuoEditorComposition *composition);
	QGraphicsItem * findNearbyPort(QPointF scenePos, bool limitPortCollisionRange=true);
	VuoRendererColors::HighlightType getEligibilityHighlightingForPort(VuoRendererPort *portToHighlight, VuoRendererPort *fixedPort, bool eventOnlyConnection, map<string, VuoCompilerType *> &types);
	vector<string> findBridgingSolutions(VuoRendererPort *fromPort, VuoRendererPort *toPort, bool toPortIsDragDestination, map<string, VuoCompilerType *> &types);
	bool selectBridgingSolution(VuoRendererPort *fromPort, VuoRendererPort *toPort, bool toPortIsDragDestination, VuoRendererPort **portToSpecialize, string &specializedTypeName, string &typecastToInsert);
	QList<QAction *> getCompatibleTypesForMenu(VuoRendererPort *genericPort, set<string> compatibleTypesInIsolation, set<string> compatibleTypesInContext, bool limitToNodeSet, string nodeSetName="", QMenu *menu=NULL);
	void addTypeActionsToMenu(QList<QAction *> actionList, QMenu *menu);
	vector<string> getAllSpecializedTypeOptions(bool lists);
	void deleteSelectedNodes(string commandDescription="");
	void clear();

	// Popovers
	VuoPortPopover * getActivePopoverForPort(string portID);
	void enablePopoverForPort(VuoRendererPort *rp);
	void updatePortPopovers(VuoRendererNode *node=NULL);
	void updateDataInPortPopoverFromRunningTopLevelComposition(VuoEditorComposition *popoverComposition, string popoverCompositionIdentifier, string portID);
	void updateDataInPortPopover(string portID);
	void movePopoversBy(int dx, int dy);
	void disableNondetachedPortPopovers(VuoRendererNode *node=NULL, bool recordWhichPopoversClosed=false);
	void disableStrandedPortPopovers();
	void disableErrorPopovers();
	void disablePopovers();
	void emitCompositionOnTop(bool top);
	void emitPublishedPortNameEditorRequested(VuoRendererPublishedPort *port);

	void receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary);
	void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary);
	void receivedTelemetryEventDropped(string compositionIdentifier, string portIdentifier);
	void receivedTelemetryNodeExecutionStarted(string compositionIdentifier, string nodeIdentifier);
	void receivedTelemetryNodeExecutionFinished(string compositionIdentifier, string nodeIdentifier);
	void lostContactWithComposition(void);

	bool getShowEventsMode();
	void setShowEventsMode(bool showEventsMode);
	bool hasHiddenInternalCables();
	bool hasHiddenPublishedCables();
	void setTriggerPortToRefire(VuoPort *port);
	VuoPort * getTriggerPortToRefire();
	void refireTriggerPortEvent();

	string takeSnapshot(void);
	string generateCompositionHeader();

	static string getDefaultNameForPath(const string &compositionPath);
	QString getFormattedName();
	static QString formatCompositionFileNameForDisplay(QString unformattedCompositionFileName);
	static QString formatNodeSetNameForDisplay(QString nodeSetName);
	QString formatTypeNameForDisplay(VuoType *type);
	string getDefaultPublishedPortNameForType(VuoType *type);

	void beginDisplayingActivity(bool includePorts=true);
	void stopDisplayingActivity();

#ifdef VUO_PRO
#include "pro/VuoEditorComposition_Pro.hh"
#endif

public slots:
	void updateFeedbackErrors(VuoRendererPort *targetPort=NULL);
	void renameSelectedNodes();
	void deleteSelectedCompositionComponents();
	void deleteSelectedCompositionComponents(string commandDescription);
	void togglePortPublicationStatus();
	void tintSelectedItems(int tintColor);
	void selectAllCompositionComponents();
	void selectAllComments();
	void deselectAllCompositionComponents();
	void moveSelectedItemsBy(qreal dx, qreal dy);
	void updateGeometryForAllComponents();
	void emitNodeSourceEditorRequested();

	// Popovers
	void setPopoverEventsEnabled(bool enable);
	void disablePopoverForPort(string);

signals:
	void leftMouseButtonReleased(void); ///< Emitted when the left mouse button is released.
	void emptyCanvasLocationLeftClicked(); ///< Emitted when the left mouse button is released on an empty region of the canvas.
	void itemsMoved(set<VuoRendererNode *> nodes, set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging); ///< Emitted in order to move nodes and comments.
	void commentResized(VuoRendererComment *comment, qreal dx, qreal dy);  ///< Emitted when a comment is resized.
	void selectedComponentsDuplicated(); ///< Emitted in order to duplicate components.
	void componentsAdded(QList<QGraphicsItem *> addedComponents, VuoEditorComposition *target); ///< Emitted in order to add components.
	void componentsRemoved(QList<QGraphicsItem *> removedComponents, string commandDescription="Remove"); ///< Emitted in order to remove components.
	void cablesHidden(set<VuoRendererCable *> cables); ///< Emitted when cables are to be hidden.
	void selectedInternalCablesHidden();  ///< Emitted when selected internal cables are to be hidden.
	void cablesUnhidden(set<VuoRendererCable *> cables); ///< Emitted when cables are to be unhidden.
	void changeInHiddenCables(); ///< Emitted when a hidden cable has been added to or removed from the composition.
	void portPublicationRequested(VuoPort *port, bool forceEventOnlyPublication);  ///< Emitted when a port is to be published.
	void portPublicationRequested(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoPort *portToSpecialize, string specializedTypeName, string typecastToInsert, bool useUndoStackMacro);  ///< Emitted when an internal port is to be published in association with a specific external published port.
	void portUnpublicationRequested(VuoPort *port);  ///< Emitted when a port is to be unpublished.
	void publishedPortNameEditorRequested(VuoRendererPublishedPort *port, bool useUndoStack);  ///< Emitted when a published port name editor should be displayed.
	void publishedPortModified();  ///< Emitted when the composition has had published ports added, removed, or re-named.
	void protocolPortChangesRequested(map<VuoPublishedPort *, string> publishedPortsToRename, set<VuoPublishedPort *> publishedPortsToRemove, vector<VuoPublishedPort *> publishedPortsToAdd, bool beginUndoStackMacro, bool endUndoStackMacro);  ///< Emitted when published ports need to be modified for compatibility with a newly activated protocol.
	void activeProtocolChanged();  ///< Emitted when the active protocol has changed.
	void triggerThrottlingUpdated(VuoPort *port, enum VuoPortClass::EventThrottling eventThrottling);  ///< Emitted when a trigger port's event-throttling mode is to be updated.
	void inputPortCountAdjustmentRequested(VuoRendererNode *node, int inputPortCountDelta, bool requestedByDragging);  ///< Emitted when a node is to have its port count adjusted.
	void commentEditorRequested(VuoRendererComment *comment);  ///< Emitted in order to display a comment editor.
	void commentZoomRequested(VuoRendererComment *comment);  ///< Emitted in order to zoom in on a comment.
	void nodeSwapRequested(VuoRendererNode *node, string newNodeClassName);   ///< Emitted when a node is to be replaced with a new node of a similar class.
	void specializePort(VuoRendererPort *port, string specializedTypeName);  ///< Emitted when a generic port is to be specialized.
	void respecializePort(VuoRendererPort *port, string specializedTypeName);  ///< Emitted when a specialized port is to be re-specialized.
	void unspecializePort(VuoRendererPort *port);  ///< Emitted when a specialized port is to be reverted to its generic origins.
	void highlightPublishedSidebarDropLocationsRequested(VuoRendererPort *port, bool eventOnlyConnection); ///< Emitted when eligible published port sidebar drop locations for @c port should be visually highlighted.
	void clearPublishedSidebarDropLocationHighlightingRequested(); ///< Emitted when eligible published port sidebar drop locations should no longer be highlighted.
	void connectionCompletedByDragging(VuoRendererCable *cableInProgress, VuoRendererPort *targetPort, pair<VuoRendererCable *, VuoRendererCable *> cableArgs, VuoRendererNode *typecastNodeToDelete, pair<string, string> typeArgs, pair<VuoRendererPort *, VuoRendererPort *> portArgs); ///< Emitted after a cable has been connected to a node.
	void duplicationOperationCancelled(); ///< Emitted in order to cancel duplication of components.
	void inputEditorRequested(VuoRendererPort *port); ///< Emitted in order to display an input editor.
	void portConstantChangeRequested(VuoRendererPort *port, string value); ///< Emitted in order to change the value of a port constant.
	void nodeTitleEditorRequested(VuoRendererNode *node); ///< Emitted in order to display a node title editor.
	void nodeSourceEditorRequested(VuoRendererNode *node); ///< Emitted in order to open an editable node's source file.
	void subcompositionInsertionRequested(QPointF scenePos); ///< Emitted in order to insert a new subcomposition node at the provided scene position.
	void commentInsertionRequested(QPointF scenePos); ///< Emitted in order to insert a new comment at the provided scene position.
	void tintSelectedItemsRequested(VuoNode::TintColor tintColor); ///< Emitted when the user requests that the selected items be tinted.
	void compositionStoppedItself(); ///< Emitted when the user quits the composition or the composition crashes.
	void buildStarted(); ///< Emitted when starting to compile and link the composition.
	void buildFinished(QString error); ///< Emitted when finished attempting to compile and link the composition to run within the editor. If the build failed, the argument is an error message.
	void stopFinished(); ///< Emitted when finished stopping the composition.
	void nodePopoverRequestedForClass(VuoNodeClass *nodeClass); ///< Emitted when a node popover is to be displayed.
	void popoverDetached(); ///< Emitted when one of this composition's popovers has just been detached.
	void compositionOnTop(bool); ///< Emitted when the composition detects that it is or is no longer the topmost composition.
	void applicationActive(bool); ///< Emitted (if this composition is topmost) when the application has been activated or deactivated.
	void refirePortChanged(); ///< Emitted when the trigger port to be re-fired changes.
	void cableDragInitiated(); ///< Emitted when the user begins dragging a cable.
	void cableDragEnded();///< Emitted when the user ends a cable drag, whether a connection was completed or not.
	void undoStackMacroBeginRequested(QString commandName); ///< Emitted when the upcoming sequence of requested operations should be coalesced in an Undo stack macro.
	void undoStackMacroEndRequested(); ///< Emitted when the sequence of operations to be coalesced into an Undo stack macro has completed.
	void refactorRequested(); ///< Emitted when the selected nodes should be refactored into a subcomposition.

protected:
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
	void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);

	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

private:
	void setPortConstantToValue(VuoRendererPort *port, string value);
	void setCustomConstantsForNewNode(VuoRendererNode *newNode);
	void openSelectedEditableNodes();
	string getNonProtocolVariantForPortName(string portName);
	set<string> getRespecializationOptionsForPortInNetwork(VuoRendererPort *port);
	bool canConnectDirectlyWithRespecializationNondestructively(VuoRendererPort *fromPort,
																VuoRendererPort *toPort,
																bool eventOnlyConnection,
																bool forwardConnection);
	bool canConnectDirectlyWithRespecializationNondestructively(VuoRendererPort *fromPort,
																VuoRendererPort *toPort,
																bool eventOnlyConnection,
																bool forwardConnection,
																VuoRendererPort **portToRespecialize,
																string &respecializedTypeName);
	bool canConnectDirectlyWithRespecialization(VuoRendererPort *fromPort,
												VuoRendererPort *toPort,
												bool eventOnlyConnection,
												bool forwardConnection,
												VuoRendererPort **portToRespecialize,
												string &respecializedTypeName);
	bool canSwapWithoutBreakingCables(VuoRendererNode *origNode, VuoNodeClass *newNodeClass);
	bool portCanBeUnspecializedNondestructively(VuoPort *port);
	bool portsPassSanityCheckToBridge(VuoRendererPort *fromPort, VuoRendererPort *toPort);
	bool portsPassSanityCheckToTypeconvert(VuoRendererPort *fromPort, VuoRendererPort *toPort, VuoType *candidateFromType=NULL, VuoType *candidateToType=NULL);

	void fireTriggerPortEvent(VuoPort *port);
	QList<QAction *> promoteSingletonsFromSubmenus(QList<QAction *> actionList);
	static bool nodeSetMenuActionLessThan(QAction *action1, QAction *action2);
	static bool itemHigherOnCanvas(QGraphicsItem *item1, QGraphicsItem *item2);
	static double calculateNodeSimilarity(VuoNodeClass *node1, VuoNodeClass *node2);

	// Popovers
	void enableInactivePopoverForPort(VuoRendererPort *rp);
	void disablePopoverForPortThreadSafe(string portID);
	void disablePortPopovers(VuoRendererNode *node=NULL);
	void moveDetachedPortPopoversBy(int dx, int dy);
	void moveErrorPopoversBy(int dx, int dy);
	VuoErrorMark *errorMark;
	set<VuoErrorPopover *> errorPopovers;
	bool errorMarkingUpdatesEnabled;

private slots:
	void moveNodesBy(set<VuoRendererNode *> nodes, qreal dx, qreal dy, bool movedByDragging);
	void moveCommentsBy(set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging);
	void resizeCommentBy(VuoRendererComment *comment, qreal dx, qreal dy);
	void editSelectedComments();
	void insertNode();
	void insertComment();
	void insertSubcomposition();
	void deleteConnectedCables();
	void hideConnectedCables();
	void unhideConnectedCables();
	void fireTriggerPortEvent();
	void setPortConstant();
	void setTriggerThrottling(int eventThrottling);
	void specializeGenericPortType();
	void unspecializePortType();
	void addInputPort();
	void removeInputPort();
	void swapNode();
	void setDisableDragStickiness(bool disable);
	void updatePortAnimation(qreal value);
	void endPortAnimation(void);
	void expandChangeNodeMenu();

	// Popovers
	void updatePopoversForActiveWindowChange(QWidget *old, QWidget *now);
	void updatePopoversForApplicationStateChange(bool active);
	void enablePopoverForNode(VuoRendererNode *rn);
	void repositionPopover();

private:
	static const qreal nodeMoveRate;
	static const qreal nodeMoveRateMultiplier;
	static const qreal showEventsModeUpdateInterval;
	static const int initialChangeNodeSuggestionCount;
	VuoMainWindow *window;
	VuoCompiler *compiler;
	VuoModuleManager *moduleManager;
	VuoInputEditorManager *inputEditorManager;
	VuoProtocol *activeProtocol;
	VuoRunner *runner;
	VuoCompilerComposition *runningComposition;
	VuoCompilerDriver *runningCompositionActiveDriver;
	dispatch_queue_t runCompositionQueue;  ///< VuoSubcompositionRouter functions should not be called from this queue.
	dispatch_queue_t activePortPopoversQueue;  ///< @ref runCompositionQueue and VuoSubcompositionRouter functions should not be called from this queue.
	bool stopRequested;
	string linkedCompositionPath;
	VuoRunningCompositionLibraries *runningCompositionLibraries;
	QAction *contextMenuDeleteSelected;
	QAction *contextMenuHideSelectedCables;
	QAction *contextMenuRenameSelected;
	QAction *contextMenuRefactorSelected;
	QAction *contextMenuPublishPort;
	QAction *contextMenuDeleteCables;
	QAction *contextMenuHideCables;
	QAction *contextMenuUnhideCables;
	QAction *contextMenuFireEvent;
	QAction *contextMenuAddInputPort;
	QAction *contextMenuRemoveInputPort;
	QAction *contextMenuSetPortConstant;
	QAction *contextMenuEditSelectedComments;
	QMenu *contextMenuChangeNode;
	QList<QAction *> contextMenuThrottlingActions;
	QList<QAction *> contextMenuTintActions;
	bool duplicateOnNextMouseMove;
	bool duplicationDragInProgress;
	bool duplicationCancelled;
	QPointF cursorPosBeforeDuplicationDragMove;
	VuoCable *cableInProgress;
	bool cableInProgressWasNew;
	bool cableInProgressShouldBeWireless;
	VuoRendererPort *portWithDragInitiated;
	VuoRendererCable *cableWithYankInitiated;
	bool menuSelectionInProgress;
	QGraphicsItem *previousNearbyItem;
	VuoNodeAndPortIdentifierCache *identifierCache;
	bool dragStickinessDisabled;
	bool ignoreApplicationStateChangeEvents;
	bool showEventsMode;
	QTimer *refreshComponentAlphaLevelTimer;
	set<QGraphicsItemAnimation *> preparedAnimations;
	map<QTimeLine *, QGraphicsItemAnimation *> animationForTimeline;
	string triggerPortToRefire;

	// Popovers
	bool popoverEventsEnabled;
	map<string, VuoPortPopover *> activePortPopovers;
	set<string> portsWithPopoversClosedAtLastEvent;

	bool isPortCurrentlyRevertible(VuoRendererPort *port);
	void addActionToMenuAndMapper(QMenu *menu, QSignalMapper *mapper, QString name, int index);
	void mousePressEventNonLeftButton(QGraphicsSceneMouseEvent *event);
	void correctForCancelledDuplication(QGraphicsSceneMouseEvent *event);
	void initiateCableDrag(VuoRendererPort *currentPort, VuoRendererCable *cableYankedDirectly, QGraphicsSceneMouseEvent *event);
	void concludeCableDrag(QGraphicsSceneMouseEvent *event);
	void concludePublishedCableDrag(QGraphicsSceneMouseEvent *event);
	void updateHoverHighlighting(QPointF scenePos, bool disablePortHoverHighlighting=false);
	void highlightEligibleEndpointsForCable(VuoCable *cable);
	void highlightInternalPortsConnectableToPort(VuoRendererPort *port, VuoRendererCable *cable);
	void updateEligibilityHighlightingForPort(VuoRendererPort *portToHighlight, VuoRendererPort *fixedPort, bool eventOnlyConnection, map<string, VuoCompilerType *> &types);
	void updateEligibilityHighlightingForNode(VuoRendererNode *node);
	bool selectBridgingSolutionFromOptions(vector<string> suitableTypecasts, map<string, VuoRendererPort *> portToSpecializeForTypecast, map<string, string> specializedTypeNameForTypecast, string &selectedTypecast);
	vector<string> findBridgingSolutions(VuoRendererPort *fromPort, VuoRendererPort *toPort, bool toPortIsDragDestination, map<string, VuoRendererPort *> &portToSpecializeForTypecast, map<string, string> &specializedTypeNameForTypecast, map<string, VuoCompilerType *> &types);
	bool promptForBridgingSelectionFromOptions(vector<string> suitableTypecasts, map<string, VuoRendererPort *> portToSpecializeForTypecast, map<string, string> specializedTypeNameForTypecast, string &selectedTypecast);
	QString getDisplayTextForSpecializationOption(VuoRendererPort *portToSpecialize, string specializedTypeName);
	bool hasFeedbackErrors(void);
	void buildComposition(string compositionSnapshot, const set<string> &dependenciesUninstalled = set<string>());
	bool isRunningThreadUnsafe(void);
	string getIdentifierForRunningPort(VuoPort *port);
	void moveItemsBy(set<VuoRendererNode *> nodes, set<VuoRendererComment *> comments, qreal dx, qreal dy, bool movedByDragging);
	void setPopoversHideOnDeactivate(bool shouldHide);

	QGraphicsItemAnimation * setUpAnimationForPort(QGraphicsItemAnimation *animation, VuoRendererPort *port);
	void animatePort(VuoRendererPort *port);
	QGraphicsItemAnimation * getAvailableAnimationForPort(VuoRendererPort *port);

	friend class TestVuoEditor;
};
