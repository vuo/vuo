/**
 * @file
 * VuoCompilerBitcodeGenerator implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stack>
#include "VuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedInputPort.hh"
#include "VuoFileUtilities.hh"
#include "VuoPort.hh"

/**
 * Creates a bitcode generator from the specified composition.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator(composition, compiler);
	return cg;
}

/**
 * Private constructor.
 */
VuoCompilerBitcodeGenerator::VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition, VuoCompiler *compiler)
{
	module = NULL;
	debugMode = false;

	this->composition = composition;
	this->compiler = compiler;

	graph = new VuoCompilerGraph(composition);
	chainsForTrigger = graph->getChains();  // store in a data member, rather than calling getChains() multiple times, to preserve order of chains
	makeOrderedNodes();
}

/**
 * Destructor.
 */
VuoCompilerBitcodeGenerator::~VuoCompilerBitcodeGenerator(void)
{
	delete graph;
}

/**
 * Helper function for VuoCompilerBitcodeGenerator::makeOrderedNodes().
 */
bool VuoCompilerBitcodeGenerator::areNodeListsSortedBySize(const vector<VuoCompilerNode *> &nodes1, const vector<VuoCompilerNode *> &nodes2)
{
	return nodes1.size() < nodes2.size();
}

/**
 * Sets up VuoCompilerBitcodeGenerator::orderedNodes.
 */
void VuoCompilerBitcodeGenerator::makeOrderedNodes(void)
{
	// For each trigger, put its downstream nodes into topological order.
	vector< vector<VuoCompilerNode *> > orderedNodesPerTrigger;
	for (map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> >::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
	{
		vector<VuoCompilerChain *> chain = i->second;

		vector<VuoCompilerNode *> nodesInProgress;
		for (vector<VuoCompilerChain *>::iterator j = chain.begin(); j != chain.end(); ++j)
		{
			vector<VuoCompilerNode *> nodes = (*j)->getNodes();
			nodesInProgress.insert(nodesInProgress.end(), nodes.begin(), nodes.end());
		}
		orderedNodesPerTrigger.push_back(nodesInProgress);
	}

	// Visit each trigger, in descending order of number of downstream nodes (so that orderedNodes will be more likely
	// to match the ordering of the triggers with more downstream nodes, and thus be more likely to wait on them one at
	// a time instead of less efficiently having to wait on all initially).
	sort(orderedNodesPerTrigger.begin(), orderedNodesPerTrigger.end(), areNodeListsSortedBySize);
	for (vector< vector<VuoCompilerNode *> >::reverse_iterator i = orderedNodesPerTrigger.rbegin(); i != orderedNodesPerTrigger.rend(); ++i)
	{
		// Merge the trigger's downstream nodes into orderedNodes.
		int previousNodeIndex = -1;
		for (vector<VuoCompilerNode *>::iterator j = (*i).begin(); j != (*i).end(); ++j)
		{
			VuoCompilerNode *node = *j;
			vector<VuoCompilerNode *>::iterator nodeIter = find(orderedNodes.begin(), orderedNodes.end(), node);
			if (nodeIter == orderedNodes.end())
				nodeIter = orderedNodes.insert(orderedNodes.begin() + previousNodeIndex + 1, node);
			previousNodeIndex = nodeIter - orderedNodes.begin();
		}
	}

	// For any node that contains a trigger port, add it at to orderedNodes (at the beginning) if it's not already there.
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = graph->getNodesForTriggerPorts();
	for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
	{
		VuoCompilerNode *node = i->second;
		if (find(orderedNodes.begin(), orderedNodes.end(), node) == orderedNodes.end())
			orderedNodes.insert(orderedNodes.begin(), node);
	}
}

/**
 * Puts the nodes into the same order as VuoCompilerBitcodeGenerator::orderedNodes.
 */
void VuoCompilerBitcodeGenerator::sortNodes(vector<VuoCompilerNode *> &nodes)
{
	vector<VuoCompilerNode *> sortedNodes;
	for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;
		if (find(nodes.begin(), nodes.end(), node) != nodes.end())
			sortedNodes.push_back(node);
	}
	nodes = sortedNodes;
}

/**
 * Returns the nodes that need to be waited on before transmitting an event out @a triggger.
 */
vector<VuoCompilerNode *> VuoCompilerBitcodeGenerator::getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger)
{
	// Does the trigger port have a gather somewhere downstream of it?
	// If so, then when the event reaches its first scatter, all downstream nodes will be locked before the event can proceed.
	// This is an (imprecise) way to prevent deadlock in the situation where one trigger port has scatter and a gather,
	// and another trigger port overlaps with some branches of the scatter but not others (https://b33p.net/kosada/node/6696).

	bool isGatherDownstreamOfTrigger = graph->hasGatherDownstream(trigger);
	bool isScatterAtTrigger = graph->getNodesImmediatelyDownstream(trigger).size() > 1;


	// Would the trigger port wait on nodes in a different order than orderedNodes?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents deadlock where the events from two different trigger ports reach the downstream nodes in a different order
	// (https://b33p.net/kosada/node/7924).

	vector<VuoCompilerNode *> downstreamNodes;
	vector<VuoCompilerChain *> chains = chainsForTrigger[trigger];
	for (vector<VuoCompilerChain *>::iterator i = chains.begin(); i != chains.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		vector<VuoCompilerNode *> chainNodes = chain->getNodes();
		downstreamNodes.insert(downstreamNodes.end(), chainNodes.begin(), chainNodes.end());
	}

	VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];
	bool shouldWaitOnTriggerNode = (triggerNode->getBase() != composition->getPublishedInputNode());
	if (shouldWaitOnTriggerNode)
	{
		vector<VuoCompilerNode *>::iterator triggerNodeIter = find(downstreamNodes.begin(), downstreamNodes.end(), triggerNode);
		if (triggerNodeIter != downstreamNodes.end())
			downstreamNodes.erase(triggerNodeIter);

		// Wait for the node containing the trigger port, to avoid firing corrupted data if the trigger fires
		// on its own at the same time that it's being manually fired (https://b33p.net/kosada/node/6497).
		// This node will be waited for first, before any downstream nodes (https://b33p.net/kosada/node/9466).
		downstreamNodes.insert(downstreamNodes.begin(), triggerNode);
	}

	vector<VuoCompilerNode *> sortedDownstreamNodes = downstreamNodes;
	sortNodes(sortedDownstreamNodes);
	bool hasOutOfOrderDownstreamNodes = (downstreamNodes != sortedDownstreamNodes);


	// Wait for either all nodes downstream of the trigger or the nodes directly connected to the trigger.
	vector<VuoCompilerNode *> nodesToWaitOn;
	if ((isGatherDownstreamOfTrigger && isScatterAtTrigger) || hasOutOfOrderDownstreamNodes)
		nodesToWaitOn = downstreamNodes;
	else
	{
		nodesToWaitOn = graph->getNodesImmediatelyDownstream(trigger);
		if (shouldWaitOnTriggerNode)
			nodesToWaitOn.push_back(triggerNode);
	}

	return nodesToWaitOn;
}

/**
 * Returns the nodes that need to be waited on before transmitting an event from @a trigger out of @a node.
 */
vector<VuoCompilerNode *> VuoCompilerBitcodeGenerator::getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger, VuoCompilerNode *node)
{
	// Does the trigger port have a gather somewhere downstream of it?
	// If so, then when the event reaches its first scatter, all downstream nodes will be locked before the event can proceed.
	// This is an (imprecise) way to prevent deadlock in the situation where one trigger port has scatter and a gather,
	// and another trigger port overlaps with some branches of the scatter but not others (https://b33p.net/kosada/node/6696).
	bool isGatherDownstreamOfTrigger = graph->hasGatherDownstream(trigger);
	bool isScatterAtNode = graph->getNodesImmediatelyDownstream(node, trigger).size() > 1;

	// Wait for either all nodes downstream of the node or the nodes directly connected to the node.
	vector<VuoCompilerNode *> nodesToWaitOn =
			(isGatherDownstreamOfTrigger && isScatterAtNode) ?
				graph->getNodesDownstream(node, trigger) :
				graph->getNodesImmediatelyDownstream(node, trigger);

	return nodesToWaitOn;
}


/**
 * Generates bitcode that can be read in as a node class.
 *
 * @return The LLVM module in which bitcode has been generated. It is owned by the VuoCompilerComposition with which
 *		this VuoCompilerBitcodeGenerator was constructed.
 */
Module * VuoCompilerBitcodeGenerator::generateBitcode(void)
{
	module = new Module("", getGlobalContext());  /// @todo Set module identifier (https://b33p.net/kosada/node/2639)

	generateMetadata();

	generateAllocation();

	generateGetPortValueOrSummaryFunctions();

	generatePublishedPortGetters();
	generateGetPublishedPortValueFunction(false);
	generateGetPublishedPortValueFunction(true);
	generateSetPublishedInputPortValueFunction();

	generateSerializeFunction();
	generateUnserializeFunction();

	generateSetupFunction();
	generateCleanupFunction();

	// First set the function header for each trigger, then generate the function body for each trigger.
	// This has to be split into 2 steps because the 2nd step assumes that the 1st step is complete for all triggers.
	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;
		Function *f = generateTriggerFunctionHeader(trigger);
		trigger->setFunction(f);
	}
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;
		generateTriggerFunctionBody(trigger);
	}

	generateSetInputPortValueFunction();
	generateFireTriggerPortEventFunction();
	generateFirePublishedInputPortEventFunction();

	/// @todo These should only be generated for stateful compositions - https://b33p.net/kosada/node/2639
	generateInitFunction();
	generateFiniFunction();
	generateCallbackStartFunction();
	generateCallbackStopFunction();

	composition->setModule(module);
	return module;
}

/**
 *  Generates metadata (name, description, ...) for this composition.
 */
void VuoCompilerBitcodeGenerator::generateMetadata(void)
{
	{
		// const char *moduleName = ...;
		string displayName = module->getModuleIdentifier();
		Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		Constant *moduleNameValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, displayName, ".str");  // VuoCompilerBitcodeParser::resolveGlobalToConst requires that the variable have a name
		GlobalVariable *moduleNameVariable = new GlobalVariable(*module, pointerToCharType, false, GlobalValue::ExternalLinkage, 0, "moduleName");
		moduleNameVariable->setInitializer(moduleNameValue);
	}

	{
		// const char *moduleDependencies[] = ...;
		set<string> dependenciesSeen;
		set<VuoNode *> nodes = composition->getBase()->getNodes();
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			string nodeClassName = (*i)->getNodeClass()->getClassName();
			dependenciesSeen.insert(nodeClassName);
		}
		vector<string> dependencies(dependenciesSeen.begin(), dependenciesSeen.end());
		VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfStrings(module, dependencies, "moduleDependencies");
	}

	/// @todo Generate rest of metadata (https://b33p.net/kosada/node/2639)
}

/**
 * Generate the allocation of all global variables.
 */
