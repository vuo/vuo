/**
 * @file
 * VuoCompilerPublishedPortClass implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPortClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerPublishedPortClass.hh"
#include "VuoPublishedPort.hh"
#include <stdexcept>

/**
 * Creates a published port type.
 */
VuoCompilerPublishedPortClass::VuoCompilerPublishedPortClass(string name, VuoPortClass::PortType portType, Type *type)
	: VuoCompilerPortClass(name, portType, type)
{
	this->vuoType = NULL;
	this->details = json_object_new_object();
}

/**
 * Creates a new port based on this port type, and creates its corresponding base @c VuoPort.
 */
VuoCompilerPort * VuoCompilerPublishedPortClass::newPort(void)
{
	return new VuoCompilerPublishedPort(new VuoPublishedPort(getBase()));
}

/**
 * Creates a new port based on this port type, using the pre-existing @c port as its base.
 */
VuoCompilerPort * VuoCompilerPublishedPortClass::newPort(VuoPort *port)
{
	return new VuoCompilerPublishedPort(port);
}

/**
 * Returns the type of data carried by this port, or null if this port is event-only.
 */
VuoType * VuoCompilerPublishedPortClass::getDataVuoType(void)
{
	return vuoType;
}

/**
 * Sets the type of the data carried by this port.
 *
 * Assumes this port is not event-only.
 */
void VuoCompilerPublishedPortClass::setDataVuoType(VuoType *type)
{
	this->vuoType = type;
}

/**
 * Returns the port class's display name.
 */
string VuoCompilerPublishedPortClass::getDisplayName(void)
{
	return getBase()->getName();
}

/**
 * Sets the value of a detail for this published port.
 * The previous detail value for @a key (if any) is replaced by @a value.
 */
void VuoCompilerPublishedPortClass::setDetail(string key, string value)
{
	json_object_object_add(details, key.c_str(), json_tokener_parse(value.c_str()));
}

/**
 * Unsets the value of a detail for this published port.
 * The previous detail value for @a key (if any) is removed.
 */
void VuoCompilerPublishedPortClass::unsetDetail(string key)
{
	json_object_object_del(details, key.c_str());
}
