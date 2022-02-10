/**
 * @file
 * VuoCompilerEventPort implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerData.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerEventPort.hh"
#include "VuoCompilerEventPortClass.hh"
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
	return VuoCompilerCodeGenUtilities::generateCreatePortContext(module, block, dataType, false, "");
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
 * Generates code to get a pointer to the current data value for this port.
 */
Value * VuoCompilerEventPort::generateRetrieveData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextDataVariable(module, block, portContextValue, getDataType());
}

/**
 * Generates code to update the data value for this port, handling the memory management for replacing the old data value.
 */
void VuoCompilerEventPort::generateReplaceData(Module *module, BasicBlock *block, Value *nodeContextValue, Value *dataPointer, Value *portContextValue)
{
	if (! portContextValue)
		portContextValue = generateGetPortContext(module, block, nodeContextValue);

	Value *oldDataPointer = VuoCompilerCodeGenUtilities::generateGetPortContextDataVariable(module, block, portContextValue, getDataType());

	VuoCompilerType *type = getDataType();
	type->generateRetainCall(module, block, dataPointer);
	type->generateReleaseCall(module, block, oldDataPointer);
	/// @todo if dataPointer == oldDataPointer, don't bother retaining/releasing

	Value *dataValue = new LoadInst(dataPointer, "", false, block);
	VuoCompilerCodeGenUtilities::generateSetPortContextData(module, block, portContextValue, dataValue, type);
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
