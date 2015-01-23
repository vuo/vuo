/**
 * @file
 * VuoPortClass implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
}

/**
 * Returns the name of this port class.
 */
string VuoPortClass::getName(void)
{
	return name;
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
