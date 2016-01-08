/**
 * @file
 * VuoCompilerChain implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerChain.hh"
#include "VuoCompilerCodeGenUtilities.hh"


/**
 * Creates a linear chain of nodes that would be pushed by the given trigger.
 *
 * Assumes nodes is not empty.
 */
VuoCompilerChain::VuoCompilerChain(vector<VuoCompilerNode *> nodes, bool isLastNodeInLoop)
{
	this->nodes = nodes;
	this->isLastNodeInLoop = isLastNodeInLoop;
	dispatchGroupVariable = NULL;
	numUpstreamChains = 0;
}

/**
 * Generates the allocation of this chain's dispatch group.
 */
void VuoCompilerChain::generateAllocationForDispatchGroup(Module *module, BasicBlock *block, string triggerIdentifier)
{
	string identifier = triggerIdentifier + "__" + nodes.at(0)->getIdentifier();
	if (isLastNodeInLoop)
		identifier += "__loop";
	dispatchGroupVariable = VuoCompilerCodeGenUtilities::generateAllocationForDispatchGroup(module, block, identifier);
}

/**
 * Generates the initialization of this chain's dispatch group, initializing it to a new dispatch group.
 */
void VuoCompilerChain::generateInitializationForDispatchGroup(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateInitializationForDispatchGroup(module, block, dispatchGroupVariable);
}

/**
 * Generates the finalization of this chain's dispatch group.
 */
void VuoCompilerChain::generateFinalizationForDispatchGroup(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, dispatchGroupVariable);
}

/**
 * Generates code that submits a function for asynchronous execution and associates it with
 * this chain's dispatch group.
 *
 * The event ID and the dispatch groups for upstream chains are passed in the context argument for
 * the submitted function.
 *
 * @return The submitted function. The caller is responsible for filling in the body of this function.
 */
Function * VuoCompilerChain::generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, Value *eventIdValue, vector<VuoCompilerChain *> upstreamChains)
{
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(voidPointerType);
	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()),
														 workerFunctionParams,
														 false);

	string workerFunctionName = dispatchGroupVariable->getName().str() + "__worker";
	Function *workerFunction = module->getFunction(workerFunctionName);
	if (! workerFunction) {
		workerFunction = Function::Create(workerFunctionType,
										  GlobalValue::ExternalLinkage,
										  workerFunctionName,
										  module);
	}


	// void **context = malloc(sizeof(void *) * (1 + numUpstreamChains));
	// unsigned long *eventIDPtr = malloc(sizeof(unsigned long));
	// *eventIDPtr = eventID;
	// context[0] = (void *)eventIDPtr;
	// for (int i = 0; i < numUpstreamChains; ++i)
	// {
	//   context[i] = (void *)dispatchGroupForChain[i];
	//   dispatch_retain(dispatchGroupForChain[i]);
	// }

	numUpstreamChains = upstreamChains.size();
	ConstantInt *arraySizeValue = ConstantInt::get(module->getContext(), APInt(64, upstreamChains.size() + 1));
	Value *contextAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, voidPointerType, arraySizeValue);

	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
	Instruction *eventIdElement = GetElementPtrInst::Create(contextAddress, zeroValue, "", block);
	Value *eventIdAddress = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, eventIdValue->getType(), oneValue);
	new StoreInst(eventIdValue, eventIdAddress, false, block);
	BitCastInst *eventIdAddressAsVoidPointer = new BitCastInst(eventIdAddress, voidPointerType, "", block);
	new StoreInst(eventIdAddressAsVoidPointer, eventIdElement, false, block);

	for (int i = 0; i < upstreamChains.size(); ++i)
	{
		AllocaInst *dispatchGroupVariableForChain = upstreamChains.at(i)->dispatchGroupVariable;

		ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(64, i + 1));
		Instruction *dispatchGroupElement = GetElementPtrInst::Create(contextAddress, iValue, "", block);
		Value *dispatchGroup = new LoadInst(dispatchGroupVariableForChain, "", false, block);
		Value *dispatchGroupAsVoidPointer = new BitCastInst(dispatchGroup, voidPointerType, "", block);
		new StoreInst(dispatchGroupAsVoidPointer, dispatchGroupElement, false, block);

		VuoCompilerCodeGenUtilities::generateRetainForDispatchObject(module, block, dispatchGroupVariableForChain);
	}

	Value *contextAddressAsVoidPointer = new BitCastInst(contextAddress, voidPointerType, "", block);


	VuoCompilerCodeGenUtilities::generateSubmissionForDispatchGroup(module, block, dispatchGroupVariable, workerFunction, contextAddressAsVoidPointer);

	return workerFunction;
}

