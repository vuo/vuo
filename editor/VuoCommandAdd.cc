/**
 * @file
 * VuoCommandAdd implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCommandCommon.hh"
#include "VuoCommandAdd.hh"

#include "VuoComment.hh"
#include "VuoCompilerComment.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerType.hh"
#include "VuoRendererComment.hh"
#include "VuoEditorComposition.hh"
#include "VuoEditorWindow.hh"
#include "VuoNodeClass.hh"
#include "VuoGenericType.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a command for adding the given nodes, cables, and/or comments.
 */
VuoCommandAdd::VuoCommandAdd(QList<QGraphicsItem *> addedComponents, VuoEditorWindow *window, string commandDescription, bool disableAttachmentInsertion)
	: VuoCommandCommon(window)
{
	setText(commandDescription.c_str());
	this->window = window;
	this->revertedSnapshot = window->getComposition()->takeSnapshot();
	vector<string> addedComponentDescriptions;

	// Start of command content.
	{
		VuoEditorComposition *composition = window->getComposition();
		set<VuoRendererNode *> addedNodes;
		set<VuoRendererCable *> addedCables;
		map<VuoRendererCable *, VuoPort *> fromPortForCable;
		map<VuoRendererCable *, VuoPort *> toPortForCable;
		set<VuoRendererComment *> addedComments;

		for (QList<QGraphicsItem *>::iterator i = addedComponents.begin(); i != addedComponents.end(); ++i)
		{
			QGraphicsItem *compositionComponent = *i;
			VuoRendererNode *rn = dynamic_cast<VuoRendererNode *>(compositionComponent);
			if (rn)
			{
				addedNodes.insert(rn);
				addedComponentDescriptions.push_back("node "
					+ (rn->getBase()->hasCompiler()
						? rn->getBase()->getCompiler()->getIdentifier()
						: "\"" + rn->getBase()->getTitle() + "\"")
					+ " (" + rn->getBase()->getNodeClass()->getClassName() + ")");
			}
			else
			{
				VuoRendererCable *rc = dynamic_cast<VuoRendererCable *>(compositionComponent);
				if (rc)
				{
					addedCables.insert(rc);
					fromPortForCable[rc] = rc->getBase()->getFromPort();
					toPortForCable[rc] = rc->getBase()->getToPort();

					addedComponentDescriptions.push_back("cable "
						+ (rc->getBase()->getFromNode()->hasCompiler()
							? rc->getBase()->getFromNode()->getCompiler()->getIdentifier()
							: "?")
						+ ":" + rc->getBase()->getFromPort()->getClass()->getName()
						+ " -> "
						+ (rc->getBase()->getToNode()->hasCompiler()
							? rc->getBase()->getToNode()->getCompiler()->getIdentifier()
							: "?")
						+ ":" + rc->getBase()->getToPort()->getClass()->getName());
				}
				else
				{
					VuoRendererComment *rcomment = dynamic_cast<VuoRendererComment *>(compositionComponent);
					if (rcomment)
					{
						addedComments.insert(rcomment);
						addedComponentDescriptions.push_back(rcomment->getBase()->getCompiler()->getGraphvizIdentifier());
					}
				}
			}
		}

		// Insert appropriate input port attachments.
		if (! disableAttachmentInsertion)
		{
			foreach (VuoRendererNode *rendererNode, addedNodes)
			{
				VuoNode *node = rendererNode->getBase();

				if (! node->getNodeClass()->hasCompiler())
					continue;

				// If this is a "Calculate" node, insert and connect the components that it needs
				// to provide its read-only input keys/values dictionary.
				if (VuoStringUtilities::beginsWith(node->getNodeClass()->getClassName(), "vuo.math.calculate"))
				{
					VuoPort *valuesInputPort = node->getInputPortWithName("values");
					if (valuesInputPort)
					{
						bool hasIncomingDataCable = false;
						vector<VuoCable *> connectedCables = valuesInputPort->getConnectedCables(true);
						foreach (VuoRendererCable *addedCable, addedCables)
							if (addedCable->getBase()->getToPort() == valuesInputPort)
								connectedCables.push_back(addedCable->getBase());
						foreach (VuoCable *connectedCable, connectedCables)
							if (static_cast<VuoCompilerPortClass *>(connectedCable->getFromPort()->getClass()->getCompiler())->getDataVuoType())
								hasIncomingDataCable = true;

						if (! hasIncomingDataCable)
						{
							set<VuoRendererNode *> nodesToAdd;
							set<VuoRendererCable *> cablesToAdd;
							composition->createAndConnectDictionaryAttachmentsForNode(node, nodesToAdd, cablesToAdd);

							addedNodes.insert(nodesToAdd.begin(), nodesToAdd.end());
							addedCables.insert(cablesToAdd.begin(), cablesToAdd.end());

							foreach (VuoRendererCable *cable, cablesToAdd)
							{
								fromPortForCable[cable] = cable->getBase()->getFromPort();
								toPortForCable[cable] = cable->getBase()->getToPort();
							}
						}
					}
				}

				// Connect a "Make List" node to each list input port with no incoming data cable.
				{
					foreach (VuoPort *port, node->getInputPorts())
					{
						VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>(port->getCompiler());
						if (inputEventPort && VuoCompilerType::isListType(inputEventPort->getDataType()))
						{
							bool hasIncomingDataCable = false;
							vector<VuoCable *> connectedCables = port->getConnectedCables(true);
							foreach (VuoRendererCable *addedCable, addedCables)
								if (addedCable->getBase()->getToPort() == port)
									connectedCables.push_back(addedCable->getBase());
							foreach (VuoCable *connectedCable, connectedCables)
								if (static_cast<VuoCompilerPortClass *>(connectedCable->getFromPort()->getClass()->getCompiler())->getDataVuoType())
									hasIncomingDataCable = true;

							if (! hasIncomingDataCable)
							{
								VuoRendererCable *cable = NULL;
								VuoRendererNode *makeListNode = composition->createAndConnectMakeListNode(node, port, cable);

								addedNodes.insert(makeListNode);
								addedCables.insert(cable);
								fromPortForCable[cable] = cable->getBase()->getFromPort();
								toPortForCable[cable] = port;
							}
						}
					}
				}
			}
		}

		// Determine whether the operation will involve at least one generic port.
		this->operationInvolvesGenericPort = false;

		foreach (VuoRendererNode *rn, addedNodes)
		{
			if (rn->hasGenericPort())
			{
				this->operationInvolvesGenericPort = true;
				break;
			}
		}

		if (!this->operationInvolvesGenericPort)
		{
			foreach (VuoRendererCable *rc, addedCables)
			{
				bool fromPortIsGeneric = (fromPortForCable[rc] && dynamic_cast<VuoGenericType *>(fromPortForCable[rc]->getRenderer()->getDataType()));
				bool toPortIsGeneric = (toPortForCable[rc] && dynamic_cast<VuoGenericType *>(toPortForCable[rc]->getRenderer()->getDataType()));
				if (fromPortIsGeneric || toPortIsGeneric)
				{
					operationInvolvesGenericPort = true;
					break;
				}
			}
		}

		this->operationRequiresRunningCompositionUpdate = (addedComments.size() < addedComponents.size());

		// Now that the inventory of necessary changes is complete, apply the changes.
		for (set<VuoRendererCable *>::iterator i = addedCables.begin(); i != addedCables.end(); ++i)
			VuoCommandCommon::addCable(*i, fromPortForCable[*i], toPortForCable[*i], composition);

		for (set<VuoRendererNode *>::iterator i = addedNodes.begin(); i != addedNodes.end(); ++i)
			composition->addNode((*i)->getBase());

		for (set<VuoRendererComment *>::iterator i = addedComments.begin(); i != addedComments.end(); ++i)
			composition->addComment((*i)->getBase());

		// Collapse any typecasts possible.
		composition->collapseTypecastNodes();
	}
	// End of command content.

	this->updatedSnapshot = window->getComposition()->takeSnapshot();

	if (!addedComponentDescriptions.empty())
		setDescription("%s %s",
			commandDescription.empty() ? "Add" : commandDescription.c_str(),
			VuoStringUtilities::join(addedComponentDescriptions, ", ").c_str());
}

/**
 * Returns the ID of this command.
 */
int VuoCommandAdd::id() const
{
	return VuoCommandCommon::addCommandID;
}

/**
 * Removes the added nodes and cables.
 */
void VuoCommandAdd::undo()
{
	VuoCommandCommon_undo;

	window->resetCompositionWithSnapshot(revertedSnapshot);

	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	if (operationRequiresRunningCompositionUpdate)
		window->coalesceSnapshots(updatedSnapshot, revertedSnapshot);
}

/**
 * Adds the nodes and cables.
 */
void VuoCommandAdd::redo()
{
	VuoCommandCommon_redo;

	window->resetCompositionWithSnapshot(updatedSnapshot);

	// Update generic types.
	if (operationInvolvesGenericPort)
		window->getComposition()->updateGenericPortTypes();

	if (operationRequiresRunningCompositionUpdate)
		window->coalesceSnapshots(revertedSnapshot, updatedSnapshot);
}
