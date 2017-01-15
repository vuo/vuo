/**
 * @file
 * VuoRendererKeyListForReadOnlyDictionary implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererKeyListForReadOnlyDictionary.hh"
#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoCompilerMakeListNodeClass.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates the compact form for a collapsed "Make List" node that outputs a list of keys
 * as input to a read-only input "Make Dictionary" node.
 */
VuoRendererKeyListForReadOnlyDictionary::VuoRendererKeyListForReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererHiddenInputAttachment(baseNode, signaler)
{
}

/**
 * Returns a boolean indicating whether the provided @c baseNode should be rendered
 * as a collapsed "Make List" node that outputs a list of keys as input to a read-only
 * input "Make Dictionary" node, as determined from its connected components.
 */
bool VuoRendererKeyListForReadOnlyDictionary::isKeyListForReadOnlyDictionary(VuoNode *baseNode)
{
	if (!VuoCompilerMakeListNodeClass::isMakeListNodeClassName(baseNode->getNodeClass()->getClassName()))
		return false;

	VuoPort *hostPort = getUnderlyingHostPortForNode(baseNode);
	VuoNode *hostNode = getUnderlyingHostNodeForNode(baseNode);
	bool providesKeysToMakeReadOnlyDictionaryNode = (hostPort &&
												(hostPort->getClass()->getName() == "keys") &&
												(VuoRendererReadOnlyDictionary::isReadOnlyDictionary(hostNode)));
	return providesKeysToMakeReadOnlyDictionaryNode;
}

/**
  * Returns the "Make List" node that outputs the list of values corresponding to
  * this node's output list of keys.
  */
VuoNode * VuoRendererKeyListForReadOnlyDictionary::getValueListNode()
{
	VuoNode *hostNode = getUnderlyingHostNode();
	if (!hostNode)
		return NULL;

	VuoPort *valueListInputPort = hostNode->getInputPortWithName("values");
	return getListNodeConnectedToInputPort(valueListInputPort);
}

/**
  * Returns the input port to which this item is visually attached in the rendered composition.
  */
VuoPort * VuoRendererKeyListForReadOnlyDictionary::getRenderedHostPort()
{
	// The port that will ultimately make use of this node's output values is two nodes downstream.
	return getUnderlyingHostPortForNode(getUnderlyingHostNodeForNode(this->getBase()));
}

/**
  * Returns the node to which this item is visually attached in the composition rendering.
  */
VuoNode * VuoRendererKeyListForReadOnlyDictionary::getRenderedHostNode()
{
	// The node that will ultimately make use of this node's output values is two nodes downstream.
	return getUnderlyingHostNodeForNode(getUnderlyingHostNodeForNode(this->getBase()));
}

/**
 * Returns the set of co-attachments expected to co-exist with this attachment.
 * For a key list, that includes its sibling value list and the "Make Dictionary"
 * node that they each provide to.
 */
set<VuoNode *> VuoRendererKeyListForReadOnlyDictionary::getCoattachments(void)
{
	VuoNode *dictionaryNode = getUnderlyingHostNode();
	VuoNode *valueListNode = getValueListNode();

	set<VuoNode *> coattachments;

	if (dictionaryNode)
		coattachments.insert(dictionaryNode);
	if (valueListNode)
		coattachments.insert(valueListNode);
	return coattachments;
}
