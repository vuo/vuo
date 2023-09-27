/**
 * @file
 * VuoCommandCommon implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoEditor.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoNodeClass.hh"
#include "VuoRendererCable.hh"
#include "VuoSubcompositionMessageRouter.hh"
#include "VuoPublishedPort.hh"

/**
 * Helper function for VuoCommandAdd::redo() and VuoCommandRemove::undo().
 * Adds a cable to the canvas.
 */
void VuoCommandCommon::addCable(VuoRendererCable *rc, VuoPort *fromPortAfterAdding, VuoPort *toPortAfterAdding,
								VuoEditorComposition *composition)
{
	// Case: The cable had already been disconnected from its 'To' port at the time it was deleted.
	if (! toPortAfterAdding)
		toPortAfterAdding = rc->getFloatingEndpointPreviousToPort();

	bool willBePublishedInputCable = dynamic_cast<VuoPublishedPort *>(fromPortAfterAdding);
	bool willBePublishedOutputCable = dynamic_cast<VuoPublishedPort *>(toPortAfterAdding);

	VuoNode *fromNode = (willBePublishedInputCable?
							 composition->getPublishedInputNode() :
							 fromPortAfterAdding->getRenderer()->getUnderlyingParentNode()->getBase()
						);
	VuoNode *toNode = (willBePublishedOutputCable?
						   composition->getPublishedOutputNode() :
						   toPortAfterAdding->getRenderer()->getUnderlyingParentNode()->getBase()
						   );

	rc->setFrom(fromNode, fromPortAfterAdding);
	rc->setTo(toNode, toPortAfterAdding);

	composition->addCable(rc->getBase());
}

/**
 * Helper function for VuoCommandRemove::redo() and VuoCommandAdd::undo().
 * Removes a cable from the canvas.
 */
void VuoCommandCommon::removeCable(VuoRendererCable *rc, VuoEditorComposition *composition)
{
	if (rc->scene() != composition)
		return;

	composition->removeCable(rc);
}

/**
 * Helper function for VuoCommandConnect::redo(), VuoCommandConnect::undo(),
 * VuoCommandRemove::redo(), and VuoCommandRemove::undo().
 * Connects the input cable @c rc to the 'From' and 'To' ports specified
 * in the input maps @c fromPortForCable and @c toPortForCable, respectively.
 *
 * Note: This function should be called *after* any calls to addNode() or removeNode() that
 * might affect the cable's endpoints, to ensure that the cable is added to or removed from
 * the composition as appropriate.
 */
void VuoCommandCommon::updateCable(VuoRendererCable *rc, VuoPort *updatedFromPort, VuoPort *updatedToPort,
									VuoEditorComposition *composition, bool preserveDanglingCables)
{
	VuoNode *updatedFromNode = (updatedFromPort? composition->getUnderlyingParentNodeForPort(updatedFromPort, composition):NULL);
	VuoNode *updatedToNode = (updatedToPort? composition->getUnderlyingParentNodeForPort(updatedToPort, composition):NULL);

	// Perform port updates
	rc->setFrom(updatedFromNode, updatedFromPort);
	rc->setTo(updatedToNode, updatedToPort);

	// Add cable to composition if appropriate
	if (updatedFromPort && updatedToPort &&
		compositionContainsNode(composition, updatedFromNode) &&
		compositionContainsNode(composition, updatedToNode) &&
		(rc->scene() != composition))
	{
		composition->addCable(rc->getBase());
	}

	// Remove cable from composition if appropriate
	else if ((!	(preserveDanglingCables ||
				(updatedFromPort && updatedToPort &&
				compositionContainsNode(composition, updatedFromNode) &&
				compositionContainsNode(composition, updatedToNode)))
			  )
			 && (rc->scene() == composition))
	{
		composition->removeCable(rc);
	}
}

/**
 * Returns a boolean indicating whether the provided @c composition currently contains the provided @c node.
 */
bool VuoCommandCommon::compositionContainsNode(VuoEditorComposition *composition, VuoNode *node)
{
	if (node->hasRenderer() && node->getRenderer()->scene() == composition)
		return true;

	else if (node->getNodeClass()->getClassName() == VuoNodeClass::publishedInputNodeClassName)
		return true;

	else if (node->getNodeClass()->getClassName() == VuoNodeClass::publishedOutputNodeClassName)
		return true;

	return false;
}

/**
 * Publishes the @c internalPort under externally visible name @c publishedPortName,
 * or under a derived name if an externally visible published port with that name
 * already exists and @c attemptMerge is set to false.
 *
 * @param internalPort The port to publish.
 * @param forceEventOnlyPublication A boolean indicating whether the cable connecting the internal and external ports
 * should be event-only regardless of whether the ports themselves carry data.
 * @param publishedPortName The externally-visible name to give the new published port.
 * @param composition The composition in which to publish the port.
 * @param attemptMerge A boolean indicating whether the port should be published in association
 * with a pre-existing (rather than a newly created) external published port of the given name, if possible.
 * @return A pointer to the resulting externally visible VuoPublishedPort object.
 */
VuoPublishedPort * VuoCommandCommon::publishInternalPort(VuoPort *internalPort, bool forceEventOnlyPublication, string publishedPortName, VuoEditorComposition *composition, bool attemptMerge)
{
	internalPort->getRenderer()->updateGeometry();
	VuoType *publishedPortType = (forceEventOnlyPublication? NULL : ((VuoCompilerPortClass *)(internalPort->getClass()->getCompiler()))->getDataVuoType());

	bool mergePerformed = false;
	VuoRendererPublishedPort *externalPort = composition->publishInternalPort(internalPort,
																			  forceEventOnlyPublication,
																			  publishedPortName,
																			  publishedPortType,
																			  attemptMerge,
																			  &mergePerformed);
	if (!mergePerformed)
		composition->emitPublishedPortNameEditorRequested(externalPort);

	return dynamic_cast<VuoPublishedPort *>(externalPort->getBase());
}

