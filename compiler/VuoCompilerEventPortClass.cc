/**
 * @file
 * VuoCompilerEventPortClass implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerEventPortClass.hh"

#include "VuoPortClass.hh"

/**
 * Creates a passive (non-trigger) port with the specified data type @c type.
 */
VuoCompilerEventPortClass::VuoCompilerEventPortClass(string name, Type *type) :
	VuoCompilerPortClass(name, VuoPortClass::eventOnlyPort, type)
{
	dataClass = NULL;
}

/**
 * Creates a passive (non-trigger) port.
 */
VuoCompilerEventPortClass::VuoCompilerEventPortClass(string name) :
	VuoCompilerPortClass(name, VuoPortClass::eventOnlyPort, NULL)
{
	type = IntegerType::get(getGlobalContext(), 1);
	dataClass = NULL;
}

/**
 * Returns this port's data class, or @c NULL if none.
 */
VuoCompilerDataClass * VuoCompilerEventPortClass::getDataClass(void)
{
	return dataClass;
}

/**
 * Sets this port's data class.
 */
void VuoCompilerEventPortClass::setDataClass(VuoCompilerDataClass *dataClass)
{
	this->dataClass = dataClass;

	VuoPortClass * newBase = new VuoPortClass(getBase()->getName(), dataClass ? VuoPortClass::dataAndEventPort : VuoPortClass::eventOnlyPort);
	newBase->setCompiler(this);
	delete getBase();
	setBase(newBase);
}

/**
 * Returns the type of data carried by this port, or null if this port is event-only.
 */
VuoType * VuoCompilerEventPortClass::getDataVuoType(void)
{
	return (dataClass ? dataClass->getVuoType() : NULL);
}

/**
 * Sets the type of the data carried by this port.
 *
 * Assumes this port is not event-only.
 */
void VuoCompilerEventPortClass::setDataVuoType(VuoType *type)
{
	dataClass->setVuoType(type);
}
