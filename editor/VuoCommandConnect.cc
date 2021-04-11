/**
 * @file
 * VuoCommandConnect implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandConnect.hh"

#include "VuoCable.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerType.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoInputEditorManager.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoRendererCable.hh"
#include "VuoRendererPort.hh"
#include "VuoRendererPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoGenericType.hh"

/**
 * Creates a command for connecting a cable to a port.
 */
VuoCommandConnect::VuoCommandConnect(VuoRendererCable *cableInProgress,
									 VuoRendererPort *targetPort,
									 VuoRendererCable *displacedCable,
									 VuoRendererPort *portToUnpublish,
									 VuoEditorWindow *window,
									 VuoInputEditorManager *inputEditorManager)
	: VuoCommandCommon(window)
{
	setText(QApplication::translate("VuoEditorWindow", "Cable Connection"));
	this->window = window;

	// Normally we would take the composition's "Before" snapshot here, but in the case
	// of a cable connection we need to reconstruct what the composition looked like before
	// the cable drag began, so we do this a little bit later within the constructor.

	this->cableInProgress = cableInProgress;
	this->displacedCable = displacedCable;
	this->fromPortForAddedCable = NULL;
	this->toPortForAddedCable = NULL;
	this->addedNode = NULL;
	this->inputEditorManager = inputEditorManager;

	// @todo: Let cable deletion handle port unpublication.
	VuoPort *internalPortToUnpublish = (portToUnpublish? portToUnpublish->getBase() : NULL);
	vector<VuoRendererPublishedPort *> externalPortsToUnpublish = (portToUnpublish? portToUnpublish->getPublishedPortsConnectedByDataCarryingCables() : vector<VuoRendererPublishedPort *>());

	// Start of command content.
	{
		VuoEditorComposition *composition = window->getComposition();
		VuoRendererCable *addedCable = NULL;

		// Completing a connection to an input port
		if (targetPort->getInput())
		{
			// Re-connecting an existing cable to a new input port
			if (cableInProgress->getFloatingEndpointPreviousToPort() &&
					cableInProgress->getFloatingEndpointPreviousToPort()->hasRenderer() &&
					cableInProgress->getFloatingEndpointPreviousToPort()->getRenderer()->scene())
			{
				revertedFromPortForCable[cableInProgress] = cableInProgress->getBase()->getFromPort();
				revertedToPortForCable[cableInProgress] = cableInProgress->getFloatingEndpointPreviousToPort();

				// Mark the previous 'To' port's constant value to be updated to match
				// the port's last value in the running composition, if applicable.
				if (cableInProgress->effectivelyCarriesData())
				{
					VuoPort *toPort = cableInProgress->getBase()->getToPort();
					if (!toPort)
						toPort = cableInProgress->getFloatingEndpointPreviousToPort();

					if (toPort && toPort->hasRenderer())
					{
						VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>(toPort->getCompiler());
						if (eventPort)
						{
							if (inputEditorManager->doesTypeAllowOfflineSerialization(eventPort->getDataVuoType()))
							{
								revertedConstantForPort[toPort] = eventPort->getData()->getInitialValue();

								json_object *currentRunningValue = composition->getPortValueInRunningComposition(toPort);
								updatedConstantForPort[toPort] = (currentRunningValue? json_object_to_json_string_ext(currentRunningValue, JSON_C_TO_STRING_PLAIN) :
																					   revertedConstantForPort[toPort]);
							}
						}
					}
				}
			}

			// Connecting the cable for the first time
			else
			{
				revertedFromPortForCable[cableInProgress] = NULL;
				revertedToPortForCable[cableInProgress] = NULL;
			}

			updatedFromPortForCable[cableInProgress] = cableInProgress->getBase()->getFromPort();
			updatedToPortForCable[cableInProgress] = targetPort->getBase();

			// Displacing some other cable
			if (displacedCable)
			{
				revertedFromPortForCable[displacedCable] = displacedCable->getBase()->getFromPort();
				revertedToPortForCable[displacedCable] = displacedCable->getBase()->getToPort();

				updatedFromPortForCable[displacedCable] = NULL;
				updatedToPortForCable[displacedCable] = NULL;
			}
		}

		// Completing a ("backwards") connection to an output port
		else
		{
			// Only possibility: Connecting the cable for the first time
			revertedFromPortForCable[cableInProgress] = NULL;
			revertedToPortForCable[cableInProgress] = NULL;

			updatedFromPortForCable[cableInProgress] = targetPort->getBase();
			updatedToPortForCable[cableInProgress] = cableInProgress->getBase()->getToPort();
		}

		// Reconstruct the state of the composition before the beginning of the cable drag
		// that concluded with this connection, for the composition's "Before" snapshot.
		{
			VuoPort *currentFromPortForCableInProgress = cableInProgress->getBase()->getFromPort();
			VuoPort *currentToPortForCableInProgress = cableInProgress->getBase()->getToPort();
			bool currentAlwaysEventOnlyStatusForCableInProgress = cableInProgress->getBase()->getCompiler()->getAlwaysEventOnly();

			bool mustReconstructRevertedSnapshot = ((currentFromPortForCableInProgress != revertedFromPortForCable[cableInProgress]) ||
													(currentToPortForCableInProgress != revertedToPortForCable[cableInProgress]));

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

		// Connect a "Make List" node if the connection leaves a list input port without an incoming data cable.
		if (targetPort->getInput())
		{
			VuoPort *previousInputPort = cableInProgress->getFloatingEndpointPreviousToPort();
			if (previousInputPort && previousInputPort->hasRenderer() && previousInputPort->getRenderer()->scene() &&
				(previousInputPort != targetPort->getBase()))
			{
				VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(previousInputPort->getCompiler());
				if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()) && !inputEventPort->hasConnectedDataCable())
				{
					VuoNode *toNode = previousInputPort->getRenderer()->getUnderlyingParentNode()->getBase();
					VuoRendererCable *cable = NULL;
					VuoRendererNode *makeListNode = composition->createAndConnectMakeListNode(toNode, previousInputPort, cable);

					addedNode = makeListNode;
					addedCable = cable;
					fromPortForAddedCable = addedCable->getBase()->getFromPort();
					toPortForAddedCable = previousInputPort;
				}
			}
		}

		// If connecting the first data+event cable from the output port of a "Share Value" node to an input port
		// with a constant value, propagate the constant value back to the "Share Value" node's input port.
		inventorySharedValueToBackpropagate();

		this->operationInvolvesGenericPort = modifiedComponentsIncludeGenericPorts();

		// Disconnect the displaced cable, if applicable.
		if (displacedCable)
			VuoCommandCommon::updateCable(displacedCable, updatedFromPortForCable[displacedCable], updatedToPortForCable[displacedCable], composition);

		// Unpublish the published port, if applicable.
		if (internalPortToUnpublish)
		{
			bool unpublishIsolatedExternalPorts = false;
			foreach (VuoRendererPublishedPort *externalPortToUnpublish, externalPortsToUnpublish)
				VuoCommandCommon::unpublishInternalExternalPortCombination(internalPortToUnpublish, dynamic_cast<VuoPublishedPort *>(externalPortToUnpublish->getBase()), composition, unpublishIsolatedExternalPorts);
		}

		// Connect the new cable
		VuoCommandCommon::updateCable(cableInProgress, updatedFromPortForCable[cableInProgress], updatedToPortForCable[cableInProgress], composition);

		// Add the "Make List" node and cable, if applicable.
		if (addedNode)
		{
			VuoCommandCommon::addCable(addedCable, fromPortForAddedCable, toPortForAddedCable, composition);
			composition->addNode(addedNode->getBase());
		}

		// Collapse any typecasts possible.
		composition->collapseTypecastNodes();

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

	setDescription("Connect %s:%s -> %s:%s",
				   cableInProgress->getBase()->getFromNode() && cableInProgress->getBase()->getFromNode()->hasCompiler()?
					   cableInProgress->getBase()->getFromNode()->getCompiler()->getIdentifier().c_str() : "?",
				   cableInProgress->getBase()->getFromPort()?
					   cableInProgress->getBase()->getFromPort()->getClass()->getName().c_str() : "?",
				   cableInProgress->getBase()->getToNode() && cableInProgress->getBase()->getToNode()->hasCompiler()?
					   cableInProgress->getBase()->getToNode()->getCompiler()->getIdentifier().c_str() : "?",
				   cableInProgress->getBase()->getToPort()?
					   cableInProgress->getBase()->getToPort()->getClass()->getName().c_str() : "?");
}

