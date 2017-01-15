/**
 * @file
 * VuoCompilerOutputEventPortClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerOutputEventPortClass.hh"
#include "VuoCompilerOutputEventPort.hh"

#include "VuoPort.hh"

/**
 * Creates a passive (non-trigger) output port type with the specified data type @c type.
 */
VuoCompilerOutputEventPortClass::VuoCompilerOutputEventPortClass(string name, Type *type) :
	VuoCompilerEventPortClass(name, type)
{
}

/**
 * Creates a passive (non-trigger) output port type.
 */
VuoCompilerOutputEventPortClass::VuoCompilerOutputEventPortClass(string name) :
	VuoCompilerEventPortClass(name)
{
}

/**
 * Creates a passive (non-trigger) output port based on this port type, and creates its corresponding base @c VuoPort.
 */
VuoCompilerPort * VuoCompilerOutputEventPortClass::newPort(void)
{
	return new VuoCompilerOutputEventPort(new VuoPort(getBase()));
}

/**
 * Creates a passive (non-trigger) output port based on this port type, using the pre-existing @c port as its base.
 */
VuoCompilerPort * VuoCompilerOutputEventPortClass::newPort(VuoPort *port)
{
	return new VuoCompilerOutputEventPort(port);
}

/**
 * Returns the data class for this port.  NULL if the port is event-only.
 */
VuoCompilerOutputDataClass * VuoCompilerOutputEventPortClass::getDataClass(void)
{
	return (VuoCompilerOutputDataClass *)VuoCompilerEventPortClass::getDataClass();
}
