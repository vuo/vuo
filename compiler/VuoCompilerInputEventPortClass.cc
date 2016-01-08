/**
 * @file
 * VuoCompilerInputEventPortClass implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerInputEventPort.hh"

#include "VuoPort.hh"

/**
 * Creates an input port type with the specified data type @c type.
 */
VuoCompilerInputEventPortClass::VuoCompilerInputEventPortClass(string name, Type *type) :
	VuoCompilerEventPortClass(name, type)
{
	portAction = false;
}

/**
 * Creates an input port type.
 */
VuoCompilerInputEventPortClass::VuoCompilerInputEventPortClass(string name) :
	VuoCompilerEventPortClass(name)
{
	portAction = false;
}

/**
 * Creates an input port based on this port type, and creates its corresponding base @c VuoPort.
 */
VuoCompilerPort * VuoCompilerInputEventPortClass::newPort(void)
{
	return new VuoCompilerInputEventPort(new VuoPort(getBase()));
}

/**
 * Creates an input port based on this port type, using the pre-existing @c port as its base.
 */
VuoCompilerPort * VuoCompilerInputEventPortClass::newPort(VuoPort *port)
{
	return new VuoCompilerInputEventPort(port);
}

/**
 * Returns the data class for this port.  NULL if the port is event-only.
 */
VuoCompilerInputDataClass * VuoCompilerInputEventPortClass::getDataClass(void)
{
	return (VuoCompilerInputDataClass *)VuoCompilerEventPortClass::getDataClass();
}

/**
 * Sets whether this port has a port action.
 */
void VuoCompilerInputEventPortClass::setPortAction(bool portAction)
{
	this->portAction = portAction;
}

/**
 * Returns true if this port has a port action (causes something special to happen when it receives an event).
 */
bool VuoCompilerInputEventPortClass::hasPortAction(void)
{
	return portAction;
}
