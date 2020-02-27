/**
 * @file
 * VuoCommandRemove implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandRemove.hh"

#include "VuoComment.hh"
#include "VuoCompilerComment.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoSubcompositionMessageRouter.hh"
#include "VuoNodeClass.hh"
#include "VuoCable.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoInputEditorManager.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererComment.hh"
#include "VuoRendererNode.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for removing the given nodes, comments, and cables,
 * along with any dependent connected components.
 */
VuoCommandRemove::VuoCommandRemove(QList<QGraphicsItem *> explicitlyRemovedComponents,
								   VuoEditorWindow *window,
								   VuoInputEditorManager *inputEditorManager,
								   string commandDescription,
								   bool disableAttachmentInsertion)
	: VuoCommandCommon(window)
{
	setText(commandDescription.c_str());
	this->window = window;
	this->cableInProgress = NULL;
	vector<string> removedComponentDescriptions;

	// Normally we would take the composition's "Before" snapshot here, but in the case
	// of a cable disconnection we need to reconstruct what the composition looked like before
	// the cable drag began, so we do this a little bit later within the constructor.

	// Start of command content.
	{
		// Inventory the set of explicitly deleted composition components and their dependent components
		// that will also need to be deleted or re-routed.
		VuoEditorComposition *composition = window->getComposition();

		// First pass: Inventory all attachments or co-attachments of nodes marked for deletion.
		QList<QGraphicsItem *> explicitlyRemovedComponentsAndTheirAttachments;
		foreach (QGraphicsItem *component, explicitlyRemovedComponents)
		{
			if (!explicitlyRemovedComponentsAndTheirAttachments.contains(component))
				explicitlyRemovedComponentsAndTheirAttachments.append(component);

			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(component);
			if (rn)
			{
				set<QGraphicsItem *> attachments = getAttachmentsDependentOnNode(rn);
				foreach (QGraphicsItem *attachment, attachments)
				{
					if (!explicitlyRemovedComponentsAndTheirAttachments.contains(attachment))
						explicitlyRemovedComponentsAndTheirAttachments.append(attachment);
				}

				removedComponentDescriptions.push_back("node " + (rn->getBase()->hasCompiler() ? rn->getBase()->getCompiler()->getIdentifier() : "?"));
			}

			else
			{
				VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(component);
				if (rcomment)
				{
					removedComments.insert(rcomment);
					removedComponentDescriptions.push_back("comment " + rcomment->getBase()->getCompiler()->getGraphvizIdentifier());
				}
			}
		}

		// Second pass: Inventory cables and typecasts dependent on components marked for deletion.
		for (QList<QGraphicsItem *>::iterator i = explicitlyRemovedComponentsAndTheirAttachments.begin(); i != explicitlyRemovedComponentsAndTheirAttachments.end(); ++i)
		{
			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(*i);
			VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(*i);

			if (rn)
				inventoryNodeAndDependentCables(rn);
			else if (rc)
				inventoryCableAndDependentTypecasts(rc);
		}

		// Marking a cable for deletion trumps marking it for re-routing.
		for (set<VuoRendererCable *>::iterator rc = reroutedCables.begin(); rc != reroutedCables.end();)
		{
			if (removedCables.find(*rc) != removedCables.end())
				reroutedCables.erase(rc++);
			else
				++rc;
		}

		// For each deleted data+event cable, mark the 'To' port's constant value to be updated to match
		// the port's last value in the running composition, if applicable.
		set<VuoRendererCable *> removedInternalAndPublishedCables;
		removedInternalAndPublishedCables.insert(removedCables.begin(), removedCables.end());
		removedInternalAndPublishedCables.insert(removedPublishedCables.begin(), removedPublishedCables.end());
		foreach (VuoRendererCable *removedCable, removedInternalAndPublishedCables)
		{
			removedComponentDescriptions.push_back("cable "
				+ (removedCable->getBase()->getFromNode() && removedCable->getBase()->getFromNode()->hasCompiler()
					? removedCable->getBase()->getFromNode()->getCompiler()->getIdentifier() + ":" + removedCable->getBase()->getFromPort()->getClass()->getName()
					: "")
				+ " -> "
				+ (removedCable->getBase()->getToNode() && removedCable->getBase()->getToNode()->hasCompiler()
					? removedCable->getBase()->getToNode()->getCompiler()->getIdentifier() + ":" + removedCable->getBase()->getToPort()->getClass()->getName()
					: ""));

			if (!removedCable->effectivelyCarriesData())
				continue;

			VuoPort *toPort = removedCable->getBase()->getToPort();
			if (!toPort)
				toPort = removedCable->getFloatingEndpointPreviousToPort();

			if (!(toPort && toPort->hasRenderer() && !dynamic_cast<VuoRendererPublishedPort *>(toPort->getRenderer()) &&
				  (removedNodes.find(toPort->getRenderer()->getUnderlyingParentNode()) == removedNodes.end())))
				continue;

			VuoCompilerInputEventPort *eventPort = (toPort->hasCompiler()? static_cast<VuoCompilerInputEventPort *>(toPort->getCompiler()) : NULL);
			if (!eventPort)
				continue;

			if (!inputEditorManager->doesTypeAllowOfflineSerialization(eventPort->getDataVuoType()))
				continue;

			revertedConstantForPort[toPort] = eventPort->getData()->getInitialValue();

			json_object *currentRunningValue = composition->getPortValueInRunningComposition(toPort);
			updatedConstantForPort[toPort] = (currentRunningValue? json_object_to_json_string_ext(currentRunningValue, JSON_C_TO_STRING_PLAIN) :
																   revertedConstantForPort[toPort]);
		}

		// If applicable, reconstruct the state of the composition before the beginning of the cable drag
		// that concluded with the cable deletion, for the composition's "Before" snapshot.
		{
			VuoPort *currentFromPortForCableInProgress = NULL;
			VuoPort *currentToPortForCableInProgress = NULL;
			bool currentAlwaysEventOnlyStatusForCableInProgress = false;
			bool mustReconstructRevertedSnapshot = false;

			if (cableInProgress)
			{
				currentFromPortForCableInProgress = cableInProgress->getBase()->getFromPort();
				currentToPortForCableInProgress = cableInProgress->getBase()->getToPort();
				currentAlwaysEventOnlyStatusForCableInProgress = cableInProgress->getBase()->getCompiler()->getAlwaysEventOnly();

				mustReconstructRevertedSnapshot = ((currentFromPortForCableInProgress != revertedFromPortForCable[cableInProgress]) ||
														(currentToPortForCableInProgress != revertedToPortForCable[cableInProgress]));
			}

			if (mustReconstructRevertedSnapshot)
			{
				VuoCommandCommon::updateCable(cableInProgress, revertedFromPortForCable[cableInProgress], revertedToPortForCable[cableInProgress], composition, true);
				cableInProgress->getBase()->getCompiler()->setAlwaysEventOnly(cableInProgress->getPreviouslyAlwaysEventOnly());
			}

			this->revertedSnapshot = window->getComposition()->takeSnapshot();

			if (mustReconstructRevertedSnapshot)
			{
				cableInProgress->getBase()->getCompiler()->setAlwaysEventOnly(currentAlwaysEventOnlyStatusForCableInProgress);
				VuoCommandCommon::updateCable(cableInProgress, currentFromPortForCableInProgress, currentToPortForCableInProgress, composition, true);
			}
		}

		// Connect a "Make List" node to each list input port whose incoming data cable was removed.
		if (! disableAttachmentInsertion)
		{
			foreach (VuoRendererCable *rc, removedCables)
				prepareMakeListToReplaceDeletedCable(rc);

			foreach (VuoRendererCable *rc, removedPublishedCables)
				prepareMakeListToReplaceDeletedCable(rc);
		}

		this->operationInvolvesGenericPort = modifiedComponentsIncludeGenericPorts();
		this->operationRequiresRunningCompositionUpdate = (removedComments.size() < explicitlyRemovedComponents.size());

		// Now apply required changes.

		// Uncollapse any typecasts collapsed during the previous 'Undo' of this component removal, if applicable.
		for (vector<VuoRendererNode *>::iterator i = typecastsCollapsedUponUndo.begin(); i != typecastsCollapsedUponUndo.end(); ++i)
			composition->uncollapseTypecastNode(*i);

		// Re-uncollapse any typecasts that had to be uncollapsed when initially inventorying components for removal,
		// but that were re-collapsed during a subsequent 'Undo' operation.
		for (vector<VuoRendererNode *>::iterator i = typecastsUncollapsedDuringInventory.begin(); i != typecastsUncollapsedDuringInventory.end(); ++i)
			composition->uncollapseTypecastNode(*i);

		for (set<VuoRendererNode *>::iterator i = removedNodes.begin(); i != removedNodes.end(); ++i)
			composition->removeNode(*i);

		for (set<VuoRendererCable *>::iterator i = removedCables.begin(); i != removedCables.end(); ++i)
			VuoCommandCommon::removeCable(*i, composition);

		for (set<VuoRendererCable *>::iterator i = reroutedCables.begin(); i != reroutedCables.end(); ++i)
			VuoCommandCommon::updateCable(*i, updatedFromPortForCable[*i], updatedToPortForCable[*i], composition);

		for (set<VuoRendererNode *>::iterator i = addedNodes.begin(); i != addedNodes.end(); ++i)
			composition->addNode((*i)->getBase());

		for (set<VuoRendererCable *>::iterator i = addedCables.begin(); i != addedCables.end(); ++i)
			VuoCommandCommon::addCable(*i, updatedFromPortForCable[*i], updatedToPortForCable[*i], composition);

		for (set<VuoRendererComment *>::iterator i = removedComments.begin(); i != removedComments.end(); ++i)
			composition->removeComment(*i);

		for (vector<pair<VuoPort *, VuoPublishedPort *> >::iterator i = unpublishedInternalExternalPortCombinations.begin(); i != unpublishedInternalExternalPortCombinations.end(); ++i)
		{
			bool unpublishIsolatedExternalPort = false;
			VuoCommandCommon::unpublishInternalExternalPortCombination((*i).first, (*i).second, composition, unpublishIsolatedExternalPort);
		}

		// Collapse any typecasts possible following the removal of the specified components.
		this->typecastsCollapsedFollowingComponentRemoval = composition->collapseTypecastNodes();

		// Update generic types.
		if (operationInvolvesGenericPort)
			composition->updateGenericPortTypes();

		// For each deleted data+event cable, set the 'To' port's constant value to the last value
		// that flowed through the cable before its disconnection, if applicable.
		for (map<VuoPort *, string>::iterator i = updatedConstantForPort.begin(); i != updatedConstantForPort.end(); ++i)
		{
			VuoPort *toPort = i->first;

			if (revertedConstantForPort[toPort] != updatedConstantForPort[toPort])
			{
				composition->updatePortConstant(static_cast<VuoCompilerInputEventPort *>(toPort->getCompiler()), updatedConstantForPort[toPort], false);
				updatedPortIDs.insert(window->getComposition()->getIdentifierForStaticPort(toPort));
			}
		}
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	// Inventory the subcomposition nodes, so that each can be unlinked from / relinked to its open subcomposition window (if any)
	// when the node is removed / re-added.
	for (VuoRendererNode *removedNode : removedNodes)
	{
		if (removedNode->getBase()->getNodeClass()->hasCompiler() && removedNode->getBase()->getNodeClass()->getCompiler()->isSubcomposition())
		{
			string nodeIdentifier = removedNode->getBase()->getCompiler()->getGraphvizIdentifier();
			removedSubcompositionNodeIdentifiers.insert(nodeIdentifier);
		}
	}

	if (!removedComponentDescriptions.empty())
		setDescription("%s %s",
			commandDescription.c_str(),
			VuoStringUtilities::join(removedComponentDescriptions, ", ").c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandRemove::id() const
{
	return VuoCommandCommon::removeCommandID;
}

/**
 * Restores the removed nodes and cables.
 */
void VuoCommandRemove::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	if (operationRequiresRunningCompositionUpdate)
		window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);

	foreach (string updatedPortID, updatedPortIDs)
		window->coalesceInternalPortConstantsToSync(updatedPortID);

	for (string nodeIdentifier : removedSubcompositionNodeIdentifiers)
		window->coalesceNodesToRelink(nodeIdentifier);
}

/**
 * Removes the nodes and cables.
 */
void VuoCommandRemove::redo()
{
	VuoCommandCommon_redo;

	for (string nodeIdentifier : removedSubcompositionNodeIdentifiers)
		window->coalesceNodesToUnlink(nodeIdentifier);

	window->resetCompositionWithSnapshot(updatedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	if (operationRequiresRunningCompositionUpdate)
		window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);

	foreach (string updatedPortID, updatedPortIDs)
		window->coalesceInternalPortConstantsToSync(updatedPortID);
}

/**
 * Marks the input VuoRendererCable* @c rc for deletion.
 * If the cable is a data+event cable connected to a collapsed
 * typecast, marks the typecast for deletion as well.
 */
void VuoCommandRemove::inventoryCableAndDependentTypecasts(VuoRendererCable *rc)
{
	// If this cable has already been marked for deletion, do nothing.
	if ((removedCables.find(rc) != removedCables.end()) || (removedPublishedCables.find(rc) != removedPublishedCables.end()))
		return;

	bool wasPublishedCable = (rc->getBase()->isPublished() ||
			(!rc->getBase()->getToPort() &&
			 rc->getFloatingEndpointPreviousToPort() &&
			 dynamic_cast<VuoRendererPublishedPort *>(rc->getFloatingEndpointPreviousToPort()->getRenderer())));

	if (wasPublishedCable)
	{
		removedPublishedCables.insert(rc);

		bool isInputCable = rc->getBase()->isPublishedInputCable();
		VuoPort *toPort = (rc->getBase()->getToPort()? rc->getBase()->getToPort() : rc->getFloatingEndpointPreviousToPort());
		VuoPort *fromPort = rc->getBase()->getFromPort();

		VuoPort *internalPublishedPort = (isInputCable? toPort : fromPort);
		VuoPublishedPort *externalPublishedPort = (isInputCable? dynamic_cast<VuoPublishedPort *>(fromPort) : dynamic_cast<VuoPublishedPort *>(toPort));

		this->unpublishedInternalExternalPortCombinations.push_back(make_pair(internalPublishedPort, externalPublishedPort));
		this->publishedConnectionCarriedData[make_pair(internalPublishedPort, externalPublishedPort)] = rc->effectivelyCarriesData();
	}

	else
		removedCables.insert(rc);

	// Record the cable's original endpoints in case this operation is undone.
	revertedFromPortForCable[rc] = rc->getBase()->getFromPort();
	revertedToPortForCable[rc] = rc->getBase()->getToPort();

	// Case: The cable had already been disconnected from its 'To' port at the time it was deleted.
	if (! revertedToPortForCable[rc])
	{
		cableInProgress = rc;
		revertedToPortForCable[rc] = rc->getFloatingEndpointPreviousToPort();
	}

	// If the cable is a data+event cable connected to a collapsed typecast, mark the typecast for deletion as well.
	if (rc->effectivelyCarriesData())
	{
		if (revertedToPortForCable[rc] && revertedToPortForCable[rc]->hasRenderer())
		{
			VuoRendererTypecastPort *typecastToPort = (VuoRendererTypecastPort *)(revertedToPortForCable[rc]->getRenderer()->getTypecastParentPort());
			if (typecastToPort)
				inventoryTypecastAndDependentCables(typecastToPort, true);
		}

		if (rc->getBase()->getFromNode() && rc->getBase()->getFromNode()->hasRenderer())
		{
			VuoRendererTypecastPort *typecastFromPort = (rc->getBase()->getFromNode()->getRenderer()->getProxyCollapsedTypecast());
			if (typecastFromPort)
				inventoryTypecastAndDependentCables(typecastFromPort, false);
		}
	}
}

/**
 * Helper function for VuoCommandRemove::inventoryCableAndDependentTypecasts().
 * Marks the input VuoRendererTypecastPort* @c tp for deletion.
 * Marks connected cables for deletion or re-routing, as appropriate.
 */
void VuoCommandRemove::inventoryTypecastAndDependentCables(VuoRendererTypecastPort *tp, bool triggeredByIncomingCableDeletion)
{
	// We do not have to worry about this function being called on the same collapsed typecast twice, since
	// the first thing we do here is uncollapse the typecast to its original node form,
	// causing it effectively no longer to exist in collapsed typecast form.
	VuoRendererNode *uncollapsedNode = window->getComposition()->uncollapseTypecastNode(tp);
	this->typecastsUncollapsedDuringInventory.push_back(uncollapsedNode);

	// Re-route any incoming event-only cables connected to the typecast.
	// Assumption: Typeconverters will not be chained.
	VuoPort *typecastInPort = uncollapsedNode->getBase()->getInputPorts()[VuoNodeClass::unreservedInputPortStartIndex];
	VuoPort *typecastOutPort = uncollapsedNode->getBase()->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];

	vector<VuoCable *> inputCables = typecastInPort->getConnectedCables(false);
	VuoCable *outCable = *(typecastOutPort->getConnectedCables(false).begin());
	VuoPort *reroutedToPort = outCable->getToPort();

	for (vector<VuoCable *>::iterator inputCable = inputCables.begin(); inputCable != inputCables.end(); ++inputCable)
	{
		VuoRendererCable *inputRc = (*inputCable)->getRenderer();
		if (!inputRc->effectivelyCarriesData())
		{
			revertedFromPortForCable[inputRc] = inputRc->getBase()->getFromPort();
			revertedToPortForCable[inputRc] = inputRc->getBase()->getToPort();

			updatedFromPortForCable[inputRc] = inputRc->getBase()->getFromPort();
			updatedToPortForCable[inputRc] = reroutedToPort;

			reroutedCables.insert(inputRc);
		}
	}

	// All incoming cables have now been marked for re-routing (if event-only) or deletion
	// (if deletion of the incoming data+event cable is what triggered deletion of this typecast), so in
	// deleting the newly uncollapsed typecast node, be sure to tell inventoryNodeAndDependentCables()
	// not to blindly mark all of its incoming cables for deletion.
	inventoryNodeAndDependentCables(uncollapsedNode, triggeredByIncomingCableDeletion);
}

