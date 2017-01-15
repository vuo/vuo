/**
 * @file
 * VuoCompilerChain interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERCHAIN_H
#define VUOCOMPILERCHAIN_H

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
	static Value * generateMakeContext(Module *module, BasicBlock *block, Value *compositionIdentifierValue, Value *eventIdValue, const vector<VuoCompilerChain *> &chains, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream);
	static Value * generateCompositionIdentifierValue(Module *module, BasicBlock *block, Value *contextValue);
	static Value * generateEventIdValue(Module *module, BasicBlock *block, Value *contextValue);
	static Function * getFreeContextFunction(Module *module);
	Function * generateSubmissionForDispatchGroup(Module *module, BasicBlock *block, Value *contextValue, string triggerIdentifier);
	void generateWaitForUpstreamChains(Module *module, BasicBlock *block, Value *contextValue, const vector<size_t> &chainIndices);
	void generateCleanupForWorkerFunction(Module *module, BasicBlock *block, Value *contextValue, size_t chainIndex, bool hasDownstreamChains);
	vector<VuoCompilerNode *> getNodes(void);
	bool isLastNodeInLoop(void);
};


#endif
