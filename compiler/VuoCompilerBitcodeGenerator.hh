/**
 * @file
 * VuoCompilerBitcodeGenerator interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERBITCODEGENERATOR_H
#define VUOCOMPILERBITCODEGENERATOR_H

#include "VuoCompilerConstantStringCache.hh"

class VuoCompiler;
class VuoCompilerChain;
class VuoCompilerComposition;
class VuoCompilerGraph;
class VuoCompilerNode;
class VuoCompilerPort;
class VuoCompilerTriggerDescription;
class VuoCompilerTriggerPort;
class VuoPort;
class VuoType;

/**
 * Generates code for a composition. Creates a .bc file from a .vuo composition file.
 */
class VuoCompilerBitcodeGenerator
{
private:
	VuoCompilerComposition *composition;
	VuoCompiler *compiler;
	VuoCompilerGraph *graph;

	/// True if extra code should be generated for this composition to interact with the Vuo runtime as a top-level composition,
	/// false if the extra code should not be generated since this composition will be used as a subcomposition.
	bool isTopLevelComposition;

	/// True if extra code should be generated so that this composition can used in live coding.
	bool isLiveCodeable;

	/// For subcompositions, the node class name. This is used as a unique prefix for function names.
	string moduleKey;

	/// A node instantiated from VuoCompilerPublishedInputNodeClass that stores data values of the published input ports.
	VuoNode *publishedInputNode;

	/// A node instantiated from VuoCompilerPublishedOutputNodeClass that synchronizes events through and
	/// stores data values of the published output ports.
	VuoNode *publishedOutputNode;

	/// The names of all published output ports that are trigger ports.
	set<string> publishedOutputTriggerNames;

	/// Ports mirroring the composition's published input ports. Used to create node function parameters.
	vector<VuoPort *> modelInputPorts;

	/// Ports mirroring the composition's published output ports. Used to create node function parameters.
	vector<VuoPort *> modelOutputPorts;

	/// A topologically sorted list of the linear chains of nodes reachable from each trigger port.
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger;

	/// A standard order in which nodes must be waited on to avoid deadlock.
	vector<VuoCompilerNode *> orderedNodes;

	/// A standard order for types, used to refer to types by numerical index.
	vector<VuoCompilerType *> orderedTypes;

	/// The LLVM module in which code is generated.
	Module *module;

	/// The LLVM constants that have been generated so far in the current module.
	VuoCompilerConstantStringCache constantStrings;

	/// The functions called by chain workers to execute each node.
	map<VuoCompilerNode *, Function *> executionFunctionForNode;

	/// The functions called by chain workers to transmit data and events from the outputs of each node.
	map<VuoCompilerNode *, Function *> transmissionFunctionForNode;

	/// For top-level compositions only: the trigger scheduler function for each trigger on a node in the top-level composition.
	map<VuoCompilerTriggerPort *, Function *> topLevelTriggerFunctions;

	/// For top-level compositions only: the trigger scheduler function for each trigger on a node within a subcomposition.
	map<string, map<VuoCompilerTriggerDescription *, Function *> > subcompositionTriggerFunctions;

	bool debugMode;

	/// The key of the top-level composition when looking up node contexts.
	static const string topLevelCompositionIdentifier;

	VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition, bool isTopLevelComposition, bool isLiveCodeable, string moduleKey, VuoCompiler *compiler);
	void makeOrderedNodes(void);
	void sortNodes(vector<VuoCompilerNode *> &nodes);
	static bool areNodeListsSortedBySize(const vector<VuoCompilerNode *> &nodes1, const vector<VuoCompilerNode *> &nodes2);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger, VuoCompilerNode *node);
	void makeOrderedTypes(void);
	void makePublishedOutputTriggers(void);
	void makePortContextInfo(void);
	void makeSubcompositionModelPorts(void);
	VuoCompilerTriggerPort * getPublishedInputTrigger(void);
	void generateCompositionMetadata(void);
	void generateCompositionContextInitFunction(bool isStatefulComposition);
	void generateCompositionContextFiniFunction(void);
	void generateSetInputDataFromNodeFunctionArguments(Function *function, BasicBlock *block, Value *compositionIdentifierValue, map<VuoPort *, size_t> indexOfParameter, bool shouldUpdateTriggers);
	void generateNodeEventFunction(bool isStatefulComposition);
	void generateNodeInstanceInitFunction(void);
	void generateNodeInstanceFiniFunction(void);
	void generateNodeInstanceTriggerStartFunction(void);
	void generateNodeInstanceTriggerStopFunction(void);
	void generateNodeInstanceTriggerUpdateFunction(void);
	Value * generateGetNextEventID(Module *module, BasicBlock *block);
	Value * generateWaitForNodes(Module *module, Function *function, BasicBlock *&block, Value *compositionIdentifierValue, vector<VuoCompilerNode *> nodes, Value *eventIdValue = NULL, bool shouldBlock = true);
	void generateCompositionWaitForNodeFunction(void);
	void generateSignalForNodes(Module *module, BasicBlock *block, Value *compositionIdentifierValue, vector<VuoCompilerNode *> nodes);
	void generateCompositionGetPortValueFunction(void);
	void generateCompositionSetPortValueFunction(void);
	void generateGetPortValueFunction(void);
	void generateSetInputPortValueFunction(void);
	void generateInitialEventlessTransmissions(Function *function, BasicBlock *&block, Value *compositionIdentifierValue);
	void generateFireTriggerPortEventFunction(void);
	void generateGetPublishedPortCountFunction(bool input);
	void generateGetPublishedPortNamesFunction(bool input);
	void generateGetPublishedPortTypesFunction(bool input);
	void generateGetPublishedPortDetailsFunction(bool input);
	void generateFunctionReturningStringArray(string functionName, vector<string> stringValues);
	void generateFirePublishedInputPortEventFunction(void);
	void generateGetPublishedPortValueFunction(bool input);
	void generateSetPublishedInputPortValueFunction(void);
	void generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock, Value *compositionIdentifierValue, VuoCompilerPort *outputPort, Value *eventValue, Value *dataValue, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateTransmissionFromNode(Function *function, BasicBlock *&currentBlock, Value *compositionIdentifierValue, Value *nodeContextValue, VuoCompilerNode *node, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateTelemetryFromPublishedOutputNode(Function *function, BasicBlock *&currentBlock, Value *nodeContextValue, VuoCompilerNode *node);
	void generateEventlessTransmission(Function *function, BasicBlock *&currentBlock, Value *compositionIdentifierValue, VuoCompilerNode *firstNode, bool isCompositionStarted);
	void generateNodeExecution(Function *function, BasicBlock *&currentBlock, Value *compositionIdentifierValue, VuoCompilerNode *node, bool shouldSendTelemetry = true);
	void generateSendInputPortUpdated(BasicBlock *block, VuoCompilerPort *inputPort, Value *receivedDataValue, Value *dataSummaryValue);
	void generateSendOutputPortUpdated(BasicBlock *block, VuoCompilerPort *outputPort, Value *sentDataValue, Value *outputDataSummaryValue);
	void generateSendPublishedOutputPortUpdated(BasicBlock *block, VuoCompilerPort *outputPort, Value *sentDataValue, Value *outputDataSummaryValue);
	void generateSendEventDropped(BasicBlock *block, string portIdentifier);
	void generateCompositionSerializeFunction(void);
	void generateCompositionUnserializeFunction(void);
	void generateAllocation(void);
	void generateSetupFunction(void);
	void generateCleanupFunction(void);
	void generateInstanceInitFunction(bool isStatefulComposition);
	void generateInstanceFiniFunction(bool isStatefulComposition);
	void generateInstanceTriggerStartFunction(bool isStatefulComposition);
	void generateInstanceTriggerStopFunction(bool isStatefulComposition);
	void generateSerializeFunction(void);
	void generateUnserializeFunction(void);
	void generateTriggerFunctions(void);
	Function * generateTriggerSchedulerFunction(VuoType *dataType, string compositionIdentifier, string nodeIdentifier,
												string portIdentifier, int portContextIndex, bool canDropEvents, Function *workerFunction);
	Function * generateTriggerWorkerFunction(VuoCompilerTriggerPort *trigger);
	void generateAndScheduleChainWorkerFunctions(BasicBlock *schedulerBlock, Value *contextValueInScheduler, const vector<VuoCompilerChain *> &chainsToSchedule, VuoCompilerTriggerPort *trigger, const vector<VuoCompilerChain *> &allChains,
												const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
												set<VuoCompilerChain *> &chainsScheduled, set<VuoCompilerNode *> &nodesScheduled);
	void generateAndScheduleChainWorkerFunction(BasicBlock *schedulerBlock, Value *contextValueInScheduler, VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger, const vector<VuoCompilerChain *> &allChains,
												const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
												set<VuoCompilerChain *> &chainsScheduled, set<VuoCompilerNode *> &nodesScheduled);
	Function * generateNodeExecutionFunction(Module *module, VuoCompilerNode *node);
	Function * generateNodeTransmissionFunction(Module *module, VuoCompilerNode *node);

	friend class TestVuoCompilerBitcodeGenerator;
	friend class TestVuoCompilerGraphExecution;
	friend class TestControlAndTelemetry;
	friend class TestNodeExecutionOrderRunnerDelegate;

public:
	static VuoCompilerBitcodeGenerator * newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition,
																			bool isTopLevelComposition, bool isLiveCodeable,
																			string moduleKey, VuoCompiler *compiler);
	~VuoCompilerBitcodeGenerator(void);
	Module * generateBitcode(void);
	void setDebugMode(bool debugMode);
};

#endif