/**
 * Helper function for the VuoCommandRemove constructor and VuoCommandRemove::inventoryTypecastAndDependentCables().
 * Marks the input VuoRendererNode* @c rn for deletion.
 * Marks any connected cables for deletion as well.
 */
void VuoCommandRemove::inventoryNodeAndDependentCables(VuoRendererNode *rn, bool inputCablesPreprocessed)
{
	// If this node has already been marked for deletion, do nothing.
	if (removedNodes.find(rn) != removedNodes.end())
	{
		return;
	}

	// Mark the node for deletion.
	removedNodes.insert(rn);

	// Mark the node's connected cables for deletion.
	if (! inputCablesPreprocessed)
	{
		set<VuoCable *> connectedInputCables = rn->getConnectedInputCables(true);
		for (set<VuoCable *>::iterator cable = connectedInputCables.begin(); cable != connectedInputCables.end(); ++cable)
		{
			VuoRendererCable *rc = (*cable)->getRenderer();
			inventoryCableAndDependentTypecasts(rc);
		}
	}

	set<VuoCable *> connectedOutputCables = rn->getConnectedOutputCables(true);
	for (set<VuoCable *>::iterator cable = connectedOutputCables.begin(); cable != connectedOutputCables.end(); ++cable)
	{
		VuoRendererCable *rc = (*cable)->getRenderer();
		inventoryCableAndDependentTypecasts(rc);
	}
}

