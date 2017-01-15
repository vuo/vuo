/**
 * @file
 * VuoCompilerChain implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
 * The context contains an event ID and an array of dispatch groups.
 * Each dispatch group has a block count of 1.
 */
Value * VuoCompilerChain::generateMakeContext(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *eventIdValue,
											  const vector<VuoCompilerChain *> &chains,
											  const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream)
{
	Type *dispatchGroupType = VuoCompilerCodeGenUtilities::getDispatchGroupType(module);
	PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));

	// size_t chainGroupsBytes = 3 * sizeof(dispatch_group_t);
	// dispatch_group_t *chainGroups = (dispatch_group_t *)malloc(chainGroupsBytes);

	ConstantInt *chainGroupsArrayLength = ConstantInt::get(module->getContext(), APInt(64, chains.size()));
	Value *chainGroupsArrayValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, dispatchGroupType, chainGroupsArrayLength);

	// chainGroups[0] = dispatch_group_create();
	// dispatch_group_enter(chainGroups[0]);
	// chainGroups[1] = dispatch_group_create();
	// dispatch_group_enter(chainGroups[1]);
	// ...

	for (size_t i = 0; i < chains.size(); ++i)
	{
		Value *chainGroupValue = VuoCompilerCodeGenUtilities::generateCreateDispatchGroup(module, block);
		ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(64, i));
		Value *chainGroupVariable = GetElementPtrInst::Create(chainGroupsArrayValue, iValue, "", block);
		new StoreInst(chainGroupValue, chainGroupVariable, false, block);
		VuoCompilerCodeGenUtilities::generateEnterDispatchGroup(module, block, chainGroupValue);

		// Increment the retain count to equal the number of scheduled chains that will release the dispatch group —
		// 1 for each downstream chain that will wait on the dispatch group, plus 1 for this chain.
		// The retain count is 1 after dispatch_group_create(), so increment it by 1 less than the above sum.
		map<VuoCompilerChain *, vector<VuoCompilerChain *> >::const_iterator downstreamIter = chainsImmediatelyDownstream.find(chains[i]);
		if (downstreamIter != chainsImmediatelyDownstream.end())
		{
			size_t numChainsImmediatelyDownstream = downstreamIter->second.size();
			for (size_t j = 0; j < numChainsImmediatelyDownstream; ++j)
				VuoCompilerCodeGenUtilities::generateRetainForDispatchObject(module, block, chainGroupVariable);
		}
	}

	// unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long));
	// *eventIdPtr = eventId;

	Value *eventIdPtrValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, eventIdValue->getType(), oneValue);
	new StoreInst(eventIdValue, eventIdPtrValue, "", block);

	// size_t contextBytes = 2 * sizeof(void *);
	// void **context = (void **)malloc(contextBytes);
	// context[0] = (void *)compositionIdentifier;
	// context[1] = (void *)eventIdPtr;
	// context[2] = (void *)chainGroups;

	ConstantInt *contextArrayLength = ConstantInt::get(module->getContext(), APInt(64, 3));
	Value *contextValue = VuoCompilerCodeGenUtilities::generateMemoryAllocation(module, block, voidPointerType, contextArrayLength);

	BitCastInst *compositionIdentifierAsVoidPointer = new BitCastInst(compositionIdentifierValue, voidPointerType, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 0, compositionIdentifierAsVoidPointer);

	BitCastInst *eventIdPtrValueAsVoidPointer = new BitCastInst(eventIdPtrValue, voidPointerType, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 1, eventIdPtrValueAsVoidPointer);

	BitCastInst *chainGroupsAsVoidPointer = new BitCastInst(chainGroupsArrayValue, voidPointerType, "", block);
	VuoCompilerCodeGenUtilities::generateSetArrayElement(module, block, contextValue, 2, chainGroupsAsVoidPointer);

	// VuoRegister(context, vuoFreeChainWorkerContext);

	VuoCompilerCodeGenUtilities::generateRegisterCall(module, block, contextValue, getFreeContextFunction(module));

	return new BitCastInst(contextValue, voidPointerType, "", block);
}

/**
 * Generates code that retrieves the composition identifier from the context argument of a chain worker function.
 */
Value * VuoCompilerChain::generateCompositionIdentifierValue(Module *module, BasicBlock *block, Value *contextValue)
{
	Type *charPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	// unsigned long *eventIdPtr = (unsigned long *)((void **)context)[0]);
	// unsigned long eventId = *eventIdPtr;

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *compositionIdentifierAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 0);
	return new BitCastInst(compositionIdentifierAsVoidPointer, charPointerType, "", block);
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
 * Generates code that retrieves the array of dispatch groups from the context argument of a chain worker function.
 */
Value * VuoCompilerChain::generateChainGroupsValue(Module *module, BasicBlock *block, Value *contextValue)
{
	Type *dispatchGroupType = VuoCompilerCodeGenUtilities::getDispatchGroupType(module);
	Type *dispatchGroupPointerType =  PointerType::get(dispatchGroupType, 0);
	Type *voidPointerType = contextValue->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);

	// dispatch_group_t *chainGroups = (dispatch_group_t *)((void **)context)[2]);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);
	Value *chainGroupsValueAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, block, contextValueAsVoidPointerArray, 2);
	return new BitCastInst(chainGroupsValueAsVoidPointer, dispatchGroupPointerType, "", block);
}

/**
 * Generates code that deallocates the context argument of a chain worker function.
 */
