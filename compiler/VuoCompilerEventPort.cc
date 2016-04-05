/**
 * @file
 * VuoCompilerEventPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerEventPort.hh"

#include "VuoPort.hh"

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
 * Generates code to allocate this port.
 */
void VuoCompilerEventPort::generateAllocation(Module *module, string nodeInstanceIdentifier)
{
	VuoCompilerPort::generateAllocation(module, nodeInstanceIdentifier);

	if (data)
		data->generateAllocation(module, nodeInstanceIdentifier);
}

/**
 * Generates code to set the port's value to @c value.
 */
StoreInst * VuoCompilerEventPort::generateStore(Value *value, BasicBlock *block)
{
	// Convert value to match variable's bit width.
	unsigned valueBitWidth = ((IntegerType *)value->getType())->getBitWidth();
	unsigned variableBitWidth = ((IntegerType *)((PointerType *)variable->getType())->getElementType())->getBitWidth();
	if (valueBitWidth > variableBitWidth)
		value = new TruncInst(value, IntegerType::get(block->getContext(), variableBitWidth), "", block);
	else if (valueBitWidth < variableBitWidth)
		value = new ZExtInst(value, IntegerType::get(block->getContext(), variableBitWidth), "", block);

	return VuoCompilerPort::generateStore(value, block);
}

/**
 * Generates code to set the port's value to bool @c value.
 */
StoreInst * VuoCompilerEventPort::generateStore(bool value, BasicBlock *block)
{
	Constant *valueConstant = getBoolConstant(value);
	return VuoCompilerPort::generateStore(valueConstant, block);
}

/**
 * Returns the variable that stores this port's data, or NULL if this port is event-only.
 */
GlobalVariable * VuoCompilerEventPort::getDataVariable(void)
{
	VuoCompilerData *data = getData();
	return (data == NULL ? NULL : data->getVariable());
}

/**
 * Returns constant with value @c value.
 */
Constant * VuoCompilerEventPort::getBoolConstant(bool value)
{
	return ConstantInt::get(getBase()->getClass()->getCompiler()->getType(), value ? 1 : 0);
}

/**
 * Distinguishes the variable for the event from the variable for the data in data-and-event ports.
 */
string VuoCompilerEventPort::getVariableBaseName(void)
{
	return VuoCompilerPort::getVariableBaseName() + "_event";
}

/**
 * Returns a unique, consistent identifier for this port.
 *
 * Assumes @c generateAllocation has been called.
 */
string VuoCompilerEventPort::getIdentifier(void)
{
	return getVariable()->getName();
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
