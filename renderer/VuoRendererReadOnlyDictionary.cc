/**
 * @file
 * VuoRendererReadOnlyDictionary implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoRendererReadOnlyDictionary.hh"
#include "VuoNodeClass.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a collapsed read-only input "Make Dictionary" node.
 */
VuoRendererReadOnlyDictionary::VuoRendererReadOnlyDictionary(VuoNode *baseNode, VuoRendererSignaler *signaler)
	: VuoRendererHiddenInputAttachment(baseNode, signaler)
{
}

/**
 * Returns a boolean indicating whether the provided @c baseNode should be rendered as a collapsed
 * read-only input "Make Dictionary" node, as determined from its connected base components.
 */
bool VuoRendererReadOnlyDictionary::isReadOnlyDictionary(VuoNode *baseNode)
{
	const string makeDictionaryNodeClassNamePrefix = "vuo.dictionary.make.";
	if (!VuoStringUtilities::beginsWith(baseNode->getNodeClass()->getClassName(), makeDictionaryNodeClassNamePrefix))
		return false;

	VuoPort *hostPort = getUnderlyingHostPortForNode(baseNode);
	VuoNode *hostNode = getUnderlyingHostNodeForNode(baseNode);
	bool isInputToCalculateNode = ((hostPort &&
									(hostPort->getClass()->getName() == "values") &&
									hostNode->getNodeClass()->getClassName() == "vuo.math.calculate"));
	return isInputToCalculateNode;
}

/**
  * Returns the node that provides this dictionary with its input value list.
  */
VuoNode * VuoRendererReadOnlyDictionary::getValueListNode()
{
	VuoPort *valueListInputPort = getBase()->getInputPortWithName("values");
	if (!valueListInputPort)
		return NULL;

	VuoNode *makeValueListNode = getListNodeConnectedToInputPort(valueListInputPort);
	return makeValueListNode;
}

/**
  * Returns the node that provides this dictionary with its input key list.
  */
VuoNode * VuoRendererReadOnlyDictionary::getKeyListNode()
{
	VuoPort *valueListInputPort = getBase()->getInputPortWithName("keys");
	return getListNodeConnectedToInputPort(valueListInputPort);
}

/**
 * Returns the set of co-attachments expected to co-exist with this attachment.
 * For a read-only dictionary, that includes its input key list and value list nodes.
 */
set<VuoNode *> VuoRendererReadOnlyDictionary::getCoattachments(void)
{
	VuoNode *keyListNode = getKeyListNode();
	VuoNode *valueListNode = getValueListNode();

	set<VuoNode *> coattachments;
	if (keyListNode)
		coattachments.insert(keyListNode);
	if (valueListNode)
		coattachments.insert(valueListNode);
	return coattachments;
}
