﻿/**
 * @file
 * VuoCompilerPort implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCable.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoException.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include "VuoType.hh"

/**
 * Creates a compiler detail from a given @c basePort.
 */
VuoCompilerPort::VuoCompilerPort(VuoPort *basePort)
	: VuoCompilerNodeArgument(basePort)
{
	dataType = NULL;
	indexInPortContexts = -1;
	constantsCache = NULL;
}

/**
 * Returns a boolean indicating whether this port has any attached cables.
 */
bool VuoCompilerPort::hasConnectedCable(void) const
{
	return (! getBase()->getConnectedCables().empty());
}

/**
 * Returns a boolean indicating whether this port has any attached data+event cables.
 */
bool VuoCompilerPort::hasConnectedDataCable() const
{
	vector<VuoCable *> connectedCables = getBase()->getConnectedCables();
	for (vector<VuoCable *>::iterator cable = connectedCables.begin(); cable != connectedCables.end(); ++cable)
		if ((*cable)->hasCompiler() && (*cable)->getCompiler()->carriesData())
			return true;
	return false;
}

/**
 * Returns this port's data type, or null if this port is event-only.
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

/**
 * Sets an identifier that will be part of the string returned by getIdentifier().
 */
void VuoCompilerPort::setNodeIdentifier(string nodeIdentifier)
{
	this->nodeIdentifier = nodeIdentifier;
}

/**
 * Returns a unique, consistent identifier for this port.
 *
 * This needs to be kept consistent with runtime function `vuoReplacePortData()`.
 */
string VuoCompilerPort::getIdentifier(void)
{
	if (nodeIdentifier.empty())
		throw VuoException("VuoCompilerPort::setNodeIdentifier() must be called before VuoCompilerPort::getIdentifier().");

	return VuoStringUtilities::buildPortIdentifier(nodeIdentifier, getBase()->getClass()->getName());
}

/**
 * Sets the index of this node within the array of port contexts within the node context.
 */
void VuoCompilerPort::setIndexInPortContexts(int indexInPortContexts)
{
	this->indexInPortContexts = indexInPortContexts;
}

/**
 * Returns the index of this node within the array of port contexts within the node context.
 */
int VuoCompilerPort::getIndexInPortContexts(void)
{
	return indexInPortContexts;
}

/**
 * Sets the cache used to generate constant string values. This must be called before generating bitcode.
 */
void VuoCompilerPort::setConstantsCache(VuoCompilerConstantsCache *constantsCache)
{
	this->constantsCache = constantsCache;
}

/**
 * Returns the address of the `data` field within this port's context.
 */
Value * VuoCompilerPort::getDataVariable(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextDataVariable(module, block, portContextValue, getDataVuoType()->getCompiler());
}

/**
 * Returns this port's context.
 */
Value * VuoCompilerPort::generateGetPortContext(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	if (indexInPortContexts < 0)
		throw VuoException("VuoCompilerPort::setIndexInPortContexts() must be called before VuoCompilerPort::generateGetPortContext().");

	return VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, block, nodeContextValue, indexInPortContexts);
}
