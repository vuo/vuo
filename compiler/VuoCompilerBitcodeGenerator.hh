/**
 * @file
 * VuoCompilerBitcodeGenerator interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERBITCODEGENERATOR_H
#define VUOCOMPILERBITCODEGENERATOR_H

#include "VuoCompilerGraph.hh"
#include "VuoCompilerComposition.hh"

class VuoCompiler;

/**
 * Generates code for a composition. Creates a .bc file from a .vuo composition file.
 */
class VuoCompilerBitcodeGenerator
{
private:
	VuoCompilerComposition *composition;
	VuoCompiler *compiler;
	VuoCompilerGraph *graph;

	/// A topologically sorted list of the linear chains of nodes reachable from each trigger port.
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger;

	/// A standard order in which nodes must be waited on to avoid deadlock.
	vector<VuoCompilerNode *> orderedNodes;

	/// The LLVM module in which code is generated.
	Module *module;

	/// A semaphore to wait on while a node's event function is executing.
	map<VuoCompilerNode *, GlobalVariable *> semaphoreVariableForNode;

	/// The ID of the event that currently has exclusive claim on the node.
	map<VuoCompilerNode *, GlobalVariable *> claimingEventIdVariableForNode;

	/// A dummy ID to represent that no event is claiming a node.
	ConstantInt *noEventIdConstant;

	/// The ID most recently assigned to any event, composition-wide. Used to generate a unique ID for each event.
	GlobalVariable *lastEventIdVariable;

	/// A semaphore to synchronize access to @ref lastEventIdVariable.
	GlobalVariable *lastEventIdSemaphoreVariable;

	bool debugMode;

	VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition, VuoCompiler *compiler);
	void makeOrderedNodes(void);
	void sortNodes(vector<VuoCompilerNode *> &nodes);
	static bool areNodeListsSortedBySize(const vector<VuoCompilerNode *> &nodes1, const vector<VuoCompilerNode *> &nodes2);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger, VuoCompilerNode *node);
	void generateMetadata(void);
	void generateAllocation(void);
	void generateSetupFunction(void);
	void generateCleanupFunction(void);
	void generateInitFunction(void);
	void generateFiniFunction(void);
	void generateCallbackStartFunction(void);
	void generateCallbackStopFunction(void);
	Value * generateGetNextEventID(Module *module, BasicBlock *block);
	Value * generateWaitForNodes(Module *module, Function *function, BasicBlock *&block, vector<VuoCompilerNode *> nodes, Value *eventIdValue = NULL, bool shouldBlock = true);
	void generateSignalForNodes(Module *module, BasicBlock *block, vector<VuoCompilerNode *> nodes);
	Value * generatePortSerialization(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSerializationInterprocess(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSummary(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSerializationOrSummary(Module *module, BasicBlock *block, VuoCompilerPort *port, bool isSummary, bool isInterprocess);
	void generateGetPortValueOrSummaryFunctions(void);
	void generateGetPortValueOrSummaryFunction(bool isSummary, bool isInput, bool isThreadSafe);
	void generateSetInputPortValueFunction(void);
	void generateInitializationForPorts(BasicBlock *block, bool input);
	void generateInitialEventlessTransmissions(Function *function, BasicBlock *&block);
	void generateFireTriggerPortEventFunction(void);
	void generatePublishedPortGetters(void);
	void generateGetPublishedPortCountFunction(bool input);
	void generateGetPublishedPortNamesFunction(bool input);
	void generateGetPublishedPortTypesFunction(bool input);
	void generateGetPublishedPortDetailsFunction(bool input);
	void generateFunctionReturningStringArray(string functionName, vector<string> stringValues);
	void generateGetPublishedPortConnectedIdentifierCount(bool input);
	void generateGetPublishedPortConnectedIdentifiers(bool input);
	void generateFirePublishedInputPortEventFunction(void);
	void generateGetPublishedPortValueFunction(bool isInput);
	void generateSetPublishedInputPortValueFunction(void);
	void generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock, VuoCompilerPort *outputPort, Value *dataValue, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateTransmissionFromNode(Function *function, BasicBlock *&currentBlock, VuoCompilerNode *node, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateEventlessTransmission(Function *function, BasicBlock *&currentBlock, VuoCompilerNode *firstNode, bool isCompositionStarted);
	void generateNodeExecution(Function *function, BasicBlock *&currentBlock, VuoCompilerNode *node, bool shouldSendTelemetry = true);
	void generateSendOutputPortUpdated(BasicBlock *block, VuoCompilerPort *outputPort, Value *sentDataValue, Value *outputDataSummaryValue);
	void generateSendInputPortUpdated(BasicBlock *block, VuoCompilerPort *inputPort, Value *sentDataValue, Value *outputDataSummaryValue);
	void generateSerializeFunction(void);
	void generateUnserializeFunction(void);
	Function * generateTriggerFunctionHeader(VuoCompilerTriggerPort *trigger);
	void generateTriggerFunctionBody(VuoCompilerTriggerPort *trigger);

	friend class TestVuoCompilerBitcodeGenerator;
	friend class TestVuoCompilerGraphExecution;
	friend class TestControlAndTelemetry;
	friend class TestNodeExecutionOrderRunnerDelegate;

public:
	static VuoCompilerBitcodeGenerator * newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition, VuoCompiler *compiler);
	Module * generateBitcode(void);
	void setDebugMode(bool debugMode);
};

#endif