void VuoCompilerBitcodeGenerator::generateAllocation(void)
{
	noEventIdConstant = ConstantInt::get(module->getContext(), APInt(64, 0));
	lastEventIdVariable = new GlobalVariable(*module,
											 IntegerType::get(module->getContext(), 64),
											 false,
											 GlobalValue::InternalLinkage,
											 noEventIdConstant,
											 "lastEventId");
	lastEventIdSemaphoreVariable = VuoCompilerCodeGenUtilities::generateAllocationForSemaphore(module, "lastEventId__semaphore");

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	VuoNode *publishedInputNode = composition->getPublishedInputNode();
	if (publishedInputNode)
		nodes.insert(publishedInputNode);
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		node->generateAllocation(module);

		string semaphoreIdentifier = node->getIdentifier() + "__semaphore";
		GlobalVariable *semaphoreVariable = VuoCompilerCodeGenUtilities::generateAllocationForSemaphore(module, semaphoreIdentifier);
		semaphoreVariableForNode[node] = semaphoreVariable;

		string claimingEventIdIdentifier = node->getIdentifier() + "__claimingEventId";
		GlobalVariable *claimingEventIdVariable = new GlobalVariable(*module,
																	 IntegerType::get(module->getContext(), 64),
																	 false,
																	 GlobalValue::InternalLinkage,
																	 noEventIdConstant,
																	 claimingEventIdIdentifier);
		claimingEventIdVariableForNode[node] = claimingEventIdVariable;
	}
}

/**
 * Generate the setup function, which initializes all global variables except nodes' instance data.
 *
 * Assumes the global variables have been allocated.
 *
 * \eg{void setup(void);}
 */
void VuoCompilerBitcodeGenerator::generateSetupFunction(void)
{
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "setup", module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(module, block, lastEventIdSemaphoreVariable);

	for (map<VuoCompilerNode *, GlobalVariable *>::iterator i = semaphoreVariableForNode.begin(); i != semaphoreVariableForNode.end(); ++i)
		VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(module, block, i->second);

	generateInitializationForPorts(block, false);
	generateInitializationForPorts(block, true);

	generateInitialEventlessTransmissions(function, block);

	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
		(*i)->generateInitialization(module, block);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generate the cleanup function, which finalizes all global variables.
 *
 * \eg{void cleanup(void);}
 */
void VuoCompilerBitcodeGenerator::generateCleanupFunction(void)
{
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "cleanup", module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
		(*i)->generateFinalization(module, block);

	VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, lastEventIdSemaphoreVariable);

	for (map<VuoCompilerNode *, GlobalVariable *>::iterator i = semaphoreVariableForNode.begin(); i != semaphoreVariableForNode.end(); ++i)
		VuoCompilerCodeGenUtilities::generateFinalizationForDispatchObject(module, block, i->second);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generate the nodeInstanceInit function, which initializes all stateful nodes' instance data.
 *
 * \eg{void nodeInstanceInit(void);}
 */
void VuoCompilerBitcodeGenerator::generateInitFunction(void)
{
	/// @todo This should return the instance data - https://b33p.net/kosada/node/2639
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "nodeInstanceInit", module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "checkInitNode", function, NULL);

	Function *isNodeInBothCompositionsFunction = VuoCompilerCodeGenUtilities::getIsNodeInBothCompositionsFunction(module);
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->getInstanceDataClass())
		{
			// bool shouldNotInit = isNodeInBothCompositions(/*node identifier*/);
			Value *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, node->getCompiler()->getGraphvizIdentifier());
			CallInst *shouldNotInitValue = CallInst::Create(isNodeInBothCompositionsFunction, nodeIdentifierValue, "", block);

			// if (! shouldNotInit)
			BasicBlock *initBlock = BasicBlock::Create(module->getContext(), "initNode", function, NULL);
			BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "checkInitNode", function, NULL);
			ICmpInst *shouldNotInitIsFalse = new ICmpInst(*block, ICmpInst::ICMP_EQ, shouldNotInitValue, zeroValue, "");
			BranchInst::Create(initBlock, nextBlock, shouldNotInitIsFalse, block);

			// { /* call nodeInstanceInit() for node */ }
			node->getCompiler()->generateInitFunctionCall(module, initBlock);

			BranchInst::Create(nextBlock, initBlock);
			block = nextBlock;
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generate the nodeInstanceFini function, which does the following for each node that needs to be finalized (either the composition is stopping, or the node has been removed during live coding) —
 *
 *    - calls nodeInstanceFini for each stateful node
 *    - finalizes (releases) input and output port data values
 *
 * \eg{void nodeInstanceFini(void);}
 */
void VuoCompilerBitcodeGenerator::generateFiniFunction(void)
{
	/// @todo This should take the instance data - https://b33p.net/kosada/node/2639
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), false);
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "nodeInstanceFini", module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "checkFiniNode", function, NULL);

	Function *isNodeInBothCompositionsFunction = VuoCompilerCodeGenUtilities::getIsNodeInBothCompositionsFunction(module);
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;

		// release output port values
		node->getCompiler()->generateFinalization(module, block, false);

		// bool shouldNotFini = isNodeInBothCompositions(/*node identifier*/);
		Value *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, node->getCompiler()->getGraphvizIdentifier());
		CallInst *shouldNotFiniValue = CallInst::Create(isNodeInBothCompositionsFunction, nodeIdentifierValue, "", block);

		// if (! shouldNotFini)
		BasicBlock *finiBlock = BasicBlock::Create(module->getContext(), "finiNode", function, NULL);
		BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "checkFiniNode", function, NULL);
		ICmpInst *shouldNotFiniIsFalse = new ICmpInst(*block, ICmpInst::ICMP_EQ, shouldNotFiniValue, zeroValue, "");
		BranchInst::Create(finiBlock, nextBlock, shouldNotFiniIsFalse, block);

		// call nodeInstanceFini() for node
		if (node->getNodeClass()->getCompiler()->getInstanceDataClass())
			node->getCompiler()->generateFiniFunctionCall(module, finiBlock);

		// release input port values
		node->getCompiler()->generateFinalization(module, finiBlock, true);

		BranchInst::Create(nextBlock, finiBlock);
		block = nextBlock;
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the nodeInstanceTriggerStart() function, which starts calls to @c VuoOutputTrigger functions in all stateful nodes.
 *
 * Assumes the trigger function for each node's trigger port has been generated and associated with the port.
 *
 * \eg{void nodeInstanceTriggerStart(void);}
 */
void VuoCompilerBitcodeGenerator::generateCallbackStartFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCallbackStartFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	// Since a node's nodeInstanceTriggerStart() function can generate an event,
	// make sure trigger functions wait until all nodes' init functions have completed.
	generateWaitForNodes(module, function, block, orderedNodes);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->getInstanceDataClass())
		{
			// { /* call nodeInstanceTriggerStart() for node */ }
			node->getCompiler()->generateCallbackStartFunctionCall(module, block);
		}
	}

	generateSignalForNodes(module, block, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the nodeInstanceTriggerStop() function, which stops calls to @c VuoOutputTrigger functions in all stateful nodes.
 *
 * \eg{void nodeInstanceTriggerStop(void);}
 */
void VuoCompilerBitcodeGenerator::generateCallbackStopFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCallbackStopFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	// Wait for any in-progress events to complete.
	generateWaitForNodes(module, function, block, orderedNodes);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->getInstanceDataClass())
		{
			// { /* call nodeInstanceTriggerStop() for node */ }
			node->getCompiler()->generateCallbackStopFunctionCall(module, block);
		}
	}

	// Signal semaphores so they can be safely released.
	generateSignalForNodes(module, block, orderedNodes);

	// Flush any pending events.
	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;
		Function *barrierWorkerFunction = trigger->generateSynchronousSubmissionToDispatchQueue(module, block, trigger->getIdentifier() + "__barrier");
		BasicBlock *barrierBlock = BasicBlock::Create(module->getContext(), "", barrierWorkerFunction, NULL);
		ReturnInst::Create(module->getContext(), barrierBlock);
	}
	generateWaitForNodes(module, function, block, orderedNodes);
	generateSignalForNodes(module, block, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to get a unique event ID.
 *
 * @eg{
 * dispatch_semaphore_wait(lastEventIdSemaphore, DISPATCH_TIME_FOREVER);
 * unsigned long eventId = ++lastEventId;
 * dispatch_semaphore_signal(lastEventIdSemaphore);
 * }
 */
Value * VuoCompilerBitcodeGenerator::generateGetNextEventID(Module *module, BasicBlock *block)
{
	VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, block, lastEventIdSemaphoreVariable);

	LoadInst *lastEventIdValue = new LoadInst(lastEventIdVariable, "", false, block);
	ConstantInt *oneValue = ConstantInt::get(module->getContext(), APInt(64, 1));
	Value *incrementedLastEventIdValue = BinaryOperator::Create(Instruction::Add, lastEventIdValue, oneValue, "", block);
	new StoreInst(incrementedLastEventIdValue, lastEventIdVariable, false, block);

	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, lastEventIdSemaphoreVariable);

	return incrementedLastEventIdValue;
}

/**
 * Generates code to wait on the semaphore of each of the given nodes.
 *
 * If @a shouldBlock is true, the generated code waits indefinitely for each semaphore.
 * Otherwise, if any of the semaphores is not immediately available, the generated code signals any
 * semaphores claimed so far and returns.
 *
 * Returns a value that is true if all semaphore were successfully claimed and false if none were claimed.
 *
 * @eg{
 * // When an event reaches a node through just one edge into the node, this code is generated just once.
 * // When an event reaches a node through multiple edges (i.e., the node is at the hub of a feedback loop
 * // or at a gather), this code is generated for each of those edges. When one of those edges claims the
 * // semaphore, the rest of the edges for the same event need to recognize this and also stop waiting.
 * // Hence the checking of event IDs and the limited-time wait.
 *
 * int64_t timeoutDelta = (shouldBlock ? ... : 0);
 * dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeoutDelta);
 * bool keepTrying = true;
 *
 * while (vuo_math_count__Count__eventIDClaimingSemaphore != eventId && keepTrying)
 * {
 *    int ret = dispatch_semaphore_wait(vuo_math_count__Count__semaphore, timeout);
 *    if (ret == 0)
 *       vuo_math_count__Count__eventIDClaimingSemaphore = eventId;
 *    else if (! shouldBlock)
 *       keepTrying = false;
 * }
 * if (! keepTrying)
 * {
 *    goto END;
 * }
 *
 * while (vuo_math_add__Add__eventIDClaimingSemaphore != eventId && keepTrying)
 * {
 *    int ret = dispatch_semaphore_wait(vuo_math_add__Add__semaphore, timeout);
 *    if (ret == 0)
 *       vuo_math_add__Add__eventIDClaimingSemaphore = eventId;
 *    else if (! shouldBlock)
 *       keepTrying = false;
 * }
 * if (! keepTrying)
 * {
 *    dispatch_semaphore_signal(vuo_math_count__Count__semaphore);
 *    goto END;
 * }
 */
