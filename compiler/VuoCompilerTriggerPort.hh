/**
 * @file
 * VuoCompilerTriggerPort interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGERPORT_H
#define VUOCOMPILERTRIGGERPORT_H

#include "VuoCompilerPort.hh"

class VuoCompilerTriggerPortClass;
class VuoCompilerType;

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
	static void generateAsynchronousSubmissionToDispatchQueue(Module *module, Function *function, BasicBlock *block, Value *compositionIdentifierValue, Value *portContextValue, VuoType *dataType, Function *workerFunction);
	Function * generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, Value *nodeContextValue, string workerFunctionName, Value *workerFunctionArg=NULL);
	Function * getWorkerFunction(Module *module, string functionName, bool isExternal=false);
	static Value * generateNonBlockingWaitForSemaphore(Module *module, BasicBlock *block, Value *portContextValue);
	void generateSignalForSemaphore(Module *module, BasicBlock *block, Value *nodeContextValue);
	Value * generateLoadFunction(Module *module, BasicBlock *block, Value *nodeContextValue);
	void generateStoreFunction(Module *module, BasicBlock *block, Value *nodeContextValue, Value *functionValue);
	Value * generateLoadPreviousData(Module *module, BasicBlock *block, Value *nodeContextValue);
	void generateFreeContext(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateCompositionIdentifierValue(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateDataValue(Module *module, BasicBlock *block, Function *workerFunction);
	Value * generateDataValueUpdate(Module *module, BasicBlock *block, Function *workerFunction, Value *nodeContextValue);
	static void generateDataValueDiscardFromScheduler(Module *module, Function *function, BasicBlock *block, VuoType *dataType);
	void generateDataValueDiscardFromWorker(Module *module, BasicBlock *block, Function *workerFunction);
	Type * getDataType(void);
	VuoCompilerTriggerPortClass * getClass(void);
};

#endif
