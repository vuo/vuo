/**
 * @file
 * VuoCommandChangeNode implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandRemove.hh"
#include "VuoCommandChangeNode.hh"

#include "VuoCompilerCable.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoEditor.hh"
#include "VuoRendererValueListForReadOnlyDictionary.hh"
#include "VuoRendererTypecastPort.hh"
#include "VuoGenericType.hh"
#include "VuoNodeClass.hh"
#include "VuoCompilerInputData.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoRendererInputAttachment.hh"
#include "VuoRendererInputListDrawer.hh"
#include "VuoRendererNode.hh"
#include "VuoSubcompositionMessageRouter.hh"

/**
 * Creates a command for swapping a node with a new node of a different class (e.g., via "Change Node"
 * context menu), making best guesses as to how best to preserve inputs and connections across the swap.
 */
VuoCommandChangeNode::VuoCommandChangeNode(VuoRendererNode *oldNode, VuoRendererNode *newNode, VuoEditorWindow *window)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Change Node"));
	this->window = window;
	this->composition = window->getComposition();
	this->revertedSnapshot = composition->takeSnapshot();
	this->oldNode = oldNode;
	this->newNode = newNode;

	// Start of command content.
	{
		this->operationInvolvesGenericPort = false;
		if (oldNode->hasGenericPort() || newNode->hasGenericPort())
			this->operationInvolvesGenericPort = true;

		createAllMappings();
		removeStrandedAttachments();
		swapNodes();
		composition->createAndConnectInputAttachments(newNode, false);
		diffInfo = VuoCommandCommon::addNodeReplacementToDiff(new VuoCompilerCompositionDiff(), oldNode, newNode, updatedPortForOriginalPort, composition);
	}
	// End of command content.

	this->updatedSnapshot = composition->takeSnapshot();

	setDescription("Change node %s %s -> %s",
		oldNode->getBase()->hasCompiler() ? oldNode->getBase()->getCompiler()->getIdentifier().c_str() : "?",
		oldNode->getBase()->getNodeClass()->getClassName().c_str(),
		newNode->getBase()->getNodeClass()->getClassName().c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandChangeNode::id() const
{
	return VuoCommandCommon::changeNodeCommandID;
}

/**
 * Replaces @c oldNode with its @c newNode equivalent, rewiring any connected cables
 * to connect to similar ports belonging to the new node.
 */
void VuoCommandChangeNode::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	if (operationInvolvesGenericPort)
		composition->updateGenericPortTypes();

	VuoCompilerCompositionDiff *diffInfoCopy = new VuoCompilerCompositionDiff(*diffInfo);
	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot, diffInfoCopy);
}

/**
 * Restores the original node and its connected cables.
 */
void VuoCommandChangeNode::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (operationInvolvesGenericPort)
		composition->updateGenericPortTypes();

	VuoCompilerCompositionDiff *diffInfoCopy = new VuoCompilerCompositionDiff(*diffInfo);
	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot, diffInfoCopy);
}

/**
 * Replaces the @c oldNode with the @c newNode.
 */
void VuoCommandChangeNode::swapNodes()
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
		for (map<VuoPort *, string>::iterator i = constantValueForOriginalPort.begin(); i != constantValueForOriginalPort.end(); ++i)
		{
			VuoPort *oldInputPort = (*i).first;
			VuoPort *newInputPort = updatedPortForOriginalPort[oldInputPort];
			if (newInputPort)
				composition->updatePortConstant(static_cast<VuoCompilerInputEventPort *>(newInputPort->getCompiler()), constantValueForOriginalPort[oldInputPort], false);
		}

		// Re-route cables.
		for (set<VuoCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
		{
			VuoCommandCommon::updateCable((*i)->getRenderer(),
											updatedPortForOriginalPort[originalFromPortForCable[*i] ],
											updatedPortForOriginalPort[originalToPortForCable[*i] ],
											composition);

			if (!cableCarriedData[*i] && (*i)->getRenderer()->effectivelyCarriesData() && (*i)->hasCompiler())
				(*i)->getCompiler()->setAlwaysEventOnly(true);
		}

		for (set<VuoCable *>::iterator i = incomingCables.begin(); i != incomingCables.end(); ++i)
		{
			VuoCommandCommon::updateCable((*i)->getRenderer(),
											updatedPortForOriginalPort[originalFromPortForCable[*i] ],
											updatedPortForOriginalPort[originalToPortForCable[*i] ],
											composition);

			if (!cableCarriedData[*i] && (*i)->getRenderer()->effectivelyCarriesData() && (*i)->hasCompiler())
				(*i)->getCompiler()->setAlwaysEventOnly(true);
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
 * Inventories the reverted and updated constants and cable connections associated with the node being replaced.
 */
void VuoCommandChangeNode::createAllMappings()
{
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
			{
				this->incomingCables.insert(cable);
				cableCarriedData[cable] = cable->getRenderer()->effectivelyCarriesData();
			}
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
					cableCarriedData[cable] = cable->getRenderer()->effectivelyCarriesData();
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
			{
				this->outgoingCables.insert(cable);
				cableCarriedData[cable] = cable->getRenderer()->effectivelyCarriesData();
			}
		}
	}

	createAllPortMappings();
}

