/**
 * @file
 * VuoComposition implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoComposition.hh"
#include "VuoCable.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoProtocol.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates an empty composition.
 */
VuoComposition::VuoComposition(void)
	: VuoBase<VuoCompilerComposition,VuoRendererComposition>("VuoComposition")
{
	metadata = new VuoCompositionMetadata();
	ownsMetadata = true;
	directory = "";
}

VuoComposition::~VuoComposition(void)
{
	if (ownsMetadata)
		delete metadata;
}

/**
 * Assigns @a metadata to this composition.
 *
 * If @a takeOwnership is true, this composition becomes responsible for destroying @a metadata.
 * Otherwise, the caller is responsible.
 */
void VuoComposition::setMetadata(VuoCompositionMetadata *metadata, bool takeOwnership)
{
	if (ownsMetadata)
		delete this->metadata;

	this->metadata = metadata;
	ownsMetadata = takeOwnership;
}

/**
 * Returns the metadata associated with this composition.
 */
VuoCompositionMetadata * VuoComposition::getMetadata(void)
{
	return metadata;
}

/**
 * Sets the directory containing the composition's .vuo source file.
 */
void VuoComposition::setDirectory(string directory)
{
	this->directory = directory;
}

/**
 * Returns the directory containing the composition's .vuo source file.
 */
string VuoComposition::getDirectory(void)
{
	return directory;
}

/**
 * Adds a node to the composition.
 */
void VuoComposition::addNode(VuoNode *node)
{
	nodes.insert(node);
}

/**
 * Removes a node from the composition.
 *
 * The caller is responsible for removing any cables attached to the node.
 */
void VuoComposition::removeNode(VuoNode *node)
{
	nodes.erase(node);
}

/**
 * Returns the nodes in this composition.
 */
set<VuoNode *> VuoComposition::getNodes(void)
{
	return nodes;
}

/**
 * Adds a cable to the composition.
 *
 * The caller is responsible for adding the nodes on either end of the cable to the composition.
 */
void VuoComposition::addCable(VuoCable *cable)
{
	cables.insert(cable);
}

/**
 * Disconnects and removes a cable from the composition.
 */
void VuoComposition::removeCable(VuoCable *cable)
{
	cable->setFrom(NULL, NULL);
	cable->setTo(NULL, NULL);

	cables.erase(cable);
}

/**
 * Returns the cables in this composition.
 */
set<VuoCable *> VuoComposition::getCables(void)
{
	return cables;
}

/**
 * Adds a comment to the composition.
 */
void VuoComposition::addComment(VuoComment *comment)
{
	comments.insert(comment);
}

/**
 * Removes a comment from the composition.
 */
void VuoComposition::removeComment(VuoComment *comment)
{
	comments.erase(comment);
}

/**
 * Returns the comments in this composition.
 */
set<VuoComment *> VuoComposition::getComments(void)
{
	return comments;
}

/**
 * Adds a published port to this composition's list of published input ports at the given index.
 */
void VuoComposition::addPublishedInputPort(VuoPublishedPort *port, int index)
{
	publishedInputPorts.insert(publishedInputPorts.begin() + index, port);
}

/**
 * Adds a published port to this composition's list of published output ports at the given index.
 */
void VuoComposition::addPublishedOutputPort(VuoPublishedPort *port, int index)
{
	publishedOutputPorts.insert(publishedOutputPorts.begin() + index, port);
}

/**
 * Removes the published port at the given index from this composition's list of published input ports.
 */
void VuoComposition::removePublishedInputPort(int index)
{
	publishedInputPorts.erase(publishedInputPorts.begin() + index);
}

/**
 * Removes the published port at the given index from this composition's list of published input ports.
 */
void VuoComposition::removePublishedOutputPort(int index)
{
	publishedOutputPorts.erase(publishedOutputPorts.begin() + index);
}

/**
 * Returns the published input ports in this composition.
 */
vector<VuoPublishedPort *> VuoComposition::getPublishedInputPorts(void)
{
	return publishedInputPorts;
}

/**
 * Returns the published output ports in this composition.
 */
vector<VuoPublishedPort *> VuoComposition::getPublishedOutputPorts(void)
{
	return publishedOutputPorts;
}

/**
 * Returns the published input port with the given name, or null if none matches.
 */
VuoPublishedPort * VuoComposition::getPublishedInputPortWithName(string name)
{
	return getPublishedPortWithName(name, true);
}

/**
 * Returns the published output port with the given name, or null if none matches.
 */
VuoPublishedPort * VuoComposition::getPublishedOutputPortWithName(string name)
{
	return getPublishedPortWithName(name, false);
}

/**
 * Helper function for getPublishedInputPortWithName() and getPublishedOutputPortWithName().
 */
VuoPublishedPort * VuoComposition::getPublishedPortWithName(string name, bool isInput)
{
	vector<VuoPublishedPort *> publishedPorts = (isInput ? publishedInputPorts : publishedOutputPorts);
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoPublishedPort *port = *i;
		if (port->getClass()->getName() == name)
			return port;
	}
	return NULL;
}

/**
 * Returns the index of the published port in the list of published input or output ports.
 */
int VuoComposition::getIndexOfPublishedPort(VuoPublishedPort *port, bool isInput)
{
	vector<VuoPublishedPort *> publishedPorts = (isInput ? getPublishedInputPorts() : getPublishedOutputPorts());
	vector<VuoPublishedPort *>::iterator foundPort = find(publishedPorts.begin(), publishedPorts.end(), port);
	if (foundPort != publishedPorts.end())
		return std::distance(publishedPorts.begin(), foundPort);
	else
		return -1;
}

/**
 * Returns an ordered list of the composition's published input or output ports,
 * with published ports belonging to the provided protocol listed first, followed by
 * published ports that are not part of the protocol.
 */
vector<VuoPublishedPort *> VuoComposition::getProtocolAwarePublishedPortOrder(VuoProtocol *protocol, bool publishedInputs)
{
	vector<VuoPublishedPort *> publishedPorts = (publishedInputs? publishedInputPorts : publishedOutputPorts);
	vector<VuoPublishedPort *> sortedPublishedPorts;

	// First list published ports that are part of the target protocol.
	if (protocol)
	{
		vector<pair<string, string> > protocolPorts = (publishedInputs?
															  protocol->getInputPortNamesAndTypes() :
															  protocol->getOutputPortNamesAndTypes());

		for (vector<pair<string, string> >::const_iterator i = protocolPorts.begin(); i != protocolPorts.end(); ++i)
		{
			string protocolPortName = i->first;
			VuoPublishedPort *compliantPublishedPort = (publishedInputs?
															getPublishedInputPortWithName(protocolPortName) :
															getPublishedOutputPortWithName(protocolPortName));
			if (compliantPublishedPort)
				sortedPublishedPorts.push_back(compliantPublishedPort);
		}
	}

	// Next list published ports that are not part of the target protocol.
	for (vector<VuoPublishedPort *>::const_iterator port = publishedPorts.begin(); port != publishedPorts.end(); ++port)
	{
		if (std::find(sortedPublishedPorts.begin(), sortedPublishedPorts.end(), *port) == sortedPublishedPorts.end())
			sortedPublishedPorts.push_back(*port);
	}

	return sortedPublishedPorts;
}

