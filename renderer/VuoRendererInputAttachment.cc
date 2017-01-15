/**
 * @file
 * VuoRendererInputAttachment implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererInputAttachment.hh"
#include "VuoRendererNode.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoCable.hh"

/**
 * Creates a node that is rendered as an attachment to another node's input port.
 */
VuoRendererInputAttachment::VuoRendererInputAttachment(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererNode(baseNode, signaler)
{
	setAlwaysDisplayPortNames(true);
}

/**
  * Returns the input port to which this item is attached in the underlying composition.
  */
VuoPort * VuoRendererInputAttachment::getUnderlyingHostPort()
{
	return getUnderlyingHostPortForNode(this->getBase());
}

/**
  * Returns the node to which this item is attached in the underlying composition.
  */
VuoNode * VuoRendererInputAttachment::getUnderlyingHostNode()
{
	return getUnderlyingHostNodeForNode(this->getBase());
}

/**
  * Returns the input port to which this item is visually attached in the rendered composition.
  */
VuoPort * VuoRendererInputAttachment::getRenderedHostPort()
{
	return getUnderlyingHostPort();
}

/**
  * Returns the node to which this item is visually attached in the composition rendering.
  */
VuoNode * VuoRendererInputAttachment::getRenderedHostNode()
{
	return getUnderlyingHostNode();
}

/**
  * Returns the input port to which the provided node is attached in the underlying composition.
  */
VuoPort * VuoRendererInputAttachment::getUnderlyingHostPortForNode(VuoNode *node)
{
	if (!node)
		return NULL;

	VuoPort *listOutPort = node->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
	vector<VuoCable *> outCables = listOutPort->getConnectedCables(true);
	if (outCables.size() < 1)
		return NULL;

	VuoCable *outCable = *outCables.begin();
	return outCable->getToPort();
}

/**
  * Returns the node to which the provided node is attached in the underlying composition.
  */
VuoNode * VuoRendererInputAttachment::getUnderlyingHostNodeForNode(VuoNode *node)
{
	if (!node)
		return NULL;

	VuoPort *listOutPort = node->getOutputPorts()[VuoNodeClass::unreservedOutputPortStartIndex];
	vector<VuoCable *> outCables = listOutPort->getConnectedCables(true);
	if (outCables.size() < 1)
		return NULL;

	VuoCable *outCable = *outCables.begin();
	return outCable->getToNode();
}

/**
 * Returns the set of co-attachments expected to co-exist with this attachment.
 */
set<VuoNode *> VuoRendererInputAttachment::getCoattachments(void)
{
	set<VuoNode *> coattachments;
	return coattachments;
}

/**
 * Returns the collapsed list node attached to the provided input port, or @c NULL if none.
 */
VuoNode * VuoRendererInputAttachment::getListNodeConnectedToInputPort(VuoPort *port)
{
	if (!port)
		return NULL;

	vector<VuoCable *> inCables = port->getConnectedCables(false);
	foreach (VuoCable *cable, inCables)
	{
		VuoNode *fromNode = cable->getFromNode();
		if (VuoCompilerMakeListNodeClass::isMakeListNodeClassName(fromNode->getNodeClass()->getClassName()))
			return fromNode;
	}

	return NULL;
}