Function * VuoCompilerChain::getFreeContextFunction(Module *module)
{
	// void vuoFreeChainWorkerContext(void *context)
	// {
	//   free(((void **)context)[1]);
	//   free(((void **)context)[2]);
	//   free(context);
	// }

	const char *functionName = "vuoFreeChainWorkerContext";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *voidPointerType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *voidPointerPointerType = PointerType::get(voidPointerType, 0);
		ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
		ConstantInt *twoValue = ConstantInt::get(module->getContext(), APInt(64, 2));

		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);

		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), voidPointerType, false);
		function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);
		BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);

		Value *contextValue = function->arg_begin();
		BitCastInst *contextValueAsVoidPointerArray = new BitCastInst(contextValue, voidPointerPointerType, "", block);

		Value *eventIdPtrVariable = GetElementPtrInst::Create(contextValueAsVoidPointerArray, oneValue, "", block);
		Value *chainGroupsVariable = GetElementPtrInst::Create(contextValueAsVoidPointerArray, twoValue, "", block);
		Value *eventIdPtrValue = new LoadInst(eventIdPtrVariable, "", block);
		Value *chainGroupsValue = new LoadInst(chainGroupsVariable, "", block);
		CallInst::Create(freeFunction, eventIdPtrValue, "", block);
		CallInst::Create(freeFunction, chainGroupsValue, "", block);

		CallInst::Create(freeFunction, contextValue, "", block);

		ReturnInst::Create(module->getContext(), block);
	}

	return function;
}

/**
 * Generates code that submits a function for asynchronous execution on the global dispatch queue.
 * @a contextValue is passed as an argument.
 *
 * @return The submitted function. The caller is responsible for filling in the body of this function.
 */
Function * VuoCompilerChain::generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, Value *contextValue,
																string triggerIdentifier)
{
	Type *voidPointerType = contextValue->getType();

	// dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	// dispatch_async_f(globalQueue, (void *)context, chainWorker);

	FunctionType *workerFunctionType = FunctionType::get(Type::getVoidTy(module->getContext()), voidPointerType, false);

	string workerFunctionName = triggerIdentifier + "__" + nodes.front()->getIdentifier();
	if (lastNodeInLoop)
		workerFunctionName += "__loop";
	workerFunctionName += "__worker";
	Function *workerFunction = Function::Create(workerFunctionType, GlobalValue::InternalLinkage, workerFunctionName, module);

	Value *globalQueue = VuoCompilerCodeGenUtilities::generateGetGlobalDispatchQueue(module, block);
	VuoCompilerCodeGenUtilities::generateAsynchronousSubmissionToDispatchQueue(module, block, globalQueue, workerFunction, contextValue);

	return workerFunction;
}

/**
 * Generates code that waits on the dispatch groups for all chains immediately upstream of this chain.
 */
void VuoCompilerChain::generateWaitForUpstreamChains(Module *module, BasicBlock *block, Value *contextValue,
													 const vector<size_t> &chainIndices)
{
	Type *dispatchGroupType = VuoCompilerCodeGenUtilities::getDispatchGroupType(module);

	// dispatch_group_t *chainGroups = (dispatch_group_t *)((void **)context)[2]);
	// dispatch_group_wait(chainGroups[chainIndex0]);
	// dispatch_release(chainGroups[chainIndex0]);
	// dispatch_group_wait(chainGroups[chainIndex1]);
	// dispatch_release(chainGroups[chainIndex1]);
	// ...

	Value *chainGroupsArrayValue = generateChainGroupsValue(module, block, contextValue);

	for (vector<size_t>::const_iterator i = chainIndices.begin(); i != chainIndices.end(); ++i)
	{
		ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(64, *i));
		Value *chainGroupVariable = GetElementPtrInst::Create(chainGroupsArrayValue, iValue, "", block);
		Value *chainGroupValueAsVoidPointer = new LoadInst(chainGroupVariable, "", block);
		Value *chainGroupValue = new BitCastInst(chainGroupValueAsVoidPointer, dispatchGroupType, "", block);

		VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, chainGroupValue);

		VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, chainGroupVariable);
	}
}

/**
 * Generates code that decrements the block count of the chain's dispatch group (indicating that other chains
 * no longer need to wait on it) and releases memory.
 */
void VuoCompilerChain::generateCleanupForWorkerFunction(Module *module, BasicBlock *block, Value *contextValue,
														size_t chainIndex, bool hasDownstreamChains)
{
	Type *dispatchGroupType = VuoCompilerCodeGenUtilities::getDispatchGroupType(module);

	// dispatch_group_t *chainGroups = (dispatch_group_t *)((void **)context)[2]);
	// dispatch_group_t chainGroup = chainGroups[chainIndex];

	Value *chainGroupsArrayValue = generateChainGroupsValue(module, block, contextValue);
	ConstantInt *iValue = ConstantInt::get(module->getContext(), APInt(64, chainIndex));
	Value *chainGroupVariable = GetElementPtrInst::Create(chainGroupsArrayValue, iValue, "", block);
	Value *chainGroupValueAsVoidPointer = new LoadInst(chainGroupVariable, "", block);
	Value *chainGroupValue = new BitCastInst(chainGroupValueAsVoidPointer, dispatchGroupType, "", block);

	// dispatch_group_leave(chainGroup);

	VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, block, chainGroupValue);

	// dispatch_release(chainGroup);

	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, chainGroupVariable);

	// VuoRelease(context);

	VuoCompilerCodeGenUtilities::generateReleaseCall(module, block, contextValue);
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
