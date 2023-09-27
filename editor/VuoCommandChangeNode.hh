/**
 * @file
 * VuoCommandChangeNode interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCommandCommon.hh"

class VuoCable;
class VuoCompilerCompositionDiff;
class VuoEditorComposition;
class VuoEditorWindow;
class VuoPort;
class VuoRendererNode;

/**
 * An undoable action for replacing a set of nodes with a provided set of replacements.
 */
class VuoCommandChangeNode : public VuoCommandCommon
{
public:
	VuoCommandChangeNode(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorWindow *window);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	VuoEditorComposition *composition;
	string revertedSnapshot;
	string updatedSnapshot;

	VuoCompilerCompositionDiff *diffInfo;
	bool operationInvolvesGenericPort;

	VuoRendererNode *oldNode;
	VuoRendererNode *newNode;
	map<VuoPort *, VuoPort *> updatedPortForOriginalPort;

	set<VuoCable *> incomingCables;
	set<VuoCable *> outgoingCables;
	vector<VuoRendererNode *> collapsedTypecasts;
	map<VuoRendererNode *, set<VuoCable *> > incomingCablesForTypecast;
	map<VuoRendererNode *, VuoPort *> hostPortForTypecast;
	vector<pair<VuoPort *, VuoPublishedPort *> > revertedPublishedInternalExternalPortCombinations;
	vector<pair<VuoPort *, VuoPublishedPort *> > updatedPublishedInternalExternalPortCombinations;
	map<VuoRendererNode *, vector<pair<VuoPort *, VuoPublishedPort *> > > publishedInternalExternalPortCombinationsForTypecast;
	map<pair<VuoPort *, VuoPublishedPort *>, bool> publishedConnectionCarriedData;
	map<VuoCable *, VuoPort *> originalFromPortForCable;
	map<VuoCable *, VuoPort *> originalToPortForCable;
	map<VuoCable *, bool> cableCarriedData;
	map<VuoPort *, string> constantValueForOriginalPort;

	void createAllMappings();
	void createAllPortMappings();
	void createSwappedNodePortMappings();
	void removeStrandedAttachments();
	void swapNodes();
	void createNecessaryAttachments();
};
