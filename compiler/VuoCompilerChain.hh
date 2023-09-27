/**
 * @file
 * VuoCompilerChain interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
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

public:
	VuoCompilerChain(vector<VuoCompilerNode *> nodes, bool lastNodeInLoop);
	static Value * generateMakeContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *eventIdValue);
	static Value * generateCompositionStateValue(Module *module, BasicBlock *block, Value *contextValue);
	static Value * generateEventIdValue(Module *module, BasicBlock *block, Value *contextValue);
	static Function * getFreeContextFunction(Module *module);
	Function * generateScheduleWorker(Module *module, BasicBlock *block, Value *compositionStateValue, Value *contextValue, string triggerIdentifier, int minThreadsNeeded, int maxThreadsNeeded, size_t chainIndex, vector<size_t> upstreamChainIndices);
	vector<VuoCompilerNode *> getNodes(void);
	bool isLastNodeInLoop(void);
};
