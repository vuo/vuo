/**
 * @file
 * VuoPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPort.hh"
#include "VuoCable.hh"

#include <stdio.h>


/**
 * Creates a base port instance from a @c portClass.
 */
VuoPort::VuoPort(VuoPortClass *portClass)
	: VuoBase<VuoCompilerNodeArgument,VuoRendererPort>("VuoPort")
{
	this->portClass = portClass;
	this->eventThrottling = getClass()->getDefaultEventThrottling();
}

VuoPort::~VuoPort(void)
{
}

/**
 * Returns the argument class of this argument.
 */
VuoPortClass * VuoPort::getClass(void)
{
	return portClass;
}

/**
 * Returns the cables connected to this port.
 * The cables are in the order in which they were added, from least to most recent.
 * The input @c includePublishedCables determines whether cables connected to
 * externally visible published ports are included in the list of returned cables.
 */
vector<VuoCable *> VuoPort::getConnectedCables(bool includePublishedCables)
{
	vector<VuoCable *> targetCables;
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
	{
		if (includePublishedCables || (! (*cable)->isPublished()))
			targetCables.push_back(*cable);
	}

	return targetCables;
}

/**
 * Returns the cable connecting this port to or from the provided @c otherPort,
 * or NULL if no such cable exists.
 */
VuoCable * VuoPort::getCableConnecting(VuoPort *otherPort)
{
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
	{
		if (((*cable)->getFromPort() == otherPort) || ((*cable)->getToPort() == otherPort))
			return (*cable);
	}

	return NULL;
}

/**
 * Adds a connected cable.
 * This method is called by the VuoCable constructor as well as
 * VuoCable::setTo / VuoCable::setFrom, and should not
 * normally need to be called otherwise.
 */
void VuoPort::addConnectedCable(VuoCable *cable)
{
	connectedCables.push_back(cable);
}

/**
 * Removes a connected cable.
 * This method is called by VuoCable::setTo / VuoCable::setFrom,
 * and should not normally need to be called otherwise.
 */
void VuoPort::removeConnectedCable(VuoCable *cable)
{
	connectedCables.erase(std::remove(connectedCables.begin(), connectedCables.end(), cable), connectedCables.end());
}

/**
 * Returns the event-throttling behavior of this port. Only applies to trigger ports.
 */
VuoPortClass::EventThrottling VuoPort::getEventThrottling(void)
{
	return eventThrottling;
}

/**
 * Sets the event-throttling behavior of this port. Only applies to trigger ports.
 */
void VuoPort::setEventThrottling(VuoPortClass::EventThrottling eventThrottling)
{
	this->eventThrottling = eventThrottling;
}

/**
 * Prints info about this port, for debugging.
 */
void VuoPort::print(void)
{
	printf("VuoPort(%p,\"%s\")",this,portClass->getName().c_str());
	if (hasCompiler())
		printf(" VuoCompilerPort(%p)",getCompiler());
	if (hasRenderer())
		printf(" VuoRendererPort(%p)",getRenderer());
	printf("\n");
}
