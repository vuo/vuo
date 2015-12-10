/**
 * @file
 * VuoCompilerTriggerPort implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerType.hh"
#include "VuoPort.hh"

/**
 * Creates a trigger port based on @c portClass.
 */
VuoCompilerTriggerPort::VuoCompilerTriggerPort(VuoPort * basePort)
	: VuoCompilerPort(basePort)
{
	function = NULL;
	dispatchQueueVariable = NULL;
	previousDataVariable = NULL;
}

/**
 * Generates code to allocate global variables for this trigger.
 */
void VuoCompilerTriggerPort::generateAllocation(Module *module, string nodeInstanceIdentifier)
{
	this->nodeInstanceIdentifier = nodeInstanceIdentifier;

	string identifier = getIdentifier();

	dispatchQueueVariable = VuoCompilerCodeGenUtilities::generateAllocationForDispatchQueue(module,
																							identifier + "__queue");

	Type *dataType = getDataType();
	if (dataType)
	{
		previousDataVariable = new GlobalVariable(*module,
												  dataType,
												  false,
												  GlobalValue::InternalLinkage,
												  0,
												  identifier + "__previous");
	}
}

/**
 * Generates code to initialize the global variables for this trigger.
 */
void VuoCompilerTriggerPort::generateInitialization(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateInitializationForDispatchQueue(module, block, dispatchQueueVariable,
																		"org.vuo.composition." + getIdentifier());

	Type *dataType = getDataType();
	if (dataType)
	{
		previousDataVariable->setInitializer(Constant::getNullValue(dataType));
	}
}

/**
 * Generates code to finalize the global variables for this trigger.
 */
void VuoCompilerTriggerPort::generateFinalization(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, dispatchQueueVariable);

	if (getDataType())
	{
		LoadInst *previousDataValue = new LoadInst(previousDataVariable, "", block);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, previousDataValue);
	}
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 *
 * Assumes @a block is inside the function associated with this trigger. Passes the argument of that
 * function, if any, as the context which will be passed to the worker function.
 */
Function * VuoCompilerTriggerPort::generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier)
{
	Function *workerFunction = getWorkerFunction(module, identifier);

	// Get the data value passed by the trigger, if any.
	Value *dataValueAsVoidPointer = NULL;
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Type *dataType = getDataType();
	if (dataType == NULL)
	{
		dataValueAsVoidPointer = ConstantPointerNull::get(voidPointer);
	}
	else
	{
		// PortDataType *dataCopy = malloc(sizeof(PortDataType));
		// *dataCopy = data;
		// void *workerArg = (void *)dataCopy;
		// PortDataType_retain(data);
		Function *function = block->getParent();
		Value *dataValueFromArg = function->arg_begin();
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
		Value *dataCopyAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, dataType, oneValue);
		if (dataValueFromArg->getType()->isPointerTy() && dataType->isStructTy())
		{
			// Special case: Clang has lowered the struct parameter so that, rather than having type portDataType,
			// dataValueFromArg has type pointer-to-portDataType. Before typecasting, dereference the pointer.
			dataValueFromArg = new LoadInst(dataValueFromArg, "", false, block);
		}
		Value *dataValue = VuoCompilerCodeGenUtilities::generateTypeCast(module, block, dataValueFromArg, dataType);
		new StoreInst(dataValue, dataCopyAddress, false, block);
		dataValueAsVoidPointer = new BitCastInst(dataCopyAddress, voidPointer, "", block);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateRetainCall(module, block, dataValue);
	}

	VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, dataValueAsVoidPointer);

	return workerFunction;
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 */
Function * VuoCompilerTriggerPort::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier)
{
	Function *workerFunction = getWorkerFunction(module, identifier);

	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *nullVoidPointer = ConstantPointerNull::get(voidPointer);

	VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, nullVoidPointer);

	return workerFunction;
}

/**
 * Returns a function of type @c dispatch_function_t.
 */
Function * VuoCompilerTriggerPort::getWorkerFunction(Module *module, string identifier)
{
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(voidPointer);
	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()),
														 workerFunctionParams,
														 false);

	string workerFunctionName = identifier + "__worker";
	Function *workerFunction = module->getFunction(workerFunctionName);
	if (! workerFunction) {
		workerFunction = Function::Create(workerFunctionType,
										  GlobalValue::ExternalLinkage,
										  workerFunctionName,
										  module);
	}

	return workerFunction;
}

