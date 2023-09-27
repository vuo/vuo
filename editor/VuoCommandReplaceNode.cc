/**
 * @file
 * VuoCommandReplaceNode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandReplaceNode.hh"

#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoGenericType.hh"
#include "VuoNodeClass.hh"
#include "VuoCompilerInputData.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererInputAttachment.hh"
#include "VuoRendererNode.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for replacing a set of nodes within a composition.
 */
VuoCommandReplaceNode::VuoCommandReplaceNode(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorWindow *window, string commandDescription,
											 bool preserveDanglingCables, bool resetConstantValues)
	: VuoCommandCommon(window)
{
	map<VuoRendererNode *, VuoRendererNode *> newNodeForOldNode;
	newNodeForOldNode[oldNode] = newNode;

	initialize(window, commandDescription, newNodeForOldNode, preserveDanglingCables, resetConstantValues);
}

/**
 * Creates a command for replacing a set of nodes within a composition.
 */
VuoCommandReplaceNode::VuoCommandReplaceNode(map<VuoRendererNode *, VuoRendererNode *> newNodeForOldNode, VuoEditorWindow *window, string commandDescription,
											 bool preserveDanglingCables, bool resetConstantValues)
	: VuoCommandCommon(window)
{
	initialize(window, commandDescription, newNodeForOldNode, preserveDanglingCables, resetConstantValues);
}

/**
 * Initializes the data members of a command for replacing a set of nodes within a composition.
 * Helper function for the VuoCommandReplaceNode::VuoCommandReplaceNode(...) constructors.
 */
void VuoCommandReplaceNode::initialize(VuoEditorWindow *window, string commandDescription, map<VuoRendererNode *, VuoRendererNode *> newNodeForOldNode,
									   bool preserveDanglingCables, bool resetConstantValues)
{
	// Set the graphviz identifiers of each new node the same as the node it's replacing so the runtime can transfer port data
	// from the old node to corresponding ports on the new node.
	for (map<VuoRendererNode *, VuoRendererNode *>::iterator i = newNodeForOldNode.begin(); i != newNodeForOldNode.end(); ++i)
	{
		VuoCompilerNode *oldNode = i->first->getBase()->getCompiler();
		VuoCompilerNode *newNode = i->second->getBase()->getCompiler();
		newNode->setGraphvizIdentifier(oldNode->getGraphvizIdentifier());
	}

	setText(commandDescription.c_str());
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();
	vector<string> descriptions;

	// Start of command content.
	{
		this->operationInvolvesGenericPort = false;
		vector<VuoCommandReplaceNode::SingleNodeReplacement *> singleNodeReplacements;

		for (map<VuoRendererNode *, VuoRendererNode *>::iterator i = newNodeForOldNode.begin(); i != newNodeForOldNode.end(); ++i)
		{
			VuoRendererNode *oldNode = i->first;
			VuoRendererNode *newNode = i->second;

			if (oldNode->hasGenericPort() || newNode->hasGenericPort())
				this->operationInvolvesGenericPort = true;

			VuoCommandReplaceNode::SingleNodeReplacement *nodeReplacement = new SingleNodeReplacement(oldNode,
																									  newNode,
																									  window->getComposition(),
																									  preserveDanglingCables,
																									  resetConstantValues);
			singleNodeReplacements.push_back(nodeReplacement);

			descriptions.push_back(oldNode->getBase()->getCompiler()->getIdentifier()
				+ " ("
				+ oldNode->getBase()->getNodeClass()->getClassName()
				+ " -> "
				+ newNode->getBase()->getNodeClass()->getClassName()
				+ ")");
		}

		for (vector<SingleNodeReplacement *>::iterator i = singleNodeReplacements.begin(); i != singleNodeReplacements.end(); ++i)
		{
			(*i)->createAllMappings();
			(*i)->redo();
		}

		diffInfo = new VuoCompilerCompositionDiff();
		for (SingleNodeReplacement *nodeReplacement : singleNodeReplacements)
			VuoCommandCommon::addNodeReplacementToDiff(diffInfo,
														   nodeReplacement->oldNode,
														   nodeReplacement->newNode,
														   nodeReplacement->updatedPortForOriginalPort,
														   window->getComposition());
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	setDescription("%s: %s",
		commandDescription.c_str(),
		VuoStringUtilities::join(descriptions, ", ").c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandReplaceNode::id() const
{
	return VuoCommandCommon::replaceNodeCommandID;
}

/**
 * Replaces the set of @c oldNodes with their @c newNode equivalents, re-wiring any connected cables
 * to connect to ports of the same name belonging to the new node.
 */
void VuoCommandReplaceNode::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	VuoCompilerCompositionDiff *diffInfoCopy = new VuoCompilerCompositionDiff(*diffInfo);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot, diffInfoCopy);
}

/**
 * Restores the original nodes and their connected cables.
 */
void VuoCommandReplaceNode::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	VuoCompilerCompositionDiff *diffInfoCopy = new VuoCompilerCompositionDiff(*diffInfo);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot, diffInfoCopy);
}

