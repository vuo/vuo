/**
 * @file
 * VuoCompilerInstanceData implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerInstanceData.hh"
#include "VuoCompilerInstanceDataClass.hh"
#include "VuoPort.hh"

/**
 * Creates instance data for a node, based on the specified @c instanceDataClass.
 */
VuoCompilerInstanceData::VuoCompilerInstanceData(VuoCompilerInstanceDataClass *instanceDataClass)
	: VuoCompilerNodeArgument(new VuoPort(instanceDataClass->getBase()))
{
}

/**
 * Generates code to get the current value of this instance data.
 */
Value * VuoCompilerInstanceData::generateLoad(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	return VuoCompilerCodeGenUtilities::generateGetNodeContextInstanceData(module, block, nodeContextValue, getType());
}

/**
 * Generates code to update the value of this instance data.
 */
void VuoCompilerInstanceData::generateStore(Module *module, BasicBlock *block, Value *nodeContextValue, Value *instanceDataValue)
{
	VuoCompilerCodeGenUtilities::generateSetNodeContextInstanceData(module, block, nodeContextValue, instanceDataValue);
}

/**
 * Generates code to get the address of this instance data.
 */
Value * VuoCompilerInstanceData::getVariable(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	return VuoCompilerCodeGenUtilities::generateGetNodeContextInstanceDataVariable(module, block, nodeContextValue, getType());
}

/**
 * Returns the data type of this instance data.
 */
Type * VuoCompilerInstanceData::getType(void)
{
	return static_cast<VuoCompilerInstanceDataClass *>(getBase()->getClass()->getCompiler())->getType();
}