Value * VuoCompilerBitcodeGenerator::generateWaitForNodes(Module *module, Function *function, BasicBlock *&block,
														  vector<VuoCompilerNode *> nodes, Value *eventIdValue, bool shouldBlock)
{
	sortNodes(nodes);

	if (! eventIdValue)
		eventIdValue = generateGetNextEventID(module, block);

	int64_t timeoutInNanoseconds = (shouldBlock ? NSEC_PER_SEC / 1000 : 0);  /// @todo (https://b33p.net/kosada/node/6682)
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	ConstantInt *falseValue = ConstantInt::get(module->getContext(), APInt(1, 0));
	ConstantInt *trueValue = ConstantInt::get(module->getContext(), APInt(1, 1));

	AllocaInst *keepTryingVariable = new AllocaInst(IntegerType::get(module->getContext(), 1), "keepTrying", block);
	new StoreInst(trueValue, keepTryingVariable, block);

	BasicBlock *getKeepTryingBlock = BasicBlock::Create(module->getContext(), "getKeepTrying", function);

	vector<VuoCompilerNode *> waitedNodes;

	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		BasicBlock *checkEventIdBlock = BasicBlock::Create(module->getContext(), "checkEventId", function);
		BranchInst::Create(checkEventIdBlock, block);

		GlobalVariable *claimingEventIdVariable = claimingEventIdVariableForNode[node];
		Value *claimingEventIdValue = new LoadInst(claimingEventIdVariable, "", false, checkEventIdBlock);
		ICmpInst *claimingEventIdNotEqualsEventId = new ICmpInst(*checkEventIdBlock, ICmpInst::ICMP_NE, claimingEventIdValue, eventIdValue, "");
		BasicBlock *checkKeepTryingBlock = BasicBlock::Create(module->getContext(), "checkKeepTrying", function);
		BasicBlock *doneWaitingBlock = BasicBlock::Create(module->getContext(), "doneWaitingNodeSemaphore", function);
		BranchInst::Create(checkKeepTryingBlock, doneWaitingBlock, claimingEventIdNotEqualsEventId, checkEventIdBlock);

		Value *keepTryingValue = new LoadInst(keepTryingVariable, "", false, checkKeepTryingBlock);
		ICmpInst *keepTryingIsTrue = new ICmpInst(*checkKeepTryingBlock, ICmpInst::ICMP_EQ, keepTryingValue, trueValue);
		BasicBlock *waitBlock = BasicBlock::Create(module->getContext(), "waitNodeSemaphore", function);
		BranchInst::Create(waitBlock, doneWaitingBlock, keepTryingIsTrue, checkKeepTryingBlock);

		GlobalVariable *semaphoreVariable = semaphoreVariableForNode[node];
		Value *retValue = VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, waitBlock, semaphoreVariable, timeoutInNanoseconds);

		ICmpInst *retEqualsZero = new ICmpInst(*waitBlock, ICmpInst::ICMP_EQ, retValue, zeroValue, "");
		BasicBlock *setEventIdBlock = BasicBlock::Create(module->getContext(), "setEventId", function);
		BasicBlock *setKeepTryingBlock = BasicBlock::Create(module->getContext(), "setKeepTrying", function);
		BasicBlock *endWhileBlock = BasicBlock::Create(module->getContext(), "endWhile", function);
		BranchInst::Create(setEventIdBlock, setKeepTryingBlock, retEqualsZero, waitBlock);

		new StoreInst(eventIdValue, claimingEventIdVariable, false, setEventIdBlock);
		BranchInst::Create(endWhileBlock, setEventIdBlock);

		if (! shouldBlock)
			new StoreInst(falseValue, keepTryingVariable, setKeepTryingBlock);
		BranchInst::Create(endWhileBlock, setKeepTryingBlock);

		BranchInst::Create(checkEventIdBlock, endWhileBlock);

		keepTryingValue = new LoadInst(keepTryingVariable, "", false, doneWaitingBlock);
		ICmpInst *keepTryingIsFalse = new ICmpInst(*doneWaitingBlock, ICmpInst::ICMP_EQ, keepTryingValue, falseValue);
		BasicBlock *cleanUpBlock = BasicBlock::Create(module->getContext(), "cleanUp", function);
		BasicBlock *endNodeBlock = BasicBlock::Create(module->getContext(), "endNode", function);
		BranchInst::Create(cleanUpBlock, endNodeBlock, keepTryingIsFalse, doneWaitingBlock);

		generateSignalForNodes(module, cleanUpBlock, waitedNodes);
		waitedNodes.push_back(node);
		BranchInst::Create(getKeepTryingBlock, cleanUpBlock);

		block = endNodeBlock;
	}

	BranchInst::Create(getKeepTryingBlock, block);

	Value *keepTryingValue = new LoadInst(keepTryingVariable, "", false, getKeepTryingBlock);
	BasicBlock *endBlock = BasicBlock::Create(module->getContext(), "endWaitForNodes", function);
	BranchInst::Create(endBlock, getKeepTryingBlock);

	block = endBlock;
	return keepTryingValue;
}

/**
 * Generates a call to signal the semaphore of each of the given nodes.
 *
 * @eg{
 * // For each node:
 * vuo_math_count__Count__eventClaimingSemaphore = NO_EVENT_ID;
 * dispatch_semaphore_signal(vuo_math_count__Count__semaphore);
 * }
 */
void VuoCompilerBitcodeGenerator::generateSignalForNodes(Module *module, BasicBlock *block, vector<VuoCompilerNode *> nodes)
{
	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		GlobalVariable *claimingEventIdVariable = claimingEventIdVariableForNode[node];
		new StoreInst(noEventIdConstant, claimingEventIdVariable, false, block);

		GlobalVariable *semaphoreVariable = semaphoreVariableForNode[node];
		VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, semaphoreVariable);
	}
}

/**
 * Generates a call to @c <Type>_getString(...) to get a string representation of the port's current value.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerialization(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, false, false);
}

/**
 * Generates a call to @c <Type>_getInterprocessString(...) to get an interprocess-safe string representation of the port's current value.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerializationInterprocess(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, false, true);
}

/**
 * Generates a call to @c <Type>_getSummary(...) to get a brief description of the port's current value,
 * or a null value if the port is event-only.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSummary(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, true, false);
}

/**
 * Generates a call to @c <Type>_getString() or @c <Type>_getSummary() for the port.
 * or a null value if the port is event-only.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerializationOrSummary(Module *module, BasicBlock *block, VuoCompilerPort *port, bool isSummary, bool isInterprocess)
{
	Value *stringOrSummaryValue = NULL;
	GlobalVariable *dataVariable = NULL;
	VuoCompilerType *type = NULL;

	/// @todo Refactor by replacing with VuoCompilerPort::getDataVariable() and VuoCompilerPort::getDataType()
	VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>(port);
	if (eventPort)
	{
		VuoCompilerData *data = eventPort->getData();
		if (data)
		{
			dataVariable = data->getVariable();
			type = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler())->getVuoType()->getCompiler();
		}
	}
	else
	{
		VuoCompilerTriggerPort *triggerPort = dynamic_cast<VuoCompilerTriggerPort *>(port);
		if (triggerPort)
		{
			dataVariable = triggerPort->getPreviousDataVariable();
			if (dataVariable)
			{
				type = triggerPort->getClass()->getDataVuoType()->getCompiler();
			}
		}
	}

	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	ConstantPointerNull *nullStringValue = ConstantPointerNull::get(pointerToCharType);

	if (dataVariable)
	{
		LoadInst *dataValue = new LoadInst(dataVariable, "", block);
		Type *llvmType = dataValue->getType();

		// <type>_getString(dataValue)
		stringOrSummaryValue = (isSummary ?
									type->generateSummaryFromValueFunctionCall(module, block, dataValue) :
									(isInterprocess ?
										 type->generateInterprocessStringFromValueFunctionCall(module, block, dataValue) :
										 type->generateStringFromValueFunctionCall(module, block, dataValue)
										 ));

		if (llvmType->isPointerTy())
		{
			// dataValue != NULL ? <type>_getString(dataValue) : NULL
			ConstantPointerNull *nullDataValue = ConstantPointerNull::get(static_cast<PointerType *>(llvmType));
			ICmpInst *dataValueNotNull = new ICmpInst(*block, ICmpInst::ICMP_NE, dataValue, nullDataValue, "");
			stringOrSummaryValue = SelectInst::Create(dataValueNotNull, stringOrSummaryValue, nullStringValue, "", block);
		}
	}
	else
	{
		stringOrSummaryValue = nullStringValue;
	}

	return stringOrSummaryValue;
}

/**
 * Generates the @c getOutputPortValue() function and the @c getOutputPortValueThreadUnsafe() function.
 */
void VuoCompilerBitcodeGenerator::generateGetPortValueOrSummaryFunctions(void)
{
	generateGetPortValueOrSummaryFunction(false, true, true);
	generateGetPortValueOrSummaryFunction(false, true, false);
	generateGetPortValueOrSummaryFunction(false, false, true);
	generateGetPortValueOrSummaryFunction(false, false, false);
	generateGetPortValueOrSummaryFunction(true, false, false);
	generateGetPortValueOrSummaryFunction(true, true, false);
}

