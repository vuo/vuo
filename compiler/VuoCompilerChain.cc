/**
 * @file
 * VuoCompilerChain implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerChain.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerNode.hh"

/**
 * Creates a chain consisting of the given list of nodes.
 *
 * @param nodes A linear sequence of nodes.
 * @param lastNodeInLoop True if this chain represents a single node that is the last (and only repeated) node in a feedback loop.
 */
VuoCompilerChain::VuoCompilerChain(vector<VuoCompilerNode *> nodes, bool lastNodeInLoop)
{
	this->nodes = nodes;
	this->lastNodeInLoop = lastNodeInLoop;
}

/**
 * Generates code to create a context that can be passed to chain worker functions.
 * The context contains the composition state and the event ID.
 */
Value * VuoCompilerChain::generateMakeContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *eventIdValue)
{
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	// unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long));
	// *eventIdPtr = eventId;

	Value *eventIdPtrValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, eventIdValue->getType(), 1);
	new StoreInst(eventIdValue, eventIdPtrValue, "", block);

	// size_t contextBytes = 2 * sizeof(void *);
	// void **context = (void **)malloc(contextBytes);
	// context[0] = (void *)compositionState;
	// context[1] = (void *)eventIdPtr;

	Value *contextValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, voidPointerType, 2);

	BitCastInst *compositionStateAsVoidPointer = new BitCastInst(compositionStateValue, voidPointerType, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 0, compositionStateAsVoidPointer);

	BitCastInst *eventIdPtrValueAsVoidPointer = new BitCastInst(eventIdPtrValue, voidPointerType, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 1, eventIdPtrValueAsVoidPointer);

	// VuoRegister(context, vuoFreeChainWorkerContext);

	VuoCompilerCodeGenUtilities::generateRegisterCall(module, block, contextValue, getFreeContextFunction(module));

	// VuoRetain(compositionState);

	VuoCompilerCodeGenUtilities::generateRetainCall(module, block, compositionStateValue);

	return new BitCastInst(contextValue, voidPointerType, "", block);
}

/**
 * Generates code that retrieves the composition identifier from the context argument of a chain worker function.
 */
Value * VuoCompilerChain::generateCompositionStateValue(Module *module, BasicBlock *block, Value *contextValue)
{
	Type *compositionStatePointerType = PointerType::get(VuoCompilerCodeGenUtilities::getCompositionStateType(module), 0);
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	// VuoCompositionState *compositionState = (VuoCompositionState *)((void **)context)[0]);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *compositionStateAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 0);
	return new BitCastInst(compositionStateAsVoidPointer, compositionStatePointerType, "", block);
}

/**
 * Generates code that retrieves the event ID from the context argument of a chain worker function.
 */
Value * VuoCompilerChain::generateEventIdValue(Module *module, BasicBlock *block, Value *contextValue)
{
	Type *eventIdPtrType = PointerType::get(IntegerType::get(module->getContext(), 64), 0);
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	// unsigned long *eventIdPtr = (unsigned long *)((void **)context)[1]);
	// unsigned long eventId = *eventIdPtr;

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *eventIdPtrValueAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 1);
	Value *eventIdPtrValue = new BitCastInst(eventIdPtrValueAsVoidPointer, eventIdPtrType, "", block);
	return new LoadInst(eventIdPtrValue, "", block);
}

/**
 * Generates code that deallocates the context argument of a chain worker function.
 */
Function * VuoCompilerChain::getFreeContextFunction(Module *module)
{
	// void vuoFreeChainWorkerContext(void *context)
	// {
	//   VuoRelease(((void **)context)[0]);
	//   free(((void **)context)[1]);
	//   free(context);
	// }

	const char *functionName = "vuoFreeChainWorkerContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerPointerType = PointerType::get(voidPointerType, 0);
		ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(64, 0));
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));

		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), voidPointerType, false);
		function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);

		Value *contextValue = function->arg_begin();
		BitCastInst *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);

		Value *compositionStateVariable = GetElementPtrInst::Create(contextValueAsVoidPointerArray, zeroValue, "", block);
		Value *compositionStateValue = new LoadInst(compositionStateVariable, "", block);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, compositionStateValue);

		Value *eventIdPtrVariable = GetElementPtrInst::Create(contextValueAsVoidPointerArray, oneValue, "", block);
		Value *eventIdPtrValue = new LoadInst(eventIdPtrVariable, "", block);
		CallInst::Create(freeFunction, eventIdPtrValue, "", block);

		CallInst::Create(freeFunction, contextValue, "", block);

		ReturnInst::Create(module->getContext(), block);
	}

	return function;
}

/**
 * Generates code that schedules the worker function for this chain to execute on the global dispatch queue.
 *
 * @return The scheduled function. The caller is responsible for filling in the body of this function.
 */
Function * VuoCompilerChain::generateScheduleWorker(Module *module, BasicBlock *block, Value *compositionStateValue,
													Value *contextValue, string triggerIdentifier,
													int minThreadsNeeded, int maxThreadsNeeded, size_t chainIndex,
													vector<size_t> upstreamChainIndices)
{
	Type *voidPointerType = contextValue->getType();
	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), voidPointerType, false);

	string workerFunctionName = triggerIdentifier + "__" + nodes.front()->getIdentifier();
	if (lastNodeInLoop)
		workerFunctionName += "__loop";
	workerFunctionName += "__worker";
	Function *workerFunction = Function::Create(workerFunctionType, GlobalValue::InternalLinkage, workerFunctionName, module);

	Value *globalQueue = VuoCompilerCodeGenUtilities::generateGetGlobalDispatchQueue(module, block);

	Value *eventIdValue = generateEventIdValue(module, block, contextValue);

	VuoCompilerCodeGenUtilities::generateScheduleChainWorker(module, block, globalQueue, contextValue, workerFunction,
															 minThreadsNeeded, maxThreadsNeeded,
															 eventIdValue, compositionStateValue, chainIndex, upstreamChainIndices);

	return workerFunction;
}

/**
 * Returns the sequence of nodes in this chain.
 */
vector<VuoCompilerNode *> VuoCompilerChain::getNodes(void)
{
	return nodes;
}

/**
 * Returns true if this chain represents a single node that is the last (and only repeated) node in a feedback loop.
 */
bool VuoCompilerChain::isLastNodeInLoop(void)
{
	return lastNodeInLoop;
}
