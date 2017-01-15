/**
 * @file
 * VuoCompilerTriggerPortClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerTriggerPort.hh"

#include "VuoPort.hh"

/**
 * Creates a trigger output port. Upon creation, @c isInEventFunction and @c isInCallbackStartFunction both return false.
 *
 * @param name The port's identifier and title.
 * @param type A @c PointerType pointing to a FunctionType which has 0 or 1 parameter. The parameter indicates
 *	the type of data, if any, transmitted by the port alongside the fired event.
 */
VuoCompilerTriggerPortClass::VuoCompilerTriggerPortClass(string name, PointerType *type)
	: VuoCompilerPortClass(name, VuoPortClass::triggerPort, type)
{
	vuoType = NULL;
}

/**
 * Creates a new port based on this port type, and creates its corresponding base @c VuoPort.
 */
VuoCompilerPort * VuoCompilerTriggerPortClass::newPort(void)
{
	return new VuoCompilerTriggerPort(new VuoPort(getBase()));
}

/**
 * Creates a new port based on this port type, using the pre-existing @c port as its base.
 */
VuoCompilerPort * VuoCompilerTriggerPortClass::newPort(VuoPort *port)
{
	return new VuoCompilerTriggerPort(port);
}

/**
 * Returns the @c FunctionType pointed to by this port's @c PointerType parameter in its node class's event/init functions.
 */
FunctionType * VuoCompilerTriggerPortClass::getFunctionType(void)
{
	return (FunctionType *)((PointerType *)type)->getElementType();
}

/**
 * Returns the type of the data transmitted by this trigger, as set in @c setDataVuoType.
 */
VuoType * VuoCompilerTriggerPortClass::getDataVuoType(void)
{
	return vuoType;
}

/**
 * Sets the type of the data transmitted by this trigger. Its @c VuoCompilerType may be null
 * (but needs to be non-null by the time the composition is compiled).
 *
 * If the @c VuoCompilerType is not null, then its LLVM type should equal the first parameter of this port's function type.
 */
void VuoCompilerTriggerPortClass::setDataVuoType(VuoType *type)
{
	this->vuoType = type;
}
