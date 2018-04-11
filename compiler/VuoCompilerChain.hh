/**
 * @file
 * VuoCompilerChain interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

class VuoCompilerNode;

/**
 * A linear sequence of nodes along which an event may be transmitted.
 */
class VuoCompilerChain
{
private:
	vector<VuoCompilerNode *> nodes;
	bool lastNodeInLoop;

	static Value * generateChainGroupsValue(Module *module, BasicBlock *block, Value *contextValue);

public:
	VuoCompilerChain(vector<VuoCompilerNode *> nodes, bool lastNodeInLoop);
	static Value * generateMakeContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *eventIdValue, const vector<VuoCompilerChain *> &chains, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream);
	static Value * generateCompositionStateValue(Module *module, BasicBlock *block, Value *contextValue);
	static Value * generateEventIdValue(Module *module, BasicBlock *block, Value *contextValue);
	static Function * getFreeContextFunction(Module *module);
	Function * generateScheduleWorker(Module *module, BasicBlock *block, Value *compositionStateValue, Value *contextValue, string triggerIdentifier, int minThreadsNeeded, int maxThreadsNeeded, size_t chainIndex, vector<size_t> upstreamChainIndices);
	void generateWaitForUpstreamChains(Module *module, BasicBlock *block, Value *contextValue, const vector<size_t> &chainIndices);
	void generateCleanupForWorkerFunction(Module *module, BasicBlock *block, Value *contextValue, size_t chainIndex, bool hasDownstreamChains);
	vector<VuoCompilerNode *> getNodes(void);
	bool isLastNodeInLoop(void);
};
