/**
 * @file
 * VuoCable implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCable.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoPortClass.hh"
#include "VuoPublishedPort.hh"

/**
 * Creates a cable from @c fromNode's @c fromPort to @c toNode's @c toPort.
 */
VuoCable::VuoCable(VuoNode * fromNode, VuoPort * fromPort, VuoNode * toNode, VuoPort * toPort)
	: VuoBase<VuoCompilerCable,VuoRendererCable>("VuoCable")
{
	this->fromNode = fromNode;
	this->fromPort = fromPort;
	this->toNode = toNode;
	this->toPort = toPort;

	if (fromPort)
	{
		fromPort->addConnectedCable(this);
	}

	if (toPort)
	{
		toPort->addConnectedCable(this);
	}
}

/**
 * Returns the node from which this cable is output.
 */
VuoNode * VuoCable::getFromNode(void)
{
	return fromNode;
}

/**
 * Returns the port from which this cable is output.
 */
VuoPort * VuoCable::getFromPort(void)
{
	return fromPort;
}

/**
 * Returns the node to which this cable is input.
 */
VuoNode * VuoCable::getToNode(void)
{
	return toNode;
}

/**
 * Returns the port to which this cable is input.
 */
VuoPort * VuoCable::getToPort(void)
{
	return toPort;
}

/**
 * Sets the node and port from which this cable is output.
 * Updates the lists of connected cables maintained internally
 * by this cable's previous and new fromPorts appropriately.
 */
void VuoCable::setFrom(VuoNode *fromNode, VuoPort *fromPort)
{
	if (this->fromPort)
	{
		this->fromPort->removeConnectedCable(this);
	}

	if (fromPort)
	{
		fromPort->addConnectedCable(this);
	}

	this->fromPort = fromPort;
	this->fromNode = fromNode;
}

/**
 * Sets the node and port to which this cable is input.
 * Updates the lists of connected cables maintained internally
 * by this cable's previous and new toPorts appropriately.
 */
void VuoCable::setTo(VuoNode *toNode, VuoPort *toPort)
{
	if (this->toPort)
	{
		this->toPort->removeConnectedCable(this);
	}

	if (toPort)
	{
		toPort->addConnectedCable(this);
	}

	this->toPort = toPort;
	this->toNode = toNode;
}

/**
 * Returns true if either of the cable's endpoints is an externally visible published port.
 */
bool VuoCable::isPublished(void)
{
	return (isPublishedInputCable() || isPublishedOutputCable());
}

/**
 * Returns true if this cable has an externally visible published input port as one of its endpoints.
 */
bool VuoCable::isPublishedInputCable(void)
{
	return dynamic_cast<VuoPublishedPort *>(fromPort);
}

/**
 * Returns true if this cable has an externally visible published output port as one of its endpoints.
 */
bool VuoCable::isPublishedOutputCable(void)
{
	return dynamic_cast<VuoPublishedPort *>(toPort);
}
