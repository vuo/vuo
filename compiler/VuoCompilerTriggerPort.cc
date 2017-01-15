/**
 * @file
 * VuoCompilerTriggerPort implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoPort.hh"

/**
 * Creates a trigger port based on @c portClass.
 */
VuoCompilerTriggerPort::VuoCompilerTriggerPort(VuoPort * basePort)
	: VuoCompilerPort(basePort)
{
}

/**
 * Generates code to create a heap-allocated PortContext, with the `triggerQueue` and `triggerSemaphore` initialized.
 *
 * @return A value of type `PortContext *`.
 */
Value * VuoCompilerTriggerPort::generateCreatePortContext(Module *module, BasicBlock *block)
{
	VuoType *dataType = getClass()->getDataVuoType();

	return VuoCompilerCodeGenUtilities::generateCreatePortContext(module, block,
																  dataType ? dataType->getCompiler()->getType() : NULL,
																  true, "org.vuo.composition." + getIdentifier());
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 *
 * Assumes @a block is inside the function associated with this trigger. Passes the argument of that
 * function, if any, as the context which will be passed to the worker function.
 */
void VuoCompilerTriggerPort::generateAsynchronousSubmissionToDispatchQueue(Module *module, Function *function, BasicBlock *block,
																		   Value *compositionIdentifierValue, Value *portContextValue,
																		   VuoType *dataType, Function *workerFunction)
{
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	Value *dataCopyAsVoidPointer;
	if (dataType)
	{
		// PortDataType_retain(data);
		// PortDataType *dataCopy = (void *)malloc(sizeof(VuoImage));
		// *dataCopy = data;
		Value *dataValue = VuoCompilerCodeGenUtilities::unlowerArgument(dataType->getCompiler(), function, 0, module, block);
		dataType->getCompiler()->generateRetainCall(module, block, dataValue);
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
		Value *dataCopyAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, dataType->getCompiler()->getType(), oneValue);
		new StoreInst(dataValue, dataCopyAddress, false, block);
		dataCopyAsVoidPointer = new BitCastInst(dataCopyAddress, voidPointer, "", block);
	}
	else
	{
		dataCopyAsVoidPointer = ConstantPointerNull::get(voidPointer);
	}

	// void **context = (void **)malloc(2 * sizeof(void *));
	// context[0] = (void *)compositionIdentifier;
	// context[1] = (void *)dataCopy;
	ConstantInt *twoValue = ConstantInt::get(module->getContext(), APInt(64, 2));
	Value *contextValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, voidPointer, twoValue);
	Value *compositionIdentifierAsVoidPointer = new BitCastInst(compositionIdentifierValue, voidPointer, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 0, compositionIdentifierAsVoidPointer);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 1, dataCopyAsVoidPointer);
	Value *contextAsVoidPointer = new BitCastInst(contextValue, voidPointer, "", block);

	Value *dispatchQueueValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerQueue(module, block, portContextValue);
	VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(module, block, dispatchQueueValue, workerFunction, contextAsVoidPointer);
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 */
Function * VuoCompilerTriggerPort::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *nodeContextValue,
																				string workerFunctionName, Value *workerFunctionArg)
{
	Function *workerFunction = getWorkerFunction(module, workerFunctionName);

	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *argAsVoidPointer;
	if (workerFunctionArg)
		argAsVoidPointer = new BitCastInst(workerFunctionArg, voidPointer, "", block);
	else
		argAsVoidPointer = ConstantPointerNull::get(voidPointer);

	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *dispatchQueueValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerQueue(module, block, portContextValue);
	VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(module, block, dispatchQueueValue, workerFunction, argAsVoidPointer);

	return workerFunction;
}

/**
 * Returns a function of type @c dispatch_function_t.
 */
Function * VuoCompilerTriggerPort::getWorkerFunction(Module *module, string functionName, bool isExternal)
{
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(voidPointer);
	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()),
														 workerFunctionParams,
														 false);

	Function *workerFunction = module->getFunction(functionName);
	if (! workerFunction) {
		workerFunction = Function::Create(workerFunctionType,
										  isExternal ? GlobalValue::ExternalLinkage : GlobalValue::InternalLinkage,
										  functionName,
										  module);
	}

	return workerFunction;
}

/**
 * Generates code to try to claim the trigger's semaphore. Returns the value returned by `dispatch_semaphore_wait`,
 * which can be checked to determine if the semaphore was claimed.
 */
Value * VuoCompilerTriggerPort::generateNonBlockingWaitForSemaphore(Module *module, BasicBlock *block, Value *portContextValue)
{
	ConstantInt *timeoutDeltaValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	Value *timeoutValue = VuoCompilerCodeGenUtilities::generateCreateDispatchTime(module, block, timeoutDeltaValue);
	Value *dispatchSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerSemaphore(module, block, portContextValue);
	return VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, block, dispatchSemaphoreValue, timeoutValue);
}