/**
 * Generates one of the following functions:
 *    - @c getInputPortSummary()
 *    - @c getOutputPortSummary()
 *    - @c getInputPortValue()
 *    - @c getOutputPortValue()
 *    - @c getInputPortValueThreadUnsafe()
 *    - @c getOutputPortValueThreadUnsafe()
 *
 * Each of these functions returns a string representation of the value of a data-and-event port.
 *
 * The thread-safe functions synchronize against other accesses of the node's port values.
 * The thread-unsafe functions make synchronization the responsibility of the caller.
 *
 * The caller of these functions is responsible for freeing the return value.
 *
 * Assumes the semaphore for each node has been initialized.
 *
 * Example:
 *
 * @eg{
 * char * getOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
 * {
 *	 char *ret = NULL;
 *   if (! strcmp(portIdentifier, "vuo_math_add__Add2__sum"))
 *   {
 *     dispatch_semaphore_wait(vuo_math_add__Add2__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     ret = VuoInteger_getString(vuo_math_add__Add2__sum);
 *     dispatch_semaphore_signal(vuo_math_add__Add2__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_console_read__ReadFromConsole3__string"))
 *   {
 *     dispatch_semaphore_wait(vuo_console_read__ReadFromConsole3__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     if (vuo_console_read__ReadFromConsole3__string != NULL) {
 *       ret = VuoString_getString(vuo_console_read__ReadFromConsole3__string);
 *     } else {
 *       ret = NULL;
 *     }
 *     dispatch_semaphore_signal(vuo_console_read__ReadFromConsole3__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_mouse__GetMouse__leftPressed"))
 *   {
 *     dispatch_semaphore_wait(vuo_mouse__GetMouse__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     ret = VuoPoint2d_getString(vuo_mouse__GetMouse__leftPressed__previous);
 *     dispatch_semaphore_signal(vuo_mouse__GetMouse__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_image_ripple__RippleImage__rippledImage"))
 *   {
 *     dispatch_semaphore_wait(vuo_image_ripple__RippleImage__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     if (shouldUseInterprocessSerialization)
 *       ret = VuoImage_getInterprocessString(vuo_image_ripple__RippleImage__rippledImage);
 *     else
 *       ret = VuoImage_getString(vuo_image_ripple__RippleImage__rippledImage);
 *     dispatch_semaphore_signal(vuo_image_ripple__RippleImage__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPortValueOrSummaryFunction(bool isSummary, bool isInput, bool isThreadSafe)
{
	PointerType *pointerToi8Type = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	Function *function = (isSummary ?
							  (isInput ?
								   VuoCompilerCodeGenUtilities::getGetInputPortSummaryFunction(module) :
								   VuoCompilerCodeGenUtilities::getGetOutputPortSummaryFunction(module)) :
							  (isInput ?
								   (isThreadSafe ?
										VuoCompilerCodeGenUtilities::getGetInputPortValueFunction(module) :
										VuoCompilerCodeGenUtilities::getGetInputPortValueThreadUnsafeFunction(module)) :
								   (isThreadSafe ?
										VuoCompilerCodeGenUtilities::getGetOutputPortValueFunction(module) :
										VuoCompilerCodeGenUtilities::getGetOutputPortValueThreadUnsafeFunction(module))));

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *shouldUseInterprocessSerializationValue = NULL;
	if (!isSummary)
	{
		shouldUseInterprocessSerializationValue = args++;
		shouldUseInterprocessSerializationValue->setName("shouldUseInterprocessSerialization");
	}

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	AllocaInst *retPointer = new AllocaInst(pointerToi8Type, "ret", initialBlock);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToi8Type);
	new StoreInst(nullValue, retPointer, false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> ports = (isInput ? node->getInputPorts() : node->getOutputPorts());
		for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoPort *port = *j;
			string currentPortIdentifier;
			GlobalVariable *dataVariable = NULL;

			/// @todo Refactor by replacing with VuoCompilerPort::getIdentifier() and VuoCompilerPort::getDataVariable()
			VuoCompilerEventPort *eventPort = dynamic_cast<VuoCompilerEventPort *>(port->getCompiler());
			if (eventPort)
			{
				VuoCompilerData *data = eventPort->getData();
				if (data)
				{
					currentPortIdentifier = eventPort->getIdentifier();
					dataVariable = data->getVariable();
				}
			}
			else
			{
				VuoCompilerTriggerPort *triggerPort = dynamic_cast<VuoCompilerTriggerPort *>(port->getCompiler());
				if (triggerPort)
				{
					currentPortIdentifier = triggerPort->getIdentifier();
					dataVariable = triggerPort->getPreviousDataVariable();
				}
			}

			if (dataVariable)
			{
				BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
				BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
				BasicBlock *origCurrentBlock = currentBlock;

				vector<VuoCompilerNode *> nodeList;
				nodeList.push_back(node->getCompiler());
				if (isThreadSafe)
				{
					// dispatch_semaphore_wait(currentBlock, DISPATCH_TIME_FOREVER);
					generateWaitForNodes(module, function, currentBlock, nodeList);
				}

				// <Type>_getSummary(data) or <Type>_getString(data) or <Type>_getInterprocessString(data)
				Value *dataValueAsString;
				VuoCompilerPort *compilerPort = static_cast<VuoCompilerPort *>(port->getCompiler());
				if (isSummary)
				{
					dataValueAsString = generatePortSummary(module, currentBlock, compilerPort);
					new StoreInst(dataValueAsString, retPointer, currentBlock);
					BranchInst::Create(finalBlock, currentBlock);
				}
				else
				{
					BasicBlock *serializationBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
					dataValueAsString = generatePortSerialization(module, serializationBlock, compilerPort);
					new StoreInst(dataValueAsString, retPointer, serializationBlock);
					BranchInst::Create(finalBlock, serializationBlock);

					BasicBlock *interprocessSerializationBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
					dataValueAsString = generatePortSerializationInterprocess(module, interprocessSerializationBlock, compilerPort);
					new StoreInst(dataValueAsString, retPointer, interprocessSerializationBlock);
					BranchInst::Create(finalBlock, interprocessSerializationBlock);

					// if (shouldUseInterprocessSerialization)
					//   [type]_getString(...);
					// else
					//   [type]_getInterprocessString(...);
					ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(shouldUseInterprocessSerializationValue->getType()), 0);
					ICmpInst *shouldUseInterprocessSerializationValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, shouldUseInterprocessSerializationValue, zeroValue, "");
					BranchInst::Create(interprocessSerializationBlock, serializationBlock, shouldUseInterprocessSerializationValueIsTrue, currentBlock);
				}

				if (isThreadSafe)
				{
					// dispatch_semaphore_signal(nodeSemaphore);
					generateSignalForNodes(module, finalBlock, nodeList);
				}

				blocksForString[currentPortIdentifier] = make_pair(origCurrentBlock, finalBlock);
			}
		}
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	LoadInst *retValue = new LoadInst(retPointer, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString);
}

/**
 * Generates the @c setInputPortValue() function, which provides a way to set each
 * input data-and-event port's value.
 *
 * Each definition of @c <Type>_makeFromString() that returns heap data is responsible for registering
 * its return value (with @c VuoRegister).
 *
 * Assumes the semaphore for each node has been initialized.
 *
 * Assumes the trigger function has been set for each published input port's trigger port.
 *
 * Example:
 *
 * @eg{
 * void setInputPortValue(char *portIdentifier, char *valueAsString, int shouldUpdateCallbacks)
 * {
 *   if (! strcmp(portIdentifier, "vuo_time_firePeriodically__FirePeriodically__seconds"))
 *   {
 *     VuoReal value = VuoReal_makeFromString(valueAsString);
 *     dispatch_semaphore_wait(vuo_time_firePeriodically__FirePeriodically__semaphore, DISPATCH_TIME_FOREVER);
 *     vuo_time_firePeriodically__FirePeriodically__seconds = value;
 *     if (shouldUpdateCallbacks)
 *       vuo_time_firePeriodically__FirePeriodically__nodeInstanceCallbackUpdate(...);
 *     dispatch_semaphore_signal(vuo_time_firePeriodically__FirePeriodically__semaphore);
 *     char *summary = VuoReal_getSummary(value);
 *     sendInputPortsUpdated(portIdentifier, false, true, summary);
 *     free(summary);
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_console_write__Write__string"))
 *   {
 *     VuoText value = VuoText_makeFromString(valueAsString);
 *     dispatch_semaphore_wait(vuo_console_write__Write__semaphore, DISPATCH_TIME_FOREVER);
 *     VuoRelease((void *)vuo_console_write__Write__string);
 *     vuo_console_write__Write__string = VuoText_makeFromString(valueAsString);
 *     VuoRetain((void *)vuo_console_write__Write__string);
 *     dispatch_semaphore_signal(vuo_console_write__Write__semaphore);
 *     char *summary = VuoText_getSummary(value);
 *     sendInputPortsUpdated(portIdentifier, false, true, summary);
 *     free(summary);
 *   }
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateSetInputPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");
	Value *shouldUpdateCallbacksValue = args++;
	shouldUpdateCallbacksValue->setName("shouldUpdateCallbacks");

	Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);
	Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
	Type *boolType = sendInputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	Constant *trueValue = ConstantInt::get(boolType, 1);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> inputPorts = node->getInputPorts();
		for (vector<VuoPort *>::iterator j = inputPorts.begin(); j != inputPorts.end(); ++j)
		{
			VuoCompilerInputEventPort *port = dynamic_cast<VuoCompilerInputEventPort *>((*j)->getCompiler());
			if (port)
			{
				VuoCompilerInputData *data = port->getData();
				if (data)
				{
					string currentPortIdentifier = port->getIdentifier();

					BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
					BasicBlock *initialBlockForString = currentBlock;

					// <Type> value = <Type>_makeFromString(valueAsString);
					VuoCompilerDataClass *dataClass = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler());
					VuoCompilerType *type = dataClass->getVuoType()->getCompiler();
					Value *dataValue = type->generateValueFromStringFunctionCall(module, currentBlock, valueAsStringValue);

					// dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);
					GlobalVariable *semaphoreVariable = semaphoreVariableForNode[node->getCompiler()];
					VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, currentBlock, semaphoreVariable);

					// VuoRelease((void *)data);
					LoadInst *dataValueBefore = data->generateLoad(currentBlock);
					type->generateReleaseCall(module, currentBlock, dataValueBefore);

					// data = value;
					data->generateStore(dataValue, currentBlock);

					// VuoRetain((void *)data);
					type->generateRetainCall(module, currentBlock, dataValue);

					// if (shouldUpdateCallbacks)
					//   <Node>_nodeInstanceCallbackUpdate(...);  // if this function exists
					Constant *zeroValue = ConstantInt::get(shouldUpdateCallbacksValue->getType(), 0);
					ICmpInst *shouldUpdateCallbacksIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, shouldUpdateCallbacksValue, zeroValue, "");
					BasicBlock *updateCallbacksBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
					BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
					BranchInst::Create(updateCallbacksBlock, nextBlock, shouldUpdateCallbacksIsTrue, currentBlock);
					node->getCompiler()->generateCallbackUpdateFunctionCall(module, updateCallbacksBlock);
					BranchInst::Create(nextBlock, updateCallbacksBlock);
					currentBlock = nextBlock;

					// char *summary = <Type>_getSummary(value);
					Value *summaryValue = type->generateSummaryFromValueFunctionCall(module, currentBlock, dataValue);

					// sendInputPortsUpdated(portIdentifier, false, true, valueAsString, summary);
					vector<Value *> sendInputPortsUpdatedArgs;
					sendInputPortsUpdatedArgs.push_back(portIdentifierValue);
					sendInputPortsUpdatedArgs.push_back(falseValue);
					sendInputPortsUpdatedArgs.push_back(trueValue);
					sendInputPortsUpdatedArgs.push_back(summaryValue);
					CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", currentBlock);

					// free(summary);
					CallInst::Create(freeFunction, summaryValue, "", currentBlock);

					generateEventlessTransmission(function, currentBlock, node->getCompiler(), true);

					// dispatch_semaphore_signal(nodeSemaphore);
					VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, currentBlock, semaphoreVariable);

					blocksForString[currentPortIdentifier] = make_pair(initialBlockForString, currentBlock);
				}
			}
		}
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString);
}

/**
 * Generates code to initialize each input or output data-and-event port's value.
 */
void VuoCompilerBitcodeGenerator::generateInitializationForPorts(BasicBlock *block, bool input)
{
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> ports = (input ? node->getInputPorts() : node->getOutputPorts());
		for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoCompilerEventPort *port = dynamic_cast<VuoCompilerEventPort *>((*j)->getCompiler());
			if (port)
			{
				VuoCompilerData *data = port->getData();
				if (data)
				{
					string initialValue = (input ? static_cast<VuoCompilerInputData *>(data)->getInitialValue() : "");
					Constant *initialValuePointer = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, initialValue);

					VuoCompilerDataClass *dataClass = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler());
					VuoCompilerType *type = dataClass->getVuoType()->getCompiler();
					Value *dataValue = type->generateValueFromStringFunctionCall(module, block, initialValuePointer);
					data->generateStore(dataValue, block);

					LoadInst *dataVariableAfter = data->generateLoad(block);
					type->generateRetainCall(module, block, dataVariableAfter);
				}
			}
		}
	}
}

/**
 * Generates code to initialize each input and output data-and-event port reachable by eventless transmission.
 *
 * Assumes all other input ports' values have already been initialized.
 */
void VuoCompilerBitcodeGenerator::generateInitialEventlessTransmissions(Function *function, BasicBlock *&block)
{
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		generateEventlessTransmission(function, block, node, false);
	}
}

/**
 * Generates the fireTriggerPortEvent function.
 *
 * Assumes the trigger function has been set for each trigger port.
 *
 * @eg{
 * void fireTriggerPortEvent(char *portIdentifier)
 * {
 *   if (! strcmp(portIdentifier, "vuo_time_firePeriodically__FirePeriodically__fired"))
 *   {
 *     vuo_time_firePeriodically__FirePeriodically__fired();
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_console_window__DisplayConsoleWindow__typedLine"))
 *   {
 *     waitForNodeSemaphore(vuo_console_window__DisplayConsoleWindow);
 *     vuo_console_window__DisplayConsoleWindow__typedLine( vuo_console_window__DisplayConsoleWindow__typedLine__previous );
 *     signalNodeSemaphore(vuo_console_window__DisplayConsoleWindow);
 *   }
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateFireTriggerPortEventFunction(void)
{
	string functionName = "fireTriggerPortEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		functionParams.push_back(PointerType::get(IntegerType::get(module->getContext(), 8), 0));
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;
		string currentPortIdentifier = trigger->getIdentifier();
		Function *triggerFunction = trigger->getFunction();
		VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
		BasicBlock *origCurrentBlock = currentBlock;

		vector<Value *> triggerArgs;
		GlobalVariable *dataVariable = trigger->getPreviousDataVariable();
		if (dataVariable)
		{
			generateWaitForNodes(module, function, currentBlock, vector<VuoCompilerNode *>(1, triggerNode));

			Value *arg = new LoadInst(dataVariable, "", false, currentBlock);
			Value *secondArg = NULL;
			Value **secondArgIfNeeded = (triggerFunction->getFunctionType()->getNumParams() == 2 ? &secondArg : NULL);
			arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, triggerFunction, 0, secondArgIfNeeded, module, currentBlock);
			triggerArgs.push_back(arg);
			if (secondArg)
				triggerArgs.push_back(secondArg);
		}

		CallInst::Create(triggerFunction, triggerArgs, "", currentBlock);

		if (dataVariable)
			generateSignalForNodes(module, currentBlock, vector<VuoCompilerNode *>(1, triggerNode));

		blocksForString[currentPortIdentifier] = make_pair(origCurrentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString);
}

/**
 * Generates functions for retrieving information about published ports.
 */
