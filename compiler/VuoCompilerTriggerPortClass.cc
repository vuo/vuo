/**
 * @file
 * VuoCompilerTriggerPortClass implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerType.hh"
#include "VuoGenericType.hh"
#include "VuoPort.hh"
#include "VuoCompilerCodeGenUtilities.hh"

/**
 * Creates a trigger output port. Upon creation, @c isInEventFunction and @c isInCallbackStartFunction both return false.
 *
 * @param name The port's identifier and title.
 */
VuoCompilerTriggerPortClass::VuoCompilerTriggerPortClass(string name)
	: VuoCompilerPortClass(name, VuoPortClass::triggerPort)
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
 * Returns the signature of the trigger function, which either takes no arguments if the trigger is event-only
 * or has 1 or 2 parameters (depending on C ABI lowering) to pass the data through if the trigger carries data.
 */
FunctionType * VuoCompilerTriggerPortClass::getFunctionType(Module *module)
{
	return VuoCompilerCodeGenUtilities::getFunctionType(module, vuoType);
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