/**
 * Generates code to signal the trigger's semaphore.
 */
void VuoCompilerTriggerPort::generateSignalForSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	Value *dispatchSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerSemaphore(module, block, portContextValue);
	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, dispatchSemaphoreValue);
}

/**
 * Generates code to get the function pointer for the trigger scheduler function.
 */
Value * VuoCompilerTriggerPort::generateLoadFunction(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextTriggerFunction(module, block, portContextValue, getClass()->getFunctionType());
}

/**
 * Generates code to set the function pointer for the trigger scheduler function.
 */
void VuoCompilerTriggerPort::generateStoreFunction(Module *module, BasicBlock *block, Value *nodeContextValue, Value *functionValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	VuoCompilerCodeGenUtilities::generateSetPortContextTriggerFunction(module, block, portContextValue, functionValue);
}

/**
 * Generates code to get the data most recently fired from the trigger.
 */
Value * VuoCompilerTriggerPort::generateLoadPreviousData(Module *module, BasicBlock *block, Value *nodeContextValue)
{
	Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
	return VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, getDataType());
}

/**
 * Generates code to deallocate the context and its contents created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
void VuoCompilerTriggerPort::generateFreeContext(Module *module, BasicBlock *block, Function *workerFunction)
{
	// free(((void **)context)[1]);
	// free(context);

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	PointerType *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	BitCastInst *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *dataValue = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 1);

	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, dataValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, block, contextValue);
}

/**
 * Generates code to get the composition identifier from the context created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
Value * VuoCompilerTriggerPort::generateCompositionIdentifierValue(Module *module, BasicBlock *block, Function *workerFunction)
{
	// char *compositionIdentifier = (char *)((void **)context)[0];

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);
	Type *charPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *compositionIdentifierAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 0);
	return new BitCastInst(compositionIdentifierAsVoidPointer, charPointerType, "", block);
}

/**
 * Generates code to get the trigger data value from the context created by generateAsynchronousSubmissionToDispatchQueue()
 * and passed to @a workerFunction.
 */
Value * VuoCompilerTriggerPort::generateDataValue(Module *module, BasicBlock *block, Function *workerFunction)
{
	// PortDataType *dataCopy = (PortDataType *)((void **)context)[1]);
	// PortDataType data = *dataCopy;

	Value *contextValue = workerFunction->arg_begin();
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *dataCopyAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 1);
	PointerType *pointerToDataType = PointerType::get(getDataType(), 0);
	Value *dataCopyValue = new BitCastInst(dataCopyAsVoidPointer, pointerToDataType, "", block);
	return new LoadInst(dataCopyValue, "", block);
}

/**
 * Generates code to update the trigger's data value (if any) with the worker function argument.
 *
 * @return The trigger's new data value.
 */
Value * VuoCompilerTriggerPort::generateDataValueUpdate(Module *module, BasicBlock *block, Function *workerFunction, Value *nodeContextValue)
{
	Value *currentDataValue = NULL;
	Type *dataType = getDataType();

	if (dataType)
	{
		// Load the previous data.
		Value *portContextValue = generateGetPortContext(module, block, nodeContextValue);
		Value *previousDataValue = VuoCompilerCodeGenUtilities::generateGetPortContextData(module, block, portContextValue, dataType);

		// Load the current data.
		//
		// PortDataType *dataCopy = (PortDataType *)((void **)context)[1]);
		// PortDataType data = *dataCopy;
		currentDataValue = generateDataValue(module, block, workerFunction);

		// The current data becomes the previous data.
		//
		// PortDataType_release(previousData);
		// previousData = data;
		VuoCompilerCodeGenUtilities::generateSetPortContextData(module, block, portContextValue, currentDataValue);
		VuoCompilerType *type = getClass()->getDataVuoType()->getCompiler();
		type->generateReleaseCall(module, block, previousDataValue);
	}

	return currentDataValue;
}

/**
 * Generates code to discard the scheduler function argument without updating the trigger's data value.
 */
void VuoCompilerTriggerPort::generateDataValueDiscardFromScheduler(Module *module, Function *function, BasicBlock *block,
																   VuoType *dataType)
{
	Value *dataValue = VuoCompilerCodeGenUtilities::unlowerArgument(dataType->getCompiler(), function, 0, module, block);
	dataType->getCompiler()->generateRetainCall(module, block, dataValue);
	dataType->getCompiler()->generateReleaseCall(module, block, dataValue);
}

/**
 * Generates code to discard the data value in the worker function argument without updating the trigger's data value.
 */
void VuoCompilerTriggerPort::generateDataValueDiscardFromWorker(Module *module, BasicBlock *block, Function *workerFunction)
{
	VuoType *dataType = getClass()->getDataVuoType();

	if (dataType)
	{
		Value *dataValue = generateDataValue(module, block, workerFunction);
		dataType->getCompiler()->generateReleaseCall(module, block, dataValue);
	}
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