void VuoCompilerBitcodeGenerator::generatePublishedPortGetters(void)
{
	generateGetPublishedPortCountFunction(true);
	generateGetPublishedPortCountFunction(false);

	generateGetPublishedPortNamesFunction(true);
	generateGetPublishedPortNamesFunction(false);

	generateGetPublishedPortTypesFunction(true);
	generateGetPublishedPortTypesFunction(false);

	generateGetPublishedPortDetailsFunction(true);
	generateGetPublishedPortDetailsFunction(false);

	generateGetPublishedPortConnectedIdentifierCount(true);
	generateGetPublishedPortConnectedIdentifierCount(false);

	generateGetPublishedPortConnectedIdentifiers(true);
	generateGetPublishedPortConnectedIdentifiers(false);
}

/**
 * Generates the getPublishedInputPortCount or getPublishedInputPortCount function.
 *
 * unsigned int getPublishedInputPortCount(void);
 * unsigned int getPublishedOutputPortCount(void);
 *
 * Example:
 *
 * @eg{
 * unsigned int getPublishedInputPortCount(void)
 * {
 *		return 5;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortCountFunction(bool input)
{
	size_t count;
	string functionName;
	if (input)
	{
		count = composition->getBase()->getPublishedInputPorts().size();
		functionName = "getPublishedInputPortCount";
	}
	else
	{
		count = composition->getBase()->getPublishedOutputPorts().size();
		functionName = "getPublishedOutputPortCount";
	}

	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);
	ConstantInt *countConstant = ConstantInt::get(module->getContext(), APInt(32, count));
	ReturnInst::Create(module->getContext(), countConstant, block);
}

/**
 * Generates the getPublishedInputPortNames or getPublishedOutputPortNames function.
 *
 * char ** getPublishedInputPortNames(void);
 * char ** getPublishedOutputPortNames(void);
 *
 * Example:
 *
 * @eg{
 * char ** getPublishedInputPortNames(void)
 * {
 *		return { "firstName", "secondName" };
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortNamesFunction(bool input)
{
	string functionName;
	vector<VuoPublishedPort *> publishedPorts;
	if (input)
	{
		functionName = "getPublishedInputPortNames";
		publishedPorts = composition->getBase()->getPublishedInputPorts();
	}
	else
	{
		functionName = "getPublishedOutputPortNames";
		publishedPorts = composition->getBase()->getPublishedOutputPorts();
	}

	vector<string> names;
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		names.push_back( (*i)->getName() );
	}

	generateFunctionReturningStringArray(functionName, names);
}

/**
 * Generates the getPublishedInputPortTypes or getPublishedOutputPortTypes function.
 *
 * `char ** getPublishedInputPortTypes(void);`
 * `char ** getPublishedOutputPortTypes(void);`
 *
 * Example:
 *
 * @eg{
 * char ** getPublishedInputPortTypes(void)
 * {
 *		return { "VuoInteger", "VuoText", "VuoText" };
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortTypesFunction(bool input)
{
	string functionName;
	vector<VuoPublishedPort *> publishedPorts;
	if (input)
	{
		functionName = "getPublishedInputPortTypes";
		publishedPorts = composition->getBase()->getPublishedInputPorts();
	}
	else
	{
		functionName = "getPublishedOutputPortTypes";
		publishedPorts = composition->getBase()->getPublishedOutputPorts();
	}

	vector<string> types;
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoType *type = (*i)->getType();
		string typeName = type ? type->getModuleKey() : "";
		types.push_back(typeName);
	}

	generateFunctionReturningStringArray(functionName, types);
}

/**
 * Generates the getPublishedInputPortDetails or getPublishedOutputPortDetailss function.
 *
 * `char ** getPublishedInputPortDetails(void);`
 * `char ** getPublishedOutputPortDetails(void);`
 *
 * Example:
 *
 * @eg{
 * char ** getPublishedInputPortDetails(void)
 * {
 *		return { "{\"default\":0}", ... };
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortDetailsFunction(bool input)
{
	string functionName;
	vector<VuoPublishedPort *> publishedPorts;
	if (input)
	{
		functionName = "getPublishedInputPortDetails";
		publishedPorts = composition->getBase()->getPublishedInputPorts();
	}
	else
	{
		functionName = "getPublishedOutputPortDetails";
		publishedPorts = composition->getBase()->getPublishedOutputPorts();
	}

	vector<string> details;
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoCompilerPublishedPort *publishedPort = (*i)->getCompiler();

		json_object *detailsObj = publishedPort->getDetails();
		string detailsSerialized = json_object_to_json_string_ext(detailsObj, JSON_C_TO_STRING_PLAIN);
		details.push_back(detailsSerialized);

		json_object_put(detailsObj);
	}

	generateFunctionReturningStringArray(functionName, details);
}

/**
 * Generates a function that returns a constant array of strings (char **).
 *
 * @param functionName The name for the function.
 * @param stringValues The values for the array of strings.
 */
void VuoCompilerBitcodeGenerator::generateFunctionReturningStringArray(string functionName, vector<string> stringValues)
{
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToPointerToChar = PointerType::get(pointerToChar, 0);
		vector<Type *> functionParams;
		FunctionType *functionType = FunctionType::get(pointerToPointerToChar, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);

	Constant *stringArrayGlobalPointer = VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfStrings(module, stringValues);
	ReturnInst::Create(module->getContext(), stringArrayGlobalPointer, block);
}

