/**
 * @file
 * VuoCompilerEventPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerData.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerEventPort.hh"
#include "VuoCompilerEventPortClass.hh"
#include "VuoCompilerInputData.hh"
#include "VuoCompilerType.hh"
#include "VuoPort.hh"
#include "VuoType.hh"

/**
 * Creates a passive (non-trigger) port from the specified @c portClass.
 */
VuoCompilerEventPort::VuoCompilerEventPort(VuoPort * basePort)
	: VuoCompilerPort(basePort)
{
	data = NULL;
	VuoCompilerDataClass *dataClass = ((VuoCompilerEventPortClass *)getBase()->getClass()->getCompiler())->getDataClass();
	if (dataClass)
		data = dataClass->newData();
}

/**
 * Generates code to create a heap-allocated PortContext.
 *
 * @return A value of type `PortContext *`.
 */
Value * VuoCompilerEventPort::generateCreatePortContext(Module *module, BasicBlock *block)
{
	VuoCompilerData *data = getData();
	VuoCompilerType *dataType = (data ? getDataVuoType()->getCompiler() : NULL);
	return VuoCompilerCodeGenUtilities::generateCreatePortContext(module, block, dataType ? dataType->getType() : NULL, false, "");
}

/**
 * Generates code to get the current event value for this port.
 *
 * @return A value of type `i1` (1-bit integer).
 */
Value * VuoCompilerEventPort::generateLoadEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *eventValue = VuoCompilerCodeGenUtilities::generateGetPortContextEvent(module, block, portContextValue);
	return new TruncInst(eventValue, IntegerType::get(block->getContext(), 1), "", block);
}

/**
 * Generates code to update the event value for this port.
 */
void VuoCompilerEventPort::generateStoreEvent(Module *module, BasicBlock *block, Value *nodeContextValue, Value *eventValue, Value *portContextValue)
{
	unsigned valueBitWidth = ((IntegerType *)eventValue->getType())->getBitWidth();
	unsigned variableBitWidth = 64;
	if (valueBitWidth < variableBitWidth)
		eventValue = new ZExtInst(eventValue, IntegerType::get(block->getContext(), variableBitWidth), "", block);

	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateSetPortContextEvent(module, block, portContextValue, eventValue);
}

/**
 * Generates code to update the event value for this port.
 */
void VuoCompilerEventPort::generateStoreEvent(Module *module, BasicBlock *block, Value *nodeContextValue, bool event, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *eventValue = ConstantInt::get(IntegerType::get(module->getContext(), 64), event ? 1 : 0);
	VuoCompilerCodeGenUtilities::generateSetPortContextEvent(module, block, portContextValue, eventValue);
}

/**
 * Generates code to get the current data value for this port.
 */
Value * VuoCompilerEventPort::generateLoadData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, getDataType()->getType());
}

/**
 * Generates code to update the data value for this port.
 */
void VuoCompilerEventPort::generateStoreData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *dataValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateSetPortContextData(module, block, portContextValue, dataValue);
}

/**
 * Generates code to update the data value for this port, handling the memory management for replacing the old data value.
 */
void VuoCompilerEventPort::generateReplaceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *dataValue, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);

	Value *oldDataValue = VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, getDataType()->getType());
	VuoCompilerCodeGenUtilities::generateSetPortContextData(module, block, portContextValue, dataValue);

	VuoCompilerType *type = getDataType();
	type->generateRetainCall(module, block, dataValue);
	type->generateReleaseCall(module, block, oldDataValue);
}

/**
 * Returns the type of this port's data, or NULL if none.
 */
VuoCompilerType * VuoCompilerEventPort::getDataType(void)
{
	VuoCompilerData *data = getData();
	if (data)
	{
		VuoCompilerDataClass *dataClass = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler());
		return dataClass->getVuoType()->getCompiler();
	}

	return NULL;
}
