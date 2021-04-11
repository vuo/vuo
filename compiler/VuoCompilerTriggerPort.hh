/**
 * @file
 * VuoCompilerTriggerPort interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerPort.hh"

class VuoCompilerTriggerPortClass;

/**
 * A trigger output port.
 *
 * \see{VuoCompilerTriggerPortClass}
 */
class VuoCompilerTriggerPort : public VuoCompilerPort
{
public:
	VuoCompilerTriggerPort(VuoPort * basePort);
	Value * generateCreatePortContext(Module *module, BasicBlock *block);
	static void generateScheduleWorker(Module *module, Function *function, BasicBlock *block, Value *compositionStateValue, Value *eventIdValue, Value *portContextValue, VuoType *dataType, int minThreadsNeeded, int maxThreadsNeeded, int chainCount, Function *workerFunction);
	Function * generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *nodeContextValue, string workerFunctionName, Value *workerFunctionArg=NULL);
	Function * getWorkerFunction(Module *module, string functionName, bool isExternal=false);
	static Value * generateNonBlockingWaitForSemaphore(Module *module, BasicBlock *block, Value *portContextValue);
	void generateSignalForSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue);
	Value * generateLoadFunction(Module *module, BasicBlock *block, Value *nodeContextValue);
	void generateStoreFunction(Module *module, BasicBlock *block, Value *nodeContextValue, Value *functionValue);
	Value * generateRetrievePreviousData(Module *module, BasicBlock *block, Value *nodeContextValue);
	void generateFreeContext(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateCompositionStateValue(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateDataValue(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateEventIdValue(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateDataValueUpdate(Module *module, BasicBlock *block, Function *workerFunction, Value *nodeContextValue);
	static void generateDataValueDiscardFromScheduler(Module *module, Function *function, BasicBlock *block, VuoType *dataType);
	void generateDataValueDiscardFromWorker(Module *module, BasicBlock *block, Function *workerFunction);
	VuoCompilerTriggerPortClass * getClass(void);
};
