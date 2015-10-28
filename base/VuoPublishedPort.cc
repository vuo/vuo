/**
 * @file
 * VuoPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPublishedPort.hh"

/**
 * Creates a published port that is not yet connected to any port in a running composition.
 *
 * @param name The published port's name.
 * @param type The published port's data type, or null if the port is event-only.
 * @param isOutput A boolean indicating whether the published port is an output port, as opposed to an input port.
 * @param connectedPorts The set of ports within the composition for which this published port is an alias.
 */
VuoPublishedPort::VuoPublishedPort(string name, VuoType *type, bool isOutput, set<VuoPort *> connectedPorts)
	: VuoBase<VuoCompilerPublishedPort,VuoRendererPublishedPort>("VuoPublishedPort")
{
	this->name = name;
	this->type = type;
	this->isOutput = isOutput;
	this->connectedPorts = connectedPorts;
}

/**
 * Returns the published port's name.
 */
string VuoPublishedPort::getName(void)
{
	return name;
}

/**
 * Returns the published port's type, or null if the port is event-only.
 */
VuoType * VuoPublishedPort::getType(void)
{
	return type;
}


/**
 * Returns a boolean indicating whether this published port is an input port.
 */
bool VuoPublishedPort::getInput(void) const
{
	return (! isOutput);
}

/**
 * Returns a boolean indicating whether this published port is an output port.
 */
bool VuoPublishedPort::getOutput(void) const
{
	return isOutput;
}

/**
 * Sets the published port's name.
 */
void VuoPublishedPort::setName(string name)
{
	this->name = name;
}

/**
 * Sets the published port's type.
 */
void VuoPublishedPort::setType(VuoType *type)
{
	this->type = type;
}

/**
 * Returns the set of pointers to VuoCompilerPort objects for which this published port is an alias.
 */
set<VuoPort *> VuoPublishedPort::getConnectedPorts(void)
{
	return connectedPorts;
}

/**
 * Returns a boolean indicating whether the published port is an alias for the given internal @c port.
 */
bool VuoPublishedPort::hasConnectedPort(VuoPort *port)
{
	return (find(connectedPorts.begin(), connectedPorts.end(), port) != connectedPorts.end());
}

/**
 * Adds the specified @c port to the list of internal ports for which this published port is an alias.
 */
void VuoPublishedPort::addConnectedPort(VuoPort *port)
{
	connectedPorts.insert(port);
}

/**
 * Removes the specified @c port from the list of internal ports for which this published port is an alias.
 */
void VuoPublishedPort::removeConnectedPort(VuoPort *port)
{
	connectedPorts.erase(connectedPorts.find(port));
}

