/**
 * @file
 * VuoComposition implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoComposition.hh"

/**
 * Creates an empty composition.
 */
VuoComposition::VuoComposition(void)
	: VuoBase<VuoCompilerComposition,VuoRendererComposition>("VuoComposition")
{
	this->name = "VuoComposition";
	this->directory = "";
}

/**
 * Sets the composition's display name.
 */
void VuoComposition::setName(string name)
{
	this->name = name;
}

/**
 * Returns the composition's display name.
 */
string VuoComposition::getName(void)
{
	return name;
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
 * Sets the composition's description (documentation).
 */
void VuoComposition::setDescription(string description)
{
	this->description = description;
}

/**
 * Returns the composition's description (documentation).
 */
string VuoComposition::getDescription(void)
{
	return description;
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
 * Removes a cable from the composition.
 */
void VuoComposition::removeCable(VuoCable *cable)
{
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
 * Adds a published input cable to the composition.
 *
 * The caller is responsible for adding the cable's "To" node to the composition.
 */
void VuoComposition::addPublishedInputCable(VuoCable *cable)
{
	publishedInputCables.insert(cable);
}

/**
 * Removes a published input cable from the composition.
 */
void VuoComposition::removePublishedInputCable(VuoCable *cable)
{
	publishedInputCables.erase(cable);
}

/**
 * Returns the psuedo-cables attached to the output ports of the published input pseudo-node, if any.
 */
set<VuoCable *> VuoComposition::getPublishedInputCables(void)
{
	return publishedInputCables;
}

/**
 * Adds a published output cable to the composition.
 *
 * The caller is responsible for adding the cable's "From" node to the composition.
 */
void VuoComposition::addPublishedOutputCable(VuoCable *cable)
{
	publishedOutputCables.insert(cable);
}

/**
 * Removes a published output cable from the composition.
 */
void VuoComposition::removePublishedOutputCable(VuoCable *cable)
{
	publishedOutputCables.erase(cable);
}

/**
 * Returns the psuedo-cables attached to the input ports of the published output pseudo-node, if any.
 */
set<VuoCable *> VuoComposition::getPublishedOutputCables(void)
{
	return publishedOutputCables;
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
		if (port->getName() == name)
			return port;
	}
	return NULL;
}

/**
 * Returns the published input port connected to the given port, or null if none is connected.
 */
VuoPublishedPort * VuoComposition::getPublishedInputPortConnectedToPort(VuoPort *port)
{
	for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoPublishedPort *publishedPort = *i;
		set<VuoPort *> connectedPorts = publishedPort->getConnectedPorts();
		if (connectedPorts.find(port) != connectedPorts.end())
			return publishedPort;
	}
	return NULL;
}

/**
 * Returns the set of published output ports connected to the given port.
 */
set<VuoPublishedPort *> VuoComposition::getPublishedOutputPortsConnectedToPort(VuoPort *port)
{
	set<VuoPublishedPort *> publishedPortsConnectedToPort;
	for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
	{
		VuoPublishedPort *publishedPort = *i;
		set<VuoPort *> connectedPorts = publishedPort->getConnectedPorts();
		if (connectedPorts.find(port) != connectedPorts.end())
			publishedPortsConnectedToPort.insert(publishedPort);
	}
	return publishedPortsConnectedToPort;
}

/**
 * Returns the set of published input ports connected to the node, and the port on the node to which each is connected.
 */
set<pair<VuoPublishedPort *, VuoPort *> > VuoComposition::getPublishedInputPortsConnectedToNode(VuoNode *node)
{
	set<pair<VuoPublishedPort *, VuoPort *> > publishedPortsConnectedToNode;
	vector<VuoPort *> ports = node->getInputPorts();
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoPort *port = *i;
		VuoPublishedPort *publishedPort = getPublishedInputPortConnectedToPort(port);
		if (publishedPort)
			publishedPortsConnectedToNode.insert(make_pair(publishedPort, port));
	}
	return publishedPortsConnectedToNode;
}

/**
 * Returns the set of published output ports connected to the node, and the port on the node to which each is connected.
 */
set<pair<VuoPublishedPort *, VuoPort *> > VuoComposition::getPublishedOutputPortsConnectedToNode(VuoNode *node)
{
	set<pair<VuoPublishedPort *, VuoPort *> > publishedPortsConnectedToNode;
	vector<VuoPort *> ports = node->getOutputPorts();
	for (vector<VuoPort *>::iterator i = ports.begin(); i != ports.end(); ++i)
	{
		VuoPort *port = *i;
		set<VuoPublishedPort *> publishedPorts = getPublishedOutputPortsConnectedToPort(port);
		for (set<VuoPublishedPort *>::iterator j = publishedPorts.begin(); j != publishedPorts.end(); ++j)
		{
			VuoPublishedPort *publishedPort = *j;
			publishedPortsConnectedToNode.insert(make_pair(publishedPort, port));
		}
	}
	return publishedPortsConnectedToNode;
}
