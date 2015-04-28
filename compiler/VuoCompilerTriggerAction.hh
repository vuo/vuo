/**
 * @file
 * VuoCompilerTriggerAction interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGERACTION_H
#define VUOCOMPILERTRIGGERACTION_H

#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerNode.hh"

/**
 * The callback that executes when a trigger port on a node generates an event.
 */
class VuoCompilerTriggerAction
{
private:
	VuoCompilerTriggerPort *port;
	VuoCompilerNode *node;
	GlobalVariable *dispatchQueueVariable;
	GlobalVariable *previousDataVariable;

	Function * getWorkerFunction(Module *module, BasicBlock *block, string identifier);
	Type * getDataType(void);

public:
	VuoCompilerTriggerAction(VuoCompilerTriggerPort *port, VuoCompilerNode *node);
	void generateAllocation(Module *module);
	void generateInitialization(Module *module, BasicBlock *block);
	void generateFinalization(Module *module, BasicBlock *block);
	Function * generateAsynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier);
	Function * generateSynchronousSubmissionToDispatchQueue(Module *module, BasicBlock *block, string identifier);
	Value * generateDataValueUpdate(Module *module, BasicBlock *block, Function *triggerWorker);
	void generateDataValueDiscard(Module *module, BasicBlock *block, Function *triggerWorker);
	GlobalVariable * getPreviousDataVariable(void);
};

#endif
