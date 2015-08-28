/**
 * @file
 * VuoCompilerChain interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCHAIN_H
#define VUOCOMPILERCHAIN_H

#include "VuoCompilerNode.hh"
#include "VuoCompilerTriggerPort.hh"


/**
 * A linear sequence of nodes along which an event may be transmitted.
 */
class VuoCompilerChain
{
private:
	vector<VuoCompilerNode *> nodes;
	bool isLastNodeInLoop;
	AllocaInst *dispatchGroupVariable;
	int numUpstreamChains;

public:
	VuoCompilerChain(vector<VuoCompilerNode *> nodes, bool isLastNodeInLoop);
	void generateAllocationForDispatchGroup(Module *module, BasicBlock *block, string triggerIdentifier);
	void generateInitializationForDispatchGroup(Module *module, BasicBlock *block);
	void generateFinalizationForDispatchGroup(Module *module, BasicBlock *block);
	Function * generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, Value *eventIdValue, vector<VuoCompilerChain *> upstreamChains);
	void generateWaitForDispatchGroup(Module *module, BasicBlock *block);
	static Value * generateEventIdValue(Module *module, Function *workerFunction, BasicBlock *block);
	void generateWaitForUpstreamChains(Module *module, Function *workerFunction, BasicBlock *block);
	static void generateFreeContextArgument(Module *module, Function *workerFunction, BasicBlock *block);
	vector<VuoCompilerNode *> getNodes(void);
};


#endif