/**
 * Helper function for the VuoCommandRemove constructor.
 * Returns the set of attachments dependent on the provided node, meaning that if
 * the node is deleted, the attachments should be as well.  These may include
 * upstream and/or sibling attachments.
 */
set<QGraphicsItem *> VuoCommandRemove::getAttachmentsDependentOnNode(VuoRendererNode *rn)
{
	set<QGraphicsItem *> dependentAttachments;

	// Get upstream attachments.
	vector<VuoPort *> inputPorts = rn->getBase()->getInputPorts();
	for(unsigned int i = 0; i < inputPorts.size(); ++i)
	{
		set<VuoRendererInputAttachment *> portUpstreamAttachments = inputPorts[i]->getRenderer()->getAllUnderlyingUpstreamInputAttachments();
		dependentAttachments.insert(portUpstreamAttachments.begin(), portUpstreamAttachments.end());
	}

	// Get co-attachments.
	VuoRendererInputAttachment *nodeAsAttachment = dynamic_cast<VuoRendererInputAttachment *>(rn);
	if (nodeAsAttachment)
	{
		foreach (VuoNode *coattachment, nodeAsAttachment->getCoattachments())
			dependentAttachments.insert(coattachment->getRenderer());
	}

	return dependentAttachments;
}

/**
 * Helper function for the VuoCommandRemove constructor.
 * Determines whether the deletion of provided @c cable will create the need for
 * the creation and connection of a new "Make List" node to the cable's 'To' port;
 * if so, creates the necessary components.
 */