/**
 * Inventories the original and updated ports for the replaced node and its connected cables.
 */
void VuoCommandChangeNode::createAllPortMappings()
{
	// First inventory the external cable endpoints that will not change.
	for (set<VuoCable *>::iterator i = incomingCables.begin(); i != incomingCables.end(); ++i)
	{
		VuoCable *cable = (*i);

		originalFromPortForCable[cable] = cable->getFromPort();
		originalToPortForCable[cable] = cable->getToPort();
		updatedPortForOriginalPort[cable->getFromPort()] = cable->getFromPort();
	}

	for (set<VuoCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
	{
		VuoCable *cable = (*i);

		originalFromPortForCable[cable] = cable->getFromPort();
		originalToPortForCable[cable] = cable->getToPort();
		updatedPortForOriginalPort[cable->getToPort()] = cable->getToPort();
	}

	// Now inventory the ports belonging to the swapped nodes.
	createSwappedNodePortMappings();
}

/**
 * Creates mappings between the ports of the original node and the ports of the new node.
 * Stores the associations in the @c updatedPortForOriginalPort map.
 * Ports with no reasonable equivalents are not included in the map.
 */
void VuoCommandChangeNode::createSwappedNodePortMappings()
{
	set<VuoPort *> newPortsClaimed;

	// First pass: Find ports with exact name and type matches
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		string oldPortName = oldPort->getBase()->getClass()->getName();

		// Find exact match-by-name.
		VuoPort *newPort = newNode->getBase()->getInputPortWithName(oldPortName);
		if (newPort)
		{
			// Check whether it's also an exact match-by-type.
			VuoType *oldDataType = oldPort->getDataType();
			VuoType *newDataType = newPort->getRenderer()->getDataType();
			if ((!oldDataType) ||
					((oldDataType && newDataType) && (oldDataType->getModuleKey() == newDataType->getModuleKey())))
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort;

				if (oldDataType)
					newPortsClaimed.insert(newPort);
			}
		}
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		string oldPortName = oldPort->getBase()->getClass()->getName();

		// Find exact match-by-name.
		VuoPort *newPort = newNode->getBase()->getOutputPortWithName(oldPortName);
		if (newPort)
		{
			// Check whether it's also an exact match-by-type.
			VuoType *oldDataType = oldPort->getDataType();
			VuoType *newDataType = newPort->getRenderer()->getDataType();
			if ((!oldDataType) ||
					((oldDataType && newDataType) && (oldDataType->getModuleKey() == newDataType->getModuleKey())))
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort;

				if (oldDataType)
					newPortsClaimed.insert(newPort);
			}
		}
	}

	// Second pass: Find ports with matching types to accommodate data cables connected to the original ports
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		if (!oldPort->effectivelyHasConnectedDataCable(true))
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		// Find the first available match-by-type (excluding event-only).
		foreach (VuoRendererPort *newPort, newNode->getInputPorts())
		{
			// Skip ports that have already been claimed.
			if (newPortsClaimed.find(newPort->getBase()) != newPortsClaimed.end())
				continue;

			VuoType *oldDataType = oldPort->getDataType();
			VuoType *newDataType = newPort->getDataType();
			if ((oldDataType && newDataType) && (oldDataType->getModuleKey() == newDataType->getModuleKey()))
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
				newPortsClaimed.insert(newPort->getBase());
				break;
			}
		}
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		if (!oldPort->effectivelyHasConnectedDataCable(true))
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		// Find the first available match-by-type (excluding event-only).
		foreach (VuoRendererPort *newPort, newNode->getOutputPorts())
		{
			// Skip ports that have already been claimed.
			if (newPortsClaimed.find(newPort->getBase()) != newPortsClaimed.end())
				continue;

			VuoType *oldDataType = oldPort->getDataType();
			VuoType *newDataType = newPort->getDataType();
			if ((oldDataType && newDataType) && (oldDataType->getModuleKey() == newDataType->getModuleKey()))
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
				newPortsClaimed.insert(newPort->getBase());
				break;
			}
		}
	}

	// Third pass: Match remaining data+event ports with data+event ports of identical name but any type,
	// as long as the original port had only event-only cables connected.
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		string oldPortName = oldPort->getBase()->getClass()->getName();

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		if ((oldPort->getBase()->getConnectedCables(true).size() > 0) && (!oldPort->effectivelyHasConnectedDataCable(true)))
		{
			// Find exact match-by-name.
			VuoPort *newPort = newNode->getBase()->getInputPortWithName(oldPortName);
			if (newPort)
				updatedPortForOriginalPort[oldPort->getBase()] = newPort;
		}
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		string oldPortName = oldPort->getBase()->getClass()->getName();

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		if ((oldPort->getBase()->getConnectedCables(true).size() > 0) && (!oldPort->effectivelyHasConnectedDataCable(true)))
		{
			// Find exact match-by-name.
			VuoPort *newPort = newNode->getBase()->getOutputPortWithName(oldPortName);
			if (newPort)
				updatedPortForOriginalPort[oldPort->getBase()] = newPort;
		}
	}

	// Fourth pass: Match remaining data+event ports with event-only ports if they have only
	// event-only cables connected.
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		if ((oldPort->getBase()->getConnectedCables(true).size() > 0) && (!oldPort->effectivelyHasConnectedDataCable(true)))
		{
			VuoRendererPort *newPort = composition->findDefaultPortForEventOnlyConnection(newNode, true);
			if (newPort)
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
		}
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		if ((oldPort->getBase()->getConnectedCables(true).size() > 0) && (!oldPort->effectivelyHasConnectedDataCable(true)))
		{
			VuoRendererPort *newPort = composition->findDefaultPortForEventOnlyConnection(newNode, false);
			if (newPort)
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
		}
	}

	// Fifth pass: Match remaining event-only ports with new event-only ports if possible.
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		VuoType *oldDataType = oldPort->getDataType();
		if (oldDataType)
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		foreach (VuoRendererPort *newPort, newNode->getInputPorts())
		{
			// Skip refresh ports.
			if (newPort->getBase() == newNode->getBase()->getRefreshPort())
				continue;

			VuoType *newDataType = newPort->getDataType();
			if (!newDataType)
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
				break;
			}
		}
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		VuoType *oldDataType = oldPort->getDataType();
		if (oldDataType)
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		foreach (VuoRendererPort *newPort, newNode->getOutputPorts())
		{
			// Skip refresh ports.
			if (newPort->getBase() == newNode->getBase()->getRefreshPort())
				continue;

			VuoType *newDataType = newPort->getDataType();
			if (!newDataType)
			{
				updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
				break;
			}
		}
	}

	// Sixth pass: Match any remaining event-only ports to the port in the new node that would receive
	// a cable connection dropped onto the node header.
	// Input ports
	foreach (VuoRendererPort *oldPort, oldNode->getInputPorts())
	{
		VuoType *oldDataType = oldPort->getDataType();
		if (oldDataType)
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		VuoRendererPort *newPort = composition->findDefaultPortForEventOnlyConnection(newNode, true);
		if (newPort)
			updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
	}

	// Output ports
	foreach (VuoRendererPort *oldPort, oldNode->getOutputPorts())
	{
		VuoType *oldDataType = oldPort->getDataType();
		if (oldDataType)
			continue;

		if (updatedPortForOriginalPort.find(oldPort->getBase()) != updatedPortForOriginalPort.end())
			continue;

		VuoRendererPort *newPort = composition->findDefaultPortForEventOnlyConnection(newNode, false);
		if (newPort)
			updatedPortForOriginalPort[oldPort->getBase()] = newPort->getBase();
	}
}

/**
 * Removes attachments and any dependent components that will no longer have a host port
 * once the node swap is performed.
 */
void VuoCommandChangeNode::removeStrandedAttachments()
{
	QList<QGraphicsItem *> strandedAttachments;
	vector<VuoPort *> inputPorts = oldNode->getBase()->getInputPorts();
	for(unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		if (updatedPortForOriginalPort.find(inputPorts[i]) == updatedPortForOriginalPort.end())
		{
			set<VuoRendererInputAttachment *> portUpstreamAttachments = inputPorts[i]->getRenderer()->getAllUnderlyingUpstreamInputAttachments();
			foreach (VuoRendererInputAttachment *attachment, portUpstreamAttachments)
				strandedAttachments.append(attachment);
		}
	}

	VuoCommandRemove removeAttachments(strandedAttachments, window, composition->getInputEditorManager(), "Delete", true);
	removeAttachments.redo();
}
