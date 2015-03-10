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

#include "VuoCompilerChain.hh"
#include "VuoCompilerLeaf.hh"
#include "VuoCompilerTriggerAction.hh"
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
	map<VuoCompilerNode *, set<VuoCompilerTriggerEdge *> > triggerInEdgesForNode;  ///< The trigger in-edges of each node.
	map<VuoCompilerNode *, set<VuoCompilerPassiveEdge *> > passiveInEdgesForNode;  ///< The passive in-edges of each node.
	map<VuoCompilerNode *, set<VuoCompilerPassiveEdge *> > passiveOutEdgesForNode;  ///< The passive out-edges of each node.
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger;  ///< The node containing each trigger port.
	map<VuoCompilerTriggerPort *, set<VuoCompilerTriggerEdge *> > triggerEdgesForTrigger;  ///< The out-edges of each trigger port.
	map<VuoCompilerTriggerPort *, set<VuoCompilerPassiveEdge *> > passiveEdgesForTrigger;  ///< The edges that are reachable from the trigger port but not out-edges of it.
	map<VuoCompilerEdge *, set<VuoCompilerPassiveEdge *> > downstreamEdgesForEdge;  ///< The passive edges that are reachable from each edge.
	vector<VuoCompilerNode *> orderedNodes;  ///< A topologically sorted list of all nodes in the composition. A node at the hub of a feedback loop appears only once in this list, for its first execution.
	set<VuoCompilerNode *> loopEndNodes;  ///< The set of nodes that are at the hub of a feedback loop, being executed once at the start and again at the end.
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chainsForTrigger;  ///< A topologically sorted list of all linear chains of nodes that may be pushed by the trigger.
	map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *> triggerActionForTrigger;  // TODO Refactor the above maps as member variables of VuoCompilerTriggerAction
	Module *module;  ///< The Module in which code is generated.
	map<VuoCompilerNode *, GlobalVariable *> semaphoreVariableForNode;  ///< A semaphore to wait on while a node's event function is executing.
	map<VuoCompilerNode *, GlobalVariable *> claimingEventIdVariableForNode;  ///< The ID of the event that has current exclusive claim on the node.
	ConstantInt *noEventIdConstant;  ///< A dummy ID to represent that no event is claiming a node.
	GlobalVariable *lastEventIdVariable;  ///< The ID most recently assigned to any event, composition-wide. Used to generate a unique ID for each event.
	GlobalVariable *lastEventIdSemaphoreVariable;  ///< A semaphore to synchronize access to @ref lastEventIdVariable.

	bool debugMode;

	VuoCompilerBitcodeGenerator(void);
	void initialize(void);
	void makeEdgesForNode(void);
	void makeNodeForTrigger(void);
	void makeEdgesForTrigger(VuoCompilerTriggerPort *trigger);
	void makeDownstreamEdges(void);
	void makeOrderedNodes(void);
	void makeChainsForTrigger(VuoCompilerTriggerPort *trigger);
	void makeLeaves(void);
	void makeTriggerObjects(void);
	set<VuoCompilerPassiveEdge *> outEdgesThatMayTransmitFromInEdge(VuoCompilerEdge *inEdge);
	void checkForDeadlockedFeedbackLoops(void);
	bool isNodeDownstreamOfTrigger(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool isNodeDownstreamOfNode(VuoCompilerNode *node, VuoCompilerNode *upstreamNode);
	bool edgeExists(VuoCompilerNode *fromNode, VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger);
	bool isDeadEnd(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool isScatter(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool isGather(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	set<VuoCompilerTriggerEdge *> getTriggerEdges(void);
	vector<VuoCompilerNode *> sortNodes(set<VuoCompilerNode *> originalNodes);
	void generateMetadata(void);
	void generateAllocation(void);
	void generateSetupFunction(void);
	void generateCleanupFunction(void);
	void generateInitFunction(void);
	void generateFiniFunction(void);
	void generateCallbackStartFunction(void);
	void generateCallbackStopFunction(void);
	Value * generateGetNextEventID(Module *module, BasicBlock *block);
	void generateWaitForNodes(Module *module, Function *function, BasicBlock *&block, vector<VuoCompilerNode *> nodes, Value *eventIdValue = NULL);
	void generateSignalForNodes(Module *module, BasicBlock *block, vector<VuoCompilerNode *> nodes);
	Value * generatePortSerialization(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSerializationInterprocess(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSummary(Module *module, BasicBlock *block, VuoCompilerPort *port);
	Value * generatePortSerializationOrSummary(Module *module, BasicBlock *block, VuoCompilerPort *port, bool isSummary, bool isInterprocess);
	void generateGetPortValueOrSummaryFunctions(void);
	void generateGetPortValueOrSummaryFunction(bool isSummary, bool isInput, bool isThreadSafe);
	void generateSetInputPortValueFunction(void);
	void generateInitializationForPorts(BasicBlock *block, bool input);
	void generateFireTriggerPortEventFunction(void);
	void generatePublishedPortGetters(void);
	void generateGetPublishedPortCountFunction(bool input);
	void generateGetPublishedPortNamesFunction(bool input);
	void generateGetPublishedPortTypesFunction(bool input);
	void generateFunctionReturningStringArray(string functionName, vector<string> stringValues);
	void generateGetPublishedPortConnectedIdentifierCount(bool input);
	void generateGetPublishedPortConnectedIdentifiers(bool input);
	void generateFirePublishedInputPortEventFunction(void);
	void generateSendPortsUpdatedCall(BasicBlock *initialBlock, BasicBlock *finalBlock, VuoCompilerNode *node);
	void generateSendTriggerPortValueChangedCall(BasicBlock *block, VuoCompilerTriggerPort *trigger, Value *triggerDataValue);
	void generateSerializeFunction(void);
	void generateUnserializeFunction(void);
	Function * generateTriggerFunctionHeader(VuoCompilerTriggerPort *trigger);  // TODO: Move to VuoCompilerTriggerAction?
	void generateTriggerFunctionBody(VuoCompilerTriggerPort *trigger);  // TODO: Move to VuoCompilerTriggerAction?

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
