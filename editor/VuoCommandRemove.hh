/**
 * @file
 * VuoCommandRemove interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorWindow;
class VuoInputEditorManager;
class VuoRendererCable;
class VuoRendererComment;
class VuoRendererNode;
class VuoRendererTypecastPort;

/**
 * An undoable action for removing nodes, comments, and cables from the composition.
 */
class VuoCommandRemove : public VuoCommandCommon
{
public:
	VuoCommandRemove(QList<QGraphicsItem *> removedComponents,
					 VuoEditorWindow *window,
					 VuoInputEditorManager *inputEditorManager,
					 string commandDescription="Delete",
					 bool disableAttachmentInsertion=false);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;
	set<string> removedSubcompositionNodeIdentifiers;

	// Used only for live-coding updates:
	set<string> updatedPortIDs;

	// The following are not and must not be used outside of the constructor and its helper functions:
	// (@todo: Eliminate as member variables?)
	set<VuoRendererNode *> removedNodes;
	set<VuoRendererComment *> removedComments;
	set<VuoRendererCable *> removedCables;
	set<VuoRendererCable *> reroutedCables;
	set<VuoRendererCable *> removedPublishedCables;
	VuoRendererCable *cableInProgress;
	map<VuoPort *, string> revertedConstantForPort;
	map<VuoPort *, string> updatedConstantForPort;
	vector<pair<VuoPort *, VuoPublishedPort *> > unpublishedInternalExternalPortCombinations;
	map<pair<VuoPort *, VuoPublishedPort *>, bool> publishedConnectionCarriedData;
	set<VuoRendererNode *> addedNodes;
	set<VuoRendererCable *> addedCables;
	map<VuoRendererCable *, VuoPort *> revertedFromPortForCable;
	map<VuoRendererCable *, VuoPort *> revertedToPortForCable;
	map<VuoRendererCable *, VuoPort *> updatedFromPortForCable;
	map<VuoRendererCable *, VuoPort *> updatedToPortForCable;
	vector<VuoRendererNode *> typecastsUncollapsedDuringInventory;
	vector<VuoRendererNode *> typecastsCollapsedFollowingComponentRemoval;
	vector<VuoRendererNode *> typecastsCollapsedUponUndo;
	bool operationInvolvesGenericPort;
	bool operationRequiresRunningCompositionUpdate;
	//

	set<QGraphicsItem *> getAttachmentsDependentOnNode(VuoRendererNode *rn);
	void inventoryCableAndDependentTypecasts(VuoRendererCable *rc);
	void inventoryTypecastAndDependentCables(VuoRendererTypecastPort *tp, bool triggeredByIncomingCableDeletion);
	void inventoryNodeAndDependentCables(VuoRendererNode *rn, bool inputCablesPreprocessed=false);
	void prepareMakeListToReplaceDeletedCable(VuoRendererCable *rc);
	bool modifiedComponentsIncludeGenericPorts();

};