/**
 * Returns the ID of this command.
 */
int VuoCommandConnect::id() const
{
	return VuoCommandCommon::connectCommandID;
}

/**
 * Disconnects the connected cable, restoring any cables that were previously connected to the port.
 */
void VuoCommandConnect::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);

	foreach (string updatedPortID, updatedPortIDs)
		window->coalesceInternalPortConstantsToSync(updatedPortID);
}

/**
 * Connects the cable, displacing any cables currently connected to the port.
 */
void VuoCommandConnect::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);

	foreach (string updatedPortID, updatedPortIDs)
		window->coalesceInternalPortConstantsToSync(updatedPortID);
}

/**
 * Helper function for the VuoCommandConnect constructor.
 * Returns a boolean indicating whether this operation involves
 * modifications to generic ports.
 */
bool VuoCommandConnect::modifiedComponentsIncludeGenericPorts()
{
	bool revertedFromPortIsGeneric = (revertedFromPortForCable[cableInProgress] && dynamic_cast<VuoGenericType *>(revertedFromPortForCable[cableInProgress]->getRenderer()->getDataType()));
	bool revertedToPortIsGeneric = (revertedToPortForCable[cableInProgress] && dynamic_cast<VuoGenericType *>(revertedToPortForCable[cableInProgress]->getRenderer()->getDataType()));
	bool updatedFromPortIsGeneric = (updatedFromPortForCable[cableInProgress] && dynamic_cast<VuoGenericType *>(updatedFromPortForCable[cableInProgress]->getRenderer()->getDataType()));
	bool updatedToPortIsGeneric = (updatedToPortForCable[cableInProgress] && dynamic_cast<VuoGenericType *>(updatedToPortForCable[cableInProgress]->getRenderer()->getDataType()));

	bool revertedDisplacedFromPortIsGeneric = (revertedFromPortForCable[displacedCable] && dynamic_cast<VuoGenericType *>(revertedFromPortForCable[displacedCable]->getRenderer()->getDataType()));
	bool revertedDisplacedToPortIsGeneric = (revertedToPortForCable[displacedCable] && dynamic_cast<VuoGenericType *>(revertedToPortForCable[displacedCable]->getRenderer()->getDataType()));
	bool updatedDisplacedFromPortIsGeneric = (updatedFromPortForCable[displacedCable] && dynamic_cast<VuoGenericType *>(updatedFromPortForCable[displacedCable]->getRenderer()->getDataType()));
	bool updatedDisplacedToPortIsGeneric = (updatedToPortForCable[displacedCable] && dynamic_cast<VuoGenericType *>(updatedToPortForCable[displacedCable]->getRenderer()->getDataType()));

	bool addedNodeIsGeneric = (addedNode && addedNode->hasGenericPort());
	bool fromPortForAddedCableIsGeneric = (fromPortForAddedCable && dynamic_cast<VuoGenericType *>(fromPortForAddedCable->getRenderer()->getDataType()));
	bool toPortForAddedCableIsGeneric = (toPortForAddedCable && dynamic_cast<VuoGenericType *>(toPortForAddedCable->getRenderer()->getDataType()));

	return (revertedFromPortIsGeneric || revertedToPortIsGeneric || updatedFromPortIsGeneric || updatedToPortIsGeneric ||
			revertedDisplacedFromPortIsGeneric || revertedDisplacedToPortIsGeneric || updatedDisplacedFromPortIsGeneric || updatedDisplacedToPortIsGeneric ||
			addedNodeIsGeneric || fromPortForAddedCableIsGeneric || toPortForAddedCableIsGeneric);
}