/**
 * Publishes the @c internalPort in association with pre-existing externally
 * visible published port @c externalPort, if possible.  If impossible (due, e.g.,
 * to conflicting types), publishes the internal port under a new name.
 * @return A pointer to the resulting externally visible VuoPublishedPort object.
 */
VuoPublishedPort * VuoCommandCommon::publishInternalExternalPortCombination(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoEditorComposition *composition)
{
	bool isInput = internalPort->getRenderer()->getInput();
	composition->addPublishedPort(externalPort, isInput);

	// @todo Make use of retrieved mergePerformed value? https://b33p.net/node/7659
	bool mergePerformed;
	VuoPublishedPort *actualExternalPort = dynamic_cast<VuoPublishedPort *>(composition->publishInternalPort(internalPort, forceEventOnlyPublication,
																	externalPort->getClass()->getName(),
																	static_cast<VuoCompilerPortClass *>(externalPort->getClass()->getCompiler())->getDataVuoType(),
																	true, &mergePerformed)->getBase());

	return actualExternalPort;
}

/**
 * Unpublishes the given @c internalPort in association with externally visible published
 * port @c externalPort.
 * If this leaves the external published port without connected internal ports and
 * @c unpublishIsolatedExternalPorts is true, the external port is unpublished as well.
 */
void VuoCommandCommon::unpublishInternalExternalPortCombination(VuoPort *internalPort, VuoPublishedPort *externalPort, VuoEditorComposition *composition, bool unpublishIsolatedExternalPorts)
{
	bool isInput = internalPort->getRenderer()->getInput();
	internalPort->getRenderer()->updateGeometry();

	// Remove the published cable associated with this internal-external port combination, if it has not been
	// removed already.
	VuoCable *publishedCable = internalPort->getCableConnecting(externalPort);
	if (!publishedCable)
	{
		// The cable might have been disconnected by dragging.  Look for a dangling cable that fits the description.
		if (isInput)
		{
		VuoCable *danglingCable = externalPort->getCableConnecting(NULL);
		if (danglingCable && (danglingCable->getRenderer()->getFloatingEndpointPreviousToPort() == internalPort))
			publishedCable = danglingCable;
		}
		else // if (!isInput)
		{
			VuoCable *danglingCable = internalPort->getCableConnecting(NULL);
			if (danglingCable && (danglingCable->getRenderer()->getFloatingEndpointPreviousToPort() == externalPort))
				publishedCable = danglingCable;
		}
	}

	if (publishedCable)
		composition->removeCable(publishedCable->getRenderer());

	bool noConnectedPorts = externalPort->getConnectedCables().empty();
	if (unpublishIsolatedExternalPorts && noConnectedPorts)
		composition->removePublishedPort(externalPort, isInput);
}

/**
 * Adds information about a single node replacement and associated port mappings to a VuoCompositionDiff.
 * Used for live-coding updates, since some information can't be inferred from the composition snapshot:
 * - the fact that a node was replaced if its node class hasn't changed;
 * - for `Calculate` nodes, the mapping from old to new ports on the `Make List` node that holds the variable values.
 */
VuoCompilerCompositionDiff * VuoCommandCommon::addNodeReplacementToDiff(VuoCompilerCompositionDiff *diffInfo,
																		VuoRendererNode *oldNode, VuoRendererNode *newNode,
																		map<VuoPort *, VuoPort *> updatedPortForOriginalPort,
																		VuoEditorComposition *composition)
{
		if (!oldNode->getBase()->hasCompiler() || !newNode->getBase()->hasCompiler())
			return diffInfo;

		// Tell VuoCompositionDiff about the node replacements, since some information can't be inferred from the composition snapshot:
		//  - the fact that a node was replaced if its node class hasn't changed
		//  - for `Calculate` nodes, the mapping from old to new ports on the `Make List` node that holds the variable values
		map<string, string> oldAndNewPortNames;
		for (VuoPort *oldPort : oldNode->getBase()->getInputPorts())
		{
			VuoPort *newPort = updatedPortForOriginalPort[oldPort];
			if (newPort)
			{
				string oldPortName = oldPort->getClass()->getName();
				string newPortName = newPort->getClass()->getName();
				oldAndNewPortNames[oldPortName] = newPortName;
			}
		}

		string oldNodeIdentifier = oldNode->getBase()->getCompiler()->getIdentifier();
		string newNodeIdentifier = newNode->getBase()->getCompiler()->getIdentifier();

		string compositionIdentifier = static_cast<VuoEditor *>(qApp)->getSubcompositionRouter()->getCompositionIdentifier(composition);
		diffInfo->addNodeReplacement(compositionIdentifier, oldNodeIdentifier, newNodeIdentifier, oldAndNewPortNames);

		return diffInfo;
}

/**
 * Creates a new undoable command.
 */
VuoCommandCommon::VuoCommandCommon(VuoEditorWindow *window)
{
	this->window = window;
	this->description = nullptr;
}

/**
 * Destructor.
 */
VuoCommandCommon::~VuoCommandCommon()
{
	free(description);
}

/**
 * Formats and stores a description, to be shown by @ref VuoCommandCommon_redo and @ref VuoCommandCommon_undo.
 */
void VuoCommandCommon::setDescription(const char *formatString, ...)
{
	va_list args;
	va_start(args, formatString);
	vasprintf(&description, formatString, args);
	va_end(args);
}