void VuoCommandRemove::prepareMakeListToReplaceDeletedCable(VuoRendererCable *rc)
{
	if (! (rc->getBase()->hasCompiler() && rc->effectivelyCarriesData()))
		return;

	VuoPort *toPort = rc->getBase()->getToPort();

	// Case: The cable had already been disconnected from its 'To' port at the time it was deleted.
	if (! toPort)
		toPort = rc->getFloatingEndpointPreviousToPort();

	if (!(toPort && toPort->hasRenderer() && toPort->getRenderer()->getUnderlyingParentNode()))
		return;

	VuoNode *toNode = toPort->getRenderer()->getUnderlyingParentNode()->getBase();

	// Automatically insert an input list drawer if appropriate.
	if (removedNodes.find(toNode->getRenderer()) == removedNodes.end())
	{
		VuoCompilerInputEventPort *inputEventPort = (toPort->hasCompiler()? dynamic_cast<VuoCompilerInputEventPort *>(toPort->getCompiler()) : NULL);
		if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()))
		{
			VuoRendererCable *cable = NULL;
			VuoRendererNode *makeListNode = window->getComposition()->createAndConnectMakeListNode(toNode, toPort, cable);

			addedNodes.insert(makeListNode);
			addedCables.insert(cable);
			updatedFromPortForCable[cable] = cable->getBase()->getFromPort();
			updatedToPortForCable[cable] = cable->getBase()->getToPort();
		}

		// Automatically insert an input read-only dictionary attachment if appropriate.
		else if (VuoStringUtilities::beginsWith(toNode->getNodeClass()->getClassName(), "vuo.math.calculate") &&
				 (toPort->getClass()->getName() == "values"))
		{
			set<VuoRendererNode *> createdNodes;
			set<VuoRendererCable *> createdCables;
			window->getComposition()->createAndConnectDictionaryAttachmentsForNode(toNode, createdNodes, createdCables);

			foreach (VuoRendererNode *node, createdNodes)
				addedNodes.insert(node);
			foreach (VuoRendererCable *cable, createdCables)
			{
				addedCables.insert(cable);
				updatedFromPortForCable[cable] = cable->getBase()->getFromPort();
				updatedToPortForCable[cable] = cable->getBase()->getToPort();
			}
		}
	}
}