/**
 * Generates the getPublishedInputPortConnectedIdentifierCount or getPublishedOutputPortConnectedIdentifierCount function.
 *
 * `unsigned int getPublishedInputPortConnectedIdentifierCount(char *name);`
 * `unsigned int getPublishedOutputPortConnectedIdentifierCount(char *name);`
 *
 * Example:
 *
 * @eg{
 * unsigned int getPublishedInputPortConnectedIdentifierCount(char *name)
 * {
 *		unsigned int ret = 0;
 *		if (! strcmp(name, "firstName"))
 *			ret = 2;
 *		else if (! strcmp(name, "secondName"))
 *			ret = 1;
 *		return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortConnectedIdentifierCount(bool input)
{
	string functionName;
	vector<VuoPublishedPort *> publishedPorts;
	if (input)
	{
		functionName = "getPublishedInputPortConnectedIdentifierCount";
		publishedPorts = composition->getBase()->getPublishedInputPorts();
	}
	else
	{
		functionName = "getPublishedOutputPortConnectedIdentifierCount";
		publishedPorts = composition->getBase()->getPublishedOutputPorts();
	}

	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		functionParams.push_back(PointerType::get(IntegerType::get(module->getContext(), 8), 0));
		FunctionType *functionType = FunctionType::get(IntegerType::get(module->getContext(), 32), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *nameValue = args++;
	nameValue->setName("name");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	AllocaInst *retPointer = new AllocaInst(IntegerType::get(module->getContext(), 32), "ret", initialBlock);
	ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(32, 0));
	new StoreInst(zeroValue, retPointer, false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		string currentName = (*i)->getName();
		size_t currentCount = (*i)->getCompiler()->getConnectedPortIdentifiers().size();

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);
		ConstantInt *countValue = ConstantInt::get(module->getContext(), APInt(32, currentCount));
		new StoreInst(countValue, retPointer, false, currentBlock);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	LoadInst *retValue = new LoadInst(retPointer, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, nameValue, blocksForString);
}

/**
 * Generates the getPublishedInputPortConnectedIdentifiers or getPublishedOutputPortConnectedIdentifiers function.
 *
 * `char ** getPublishedInputPortConnectedIdentifiers(char *name);`
 * `char ** getPublishedOutputPortConnectedIdentifiers(char *name);`
 *
 * Example:
 *
 * @eg{
 * char ** getPublishedInputPortConnectedIdentifiers(char *name)
 * {
 *		char **ret = NULL;
 *		if (name == "firstName")
 *			ret = { "vuo_math_count_increment__Count1__increment", "vuo_math_count_increment__Count2__decrement" };
 *		else if (name == "secondName")
 *			ret = { "vuo_string_cut__Cut1__string" };
 *		return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortConnectedIdentifiers(bool input)
{
	string functionName;
	vector<VuoPublishedPort *> publishedPorts;
	if (input)
	{
		functionName = "getPublishedInputPortConnectedIdentifiers";
		publishedPorts = composition->getBase()->getPublishedInputPorts();
	}
	else
	{
		functionName = "getPublishedOutputPortConnectedIdentifiers";
		publishedPorts = composition->getBase()->getPublishedOutputPorts();
	}

	PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	PointerType *pointerToPointerToChar = PointerType::get(pointerToChar, 0);

	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		functionParams.push_back(PointerType::get(IntegerType::get(module->getContext(), 8), 0));
		FunctionType *functionType = FunctionType::get(pointerToPointerToChar, functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *nameValue = args++;
	nameValue->setName("name");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	AllocaInst *retPointer = new AllocaInst(pointerToPointerToChar, "ret", initialBlock);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToPointerToChar);
	new StoreInst(nullValue, retPointer, false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		string currentName = (*i)->getName();
		set<string> currentIdentifiersSet = (*i)->getCompiler()->getConnectedPortIdentifiers();

		vector<string> currentIdentifiers( currentIdentifiersSet.size() );
		copy(currentIdentifiersSet.begin(), currentIdentifiersSet.end(), currentIdentifiers.begin());

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);
		Constant *identifiersValue = VuoCompilerCodeGenUtilities::generatePointerToConstantArrayOfStrings(module, currentIdentifiers);
		new StoreInst(identifiersValue, retPointer, false, currentBlock);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	LoadInst *retValue = new LoadInst(retPointer, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, nameValue, blocksForString);
}

/**
 * Generates the firePublishedInputPortEvent function.
 *
 * Assumes the trigger function has been set for each published input port's trigger port.
 *
 * Example:
 *
 * @eg{
 * void firePublishedInputPortEvent(char *name)
 * {
 *		if (name == "firstName")
 *			vuo_in__PublishedInputPorts__firstName();
 *		else if (name == "secondName")
 *			vuo_in__PublishedInputPorts__secondName();
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateFirePublishedInputPortEventFunction(void)
{
	string functionName = "firePublishedInputPortEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		vector<Type *> functionParams;
		functionParams.push_back(PointerType::get(IntegerType::get(module->getContext(), 8), 0));
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *nameValue = args++;
	nameValue->setName("name");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	vector<VuoPublishedPort *> publishedPorts = composition->getBase()->getPublishedInputPorts();
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoCompilerPublishedInputPort *publishedInputPort = static_cast<VuoCompilerPublishedInputPort *>((*i)->getCompiler());
		string currentName = publishedInputPort->getBase()->getName();
		Function *triggerFunction = publishedInputPort->getTriggerPort()->getFunction();

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);
		CallInst::Create(triggerFunction, "", currentBlock);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	VuoNode *publishedInputNode = composition->getPublishedInputNode();
	if (publishedInputNode)
	{
		string currentName = VuoNodeClass::publishedInputNodeSimultaneousTriggerName;
		VuoPort *triggerBasePort = publishedInputNode->getOutputPortWithName(currentName);
		VuoCompilerTriggerPort *triggerPort = static_cast<VuoCompilerTriggerPort *>(triggerBasePort->getCompiler());
		Function *triggerFunction = triggerPort->getFunction();

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);
		CallInst::Create(triggerFunction, "", currentBlock);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, nameValue, blocksForString);
}

/**
 * Generates the getPublishedOutputPortValue()/getPublishedInputPortValue() function.
 *
 * @eg{
 * char * getPublishedOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
 * {
 *	 char *ret = NULL;
 *   if (! strcmp(portIdentifier, "vuo_out__PublishedOutputPorts__firstName"))
 *   {
 *     ret = getOutputPortValue("vuo_math_add__Add__sum", shouldUseInterprocessSerialization);
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_out__PublishedOutputPorts__secondName"))
 *   {
 *     // no incoming data-and-event cable
 *   }
 *   return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortValueFunction(bool isInput)
{
	Function *function = isInput ?
				VuoCompilerCodeGenUtilities::getGetPublishedInputPortValueFunction(module)
			  : VuoCompilerCodeGenUtilities::getGetPublishedOutputPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *shouldUseInterprocessSerializationValue = args++;
	shouldUseInterprocessSerializationValue->setName("shouldUseInterprocessSerialization");

	Function *getValueFunction = isInput ?
				VuoCompilerCodeGenUtilities::getGetInputPortValueFunction(module)
			  : VuoCompilerCodeGenUtilities::getGetOutputPortValueFunction(module);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *retVariable = new AllocaInst(pointerToChar, "ret", initialBlock);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToChar);
	new StoreInst(nullValue, retVariable, false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	vector<VuoPublishedPort *> publishedPorts = isInput ?
				composition->getBase()->getPublishedInputPorts()
			  : composition->getBase()->getPublishedOutputPorts();
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoPublishedPort *publishedPort = *i;
		string currentName = publishedPort->getName();
		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

		set<VuoPort *> connectedPorts = publishedPort->getConnectedPorts();
		for (set<VuoPort *>::iterator j = connectedPorts.begin(); j != connectedPorts.end(); ++j)
		{
			VuoPort *port = *j;
			if (port->getClass()->getPortType() == VuoPortClass::dataAndEventPort)
			{
				string connectedPortIdentifier = static_cast<VuoCompilerEventPort *>(port->getCompiler())->getIdentifier();
				Constant *connectedPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, connectedPortIdentifier);

				vector<Value *> args;
				args.push_back(connectedPortIdentifierValue);
				args.push_back(shouldUseInterprocessSerializationValue);
				CallInst *getValueResult = CallInst::Create(getValueFunction, args, "", currentBlock);
				new StoreInst(getValueResult, retVariable, currentBlock);

				break;
			}
		}
		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	LoadInst *retValue = new LoadInst(retVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString);
}

/**
 * Generates the generateSetPublishedInputPortValue() function.
 *
 * @eg{
 * void setPublishedInputPortValue(char *portIdentifier, char *valueAsString)
 * {
 *   if (! strcmp(portIdentifier, "vuo_in__PublishedInputPorts__firstName"))
 *   {
 *     setInputPortValue("vuo_math_subtract__Subtract__a", valueAsString, true);
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_in__PublishedInputPorts__secondName"))
 *   {
 *     setInputPortValue("vuo_math_subtract__Subtract__b", valueAsString, true);
 *     setInputPortValue("vuo_math_subtract__Subtract2__b", valueAsString, true);
 *   }
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateSetPublishedInputPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetPublishedInputPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");

	Function *setValueFunction = VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(module);
	ConstantInt *trueValue = ConstantInt::get(module->getContext(), APInt(32, 1));

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	vector<VuoPublishedPort *> publishedPorts = composition->getBase()->getPublishedInputPorts();
	for (vector<VuoPublishedPort *>::iterator i = publishedPorts.begin(); i != publishedPorts.end(); ++i)
	{
		VuoPublishedPort *publishedPort = *i;
		string currentName = publishedPort->getName();
		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

		VuoPort *pseudoPort = publishedPort->getCompiler()->getVuoPseudoPort();
		set<VuoCable *> publishedInputCables = composition->getBase()->getPublishedInputCables();
		for (set<VuoCable *>::iterator j = publishedInputCables.begin(); j != publishedInputCables.end(); ++j)
		{
			VuoCable *publishedCable = *j;
			if (publishedCable->getFromPort() == pseudoPort && publishedCable->getCompiler()->carriesData())
			{
				VuoCompilerEventPort *connectedPort = static_cast<VuoCompilerEventPort *>(publishedCable->getToPort()->getCompiler());
				string connectedPortIdentifier = connectedPort->getIdentifier();
				Constant *connectedPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, connectedPortIdentifier);

				vector<Value *> args;
				args.push_back(connectedPortIdentifierValue);
				args.push_back(valueAsStringValue);
				args.push_back(trueValue);
				CallInst::Create(setValueFunction, args, "", currentBlock);
			}
		}
		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString);
}

/**
 * Generates code to transmit an event (if any) and data (if any) from @a outputPort to all connected input ports,
 * and send telemetry indicating that these output and input ports have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock,
																	 VuoCompilerPort *outputPort, Value *dataValue,
																	 bool requiresEvent, bool shouldSendTelemetry)
{
	Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);
	Type *boolType = sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(1);

	Value *sentDataValue = NULL;
	Value *dataSummaryValue = NULL;
	Constant *trueValue = ConstantInt::get(boolType, 1);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	if (shouldSendTelemetry)
	{
		if (dataValue)
		{
			// sentData = true;
			sentDataValue = trueValue;

			// dataSummary = <type>_getSummary(portValue);
			VuoCompilerType *type = outputPort->getDataVuoType()->getCompiler();
			dataSummaryValue = type->generateSummaryFromValueFunctionCall(module, currentBlock, dataValue);
		}
		else
		{
			// sentData = false;
			sentDataValue = falseValue;

			// dataSummary = NULL;
			PointerType *pointerToCharType = static_cast<PointerType *>(sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(2));
			dataSummaryValue = ConstantPointerNull::get(pointerToCharType);
		}

		generateSendOutputPortUpdated(currentBlock, outputPort, sentDataValue, dataSummaryValue);
	}

	// If the output port should transmit an event...
	bool alwaysTransmitsEvent = (dynamic_cast<VuoCompilerTriggerPort *>(outputPort) || ! requiresEvent);
	BasicBlock *transmissionBlock = NULL;
	BasicBlock *noTransmissionBlock = NULL;
	if (alwaysTransmitsEvent)
	{
		transmissionBlock = currentBlock;
	}
	else
	{
		transmissionBlock = BasicBlock::Create(module->getContext(), "", function, NULL);
		noTransmissionBlock = BasicBlock::Create(module->getContext(), "", function, NULL);

		VuoCompilerOutputEventPort *outputEventPort = dynamic_cast<VuoCompilerOutputEventPort *>(outputPort);
		LoadInst *eventValue = outputEventPort->generateLoad(currentBlock);
		ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
		ICmpInst *eventValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
		BranchInst::Create(transmissionBlock, noTransmissionBlock, eventValueIsTrue, currentBlock);
	}

	// ... then transmit the event and data (if any) to each connected input port.
	set<VuoCompilerCable *> outgoingCables = (requiresEvent ?
												  graph->getCablesImmediatelyDownstream(outputPort) :
												  graph->getCablesImmediatelyEventlesslyDownstream(outputPort));
	for (set<VuoCompilerCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
	{
		VuoCompilerCable *cable = *i;
		Value *transmittedDataValue = (cable->carriesData() ? dataValue : NULL);

		cable->generateTransmission(module, transmissionBlock, transmittedDataValue, requiresEvent);

		if (shouldSendTelemetry)
		{
			VuoCompilerPort *inputPort = static_cast<VuoCompilerPort *>(cable->getBase()->getToPort()->getCompiler());
			Value *receivedDataValue = (transmittedDataValue ? trueValue : falseValue);
			Value *dataSummaryValueOrNull = (transmittedDataValue ? dataSummaryValue : NULL);
			generateSendInputPortUpdated(transmissionBlock, inputPort, receivedDataValue, dataSummaryValueOrNull);
		}
	}

	if (! alwaysTransmitsEvent)
	{
		BranchInst::Create(noTransmissionBlock, transmissionBlock);
		currentBlock = noTransmissionBlock;
	}

	if (shouldSendTelemetry && dataValue)
	{
		// free(dataSummary)
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, dataSummaryValue, "", currentBlock);
	}
}

/**
 * Generates code to transmit an event (if any) and data (if any) through each outgoing cable from @a node,
 * and to send telemetry indicating that the output and input ports on these cables have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTransmissionFromNode(Function *function, BasicBlock *&currentBlock, VuoCompilerNode *node,
															   bool requiresEvent, bool shouldSendTelemetry)
{
	vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		// If the output port is a trigger port, do nothing.
		VuoCompilerOutputEventPort *outputEventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (! outputEventPort)
			continue;

		BasicBlock *telemetryBlock = NULL;
		BasicBlock *noTelemetryBlock = NULL;
		if (requiresEvent)
		{
			telemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);
			noTelemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);

			// If the output port isn't transmitting an event, do nothing.
			Value *eventValue = outputEventPort->generateLoad(currentBlock);
			ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
			ICmpInst *eventValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
			BranchInst::Create(telemetryBlock, noTelemetryBlock, eventValueIsTrue, currentBlock);
		}
		else
		{
			telemetryBlock = currentBlock;
		}

		// Transmit the data through the output port to each connected input port.
		VuoCompilerOutputData *outputData = outputEventPort->getData();
		Value *outputDataValue = outputData ? outputData->generateLoad(telemetryBlock) : NULL;
		generateTransmissionFromOutputPort(function, telemetryBlock, outputEventPort, outputDataValue,
										   requiresEvent, shouldSendTelemetry);

		if (requiresEvent)
		{
			BranchInst::Create(noTelemetryBlock, telemetryBlock);
			currentBlock = noTelemetryBlock;
		}
	}
}

/**
 * Generates code to transmit data without an event through each data-and-event output port of @a firstNode
 * and onward to all data-and-event ports reachable via eventless transmission.
 *
 * If @a isCompositionStarted is true, assumes that the semaphore for @a firstNode has already been claimed.
 */
void VuoCompilerBitcodeGenerator::generateEventlessTransmission(Function *function, BasicBlock *&currentBlock,
																VuoCompilerNode *firstNode, bool isCompositionStarted)
{
	if (! graph->mayTransmitEventlessly(firstNode))
		return;
	vector<VuoCompilerNode *> downstreamNodes = graph->getNodesEventlesslyDownstream(firstNode);

	if (isCompositionStarted)
	{
		// Claim the nodes downstream via eventless transmission.
		generateWaitForNodes(module, function, currentBlock, downstreamNodes, NULL, true);
	}

	// For this node and each node downstream via eventless transmission...
	vector<VuoCompilerNode *> nodesToVisit = downstreamNodes;
	nodesToVisit.insert(nodesToVisit.begin(), firstNode);
	for (vector<VuoCompilerNode *>::iterator i = nodesToVisit.begin(); i != nodesToVisit.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		if (graph->mayTransmitEventlessly(node))
		{
			// Call the node's event function, and send telemetry if needed.
			generateNodeExecution(function, currentBlock, node, isCompositionStarted);

			// Transmit data through the node's outgoing cables, and send telemetry for port updates if needed.
			generateTransmissionFromNode(function, currentBlock, node, false, isCompositionStarted);
		}

		if (isCompositionStarted && node != firstNode)
			generateSignalForNodes(module, currentBlock, vector<VuoCompilerNode *>(1, node));
	}
}

