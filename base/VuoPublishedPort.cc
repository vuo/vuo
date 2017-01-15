/**
 * @file
 * VuoPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPublishedPort.hh"

/**
 * Creates a published port that is not yet connected to any port in a running composition.
 */
VuoPublishedPort::VuoPublishedPort(VuoPortClass *portClass)
	: VuoPort(portClass)
{
	this->protocolPort = false;
}

/**
 * Sets whether this port is part of any active protocol.
 */
void VuoPublishedPort::setProtocolPort(bool protocolPort)
{
	this->protocolPort = protocolPort;
}

/**
 * Returns true if this port is part of any active protocol.
 */
bool VuoPublishedPort::isProtocolPort(void)
{
	return protocolPort;
}
