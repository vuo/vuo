/**
 * @file
 * VuoCompilerTriggerAction implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTriggerAction.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerType.hh"

/**
 * Creates a trigger callback.
 */
VuoCompilerTriggerAction::VuoCompilerTriggerAction(VuoCompilerTriggerPort *port, VuoCompilerNode *node)
{
	this->port = port;
	this->node = node;
	dispatchQueueVariable = NULL;
	previousDataVariable = NULL;
}

/**
 * Generates code to allocate global variables for this trigger.
 */
void VuoCompilerTriggerAction::generateAllocation(Module *module)
{
	string identifier = port->getIdentifier();

	dispatchQueueVariable = VuoCompilerCodeGenUtilities::generateAllocationForDispatchQueue(module,
																							identifier + "__queue");

	Type *portDataType = getDataType();
	if (portDataType)
	{
		previousDataVariable = new GlobalVariable(*module,
												  portDataType,
												  false,
												  GlobalValue::InternalLinkage,
												  0,
												  identifier + "__previous");
	}
}

/**
 * Generates code to initialize the global variables for this trigger.
 */
void VuoCompilerTriggerAction::generateInitialization(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateInitializationForDispatchQueue(module, block, dispatchQueueVariable,
																		"org.vuo.composition." + port->getIdentifier());

	Type *portDataType = getDataType();
	if (portDataType)
	{
		previousDataVariable->setInitializer(Constant::getNullValue(portDataType));
	}
}

/**
 * Generates code to finalize the global variables for this trigger.
 */
void VuoCompilerTriggerAction::generateFinalization(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, dispatchQueueVariable);

	if (getDataType())
	{
		LoadInst *previousDataValue = new LoadInst(previousDataVariable, "", block);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, previousDataValue);
	}
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 *
 * Assumes @c block is inside the function associated with a @c VuoCompilerTriggerPort. Passes the argument of that
 * function, if any, as the context which will be passed to the worker function.
 */
Function * VuoCompilerTriggerAction::generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier)
{
	Function *workerFunction = getWorkerFunction(module, block, identifier);

	// Get the data value passed by the trigger, if any.
	Value *dataValueAsVoidPointer = NULL;
	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Type *portDataType = getDataType();
	if (portDataType == NULL)
	{
		dataValueAsVoidPointer = ConstantPointerNull::get(voidPointer);
	}
	else
	{
		// PortDataType *dataCopy = malloc(sizeof(PortDataType));
		// *dataCopy = data;
		// void *workerArg = (void *)dataCopy;
		Function *function = block->getParent();
		Value *dataValueFromArg = function->arg_begin();
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
		Value *dataCopyAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, portDataType, oneValue);
		if (dataValueFromArg->getType()->isPointerTy() && portDataType->isStructTy())
		{
			// Special case: Clang has lowered the struct parameter so that, rather than having type portDataType,
			// dataValueFromArg has type pointer-to-portDataType. Before typecasting, dereference the pointer.
			dataValueFromArg = new LoadInst(dataValueFromArg, "", false, block);
		}
		Value *dataValue = VuoCompilerCodeGenUtilities::generateTypeCast(module, block, dataValueFromArg, portDataType);
		new StoreInst(dataValue, dataCopyAddress, false, block);
		dataValueAsVoidPointer = new BitCastInst(dataCopyAddress, voidPointer, "", block);

		VuoCompilerCodeGenUtilities::generateRetainCall(module, block, dataValue);
	}

	VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, dataValueAsVoidPointer);

	return workerFunction;
}

/**
 * Generates code to submit a task to this trigger's dispatch queue. Returns the worker function, which will be called
 * by the dispatch queue to execute the task. The caller is responsible for filling in the body of the worker function.
 */
Function * VuoCompilerTriggerAction::generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier)
{
	Function *workerFunction = getWorkerFunction(module, block, identifier);

	PointerType *voidPointer = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Value *nullVoidPointer = ConstantPointerNull::get(voidPointer);

	VuoCompilerCodeGenUtilities::generateSynchronousSubmissionToDispatchQueue(module, block, dispatchQueueVariable, workerFunction, nullVoidPointer);

	return workerFunction;
}

/**
 * Returns a function of type @c dispatch_function_t.
 */
Function * VuoCompilerTriggerAction::getWorkerFunction(Module *module, BasicBlock *block, string identifier)
{
	PointerType *pointerToi8 = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(pointerToi8);
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
Value * VuoCompilerTriggerAction::generateDataValueUpdate(Module *module, BasicBlock *block, Function *triggerWorker)
{
	Value *currentDataValue = NULL;
	Type *portDataType = getDataType();

	if (portDataType)
	{
		// Load the previous data.
		LoadInst *previousDataValue = new LoadInst(previousDataVariable, "", block);

		// Load the current data.
		//
		// PortDataType *dataCopy = (PortDataType *)workerArg;
		// PortDataType data = *dataCopy;
		// free(dataCopy);
		PointerType *dataCopyPointerType = PointerType::get(portDataType, 0);
		Value *dataCopyAsVoidPointer = triggerWorker->arg_begin();
		Value *dataCopyAddress = new BitCastInst(dataCopyAsVoidPointer, dataCopyPointerType, "", block);
		currentDataValue = new LoadInst(dataCopyAddress, "", false, block);
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, dataCopyAsVoidPointer, "", block);

		// The current data becomes the previous data.
		new StoreInst(currentDataValue, previousDataVariable, block);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, previousDataValue);
	}

	return currentDataValue;
}

/**
 * Generates code to discard the worker function argument without updating the trigger's data value (if any).
 */
void VuoCompilerTriggerAction::generateDataValueDiscard(Module *module, BasicBlock *block, Function *triggerWorker)
{
	Value *currentDataValue = NULL;
	Type *portDataType = getDataType();

	if (portDataType)
	{
		// PortDataType *dataCopy = (PortDataType *)workerArg;
		// PortDataType data = *dataCopy;
		// free(dataCopy);
		// PortDataType_release(data);
		PointerType *dataCopyPointerType = PointerType::get(portDataType, 0);
		Value *dataCopyAsVoidPointer = triggerWorker->arg_begin();
		Value *dataCopyAddress = new BitCastInst(dataCopyAsVoidPointer, dataCopyPointerType, "", block);
		currentDataValue = new LoadInst(dataCopyAddress, "", false, block);
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, dataCopyAsVoidPointer, "", block);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, currentDataValue);
	}
}

/**
 * Returns the variable that stores the most recent data value fired from this trigger, or NULL if the trigger is event-only.
 */
GlobalVariable * VuoCompilerTriggerAction::getPreviousDataVariable(void)
{
	return previousDataVariable;
}

/**
 * Returns the type of data associated with the trigger, or NULL if the trigger is event-only.
 */
Type * VuoCompilerTriggerAction::getDataType(void)
{
	VuoType *vuoType = port->getClass()->getDataVuoType();
	return (vuoType == NULL ? NULL : vuoType->getCompiler()->getType());
}