/**
 * Generates code that waits on this chain's dispatch group.
 */
void VuoCompilerChain::generateWaitForDispatchGroup(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, dispatchGroupVariable);
}

/**
 * Generates code that retrieves the event ID from the worker function's context argument.
 *
 * Assumes the worker function came from the return value of generateSubmissionForDispatchGroup().
 */
Value * VuoCompilerChain::generateEventIdValue(Module *module, Function *workerFunction, BasicBlock *block)
{
	// void **context = (void **)arg;
	// void *eventIdPtr = context[0];
	// free(eventIdPtr);

	Function::arg_iterator args = workerFunction->arg_begin();
	Value *contextAddressAsVoidPointer = args++;
	contextAddressAsVoidPointer->setName("context");

	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	PointerType *pointerToVoidPointerType = PointerType::get(voidPointerType, 0);

	BitCastInst *contextAddress = new BitCastInst(contextAddressAsVoidPointer, pointerToVoidPointerType, "", block);

	PointerType *pointerToIntegerType = PointerType::get(IntegerType::get(module->getContext(), 64), 0);
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	Instruction *eventIdElement = GetElementPtrInst::Create(contextAddress, zeroValue, "", block);
	LoadInst *eventIdAddressAsVoidPointer = new LoadInst(eventIdElement, "", false, block);
	BitCastInst *eventIdAddress = new BitCastInst(eventIdAddressAsVoidPointer, pointerToIntegerType, "", block);
	LoadInst *eventIdValue = new LoadInst(eventIdAddress, "", false, block);

	Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
	CallInst::Create(freeFunction, eventIdAddressAsVoidPointer, "", block);

	return eventIdValue;
}

/**
 * Generates code that waits on the dispatch groups from the worker function's context argument.
 *
 * Assumes the worker function came from the return value of generateSubmissionForDispatchGroup().
 */
void VuoCompilerChain::generateWaitForUpstreamChains(Module *module, Function *workerFunction, BasicBlock *block)
{
	// void **context = (void **)arg;
	// for (int i = 0; i < numUpstreamChains; ++i)
	// {
	//   dispatch_group_t dispatchGroupForChain = (dispatch_group_t)context[i];
	//   dispatch_group_wait(dispatchGroupForChain, DISPATCH_TIME_FOREVER);
	//   dispatch_release(dispatchGroupForChain);
	// }

	Function::arg_iterator args = workerFunction->arg_begin();
	Value *contextAddressAsVoidPointer = args++;
	contextAddressAsVoidPointer->setName("context");

	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	PointerType *pointerToVoidPointerType = PointerType::get(voidPointerType, 0);

	BitCastInst *contextAddress = new BitCastInst(contextAddressAsVoidPointer, pointerToVoidPointerType, "", block);

	for (int i = 0; i < numUpstreamChains; ++i)
	{
		AllocaInst *dispatchGroupVariable = VuoCompilerCodeGenUtilities::generateAllocationForDispatchGroup(module, block, "");
		Type *dispatchGroupType = dispatchGroupVariable->getAllocatedType();

		ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(64, i + 1));
		Instruction *dispatchGroupElement = GetElementPtrInst::Create(contextAddress, iValue, "", block);
		LoadInst *dispatchGroupAsVoidPointer = new LoadInst(dispatchGroupElement, "", false, block);
		BitCastInst *dispatchGroup = new BitCastInst(dispatchGroupAsVoidPointer, dispatchGroupType, "", block);

		new StoreInst(dispatchGroup, dispatchGroupVariable, false, block);
		VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, dispatchGroupVariable);

		VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, dispatchGroupVariable);
	}
}

/**
 * Generates code that frees the worker function's context argument.
 *
 * Assumes the worker function came from the return value of generateSubmissionForDispatchGroup().
 */
void VuoCompilerChain::generateFreeContextArgument(Module *module, Function *workerFunction, BasicBlock *block)
{
	// void **context = (void **)arg;
	// free(context);

	Function::arg_iterator args = workerFunction->arg_begin();
	Value *contextAddressAsVoidPointer = args++;
	contextAddressAsVoidPointer->setName("context");

	Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
	CallInst::Create(freeFunction, contextAddressAsVoidPointer, "", block);
}

/**
 * Returns the sequence of nodes in this chain.
 */
vector<VuoCompilerNode *> VuoCompilerChain::getNodes(void)
{
	return nodes;
}