/**
 * Creates a command for replacing a node within a composition.
 */
VuoCommandReplaceNode::SingleNodeReplacement::SingleNodeReplacement(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorComposition *composition, bool preserveDanglingCables, bool resetConstantValues)
{
	this->oldNode = oldNode;
	this->newNode = newNode;
	this->composition = composition;
	this->preserveDanglingCables = preserveDanglingCables;
	this->resetConstantValues = resetConstantValues;

	replacingDictionaryKeyList = false;
	replacingDictionaryValueList = false;
}

/**
 * Inventories the reverted and updated constants and cable connections associated with the node being replaced.
 */
void VuoCommandReplaceNode::SingleNodeReplacement::createAllMappings()
{
	replacingDictionaryKeyList   = dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(oldNode) && dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(newNode);
	replacingDictionaryValueList = dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(oldNode) && dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(newNode);

	// Inventory the port constants and connected input cables associated with the old node, to be re-associated with the new node.
	vector<VuoPort *> oldInputPorts = oldNode->getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator inputPort = oldInputPorts.begin(); inputPort != oldInputPorts.end(); ++inputPort)
	{
		if ((*inputPort)->getRenderer()->getDataType())
			constantValueForOriginalPort[*inputPort] = (*inputPort)->getRenderer()->getConstantAsString();

		vector<VuoCable *> inputCables = (*inputPort)->getConnectedCables(true);
		foreach(VuoCable *cable, inputCables)
		{
			if (cable->isPublished())
			{
				VuoPort *internalPublishedPort = cable->getToPort();
				VuoPublishedPort *externalPublishedPort = dynamic_cast<VuoPublishedPort *>(cable->getFromPort());
				this->revertedPublishedInternalExternalPortCombinations.push_back(make_pair(internalPublishedPort, externalPublishedPort));
				publishedConnectionCarriedData[make_pair(internalPublishedPort, externalPublishedPort)] = cable->getRenderer()->effectivelyCarriesData();
			}

			else
				this->incomingCables.insert(cable);
		}

		// Also inventory any typecasts attached to the old node, to be attached to the new node instead.
		VuoRendererTypecastPort *typecastPort = dynamic_cast<VuoRendererTypecastPort *>((*inputPort)->getRenderer());
		if (typecastPort)
		{
			VuoRendererNode *typecastNode = typecastPort->getUncollapsedTypecastNode();
			this->collapsedTypecasts.push_back(typecastNode);

			// Inventory the typecast's own input cables.
			VuoPort *typecastInPort = typecastNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];

			vector<VuoCable *> typecastInputCables = typecastInPort->getConnectedCables(true);
			foreach (VuoCable *cable, typecastInputCables)
			{
				if (cable->isPublished())
				{
					VuoPort *internalPublishedPort = cable->getToPort();
					VuoPublishedPort *externalPublishedPort = dynamic_cast<VuoPublishedPort *>(cable->getFromPort());
					this->publishedInternalExternalPortCombinationsForTypecast[typecastNode].push_back(make_pair(internalPublishedPort, externalPublishedPort));
					publishedConnectionCarriedData[make_pair(internalPublishedPort, externalPublishedPort)] = cable->getRenderer()->effectivelyCarriesData();
				}

				else
				{
					this->incomingCablesForTypecast[typecastNode].insert(cable);
					originalFromPortForCable[cable] = cable->getFromPort();
					originalToPortForCable[cable] = cable->getToPort();
				}
			}

			hostPortForTypecast[typecastNode] = (*inputPort);
		}
	}

	// Inventory the set of output cables connected to the old node, to be re-routed to the new node.
	vector<VuoPort *> oldOutputPorts = oldNode->getBase()->getOutputPorts();
	for(vector<VuoPort *>::iterator outputPort = oldOutputPorts.begin(); outputPort != oldOutputPorts.end(); ++outputPort)
	{
		vector<VuoCable *> outputCables = (*outputPort)->getConnectedCables(true);
		foreach(VuoCable *cable, outputCables)
		{
			if (cable->isPublished())
			{
				VuoPort *internalPublishedPort = cable->getFromPort();
				VuoPublishedPort *externalPublishedPort = dynamic_cast<VuoPublishedPort *>(cable->getToPort());
				this->revertedPublishedInternalExternalPortCombinations.push_back(make_pair(internalPublishedPort, externalPublishedPort));
				publishedConnectionCarriedData[make_pair(internalPublishedPort, externalPublishedPort)] = cable->getRenderer()->effectivelyCarriesData();
			}

			else
				this->outgoingCables.insert(cable);
		}
	}

	createPortMappings();
}

