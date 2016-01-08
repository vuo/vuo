/**
 * @file
 * VuoCompilerPublishedOutputPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPublishedOutputPort.hh"

/**
 * Creates a published port that is not connected to any port in a composition.
 */
VuoCompilerPublishedOutputPort::VuoCompilerPublishedOutputPort(string name, VuoType *type, set<VuoCompilerPort *> connectedPorts, VuoPort *vuoOutPort)
	: VuoCompilerPublishedPort(name, type, true, connectedPorts)
{
	this->vuoOutPort = vuoOutPort;
}

/**
 * Returns the base port belonging to the "vuo.out" pseudo-node and associated with this published port.
 */
VuoPort * VuoCompilerPublishedOutputPort::getVuoPseudoPort(void)
{
	return vuoOutPort;
}

/**
 * Sets the base port belonging to the "vuo.out" pseudo-node and associated with this published port.
 */
void VuoCompilerPublishedOutputPort::setVuoPseudoPort(VuoPort *port)
{
	this->vuoOutPort = port;
}
