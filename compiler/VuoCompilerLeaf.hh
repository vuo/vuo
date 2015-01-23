/**
 * @file
 * VuoCompilerLeaf interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERLEAF_H
#define VUOCOMPILERLEAF_H

#include "VuoCompilerNode.hh"
#include "VuoCompilerTriggerPort.hh"


/**
 * Provides a semaphore for waiting on a leaf node.
 *
 * A node is a leaf node if at least one of the following is true for at least one trigger:
 *  - The node is a dead end: it has no out-edges that may transmit an event.
 *  - The node is the last node (and only repeated node) in a feedback loop.
 */
class VuoCompilerLeaf
{
private:
	VuoCompilerNode *node;
	set<VuoCompilerTriggerPort *> loopTriggers;
	GlobalVariable *dispatchSemaphoreVariable;

public:
	VuoCompilerLeaf(VuoCompilerNode *node, set<VuoCompilerTriggerPort *> loopTriggers);
	void generateAllocationForSemaphore(Module *module);
	void generateInitializationForSemaphore(Module *module, BasicBlock *block);
	void generateFinalizationForSemaphore(Module *module, BasicBlock *block);
	void generateWaitForSemaphore(Module *module, BasicBlock *block);
	void generateSignalForSemaphore(Module *module, BasicBlock *block);
	VuoCompilerNode * getNode(void);
	bool isLastNodeInLoop(VuoCompilerTriggerPort *trigger);
};


#endif
