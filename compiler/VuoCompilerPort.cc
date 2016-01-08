/**
 * @file
 * VuoCompilerPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPort.hh"
#include "VuoPort.hh"
#include "VuoCompilerCable.hh"

/**
 * Creates a compiler detail from a given @c basePort.
 */
VuoCompilerPort::VuoCompilerPort(VuoPort *basePort)
	: VuoCompilerNodeArgument(basePort)
{
	dataType = NULL;
}

/**
 * Returns a boolean indicating whether this port has any attached cables.
 */
bool VuoCompilerPort::hasConnectedCable(bool includePublishedCables) const
{
	return (! getBase()->getConnectedCables(includePublishedCables).empty());
}

/**
 * Returns a boolean indicating whether this port has any attached data+event cables.
 */
bool VuoCompilerPort::hasConnectedDataCable(bool includePublishedCables) const
{
	vector<VuoCable *> connectedCables = getBase()->getConnectedCables(includePublishedCables);
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		if ((*cable)->hasCompiler() && (*cable)->getCompiler()->carriesData())
			return true;
	return false;
}

/**
 * Returns tthis port's data type, or null if this port is event-only.
 *
 * If setDataVuoType() has been called, returns that type. Otherwise returns the type from the port class.
 */
VuoType * VuoCompilerPort::getDataVuoType(void)
{
	if (dataType)
		return dataType;

	VuoType *dataTypeFromClass = static_cast<VuoCompilerPortClass *>(getBase()->getClass()->getCompiler())->getDataVuoType();
	if (dataTypeFromClass)
		return dataTypeFromClass;

	return NULL;
}

/**
 * Sets the data type for this port, overriding the type from the port class.
 */
void VuoCompilerPort::setDataVuoType(VuoType *dataType)
{
	this->dataType = dataType;
}