/**
 * Helper function for the VuoCommandRemove constructor.
 * Returns a boolean indicating whether this operation involves
 * modifications to generic ports.
 */
bool VuoCommandRemove::modifiedComponentsIncludeGenericPorts()
{
	foreach (VuoRendererNode *rn, removedNodes)
	{
		if (rn->hasGenericPort())
			return true;
	}

	foreach (VuoRendererNode *rn, addedNodes)
	{
		if (rn->hasGenericPort())
			return true;
	}

	foreach (VuoRendererCable *rc, removedCables)
	{
		bool revertedFromPortIsGeneric = (revertedFromPortForCable[rc] && dynamic_cast<VuoGenericType *>(revertedFromPortForCable[rc]->getRenderer()->getDataType()));
		bool revertedToPortIsGeneric = (revertedToPortForCable[rc] && dynamic_cast<VuoGenericType *>(revertedToPortForCable[rc]->getRenderer()->getDataType()));

		if (revertedFromPortIsGeneric || revertedToPortIsGeneric)
			return true;
	}

	foreach (VuoRendererCable *rc, addedCables)
	{
		bool updatedFromPortIsGeneric = (updatedFromPortForCable[rc] && dynamic_cast<VuoGenericType *>(updatedFromPortForCable[rc]->getRenderer()->getDataType()));
		bool updatedToPortIsGeneric = (updatedToPortForCable[rc] && dynamic_cast<VuoGenericType *>(updatedToPortForCable[rc]->getRenderer()->getDataType()));

		if (updatedFromPortIsGeneric || updatedToPortIsGeneric)
			return true;
	}

	foreach (VuoRendererCable *rc, reroutedCables)
	{
		bool revertedFromPortIsGeneric = (revertedFromPortForCable[rc] && dynamic_cast<VuoGenericType *>(revertedFromPortForCable[rc]->getRenderer()->getDataType()));
		bool revertedToPortIsGeneric = (revertedToPortForCable[rc] && dynamic_cast<VuoGenericType *>(revertedToPortForCable[rc]->getRenderer()->getDataType()));
		bool updatedFromPortIsGeneric = (updatedFromPortForCable[rc] && dynamic_cast<VuoGenericType *>(updatedFromPortForCable[rc]->getRenderer()->getDataType()));
		bool updatedToPortIsGeneric = (updatedToPortForCable[rc] && dynamic_cast<VuoGenericType *>(updatedToPortForCable[rc]->getRenderer()->getDataType()));

		if (revertedFromPortIsGeneric || revertedToPortIsGeneric || updatedFromPortIsGeneric || updatedToPortIsGeneric)
			return true;
	}

	return false;
}