/**
 * Helper function for the VuoCommandConnect constructor.
 * Determines whether the cable connection should include backpropagation of
 * a constant value to the input port of a newly connected upstream
 * "Share Value" node; updates the `revertedConstantForPort`
 * and `updatedConstantForPort` data structures appropriately.
 *
 * The `updatedFromPortForCable` and `updatedToPortForCable` maps are expected to
 * have been populated before this function is called.
 */
void VuoCommandConnect::inventorySharedValueToBackpropagate()
{

	VuoPort *fromPort = updatedFromPortForCable[cableInProgress];
	VuoPort *toPort = updatedToPortForCable[cableInProgress];
	bool fromPortIsSharedValue = (fromPort &&
								  VuoStringUtilities::beginsWith(fromPort->getRenderer()->getUnderlyingParentNode()->getBase()->getNodeClass()->getClassName(),
															"vuo.data.share."));
	bool toPortHasCopyableConstant = (toPort &&
									  toPort->getRenderer()->getDataType() &&
									  toPort->hasCompiler() &&
									  inputEditorManager->doesTypeAllowOfflineSerialization(static_cast<VuoCompilerInputEventPort *>(toPort->getCompiler())->getDataVuoType()));
	bool isReconnection = cableInProgress->getFloatingEndpointPreviousToPort();

	bool fromPortHasOtherDataConnections = false;
	if (fromPort)
	{
		vector<VuoCable *> connectedCables = fromPort->getConnectedCables(true);
		foreach (VuoCable *cable, connectedCables)
		{
			if (cable->hasRenderer() &&
					(cable->getRenderer() != cableInProgress) &&
					(cable->getRenderer()->effectivelyCarriesData()))
			{
				fromPortHasOtherDataConnections = true;
				break;
			}
		}
	}

	VuoPort *shareValueInputPort = (fromPort? fromPort->getRenderer()->getUnderlyingParentNode()->getBase()->getInputPortWithName("value") : NULL);
	bool shareValueInputPortConstantNotYetSet = shareValueInputPort &&
												shareValueInputPort->getRenderer()->isConstant() &&
												shareValueInputPort->getRenderer()->getConstantAsString().empty();

	if (fromPortIsSharedValue && toPortHasCopyableConstant && !isReconnection && !fromPortHasOtherDataConnections && cableInProgress->effectivelyCarriesData() && shareValueInputPortConstantNotYetSet)
	{
		revertedConstantForPort[shareValueInputPort] = shareValueInputPort->getRenderer()->getConstantAsString();
		updatedConstantForPort[shareValueInputPort] = toPort->getRenderer()->getConstantAsString();
	}
}
