/**
 * @file
 * VuoCompilerBitcodeGenerator interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerConstantStringCache.hh"

class VuoCompiler;
class VuoCompilerChain;
class VuoCompilerComposition;
class VuoCompilerGraph;
class VuoCompilerNode;
class VuoCompilerPort;
class VuoCompilerTriggerDescription;
class VuoCompilerTriggerPort;
class VuoCompilerType;
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

	/// For subcompositions, the node class name. This is used as a unique prefix for function names.
	string moduleKey;

	/// Ports mirroring the composition's published input ports. Used to create node function parameters.
	vector<VuoPort *> modelInputPorts;

	/// Ports mirroring the composition's published output ports. Used to create node function parameters.
	vector<VuoPort *> modelOutputPorts;

	/// A topologically sorted list of the linear chains of nodes reachable from each trigger port.
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger;

	/// A topologically sorted list of the nodes reachable from each trigger port, plus the node containing the trigger port.
	map<VuoCompilerTriggerPort *, vector<VuoCompilerNode *> > downstreamNodesForTrigger;

	/// A standard order in which nodes must be waited on to avoid deadlock.
	vector<VuoCompilerNode *> orderedNodes;

	/// A standard order for types, used to refer to types by numerical index.
	vector<VuoCompilerType *> orderedTypes;

	/// The direct dependencies of the composition.
	set<string> dependencies;

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
	map<string, map<size_t, map<VuoCompilerTriggerDescription *, Function *> > > subcompositionTriggerFunctions;

	bool debugMode;

	VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition, bool isTopLevelComposition, string moduleKey, VuoCompiler *compiler);
	void makeOrderedNodes(void);
	void sortNodes(vector<VuoCompilerNode *> &nodes);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger, VuoCompilerNode *node);
	void makeOrderedTypes(void);
	void makePortContextInfo(void);
	void makeSubcompositionModelPorts(void);
	void makeDependencies(void);
	void generateShouldShowSplashWindowFunction(bool shouldShow);
	void generateCompositionMetadata(void);
	void generateCompositionAddNodeMetadataFunction(void);
	void generateCompositionCreateContextForNodeFunction(void);
	void generateCompositionPerformDataOnlyTransmissionsFunction(void);
	void generateCompositionReleasePortDataFunction(void);
	void generateSetInputDataFromNodeFunctionArguments(Function *function, BasicBlock *&block, Value *compositionStateValue, map<VuoPort *, size_t> indexOfParameter, map<VuoPort *, size_t> indexOfEventParameter, bool shouldWaitForDataOnlyDownstreamNodes, bool shouldUpdateTriggers, bool shouldSendTelemetry);
	void generateNodeEventFunction(bool isStatefulComposition);
	void generateNodeInstanceInitFunction(void);
	void generateNodeInstanceFiniFunction(void);
	void generateNodeInstanceTriggerStartFunction(void);
	void generateNodeInstanceTriggerStopFunction(void);
	void generateNodeInstanceTriggerUpdateFunction(void);
	Value * generateWaitForNodes(Module *module, Function *function, BasicBlock *&block, Value *compositionStateValue, vector<VuoCompilerNode *> nodes, Value *eventIdValue = NULL, bool shouldBlock = true);
	void generateCompositionWaitForNodeFunction(void);
	void generateSignalForNodes(Module *module, BasicBlock *block, Value *compositionStateValue, vector<VuoCompilerNode *> nodes);
	void generateCompositionGetPortValueFunction(void);
	void generateCompositionSetPortValueFunction(void);
	void generateSetInputPortValueFunction(void);
	void generateCompositionFireTriggerPortEventFunction(void);
	void generateGetPublishedPortCountFunction(bool input);
	void generateGetPublishedPortNamesFunction(bool input);
	void generateGetPublishedPortTypesFunction(bool input);
	void generateGetPublishedPortDetailsFunction(bool input);
	void generateFunctionReturningStringArray(string functionName, vector<string> stringValues);
	void generateFirePublishedInputPortEventFunction(void);
	void generateGetPublishedPortValueFunction(bool input);
	void generateCompositionSetPublishedInputPortValueFunction(void);
	void generateSetPublishedInputPortValueFunction(void);
	void generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue, VuoCompilerPort *outputPort, Value *eventValue, Value *dataValue, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateTransmissionFromNode(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue, Value *nodeContextValue, VuoCompilerNode *node, bool requiresEvent = true, bool shouldSendTelemetry = true);
	void generateTelemetryFromPublishedOutputNode(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue, Value *nodeContextValue, VuoCompilerNode *node);
	void generateDataOnlyTransmissionFromNode(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue, VuoCompilerNode *node, bool shouldWaitForDownstreamNodes, bool shouldUpdateTriggers, bool shouldSendTelemetry);
	void generateNodeExecution(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue, VuoCompilerNode *node, bool shouldSendTelemetry = true);
	void generateAllocation(void);
	void generateSetupFunction(bool isStatefulComposition);
	void generateCleanupFunction(void);
	void generateInstanceInitFunction(bool isStatefulComposition);
	void generateInstanceFiniFunction(bool isStatefulComposition);
	void generateInstanceTriggerStartFunction(bool isStatefulComposition);
	void generateInstanceTriggerStopFunction(bool isStatefulComposition);
	void generateTriggerFunctions(void);
	Function * generateTriggerSchedulerFunction(VuoType *dataType, string compositionIdentifier, size_t nodeIndex,
												string portIdentifier, int portContextIndex, bool canDropEvents, bool isPublishedInputTrigger, bool isSpinOff, int minThreadsNeeded, int maxThreadsNeeded, int chainCount,
												Function *workerFunction);
	Function * generateTriggerWorkerFunction(VuoCompilerTriggerPort *trigger);
	void generateAndScheduleChainWorkerFunctions(BasicBlock *schedulerBlock, Value *compositionStateValueInScheduler, Value *contextValueInScheduler, const vector<VuoCompilerChain *> &chainsToSchedule, VuoCompilerTriggerPort *trigger, const vector<VuoCompilerChain *> &allChains,
												const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
												set<VuoCompilerChain *> &chainsScheduled);
	void generateAndScheduleChainWorkerFunction(BasicBlock *schedulerBlock, Value *compositionStateValueInScheduler, Value *contextValueInScheduler, VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger, const vector<VuoCompilerChain *> &allChains,
												const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
												set<VuoCompilerChain *> &chainsScheduled);
	void generateChainExecution(Function *function, BasicBlock *&block, Value *compositionStateValue, Value *contextValue, Value *eventIdValue, VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger, const vector<VuoCompilerChain *> &allChains,
								const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream, const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
								set<VuoCompilerChain *> &chainsScheduled);
	Function * generateNodeExecutionFunction(Module *module, VuoCompilerNode *node);
	Function * generateNodeTransmissionFunction(Module *module, VuoCompilerNode *node);

	friend class TestVuoCompilerBitcodeGenerator;
	friend class TestVuoCompilerGraphExecution;
	friend class TestControlAndTelemetry;
	friend class TestNodeExecutionOrderRunnerDelegate;

public:
	static VuoCompilerBitcodeGenerator * newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition,
																			bool isTopLevelComposition,
																			string moduleKey, VuoCompiler *compiler);
	~VuoCompilerBitcodeGenerator(void);
	Module * generateBitcode(void);
	void setDebugMode(bool debugMode);

#ifdef VUO_PRO
#include "pro/VuoCompilerBitcodeGenerator_Pro.hh"
#endif
};
