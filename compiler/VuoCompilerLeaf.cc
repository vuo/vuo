/**
 * @file
 * VuoCompilerLeaf implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerLeaf.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "dispatch/dispatch.h"


/**
 * Creates synchronization data for the specified leaf node.
 */
VuoCompilerLeaf::VuoCompilerLeaf(VuoCompilerNode *node, set<VuoCompilerTriggerPort *> loopTriggers)
{
	this->node = node;
	this->loopTriggers = loopTriggers;
	dispatchSemaphoreVariable = NULL;
}

/**
 * Generates code to allocate the leaf node's semaphore.
 */
void VuoCompilerLeaf::generateAllocationForSemaphore(Module *module)
{
	string identifier = node->getIdentifier() + "__leafSemaphore";
	dispatchSemaphoreVariable = VuoCompilerCodeGenUtilities::generateAllocationForSemaphore(module, identifier);
}

/**
 * Generates code to initialize the leaf node's semaphore.
 */
void VuoCompilerLeaf::generateInitializationForSemaphore(Module *module, BasicBlock *block)
{
	int initialValue = 1;
	VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(module, block, dispatchSemaphoreVariable, initialValue);
}

/**
 * Generates code to finalize the leaf node's semaphore.
 */
void VuoCompilerLeaf::generateFinalizationForSemaphore(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, dispatchSemaphoreVariable);
}

/**
 * Generates code to claim the leaf node's semaphore.
 */
void VuoCompilerLeaf::generateWaitForSemaphore(Module *module, BasicBlock *block)
{
	dispatch_time_t timeout = DISPATCH_TIME_FOREVER;
	VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, block, dispatchSemaphoreVariable, timeout);
}

/**
 * Generates code to relinquish the leaf node's semaphore.
 */
void VuoCompilerLeaf::generateSignalForSemaphore(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, dispatchSemaphoreVariable);
}

/**
 * Returns the leaf node.
 */
VuoCompilerNode * VuoCompilerLeaf::getNode(void)
{
	return node;
}

/**
 * Returns true if this node is the last node scheduled to execute in a feedback loop.
 */
bool VuoCompilerLeaf::isLastNodeInLoop(VuoCompilerTriggerPort *trigger)
{
	return loopTriggers.find(trigger) != loopTriggers.end();
}