/**
 * Generates code to call the node's event function, sending telemetry indicating that execution has started and finished.
 */
void VuoCompilerBitcodeGenerator::generateNodeExecution(Function *function, BasicBlock *&currentBlock, VuoCompilerNode *node,
														bool shouldSendTelemetry)
{
	Constant *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, node->getIdentifier());

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has started.
		Function *sendNodeExecutionStartedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionStartedFunction(module);
		CallInst::Create(sendNodeExecutionStartedFunction, nodeIdentifierValue, "", currentBlock);
	}

	// Call the node's event function.
	if (debugMode)
		VuoCompilerCodeGenUtilities::generatePrint(module, currentBlock, node->getBase()->getTitle() + "\n");
	BasicBlock *nodeEventFinalBlock = BasicBlock::Create(module->getContext(), "", function, NULL);
	node->generateEventFunctionCall(module, function, currentBlock, nodeEventFinalBlock);
	currentBlock = nodeEventFinalBlock;

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has finished.
		Function *sendNodeExecutionFinishedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionFinishedFunction(module);
		CallInst::Create(sendNodeExecutionFinishedFunction, nodeIdentifierValue, "", currentBlock);
	}
}

/**
 * Generates a call to @c sendOutputPortsUpdated() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendOutputPortUpdated(BasicBlock *block, VuoCompilerPort *outputPort,
																Value *sentDataValue, Value *dataSummaryValue)
{
	Constant *outputPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, outputPort->getIdentifier());
	Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);

	// sendOutputPortsUpdated(outputPortIdentifier, sentData, outputDataSummary);
	vector<Value *> sendOutputPortsUpdatedArgs;
	sendOutputPortsUpdatedArgs.push_back(outputPortIdentifierValue);
	sendOutputPortsUpdatedArgs.push_back(sentDataValue);
	sendOutputPortsUpdatedArgs.push_back(dataSummaryValue);
	CallInst::Create(sendOutputPortsUpdatedFunction, sendOutputPortsUpdatedArgs, "", block);
}

/**
 * Generates a call to @c sendInputPortsUpdated() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendInputPortUpdated(BasicBlock *block, VuoCompilerPort *inputPort,
															   Value *sentDataValue, Value *outputDataSummaryValue)
{
	Constant *inputPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, inputPort->getIdentifier());

	Value *inputDataSummaryValue;
	if (outputDataSummaryValue)
	{
		// inputDataSummary = triggerDataSummary;
		inputDataSummaryValue = outputDataSummaryValue;
	}
	else
	{
		// inputDataSummary = <Type>_getSummary(inputData);
		inputDataSummaryValue = generatePortSummary(module, block, inputPort);
	}

	// sendInputPortsUpdated(inputPortIdentifier, true, sentData, inputDataSummary);
	Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);
	Type *boolType = sendInputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	Constant *trueValue = ConstantInt::get(boolType, 1);
	vector<Value *> sendInputPortsUpdatedArgs;
	sendInputPortsUpdatedArgs.push_back(inputPortIdentifierValue);
	sendInputPortsUpdatedArgs.push_back(trueValue);
	sendInputPortsUpdatedArgs.push_back(sentDataValue);
	sendInputPortsUpdatedArgs.push_back(inputDataSummaryValue);
	CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", block);

	if (! outputDataSummaryValue)
	{
		// free(inputDataSummary)
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		CallInst::Create(freeFunction, inputDataSummaryValue, "", block);
	}
}

/**
 * Generates a call to @ sendEventDropped() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendEventDropped(BasicBlock *block, VuoCompilerTriggerPort *triggerPort)
{
	Constant *portIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, triggerPort->getIdentifier());

	// sendEventDropped(triggerPortIdentifier);
	Function *sendEventDroppedFunction = VuoCompilerCodeGenUtilities::getSendEventDroppedFunction(module);
	vector<Value *> sendEventDroppedArgs;
	sendEventDroppedArgs.push_back(portIdentifierValue);
	CallInst::Create(sendEventDroppedFunction, sendEventDroppedArgs, "", block);
}

/**
 * Generates the vuoSerialize() function, which returns a string representation of the current state
 * of the running composition.
 *
 * The generated function assumes that no events are firing or executing (e.g., the composition is paused),
 * and that the composition's @c nodeInstanceInit() function has run.
 *
 * @eg{
 * char * vuoSerialize();
 * }
 */
void VuoCompilerBitcodeGenerator::generateSerializeFunction(void)
{
	Function *serializeFunction = VuoCompilerCodeGenUtilities::getSerializeFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", serializeFunction, NULL);

	vector<Value *> serializedComponentValues;

	Value *headerValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, "digraph G\n{\n");
	serializedComponentValues.push_back(headerValue);

	Value *lineSeparatorValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, "\n");

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;

		Value *serializedNodeValue = node->getCompiler()->generateSerializedString(module, block);
		serializedComponentValues.push_back(serializedNodeValue);
		serializedComponentValues.push_back(lineSeparatorValue);
	}

	Value *footerValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, "}\n");
	serializedComponentValues.push_back(footerValue);

	Value *serializedCompositionValue = VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, serializedComponentValues);
	ReturnInst::Create(module->getContext(), serializedCompositionValue, block);
}

/**
 * Generates the vuoUnserialize() function, which parses the string representation of a composition
 * and sets the state of the running composition accordingly.
 *
 * The generated function assumes that no events are triggering or executing (e.g., the composition is paused),
 * and that the composition's @c nodeInstanceInit() function has run.
 *
 * @eg{
 * void vuoUnserialize(char *serializedComposition);
 * }
 */
void VuoCompilerBitcodeGenerator::generateUnserializeFunction(void)
{
	Function *unserializeFunction = VuoCompilerCodeGenUtilities::getUnserializeFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", unserializeFunction, NULL);

	Function::arg_iterator args = unserializeFunction->arg_begin();
	Value *serializedCompositionValue = args++;
	serializedCompositionValue->setName("serializedComposition");

	// graph_t graph = openGraphvizGraph(serializedComposition);
	Function *openGraphvizGraphFunction = VuoCompilerCodeGenUtilities::getOpenGraphvizGraphFunction(module);
	CallInst *graphValue = CallInst::Create(openGraphvizGraphFunction, serializedCompositionValue, "getSerializedValue", block);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	VuoNode *publishedInputNode = composition->getPublishedInputNode();
	if (publishedInputNode)
		nodes.insert(publishedInputNode);
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		node->getCompiler()->generateUnserialization(module, unserializeFunction, block, graphValue);
	}

	// closeGraphvizGraph(graph);
	Function *closeGraphvizGraphFunction = VuoCompilerCodeGenUtilities::getCloseGraphvizGraphFunction(module);
	CallInst::Create(closeGraphvizGraphFunction, graphValue, "", block);

	// If a drawer has been resized in this live-coding update, and if the list input port attached to the drawer was
	// unserialized after the drawer, then then list input port has been reverted to the old (unresized) list value.
	// So re-transmit the value from the drawer to the list input.
	generateInitialEventlessTransmissions(unserializeFunction, block);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generate the header of the function that's called each time the trigger port generates a push.
 */
Function * VuoCompilerBitcodeGenerator::generateTriggerFunctionHeader(VuoCompilerTriggerPort *trigger)
{
	string functionName = trigger->getIdentifier();
	VuoCompilerTriggerPortClass *portClass = trigger->getClass();
	FunctionType *functionType = portClass->getFunctionType();
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);

	VuoType *paramVuoType = portClass->getDataVuoType();
	if (paramVuoType)
	{
		Attributes paramAttributes = paramVuoType->getCompiler()->getFunctionParameterAttributes();
		function->addAttribute(1, paramAttributes);
	}

	return function;
}

