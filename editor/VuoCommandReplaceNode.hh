/**
 * @file
 * VuoCommandReplaceNode interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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
class VuoCommandReplaceNode : public VuoCommandCommon
{
public:
	VuoCommandReplaceNode(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorWindow *window, string commandDescription, bool preserveDanglingCables=false, bool resetConstantValues=false);
	VuoCommandReplaceNode(map<VuoRendererNode *, VuoRendererNode *> newNodeForOldNode, VuoEditorWindow *window, string commandDescription, bool preserveDanglingCables=false, bool resetConstantValues=false);

	int id() const;
	void undo();
	void redo();

private:
	static const int commandID;
	VuoEditorWindow *window;
	string revertedSnapshot;
	string updatedSnapshot;

	VuoCompilerCompositionDiff *diffInfo;

	bool operationInvolvesGenericPort;

	class SingleNodeReplacement;
	void initialize(VuoEditorWindow *window, string commandDescription, map<VuoRendererNode *, VuoRendererNode *> newNodeForOldNode, bool preserveDanglingCables, bool resetConstantValues);

	/**
	 * This class represents an undoable action for replacing one node with another.
	 */
	class SingleNodeReplacement
	{
	public:
		SingleNodeReplacement(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorComposition *composition, bool preserveDanglingCables=false, bool resetConstantValues=false);

		void createAllMappings();
		void redo();

		VuoRendererNode *oldNode;
		VuoRendererNode *newNode;
		map<VuoPort *, VuoPort *> updatedPortForOriginalPort;

	private:
		VuoEditorComposition *composition;
		bool preserveDanglingCables;
		bool resetConstantValues;

		bool replacingDictionaryKeyList;
		bool replacingDictionaryValueList;
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
		map<VuoPort *, string> constantValueForOriginalPort;

		void createPortMappings();
		VuoPort * getEquivalentInputPortInNewNode(VuoPort *oldInputPort, VuoRendererNode *oldNode, VuoRendererNode *newNode);
		bool valueShouldCarryOver(VuoPort *oldInputPort, VuoPort *newInputPort);
	};
};
