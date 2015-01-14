/**
 * @file
 * VuoCompilerChain implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
}

/**
 * Generates the allocation of this chain's dispatch group.
 */
void VuoCompilerChain::generateAllocationForDispatchGroup(Module *module, string triggerIdentifier)
{
	string identifier = triggerIdentifier + "__" + nodes.at(0)->getIdentifier();
	if (isLastNodeInLoop)
		identifier += "__loop";
	dispatchGroupVariable = VuoCompilerCodeGenUtilities::generateAllocationForDispatchGroup(module, identifier);
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
 * @return The submitted function. The caller is responsible for filling in the body of this function.
 */
Function * VuoCompilerChain::generateSubmissionForDispatchGroup(Module *module, BasicBlock *block)
{
	PointerType *pointerToi8 = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<Type *> workerFunctionParams;
	workerFunctionParams.push_back(pointerToi8);
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

	ConstantPointerNull *nullPointerToi8 = ConstantPointerNull::get(pointerToi8);
	VuoCompilerCodeGenUtilities::generateSubmissionForDispatchGroup(module, block, dispatchGroupVariable, workerFunction, nullPointerToi8);

	return workerFunction;
}

/**
 * Generates code that waits on this chain's dispatch group.
 */
void VuoCompilerChain::generateWaitForDispatchGroup(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, dispatchGroupVariable);
}


#pragma mark Getters

/**
 * Returns the sequence of nodes in this chain.
 */
vector<VuoCompilerNode *> VuoCompilerChain::getNodes(void)
{
	return nodes;
}
