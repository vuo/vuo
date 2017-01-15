/**
 * @file
 * VuoPortClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoPortClass.hh"

/**
 * Creates a base port class.
 */
VuoPortClass::VuoPortClass(string name, PortType portType)
	: VuoBase<VuoCompilerNodeArgumentClass,void>("VuoPortClass")
{
	this->name = name;
	this->portType = portType;
	this->eventBlocking = EventBlocking_None;
	this->defaultEventThrottling = EventThrottling_Enqueue;
	this->portAction = false;
}

/**
 * Returns the name of this port class.
 */
string VuoPortClass::getName(void)
{
	return name;
}

/**
 * Sets the name of this port class.
 */
void VuoPortClass::setName(string name)
{
	this->name = name;
}

/**
 * Returns this port's type.
 */
VuoPortClass::PortType VuoPortClass::getPortType(void)
{
	return portType;
}

/**
 * Returns this port's event-blocking behavior.
 */
VuoPortClass::EventBlocking VuoPortClass::getEventBlocking(void)
{
	return eventBlocking;
}

/**
 * Sets this port's event-blocking behavior.
 */
void VuoPortClass::setEventBlocking(VuoPortClass::EventBlocking eventBlocking)
{
	this->eventBlocking = eventBlocking;
}

/**
 * Returns true if this port has a port action (causes something special to happen when it receives an event).
 * Only applies to input ports.
 */
bool VuoPortClass::hasPortAction(void)
{
	return portAction;
}

/**
 * Sets whether this port has a port action. Only applies to input ports.
 */
void VuoPortClass::setPortAction(bool portAction)
{
	this->portAction = portAction;
}

/**
 * Returns the default event-throttling behavior of ports of this port class. Only applies to trigger ports.
 * A port may override this.
 */
VuoPortClass::EventThrottling VuoPortClass::getDefaultEventThrottling(void)
{
	return defaultEventThrottling;
}

/**
 * Sets the default event-throttling behavior of ports of this port class. Only applies to trigger ports.
 * A port may override this.
 */
void VuoPortClass::setDefaultEventThrottling(VuoPortClass::EventThrottling eventThrottling)
{
	this->defaultEventThrottling = eventThrottling;
}

/**
 * Prints info about this port class, for debugging.
 */
void VuoPortClass::print(void)
{
	printf("VuoPortClass(%p,\"%s\")",this,getName().c_str());
	if (hasCompiler())
		printf(" VuoCompilerPortClass(%p)",getCompiler());
	if (hasRenderer())
		printf(" VuoRendererPortClass(%p)",getRenderer());
	printf("\n");
}