/**
 * Generates code to update the trigger's data value (if any) with the worker function argument.
 *
 * @return The trigger's new data value.
 */
Value * VuoCompilerTriggerPort::generateDataValueUpdate(Module *module, BasicBlock *block, Function *triggerWorker)
{
	Value *currentDataValue = NULL;
	Type *dataType = getDataType();

	if (dataType)
	{
		// Load the previous data.
		LoadInst *previousDataValue = new LoadInst(previousDataVariable, "", block);

		// Load the current data.
		//
		// PortDataType *dataCopy = (PortDataType *)workerArg;
		// PortDataType data = *dataCopy;
		// free(dataCopy);
		PointerType *dataCopyPointerType = PointerType::get(dataType, 0);
		Value *dataCopyAsVoidPointer = triggerWorker->arg_begin();
		Value *dataCopyAddress = new BitCastInst(dataCopyAsVoidPointer, dataCopyPointerType, "", block);
		currentDataValue = new LoadInst(dataCopyAddress, "", false, block);
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, dataCopyAsVoidPointer, "", block);

		// The current data becomes the previous data.
		//
		// PortDataType_release(previousData);
		// previousData = data;
		new StoreInst(currentDataValue, previousDataVariable, block);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, previousDataValue);
	}

	return currentDataValue;
}

/**
 * Generates code to discard the worker function argument without updating the trigger's data value (if any).
 */
void VuoCompilerTriggerPort::generateDataValueDiscard(Module *module, BasicBlock *block, Function *triggerWorker)
{
	Value *currentDataValue = NULL;
	Type *dataType = getDataType();

	if (dataType)
	{
		// PortDataType *dataCopy = (PortDataType *)workerArg;
		// PortDataType data = *dataCopy;
		// free(dataCopy);
		// PortDataType_release(data);
		PointerType *dataCopyPointerType = PointerType::get(dataType, 0);
		Value *dataCopyAsVoidPointer = triggerWorker->arg_begin();
		Value *dataCopyAddress = new BitCastInst(dataCopyAsVoidPointer, dataCopyPointerType, "", block);
		currentDataValue = new LoadInst(dataCopyAddress, "", false, block);
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, dataCopyAsVoidPointer, "", block);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, currentDataValue);
	}
}

/**
 * Does nothing.
 */
LoadInst * VuoCompilerTriggerPort::generateLoad(BasicBlock *block)
{
	// do nothing -- overrides the superclass implementation
	return NULL;
}

/**
 * Does nothing.
 */
StoreInst * VuoCompilerTriggerPort::generateStore(Value *value, BasicBlock *block)
{
	// do nothing -- overrides the superclass implementation
	return NULL;
}

/**
 * Sets the function that's called each time the trigger port generates a push.
 */
void VuoCompilerTriggerPort::setFunction(Function *function)
{
	this->function = function;
}

/**
 * Returns the function that's called each time the trigger port generates a push.
 */
Function * VuoCompilerTriggerPort::getFunction(void)
{
	return function;
}

/**
 * Returns the variable that stores the most recent data value fired from this trigger, or NULL if the trigger is event-only.
 */
GlobalVariable * VuoCompilerTriggerPort::getPreviousDataVariable(void)
{
	return previousDataVariable;
}

/**
 * Returns the type of data associated with the trigger, or NULL if the trigger is event-only.
 */
Type * VuoCompilerTriggerPort::getDataType(void)
{
	VuoType *vuoType = getClass()->getDataVuoType();
	return (vuoType == NULL ? NULL : vuoType->getCompiler()->getType());
}

/**
 * Returns the trigger port class of this trigger port.
 */
VuoCompilerTriggerPortClass * VuoCompilerTriggerPort::getClass(void)
{
	return (VuoCompilerTriggerPortClass *)(getBase()->getClass()->getCompiler());
}

/**
 * Returns a unique, consistent identifier for this port.
 *
 * Assumes @c generateAllocation has been called.
 */
string VuoCompilerTriggerPort::getIdentifier(void)
{
	return nodeInstanceIdentifier + "__" + getClass()->getBase()->getName();
}