/**
 * Replaces the @c oldNode with the @c newNode, re-wiring any connected cables
 * to connect to ports of the same name belonging to the new node.
 */
void VuoCommandReplaceNode::SingleNodeReplacement::redo()
{
	{
		// If the original node is currently being rendered as collapsed typecast, uncollapse it.
		if (oldNode->getProxyCollapsedTypecast())
			composition->uncollapseTypecastNode(oldNode);

		// Uncollapse typecasts attached to the original node.
		for (vector<VuoRendererNode *>::iterator i = collapsedTypecasts.begin(); i != collapsedTypecasts.end(); ++i)
			composition->uncollapseTypecastNode(*i);

		for (vector<pair<VuoPort *, VuoPublishedPort *> >::iterator i = revertedPublishedInternalExternalPortCombinations.begin(); i != revertedPublishedInternalExternalPortCombinations.end(); ++i)
		{
			bool unpublishIsolatedExternalPort = false;
			VuoCommandCommon::unpublishInternalExternalPortCombination((*i).first, (*i).second, composition, unpublishIsolatedExternalPort);
		}

		// Swap nodes.
		composition->replaceNode(oldNode, newNode->getBase());

		// Restore port constants.
		if (!resetConstantValues)
		{
			for (map<VuoPort *, string>::iterator i = constantValueForOriginalPort.begin(); i != constantValueForOriginalPort.end(); ++i)
			{
				VuoPort *oldInputPort = (*i).first;
				VuoPort *newInputPort = updatedPortForOriginalPort[oldInputPort];
				if (valueShouldCarryOver(oldInputPort, newInputPort))
					composition->updatePortConstant(static_cast<VuoCompilerInputEventPort *>(newInputPort->getCompiler()), constantValueForOriginalPort[oldInputPort], false);
			}
		}

		// Re-route cables.
		for (set<VuoCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
		{
			VuoCommandCommon::updateCable((*i)->getRenderer(),
											updatedPortForOriginalPort[originalFromPortForCable[*i] ],
											updatedPortForOriginalPort[originalToPortForCable[*i] ],
											composition,
											preserveDanglingCables);
		}

		for (set<VuoCable *>::iterator i = incomingCables.begin(); i != incomingCables.end(); ++i)
		{
			VuoCommandCommon::updateCable((*i)->getRenderer(),
											updatedPortForOriginalPort[originalFromPortForCable[*i] ],
											updatedPortForOriginalPort[originalToPortForCable[*i] ],
											composition,
											preserveDanglingCables);
		}

		// Re-publish published ports.
		for (vector<pair<VuoPort *, VuoPublishedPort *> >::iterator i = revertedPublishedInternalExternalPortCombinations.begin(); i != revertedPublishedInternalExternalPortCombinations.end(); ++i)
		{
			if (updatedPortForOriginalPort[(*i).first])
			{
				bool forceEventOnlyPublication = !publishedConnectionCarriedData[(*i)];
				VuoPublishedPort *updatedExternalPublishedPort = VuoCommandCommon::publishInternalExternalPortCombination(updatedPortForOriginalPort[(*i).first], (*i).second, forceEventOnlyPublication, composition);
				updatedPublishedInternalExternalPortCombinations.push_back(make_pair(updatedPortForOriginalPort[(*i).first], updatedExternalPublishedPort));
				publishedConnectionCarriedData[make_pair(updatedPortForOriginalPort[(*i).first], updatedExternalPublishedPort)] = publishedConnectionCarriedData[(*i)];
			}
		}

		// Re-collapse typecasts onto the updated node.
		for (vector<VuoRendererNode *>::iterator i = collapsedTypecasts.begin(); i != collapsedTypecasts.end(); ++i)
		{
			if (updatedPortForOriginalPort[hostPortForTypecast[*i] ])
				composition->collapseTypecastNode(*i);

			else
			{
				composition->removeNode(*i);
				foreach (VuoCable *cable, incomingCablesForTypecast[*i])
					VuoCommandCommon::removeCable(cable->getRenderer(), composition);

				for (vector<pair<VuoPort *, VuoPublishedPort *> >::iterator j = publishedInternalExternalPortCombinationsForTypecast[*i].begin(); j != publishedInternalExternalPortCombinationsForTypecast[*i].end(); ++j)
				{
					bool unpublishIsolatedExternalPort = false;
					VuoCommandCommon::unpublishInternalExternalPortCombination((*j).first, (*j).second, composition, unpublishIsolatedExternalPort);
				}
			}
		}

		// Re-collapse the updated node, if applicable.
		composition->collapseTypecastNode(newNode);

		VuoRendererInputAttachment *newAttachment = dynamic_cast<VuoRendererInputAttachment *>(newNode);
		if (newAttachment)
		{
			VuoNode *hostNode = newAttachment->getRenderedHostNode();
			if (hostNode && hostNode->hasRenderer())
				hostNode->getRenderer()->layoutConnectedInputDrawers();
		}
	}
}

/**
 * Inventories and stores each of the original and updated 'To' and 'From' ports
 * for the replaced node and its connected cables.
 */
void VuoCommandReplaceNode::SingleNodeReplacement::createPortMappings()
{
	for (set<VuoCable *>::iterator i = incomingCables.begin(); i != incomingCables.end(); ++i)
	{
		VuoCable *cable = (*i);

		originalFromPortForCable[cable] = cable->getFromPort();
		originalToPortForCable[cable] = cable->getToPort();

		updatedPortForOriginalPort[cable->getFromPort()] = cable->getFromPort();
		updatedPortForOriginalPort[cable->getToPort()] = getEquivalentInputPortInNewNode(cable->getToPort(), oldNode, newNode);
	}

	for (set<VuoCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
	{
		VuoCable *cable = (*i);

		originalFromPortForCable[cable] = cable->getFromPort();
		originalToPortForCable[cable] = cable->getToPort();

		updatedPortForOriginalPort[cable->getFromPort()] = newNode->getBase()->getOutputPortWithName(cable->getFromPort()->getClass()->getName());
		updatedPortForOriginalPort[cable->getToPort()] = cable->getToPort();
	}

	foreach (VuoPort *originalInputPort, oldNode->getBase()->getInputPorts())
		updatedPortForOriginalPort[originalInputPort] = getEquivalentInputPortInNewNode(originalInputPort, oldNode, newNode);

	foreach (VuoPort *originalOutputPort, oldNode->getBase()->getOutputPorts())
		updatedPortForOriginalPort[originalOutputPort] = newNode->getBase()->getOutputPortWithName(originalOutputPort->getClass()->getName());
}

/**
 * Returns the input port belonging to the @c newNode that corresponds to the @c oldInputPort belonging to the @c oldNode.
 * In the special case that the node is a key or value list for a read-only dictionary attachment, the port returned will be the one that
 * corresponds to the same dictionary key as the old one, even if the key has moved within the list.
 * For all other nodes, the port returned will be the one that has the same name as the old port.
 */
VuoPort * VuoCommandReplaceNode::SingleNodeReplacement::getEquivalentInputPortInNewNode(VuoPort *oldInputPort, VuoRendererNode *oldNode, VuoRendererNode *newNode)
{
	if (!replacingDictionaryKeyList && !replacingDictionaryValueList)
		return newNode->getBase()->getInputPortWithName(oldInputPort->getClass()->getName());
	else if (replacingDictionaryKeyList)
	{
		VuoRendererKeyListForReadOnlyDictionary *newKeyList = dynamic_cast<VuoRendererKeyListForReadOnlyDictionary *>(newNode);
		if (!newKeyList)
			return nullptr;

		VuoCompilerInputEventPort *oldKeyInputCompilerPort = dynamic_cast<VuoCompilerInputEventPort *>(oldInputPort->getCompiler());
		if (!oldKeyInputCompilerPort || !oldKeyInputCompilerPort->getData())
			return nullptr;
		string targetKeyName = oldKeyInputCompilerPort->getData()->getInitialValue();

		vector<VuoPort *> newKeyListInputs = newKeyList->getBase()->getInputPorts();
		for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < newKeyListInputs.size(); ++i)
		{
			VuoCompilerInputEventPort *newKeyInputPort = dynamic_cast<VuoCompilerInputEventPort *>(newKeyListInputs[i]->getCompiler());
			if (!newKeyInputPort)
				return nullptr;
			if (newKeyInputPort->getData()->getInitialValue() == targetKeyName)
				return newKeyInputPort->getBase();
		}
	}
	else if (replacingDictionaryValueList)
	{
		VuoRendererValueListForReadOnlyDictionary *oldValueList = dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(oldNode);
		VuoRendererValueListForReadOnlyDictionary *newValueList = dynamic_cast<VuoRendererValueListForReadOnlyDictionary *>(newNode);
		VuoNode *oldKeyList = oldValueList->getKeyListNode();
		VuoNode *newKeyList = newValueList->getKeyListNode();

		if (!(oldKeyList && newKeyList))
			return NULL;

		vector<VuoPort *> oldKeyListInputs = oldKeyList->getInputPorts();
		vector<VuoPort *> oldValueListInputs = oldValueList->getBase()->getInputPorts();
		VuoPort *oldKeyInputPort = NULL;
		for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < oldValueListInputs.size() && i < oldKeyListInputs.size() && !oldKeyInputPort; ++i)
		{
			if (oldValueListInputs[i] == oldInputPort)
				oldKeyInputPort = oldKeyListInputs[i];
		}

		if (!oldKeyInputPort)
			return NULL;

		VuoCompilerInputEventPort *oldKeyInputCompilerPort= static_cast<VuoCompilerInputEventPort *>(oldKeyInputPort->getCompiler());
		string targetKeyName = oldKeyInputCompilerPort->getData()->getInitialValue();

		vector<VuoPort *> newKeyListInputs = newKeyList->getInputPorts();
		vector<VuoPort *> newValueListInputs = newValueList->getBase()->getInputPorts();
		VuoPort *newValueInputPort = NULL;
		for (int i = VuoNodeClass::unreservedInputPortStartIndex; i < newValueListInputs.size() && i < newKeyListInputs.size() && !newValueInputPort; ++i)
		{
				VuoCompilerInputEventPort *newKeyInputPort= static_cast<VuoCompilerInputEventPort *>(newKeyListInputs[i]->getCompiler());
				if (newKeyInputPort->getData()->getInitialValue() == targetKeyName)
					return newValueListInputs[i];
		}
	}

	return nullptr;
}

/**
 * Helper function for VuoCommandReplaceNode::redo() and ::undo().
 * Returns a boolean indicating whether the provided @c newInputPort should
 * have its input value carried over from the @c oldInputPort that it is replacing.
 */
bool VuoCommandReplaceNode::SingleNodeReplacement::valueShouldCarryOver(VuoPort *oldInputPort, VuoPort *newInputPort)
{
	if (resetConstantValues)
		return false;

	if (!(oldInputPort && newInputPort))
		return false;

	bool isOldPortGeneric = dynamic_cast<VuoGenericType *>( static_cast<VuoCompilerPort *>(oldInputPort->getCompiler() )->getDataVuoType() );
	bool isNewPortGeneric = dynamic_cast<VuoGenericType *>( static_cast<VuoCompilerPort *>( newInputPort->getCompiler() )->getDataVuoType() );
	if (isOldPortGeneric || isNewPortGeneric)
		return false;

	return (oldInputPort->getRenderer()->getDataType() == newInputPort->getRenderer()->getDataType());
}
