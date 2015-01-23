/**
 * @file
 * VuoCompilerPublishedPort implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerEventPort.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoPort.hh"

/**
 * Creates a published port that is not connected to any port in a running composition.
 *
 * @param name A name for the published port.
 * @param isOutput A boolean indicating whether the published port is an output port, as opposed to an input port.
 * @param connectedPorts The set of ports within the composition for which this published port is an alias.
 */
VuoCompilerPublishedPort::VuoCompilerPublishedPort(string name, bool isOutput, const set<VuoCompilerPort *> &connectedPorts)
	: VuoBaseDetail<VuoPublishedPort>("VuoCompilerPublishedPort",
									  new VuoPublishedPort(	name,
															getTypeForPort(*connectedPorts.begin()),
															isOutput,
															getBasePorts(connectedPorts)))
{
	getBase()->setCompiler(this);
}

/**
 * Returns the identifiers of the internal ports for which this published port is an alias.
 *
 * Assumes @c generateAllocation has been called on each @c VuoCompilerPort that was passed to the constructor.
 */
set<string> VuoCompilerPublishedPort::getConnectedPortIdentifiers(void)
{
	set<string> identifiers;
	set<VuoPort *> connectedPorts = getBase()->getConnectedPorts();
	for (set<VuoPort *>::iterator port = connectedPorts.begin(); port != connectedPorts.end(); ++port)
	{
		VuoCompilerPort *compilerPort = (VuoCompilerPort *)(*port)->getCompiler();
		VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>(compilerPort);
		if (eventPort)
			identifiers.insert(eventPort->getIdentifier());
	}

	return identifiers;
}

/**
 * Returns this published port's data type, or null if the port is event-only.
 */
VuoType * VuoCompilerPublishedPort::getTypeForPort(VuoCompilerPort *connectedPort)
{
	VuoCompilerPortClass *connectedPortClass = static_cast<VuoCompilerPortClass *>(connectedPort->getBase()->getClass()->getCompiler());
	return connectedPortClass->getDataVuoType();
}

/**
 * Returns the set of VuoPort pointers corresponding to the input @c set of VuoCompilerPort pointers.
 */
set<VuoPort *> VuoCompilerPublishedPort::getBasePorts(set<VuoCompilerPort *>list)
{
	set<VuoPort *> baseList;
	for (set<VuoCompilerPort *>::iterator i = list.begin(); i != list.end(); ++i)
		baseList.insert((*i)->getBase());

	return baseList;
}
