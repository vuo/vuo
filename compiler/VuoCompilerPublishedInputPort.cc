/**
 * @file
 * VuoCompilerPublishedInputPort implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPublishedInputPort.hh"

/**
 * Creates a published port that is not connected to any port in a composition.
 *
 * @param name A name for the published port.
 * @param connectedPorts A non-empty set of ports, all having the same data type.
 * @param triggerPort The trigger port that implements events fired via this published port.
 */
VuoCompilerPublishedInputPort::VuoCompilerPublishedInputPort(string name,
															 const set<VuoCompilerPort *> &connectedPorts,
															 VuoCompilerTriggerPort *triggerPort)
	: VuoCompilerPublishedPort(name, false, connectedPorts)
{
	this->triggerPort = triggerPort;
}

/**
 * Returns the base port belonging to the "vuo.in" pseudo-node and associated with this published port.
 */
VuoPort * VuoCompilerPublishedInputPort::getVuoPseudoPort(void)
{
	if (! getTriggerPort())
		return NULL;

	return getTriggerPort()->getBase();
}

/**
 * Returns the trigger port that implements events fired via this published port.
 */
VuoCompilerTriggerPort * VuoCompilerPublishedInputPort::getTriggerPort(void)
{
	return triggerPort;
}

/**
 * Sets the trigger port that implements events fired via this published port.
 */
void VuoCompilerPublishedInputPort::setTriggerPort(VuoCompilerTriggerPort *port)
{
	this->triggerPort = port;
}