/**
 * Generate the body of the function that's called each time the trigger port generates a push.
 * The function schedules downstream nodes for execution.
 *
 * Assumes the header of the function has already been set as the function for @c trigger.
 *
 * @eg{
 * // PlayMovie:decodedImage -> TwirlImage:image
 * // PlayMovie:decodedImage -> RippleImage:image
 * // TwirlImage:image -> BlendImages:background
 * // RippleImage:image -> BlendImages:foreground
 *
 * void PlayMovie_decodedImage(VuoImage image)
 * {
 *   VuoRetain(image);
 *   void *context = malloc(sizeof(VuoImage));
 *   *context = image;
 *   dispatch_async_f(PlayMovie_decodedImage_queue, PlayMovie_decodedImage_worker(), context);
 * }
 *
 * void PlayMovie_decodedImage_worker(void *context)
 * {
 *   // If paused, ignore this event.
 *   if (isPaused)
 *     return;
 *   // Otherwise...
 *
 *   // Get a unique ID for this event.
 *   unsigned long eventId = getNextEventId();
 *
 *   // Wait for the nodes directly downstream of the trigger port.
 *   waitForNodeSemaphore(PlayMovie, eventId);
 *   waitForNodeSemaphore(TwirlImage, eventId);
 *   waitForNodeSemaphore(RippleImage, eventId);
 *
 *   // Handle the trigger port value having changed.
 *   VuoRelease(PlayMovie_decodedImage__previousData);
 *   PlayMovie_decodedImage__previousData = (VuoImage)(*context);
 *   free(context);
 *   signalNodeSemaphore(PlayMovie);
 *
 *   // Send telemetry indicating that the trigger port value may have changed.
 *   sendTelemetry(PortHadEvent, PlayMovie_decodedImage, TwirlImage_image);
 *
 *   // Transmit data and events along each of the trigger's cables.
 *   transmitDataAndEvent(PlayMovie_decodedImage__previousData, TwirlImage_image);  // retains new TwirlImage_image, releases old
 *
 *
 *   // Schedule each chain downstream of the trigger.
 *
 *   dispatch_group_t TwirlImage_chain_group = dispatch_group_create();
 *   dispatch_group_t RippleImage_chain_group = dispatch_group_create();
 *   dispatch_group_t BlendImages_chain_group = dispatch_group_create();
 *
 *   dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
 *
 *   void **TwirlImage_chain_context = (void **)malloc(sizeof(void *));
 *   unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long);
 *   *eventIdPtr = eventId;
 *   TwirlImage_chain_context[0] = (void *)eventIdPtr;
 *   dispatch_group_async_f(globalQueue, TwirlImage_chain_group, TwirlImage_chain_worker, (void *)TwirlImage_chain_context);
 *
 *   void **RippleImage_chain_context = (void **)malloc(sizeof(void *));
 *   unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long);
 *   *eventIdPtr = eventId;
 *   RippleImage_chain_context[0] = (void *)eventIdPtr;
 *   dispatch_group_async_f(globalQueue, RippleImage_chain_group, RippleImage_chain_worker, (void *)RippleImage_chain_context);
 *
 *   void **BlendImages_chain_context = (void **)malloc(3 * sizeof(void *));
 *   unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long);
 *   *eventIdPtr = eventId;
 *   BlendImages_chain_context[0] = (void *)eventIdPtr;
 *   *BlendImages_chain_context[1] = TwirlImage_chain_group;
 *   dispatch_retain(TwirlImage_chain_group);
 *   *BlendImages_chain_context[2] = RippleImage_chain_group;
 *   dispatch_retain(RippleImage_chain_group);
 *   dispatch_group_async_f(globalQueue, BlendImages_chain_group, BlendImages_chain_worker, (void *)BlendImages_chain_context);
 *
 *   dispatch_release(TwirlImage_chain_group);
 *   dispatch_release(RippleImage_chain_group);
 *   dispatch_release(BlendImages_chain_group);
 * }
 *
 * void TwirlImage_chain_worker(void *context)
 * {
 *   unsigned long eventId = (unsigned long)(*context[0]);
 *   free(context[0]);
 *
 *   // For each node in the chain...
 *   // If the node received an event, then...
 *   if (nodeReceivedEvent(TwirlImage))
 *   {
 *      // Send telemetry indicating that the node's execution has started.
 *      sendTelemetry(NodeExecutionStarted, TwirlImage);
 *
 *      // Call the node's event function.
 *      TwirlImage_nodeEvent(...);
 *
 *      // Send telemetry indicating that the node's execution has finished.
 *      sendTelemetry(NodeExecutionEnded, TwirlImage);
 *
 *      // Wait for the nodes directly downstream of the current node that may receive an event from it.
 *      waitForNodeSemaphore(BlendImages, eventId);
 *
 *      // Send telemetry indicating that the node's output port values, and any connected input port values, may have changed.
 *      sendTelemetry(PortHadEvent, TwirlImage_image, BlendImages_background);
 *
 *      // Transmit data and events along the node's output cables.
 *      transmitDataAndEvent(TwirlImage_image, BlendImages_background);  // retains new BlendImages_background, releases old
 *
 *      // If this was the last time the node could receive a push from this event, signal the node's semaphore.
 *      signalNodeSemaphore(BlendImages);
 *   }
 * }
 *
 * void RippleImage_chain_worker(void *context)
 * {
 *   ...
 * }
 *
 * void BlendImages_chain_worker(void *context)
 * {
 *   unsigned long eventId = (unsigned long)(*context[0]);
 *   free(context[0]);
 *
 *   // Wait for any chains directly upstream to complete.
 *   dispatch_group_t TwirlImage_chain_group = (dispatch_group_t)context[1];
 *   dispatch_group_t RippleImage_chain_group = (dispatch_group_t)context[2];
 *   dispatch_group_wait(TwirlImage_chain_group);
 *   dispatch_group_wait(RippleImage_chain_group);
 *   dispatch_group_release(TwirlImage_chain_group);
 *   dispatch_group_release(RippleImage_chain_group);
 *
 *   ...
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateTriggerFunctionBody(VuoCompilerTriggerPort *trigger)
{
	Function *function = trigger->getFunction();

	// Schedule the following:
	BasicBlock *workerSchedulerBlock = BasicBlock::Create(module->getContext(), "workerSchedulerBlock", function, NULL);
	Function *triggerWorker = trigger->generateAsynchronousSubmissionToDispatchQueue(module, workerSchedulerBlock, trigger->getIdentifier());
	ReturnInst::Create(module->getContext(), workerSchedulerBlock);

	BasicBlock *isPausedComparisonBlock = BasicBlock::Create(module->getContext(), "isPausedComparisonBlock", triggerWorker, NULL);
	BasicBlock *triggerBlock = BasicBlock::Create(module->getContext(), "triggerBlock", triggerWorker, NULL);
	BasicBlock *triggerReturnBlock = BasicBlock::Create(module->getContext(), "triggerReturnBlock", triggerWorker, NULL);


	// If paused, ignore this event.
	ICmpInst *isPausedValueIsTrue = VuoCompilerCodeGenUtilities::generateIsPausedComparison(module, isPausedComparisonBlock);
	BranchInst::Create(triggerReturnBlock, triggerBlock, isPausedValueIsTrue, isPausedComparisonBlock);
	// Otherwise...

	// Get a unique ID for this event.
	Value *eventIdValueInTriggerWorker = generateGetNextEventID(module, triggerBlock);

	// Attempt to claim all necessary downstream nodes.
	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
	bool canDropEvents = (trigger->getBase()->getEventThrottling() == VuoPortClass::EventThrottling_Drop);
	Value *didClaimNodes = generateWaitForNodes(module, triggerWorker, triggerBlock, triggerWaitNodes, eventIdValueInTriggerWorker, ! canDropEvents);

	// If this trigger can drop events and the downstream nodes are not immediately available,
	// then drop this event, and send telemetry that the event was dropped.
	if (canDropEvents)
	{
		ConstantInt *trueValue = ConstantInt::get(module->getContext(), APInt(1, 1));
		ICmpInst *claimedNodesIsTrue = new ICmpInst(*triggerBlock, ICmpInst::ICMP_EQ, didClaimNodes, trueValue, "");
		BasicBlock *nextTriggerBlock = BasicBlock::Create(module->getContext(), "triggerBlock2", triggerWorker, NULL);
		BasicBlock *dropEventBlock = BasicBlock::Create(module->getContext(), "dropEvent", triggerWorker, NULL);
		BranchInst::Create(nextTriggerBlock, dropEventBlock, claimedNodesIsTrue, triggerBlock);
		triggerBlock = nextTriggerBlock;

		trigger->generateDataValueDiscard(module, dropEventBlock, triggerWorker);
		generateSendEventDropped(dropEventBlock, trigger);
		BranchInst::Create(triggerReturnBlock, dropEventBlock);
	}

	// Update the stored trigger port value.
	Value *triggerDataValue = trigger->generateDataValueUpdate(module, triggerBlock, triggerWorker);

	// If the node containing the trigger no longer needs to be claimed, signal it.
	VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];
	bool isTriggerNodeClaimed = find(triggerWaitNodes.begin(), triggerWaitNodes.end(), triggerNode) != triggerWaitNodes.end();
	if (isTriggerNodeClaimed)
	{
		vector<VuoCompilerNode *> nodesDownstreamOfTrigger = graph->getNodesDownstream(trigger);
		bool isTriggerNodeDownstream = find(nodesDownstreamOfTrigger.begin(), nodesDownstreamOfTrigger.end(), triggerNode) != nodesDownstreamOfTrigger.end();
		if (! isTriggerNodeDownstream)
			generateSignalForNodes(module, triggerBlock, vector<VuoCompilerNode *>(1, triggerNode));
	}

	// Transmit events and data (if any) out of the trigger port, and send telemetry for port updates.
	generateTransmissionFromOutputPort(triggerWorker, triggerBlock, trigger, triggerDataValue);


	// For each chain downstream of the trigger...

	// Create a dispatch group for the chain.
	vector<VuoCompilerChain *> chains = chainsForTrigger[trigger];
	for (vector<VuoCompilerChain *>::iterator i = chains.begin(); i != chains.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		chain->generateAllocationForDispatchGroup(module, triggerBlock, trigger->getIdentifier());
		chain->generateInitializationForDispatchGroup(module, triggerBlock);
	}

	set<VuoCompilerNode *> scheduledNodes;
	for (vector<VuoCompilerChain *>::iterator i = chains.begin(); i != chains.end(); ++i)
	{
		VuoCompilerChain *chain = *i;

		vector<VuoCompilerChain *> upstreamChains;
		vector<VuoCompilerNode *> chainNodes = chain->getNodes();
		VuoCompilerNode *firstNodeInThisChain = chainNodes.front();
		for (vector<VuoCompilerChain *>::iterator j = chains.begin(); j != chains.end(); ++j)
		{
			VuoCompilerChain *otherChain = *j;

			if (chain == otherChain)
				break;  // Any chains after this are downstream.

			VuoCompilerNode *lastNodeInOtherChain = otherChain->getNodes().back();
			if (graph->mayTransmit(lastNodeInOtherChain, firstNodeInThisChain, trigger))
				upstreamChains.push_back(otherChain);
		}


		// Schedule the following:
		Function *chainWorker = chain->generateSubmissionForDispatchGroup(module, triggerBlock, eventIdValueInTriggerWorker, upstreamChains);
		BasicBlock *chainSetupBlock = BasicBlock::Create(module->getContext(), "chainSetupBlock", chainWorker, 0);


		// Wait for any chains directly upstream to complete.
		chain->generateWaitForUpstreamChains(module, chainWorker, chainSetupBlock);

		// For each node in the chain...
		Value *eventIdValueInChainWorker = chain->generateEventIdValue(module, chainWorker, chainSetupBlock);
		chain->generateFreeContextArgument(module, chainWorker, chainSetupBlock);
		BasicBlock *prevBlock = chainSetupBlock;
		for (vector<VuoCompilerNode *>::iterator j = chainNodes.begin(); j != chainNodes.end(); ++j)
		{
			VuoCompilerNode *node = *j;

			// If the node received an event, then...
			BasicBlock *nodeExecutionBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BasicBlock *downstreamWaitBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			Value *nodePushedCondition = node->generateReceivedEventCondition(prevBlock);
			BranchInst::Create(nodeExecutionBlock, downstreamWaitBlock, nodePushedCondition, prevBlock);

			// Call the node's event function, and send telemetry that the node's execution has started and finished.
			generateNodeExecution(chainWorker, nodeExecutionBlock, node);


			// Regardless of whether the node received an event...
			BranchInst::Create(downstreamWaitBlock, nodeExecutionBlock);

			// Wait on any necessary downstream nodes.
			if (! graph->isRepeatedInFeedbackLoop(node, trigger) || scheduledNodes.find(node) == scheduledNodes.end())
			{
				vector<VuoCompilerNode *> outputNodes = getNodesToWaitOnBeforeTransmission(trigger, node);
				generateWaitForNodes(module, chainWorker, downstreamWaitBlock, outputNodes, eventIdValueInChainWorker);
			}


			// If the node received an event, then...
			BasicBlock *transmissionTelemetryBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BranchInst::Create(transmissionTelemetryBlock, signalBlock, nodePushedCondition, downstreamWaitBlock);

			// Transmit events and data through the node's outgoing cables, and send telemetry for port updates.
			generateTransmissionFromNode(chainWorker, transmissionTelemetryBlock, node);

			// Reset the node's event inputs.
			node->generatePushedReset(transmissionTelemetryBlock);


			// Regardless of whether the node received an event...
			BranchInst::Create(signalBlock, transmissionTelemetryBlock);

			// If this was the last time this event could reach the node, signal the node's semaphore.
			if (! graph->isRepeatedInFeedbackLoop(node, trigger) || scheduledNodes.find(node) != scheduledNodes.end())
			{
				vector<VuoCompilerNode *> nodeList;
				nodeList.push_back(node);
				generateSignalForNodes(module, signalBlock, nodeList);
			}
			scheduledNodes.insert(node);

			prevBlock = signalBlock;
		}

		ReturnInst::Create(module->getContext(), prevBlock);
	}

	// Release the dispatch group for each chain.
	for (vector<VuoCompilerChain *>::iterator i = chains.begin(); i != chains.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		chain->generateFinalizationForDispatchGroup(module, triggerBlock);
	}

	BranchInst::Create(triggerReturnBlock, triggerBlock);
	ReturnInst::Create(module->getContext(), triggerReturnBlock);
}

/**
 * Turn debug mode on/off. In debug mode, print statements are inserted into the generated bitcode.
 */
void VuoCompilerBitcodeGenerator::setDebugMode(bool debugMode)
{
	this->debugMode = debugMode;
}
