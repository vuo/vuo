/**
 * @file
 * VuoCompilerBitcodeGenerator implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sstream>
#include "VuoCable.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoComposition.hh"
#include "VuoCompositionMetadata.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoStringUtilities.hh"
#include "VuoPublishedPort.hh"
#include "VuoType.hh"

/**
 * Creates a bitcode generator from the specified composition.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition,
																							  bool isTopLevelComposition,
																							  string moduleKey, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator(composition, isTopLevelComposition, moduleKey, compiler);
	return cg;
}

/**
 * Private constructor.
 */
VuoCompilerBitcodeGenerator::VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition,
														 bool isTopLevelComposition,
														 string moduleKey, VuoCompiler *compiler)
{
#if VUO_PRO
	VuoCompilerBitcodeGenerator_Pro();
#endif
	module = NULL;
	debugMode = false;

	this->composition = composition;
	this->isTopLevelComposition = isTopLevelComposition;
	this->moduleKey = moduleKey;
	this->compiler = compiler;

	graph = composition->getCachedGraph(compiler);

	chainsForTrigger = graph->getChains();  // store in a data member, rather than calling getChains() multiple times, to preserve order of chains
	makeOrderedNodes();
	makeOrderedTypes();
	makePortContextInfo();
	makeSubcompositionModelPorts();
	makeDependencies();
}

/**
 * Destructor.
 */
VuoCompilerBitcodeGenerator::~VuoCompilerBitcodeGenerator(void)
{
	for (vector<VuoPort *>::iterator i = modelOutputPorts.begin(); i != modelOutputPorts.end(); ++i)
	{
		VuoPort *modelOutputPort = *i;
		if (modelOutputPort->getClass()->getPortType() == VuoPortClass::triggerPort)
		{
			delete modelOutputPort->getClass()->getCompiler();
			delete modelOutputPort->getCompiler();
		}
	}
}

/**
 * Helper for VuoCompilerBitcodeGenerator::makeOrderedNodes().
 */
struct ChainSort
{
	VuoCompilerGraph *graph;  ///< Used by operator().
	VuoCompilerTriggerPort *trigger;  ///< Used by operator().
	set<VuoCompilerNode *> lastNodeInLoop;  ///< Used by operator().

	/**
	 * Sort camparator for two chains.
	 */
	bool operator() (const vector<VuoCompilerNode *> &chainNodes1, const vector<VuoCompilerNode *> &chainNodes2)
	{
		// If the chains have an upstream-downstream relationship, return whether chainNodes1 is upstream of chainNodes2.

		vector<VuoCompilerNode *> downstreamOfChain1 = graph->getNodesDownstream(chainNodes1.back(), trigger);
		vector<VuoCompilerNode *> downstreamOfChain2 = graph->getNodesDownstream(chainNodes2.back(), trigger);

		bool isNode2DownstreamOfNode1 = find(downstreamOfChain1.begin(), downstreamOfChain1.end(), chainNodes2.front()) != downstreamOfChain1.end();
		bool isNode1DownstreamOfNode2 = find(downstreamOfChain2.begin(), downstreamOfChain2.end(), chainNodes1.front()) != downstreamOfChain2.end();

		if (isNode2DownstreamOfNode1 && isNode1DownstreamOfNode2)
			return (lastNodeInLoop.find(chainNodes1.front()) != lastNodeInLoop.end());
		else if (isNode2DownstreamOfNode1)
			return true;
		else if (isNode1DownstreamOfNode2)
			return false;

		// If at least one of the chains contains a trigger port, return the chain containing the trigger port with
		// the greatest number of downstream nodes.

		size_t maxNumDownstreamOfTrigger[2] = { 0, 0 };
		vector<VuoCompilerNode *> chainNodes[2] = { chainNodes1, chainNodes2 };
		for (int i = 0; i < 2; ++i)
		{
			for (vector<VuoCompilerNode *>::const_iterator j = chainNodes[i].begin(); j != chainNodes[i].end(); ++j)
			{
				VuoCompilerNode *node = *j;
				vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
				for (vector<VuoPort *>::iterator k = outputPorts.begin(); k != outputPorts.end(); ++k)
				{
					VuoPort *outputPort = *k;
					if (outputPort->getClass()->getPortType() == VuoPortClass::triggerPort)
					{
						VuoCompilerTriggerPort *trigger = dynamic_cast<VuoCompilerTriggerPort *>( outputPort->getCompiler() );
						size_t numDownstreamOfTrigger = graph->getNodesDownstream(trigger).size();
						maxNumDownstreamOfTrigger[i] = max(maxNumDownstreamOfTrigger[i], numDownstreamOfTrigger);
					}
				}
			}
		}

		if (maxNumDownstreamOfTrigger[0] != maxNumDownstreamOfTrigger[1])
			return maxNumDownstreamOfTrigger[0] > maxNumDownstreamOfTrigger[1];

		// Tiebreak: Sort alphabetically.

		return chainNodes1.front()->getIdentifier() < chainNodes2.front()->getIdentifier();
	}
};

/**
 * Sets up VuoCompilerBitcodeGenerator::orderedNodes and VuoCompilerBitcodeGenerator::downstreamNodesForTrigger.
 */
void VuoCompilerBitcodeGenerator::makeOrderedNodes(void)
{
	// For each trigger, put its downstream nodes into topological order (optimized with ChainSort so that
	// orderedNodes will be more likely to match the ordering of the triggers with more downstream nodes).
	for (VuoCompilerTriggerPort *trigger : graph->getTriggerPorts())
	{
		vector<VuoCompilerChain *> chains;
		map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> >::iterator chainsIter = chainsForTrigger.find(trigger);
		if (chainsIter != chainsForTrigger.end())
			chains = chainsIter->second;

		vector< vector<VuoCompilerNode *> > chainNodeLists;
		set<VuoCompilerNode *> lastNodeInLoop;
		for (VuoCompilerChain *chain : chains)
		{
			if (chain->isLastNodeInLoop())
				lastNodeInLoop.insert( chain->getNodes().front() );
			else
				chainNodeLists.push_back( chain->getNodes() );
		}

		ChainSort c;
		c.graph = graph;
		c.trigger = trigger;
		c.lastNodeInLoop = lastNodeInLoop;
		sort(chainNodeLists.begin(), chainNodeLists.end(), c);

		vector<VuoCompilerNode *> orderedNodeList;

		VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);
		orderedNodeList.push_back(triggerNode);

		for (vector<VuoCompilerNode *> chainNodeList : chainNodeLists)
		{
			auto triggerNodeIter = std::find(chainNodeList.begin(), chainNodeList.end(), triggerNode);
			if (triggerNodeIter != chainNodeList.end())
				chainNodeList.erase(triggerNodeIter);

			orderedNodeList.insert( orderedNodeList.end(), chainNodeList.begin(), chainNodeList.end() );
		}

		downstreamNodesForTrigger[trigger] = orderedNodeList;
	}

	vector< vector<VuoCompilerNode *> > orderedNodesPerTrigger;
	for (const map<VuoCompilerTriggerPort *, vector<VuoCompilerNode *> >::value_type &i : downstreamNodesForTrigger)
		orderedNodesPerTrigger.push_back(i.second);

	// For each node that can transmit without an event, put it and its downstream nodes in with the triggers' downstream nodes.
	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (graph->mayTransmitDataOnly(node))
		{
			vector<VuoCompilerNode *> downstreamNodes = graph->getNodesDownstreamViaDataOnlyTransmission(node);
			vector<VuoCompilerNode *> nodesInProgress;
			nodesInProgress.push_back(node);
			nodesInProgress.insert(nodesInProgress.end(), downstreamNodes.begin(), downstreamNodes.end());
			orderedNodesPerTrigger.push_back(nodesInProgress);
		}
	}

	// Put the downstream nodes per trigger in descending order of number of downstream nodes.
	std::sort(orderedNodesPerTrigger.begin(), orderedNodesPerTrigger.end(),
		[](const vector<VuoCompilerNode *> &nodes1, const vector<VuoCompilerNode *> &nodes2) {
			if (nodes1.size() != nodes2.size())
				return nodes1.size() < nodes2.size();

			// Tiebreak: Sort alphabetically.
			ostringstream oss1;
			ostringstream oss2;
			for (VuoCompilerNode *n : nodes1)
				oss1 << n->getIdentifier() << " ";
			for (VuoCompilerNode *n : nodes2)
				oss2 << n->getIdentifier() << " ";
			return oss1.str() < oss2.str();
		});

	// Visit each trigger, in descending order of number of downstream nodes (so that orderedNodes will be more likely
	// to match the ordering of the triggers with more downstream nodes, and thus be more likely to wait on them one at
	// a time instead of less efficiently having to wait on all initially).
	int previousTriggerNodeIndex = -1;
	for (vector< vector<VuoCompilerNode *> >::reverse_iterator i = orderedNodesPerTrigger.rbegin(); i != orderedNodesPerTrigger.rend(); ++i)
	{
		// Merge the trigger's downstream nodes into orderedNodes.
		int previousNodeIndex = previousTriggerNodeIndex;
		bool isFirstNode = true;
		for (VuoCompilerNode *node : *i)
		{
			vector<VuoCompilerNode *>::iterator nodeIter = find(orderedNodes.begin(), orderedNodes.end(), node);
			if (nodeIter == orderedNodes.end())
				nodeIter = orderedNodes.insert(orderedNodes.begin() + previousNodeIndex + 1, node);

			previousNodeIndex = max(previousNodeIndex, (int)(nodeIter - orderedNodes.begin()));
			if (isFirstNode)
			{
				previousTriggerNodeIndex = previousNodeIndex;
				isFirstNode = false;
			}
		}
	}

	// Add (at the end) any remaining nodes in the composition.
	for (VuoCompilerNode *node : graph->getNodes())
		if (find(orderedNodes.begin(), orderedNodes.end(), node) == orderedNodes.end())
			orderedNodes.push_back(node);

	for (size_t i = 0; i < orderedNodes.size(); ++i)
		orderedNodes[i]->setIndexInOrderedNodes(i);
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
	// Does the trigger have a scatter downstream, and another trigger overlaps with some branches of the scatter but
	// not others? If so, then all downstream nodes will be locked before the event can proceed.
	// (The analysis is an approximation. It checks if another trigger overlaps *anywhere* downstream, even if the
	// overlap is after the scatter has been fully gathered back up.)
	// This prevents deadlocks involving one trigger claiming one branch and the other trigger claiming another
	// (https://b33p.net/kosada/node/6696, https://b33p.net/kosada/node/12503).

	bool hasScatterOverlappedByAnotherTrigger = graph->hasScatterPartiallyOverlappedByAnotherTrigger(trigger);

	// Does the trigger port cause a `Spin Off` node to fire an event that has downstream nodes in common with the trigger?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents the trigger's event from getting stuck behind and having to wait on the `Spin Off` event,
	// defeating the purpose of the `Spin Off` node (https://b33p.net/kosada/node/11351).

	bool hasOverlapWithSpinOff = graph->hasOverlapWithSpinOff(trigger);

	// Would the trigger port wait on nodes in a different order than orderedNodes?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents deadlock where the events from two different trigger ports reach the downstream nodes in a different order
	// (https://b33p.net/kosada/node/7924).

	vector<VuoCompilerNode *> sortedDownstreamNodes = downstreamNodesForTrigger[trigger];
	sortNodes(sortedDownstreamNodes);
	bool hasOutOfOrderDownstreamNodes = (downstreamNodesForTrigger[trigger] != sortedDownstreamNodes);

	// Wait for either all nodes downstream of the trigger or the nodes directly connected to the trigger.
	vector<VuoCompilerNode *> nodesToWaitOn;
	if (hasScatterOverlappedByAnotherTrigger || hasOverlapWithSpinOff || hasOutOfOrderDownstreamNodes)
		nodesToWaitOn = downstreamNodesForTrigger[trigger];
	else
	{
		nodesToWaitOn = graph->getNodesImmediatelyDownstream(trigger);
		VuoCompilerNode *triggerNode = downstreamNodesForTrigger[trigger].front();
		if (find(nodesToWaitOn.begin(), nodesToWaitOn.end(), triggerNode) == nodesToWaitOn.end())
			nodesToWaitOn.push_back(triggerNode);
	}

	return nodesToWaitOn;
}

/**
 * Returns the nodes that need to be waited on before transmitting an event from @a trigger out of @a node.
 */
vector<VuoCompilerNode *> VuoCompilerBitcodeGenerator::getNodesToWaitOnBeforeTransmission(VuoCompilerTriggerPort *trigger, VuoCompilerNode *node)
{
	// Does the node have a scatter downstream, and another trigger overlaps with some branches of the scatter but
	// not others? If so, then all downstream nodes will be locked before the event can proceed.
	// (The analysis is an approximation. It checks if another trigger overlaps *anywhere* downstream, even if the
	// overlap is after the scatter has been fully gathered back up.)
	// This prevents deadlocks involving one trigger claiming one branch and the other trigger claiming another
	// (https://b33p.net/kosada/node/6696, https://b33p.net/kosada/node/12503, https://b33p.net/kosada/node/16202).

	bool hasGatherOverlappedByAnotherTrigger = graph->hasScatterPartiallyOverlappedByAnotherTrigger(node, trigger);

	// Wait for either all nodes downstream of the node or the nodes directly connected to the node.
	vector<VuoCompilerNode *> nodesToWaitOn =
			(hasGatherOverlappedByAnotherTrigger ?
				 graph->getNodesDownstream(node, trigger) :
				 graph->getNodesImmediatelyDownstream(node, trigger));

	return nodesToWaitOn;
}

/**
 * Sets up VuoCompilerBitcodeGenerator::orderedTypes.
 */
void VuoCompilerBitcodeGenerator::makeOrderedTypes(void)
{
	for (VuoCompilerNode *node : graph->getNodes())
	{
		vector<VuoPort *> inputPorts = node->getBase()->getInputPorts();
		vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
		vector<VuoPort *> ports;
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

		for (VuoPort *port : ports)
		{
			VuoType *dataType = static_cast<VuoCompilerPort *>(port->getCompiler())->getDataVuoType();

			if (dataType)
			{
				vector<VuoCompilerType *>::iterator typeIter = find(orderedTypes.begin(), orderedTypes.end(), dataType->getCompiler());
				if (typeIter == orderedTypes.end())
					orderedTypes.push_back(dataType->getCompiler());
			}
		}
	}
}

/**
 * Sets up the node identifier and port context index for each port in the composition.
 */
void VuoCompilerBitcodeGenerator::makePortContextInfo(void)
{
	for (VuoCompilerNode *node : graph->getNodes())
	{
		vector<VuoPort *> inputPorts = node->getBase()->getInputPorts();
		vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
		vector<VuoPort *> ports;
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
		for (size_t i = 0; i < ports.size(); ++i)
		{
			VuoCompilerPort *port = static_cast<VuoCompilerPort *>( ports[i]->getCompiler() );

			port->setNodeIdentifier( node->getIdentifier() );
			port->setIndexInPortContexts(i);
		}
	}
}

/**
 * Sets up VuoCompilerBitcodeGenerator::modelInputPorts and VuoCompilerBitcodeGenerator::modelOutputPorts.
 */
void VuoCompilerBitcodeGenerator::makeSubcompositionModelPorts(void)
{
	Module module("", getGlobalContext());

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();
	modelInputPorts.insert( modelInputPorts.end(), publishedInputPorts.begin(), publishedInputPorts.end() );
	modelOutputPorts.insert( modelOutputPorts.end(), publishedOutputPorts.begin(), publishedOutputPorts.end() );

	set<string> publishedOutputTriggerNames = graph->getPublishedOutputTriggers();

	for (size_t i = 0; i < modelOutputPorts.size(); ++i)
	{
		string portName = modelOutputPorts[i]->getClass()->getName();
		if (publishedOutputTriggerNames.find(portName) != publishedOutputTriggerNames.end())
		{
			VuoType *dataType = static_cast<VuoCompilerPortClass *>( modelOutputPorts[i]->getClass()->getCompiler() )->getDataVuoType();
			FunctionType *functionType = VuoCompilerCodeGenUtilities::getFunctionType(&module, dataType);
			PointerType *pointerToFunctionType = PointerType::get(functionType, 0);
			VuoCompilerTriggerPortClass *modelTriggerPortClass = new VuoCompilerTriggerPortClass(portName, pointerToFunctionType);
			modelTriggerPortClass->setDataVuoType(dataType);
			VuoCompilerPort *modelTriggerPort = modelTriggerPortClass->newPort();
			modelOutputPorts[i] = modelTriggerPort->getBase();
		}
	}
}

/**
 * Sets up VuoCompilerBitcodeGenerator::dependencies.
 */
void VuoCompilerBitcodeGenerator::makeDependencies(void)
{
	// Make sure all loading of dependencies for makeDependencies_Pro() happens before we get on llvmQueue.
	dependencies = compiler->getDirectDependenciesForComposition(composition);

#if VUO_PRO
	makeDependencies_Pro();
#endif

	// Make sure all subcompositions needed by generateTriggerFunctions() are loaded before we get on llvmQueue.
	compiler->getDependenciesForComposition(composition);
}

/**
 * Generates bitcode for the composition that can be linked to create a runnable composition (if isTopLevelComposition
 * is true) or a node class / subcomposition (if false).
 *
 * @return The LLVM module in which bitcode has been generated. It is owned by the VuoCompilerComposition with which
 *		this VuoCompilerBitcodeGenerator was constructed.
 */
Module * VuoCompilerBitcodeGenerator::generateBitcode(void)
{
	bool isStatefulComposition = false;
	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (node->getBase()->getNodeClass()->getCompiler()->isStateful())
		{
			isStatefulComposition = true;
			break;
		}
	}

	constantStrings.clear();
	for (VuoCompilerNode *node : orderedNodes)
		node->setConstantStringCache(&constantStrings);

	module = new Module(moduleKey, getGlobalContext());

	generateCompositionMetadata();

	generateTriggerFunctions();

	if (! isTopLevelComposition)
		generateNodeEventFunction(isStatefulComposition);

	if (isStatefulComposition)
	{
		generateNodeInstanceInitFunction();
		generateNodeInstanceFiniFunction();
		generateNodeInstanceTriggerStartFunction();
		if (! isTopLevelComposition)
			generateNodeInstanceTriggerUpdateFunction();
	}

	generateNodeInstanceTriggerStopFunction();

	generateCompositionReleasePortDataFunction();

	generateCompositionGetPortValueFunction();
	generateCompositionSetPortValueFunction();
	generateCompositionFireTriggerPortEventFunction();

	generateCompositionSetPublishedInputPortValueFunction();

	generateCompositionCreateContextForNodeFunction();
	generateCompositionAddNodeMetadataFunction();
	generateCompositionPerformDataOnlyTransmissionsFunction();

	generateCompositionWaitForNodeFunction();

	if (isTopLevelComposition)
	{
		generateShouldShowSplashWindowFunction(compiler->shouldShowSplashWindow());

		generateAllocation();
		generateSetupFunction(isStatefulComposition);
		generateCleanupFunction();

		generateInstanceInitFunction(isStatefulComposition);
		generateInstanceFiniFunction(isStatefulComposition);
		generateInstanceTriggerStartFunction(isStatefulComposition);
		generateInstanceTriggerStopFunction(isStatefulComposition);

		generateSetInputPortValueFunction();

		generateGetPublishedPortCountFunction(true);
		generateGetPublishedPortCountFunction(false);
		generateGetPublishedPortNamesFunction(true);
		generateGetPublishedPortNamesFunction(false);
		generateGetPublishedPortTypesFunction(true);
		generateGetPublishedPortTypesFunction(false);
		generateGetPublishedPortDetailsFunction(true);
		generateGetPublishedPortDetailsFunction(false);
		generateGetPublishedPortValueFunction(true);
		generateGetPublishedPortValueFunction(false);
		generateSetPublishedInputPortValueFunction();
		generateFirePublishedInputPortEventFunction();
	}

	composition->setModule(module);

	return module;
}

/**
 * Generates metadata (the equivalent of the `VuoModuleMetadata` macro) for this composition.
 */
void VuoCompilerBitcodeGenerator::generateCompositionMetadata(void)
{
	json_object *metadataJson = json_object_new_object();
	json_object *nodeMetadataJson = json_object_new_object();

	string title = composition->getBase()->getMetadata()->getCustomizedName();
	if (title.empty())
	{
		string nodeClassNamePart = VuoStringUtilities::split(moduleKey, '.').back();
		title = VuoStringUtilities::expandCamelCase(nodeClassNamePart);
	}
	json_object_object_add(metadataJson, "title", json_object_new_string(title.c_str()));

	string description = composition->getBase()->getMetadata()->getDescription();
	json_object_object_add(metadataJson, "description", json_object_new_string(description.c_str()));

	json_object *keywordsJson = json_object_new_array();
	for (const string &keyword : composition->getBase()->getMetadata()->getKeywords())
		json_object_array_add(keywordsJson, json_object_new_string(keyword.c_str()));
	json_object_object_add(metadataJson, "keywords", keywordsJson);

	string version = composition->getBase()->getMetadata()->getVersion();
	if (! version.empty())
		json_object_object_add(metadataJson, "version", json_object_new_string(version.c_str()));

	json_object *dependenciesJson = json_object_new_array();
	for (const string &dependency : dependencies)
		json_object_array_add(dependenciesJson, json_object_new_string(dependency.c_str()));
	json_object_object_add(metadataJson, "dependencies", dependenciesJson);

	if (! isTopLevelComposition)
	{
#if VUO_PRO
		generateCompositionMetadata_Pro(nodeMetadataJson);
#endif

		json_object *triggersJson = json_object_new_array();
		for (VuoCompilerTriggerPort *trigger : graph->getTriggerPorts())
		{
			VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);
			json_object *t = VuoCompilerTriggerDescription::getJson(triggerNode, trigger, graph);
			json_object_array_add(triggersJson, t);
		}
		for (VuoCompilerNode *node : graph->getNodes())
		{
			VuoCompilerNodeClass *nodeClass = node->getBase()->getNodeClass()->getCompiler();
			vector<VuoCompilerTriggerDescription *> triggersInSubcomposition = nodeClass->getTriggerDescriptions();
			for (VuoCompilerTriggerDescription *triggerInSubcomposition : triggersInSubcomposition)
			{
				json_object *t = triggerInSubcomposition->getJsonWithinSubcomposition(node);
				json_object_array_add(triggersJson, t);
			}
		}
		json_object_object_add(nodeMetadataJson, "triggers", triggersJson);

		json_object *nodesJson = json_object_new_object();
		for (VuoCompilerNode *node : orderedNodes)
		{
			json_object *nodeClassNameJson = json_object_new_string(node->getBase()->getNodeClass()->getClassName().c_str());
			json_object_object_add(nodesJson, node->getIdentifier().c_str(), nodeClassNameJson);
		}
		json_object_object_add(nodeMetadataJson, "nodes", nodesJson);
	}

	json_object_object_add(metadataJson, "node", nodeMetadataJson);

	string metadata = json_object_to_json_string_ext(metadataJson, JSON_C_TO_STRING_PLAIN);
	json_object_put(metadataJson);

	VuoCompilerCodeGenUtilities::generateModuleMetadata(module, metadata, moduleKey);
}

/**
 * Generates the `VuoShouldShowSplashWindow()` function.
 *
 * \eg{bool VuoShouldShowSplashWindow(void);}
 */
void VuoCompilerBitcodeGenerator::generateShouldShowSplashWindowFunction(bool shouldShow)
{
	IntegerType *returnType = IntegerType::get(module->getContext(), 8);
	FunctionType *functionType = FunctionType::get(returnType, false);
	Function *function = Function::Create(functionType, GlobalValue::ExternalLinkage, "vuoShouldShowSplashWindow", module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);
	Value *boolValue = ConstantInt::get(returnType, shouldShow);
	ReturnInst::Create(module->getContext(), boolValue, block);
}

/**
 * Generates the `compositionAddNodeMetadata()` function, which adds node metadata for all nodes in
 * the composition. For each subcomposition node, `compositionAddNodeMetadata()` is called recursively.
 *
 * \eg{void compositionAddNodeMetadata(VuoCompositionState *compositionState);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionAddNodeMetadataFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionAddNodeMetadataFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");

	Function *compositionCreateContextForNodeFunction = VuoCompilerCodeGenUtilities::getCompositionCreateContextForNodeFunction(module);
	Function *compositionSetPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);
	Function *compositionGetPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionGetPortValueFunction(module);
	Function *compositionFireTriggerPortEventFunction = VuoCompilerCodeGenUtilities::getCompositionFireTriggerPortEventFunction(module);
	Function *releasePortDataFunction = VuoCompilerCodeGenUtilities::getCompositionReleasePortDataFunction(module);

	for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		node->generateAddMetadata(module, block, compositionStateValue, orderedTypes, compositionCreateContextForNodeFunction,
								  compositionSetPortValueFunction, compositionGetPortValueFunction, compositionFireTriggerPortEventFunction,
								  releasePortDataFunction);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the `compositionCreateContextForNode()` function, which creates a node context and
 * port contexts for a selected node.
 *
 * \eg{NodeContext * compositionCreateContextForNode(unsigned long nodeIndex);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionCreateContextForNodeFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionCreateContextForNodeFunction(module);
	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *nodeIndexValue = args++;
	nodeIndexValue->setName("nodeIndex");

	PointerType *pointerToNodeContext = PointerType::get(VuoCompilerCodeGenUtilities::getNodeContextType(module), 0);
	AllocaInst *nodeContextVariable = new AllocaInst(pointerToNodeContext, "nodeContext", initialBlock);

	vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex;

	for (size_t i = 0; i < orderedNodes.size(); ++i)
	{
		VuoCompilerNode *node = orderedNodes[i];

		BasicBlock *block = BasicBlock::Create(module->getContext(), node->getIdentifier(), function, NULL);
		Value *nodeContextValue = node->generateCreateContext(module, block);
		new StoreInst(nodeContextValue, nodeContextVariable, block);

		blocksForIndex.push_back(make_pair(block, block));
	}

	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, initialBlock, finalBlock, nodeIndexValue, blocksForIndex);

	Value *nodeContextValue = new LoadInst(nodeContextVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), nodeContextValue, finalBlock);
}

/**
 * Generates the `compositionPerformDataOnlyTransmissions()` function, which initializes each input and output
 * data-and-event port that can receive data-only transmissions. The function is called recursively for subcompositions.
 *
 * The function assumes that all other input ports' values have already been initialized.
 *
 * \eg{void compositionPerformDataOnlyTransmissions(VuoCompositionState *compositionState);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionPerformDataOnlyTransmissionsFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionPerformDataOnlyTransmissionsFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");

	// Copy data along cables supporting data-only transmission at this level of the composition.

	for (VuoCompilerNode *node : graph->getSourceNodesOfDataOnlyTransmission())
		generateDataOnlyTransmissionFromNode(function, block, compositionStateValue, node, false, false, false);

	// For each subcomposition node…

	for (VuoCompilerNode *node : orderedNodes)
	{
		Function *subcompositionFunctionSrc = node->getBase()->getNodeClass()->getCompiler()->getCompositionPerformDataOnlyTransmissionsFunction();
		if (subcompositionFunctionSrc)
		{
			Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateRuntimeState(module, block, compositionStateValue);
			Value *compositionIdentifierValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateCompositionIdentifier(module, block, compositionStateValue);
			Value *subcompositionIdentifierValue = node->generateSubcompositionIdentifierValue(module, block, compositionIdentifierValue);
			Value *subcompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, runtimeStateValue, subcompositionIdentifierValue);
			Value *subcompositionStateValueDst = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(subcompositionStateValue, subcompositionFunctionSrc, 0, nullptr, module, block);

			// Copy the subcomposition node's input port values to the subcomposition's published input node's input ports.

			Function *setPublishedInputPortValueFunctionSrc = node->getBase()->getNodeClass()->getCompiler()->getCompositionSetPublishedInputPortValueFunction();
			Function *setPublishedInputPortValueFunctionDst = VuoCompilerModule::declareFunctionInModule(module, setPublishedInputPortValueFunctionSrc);
			Value *falseValue = ConstantInt::get(setPublishedInputPortValueFunctionDst->getFunctionType()->getParamType(3), 0);

			for (VuoPort *inputPort : node->getBase()->getInputPorts())
			{
				VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>(inputPort->getCompiler());
				VuoCompilerInputData *inputData = inputEventPort->getData();
				if (inputData)
				{
					Value *inputPortNameValue = constantStrings.get(module, inputPort->getClass()->getName());
					Value *dataValue = constantStrings.get(module, inputData->getInitialValue());

					vector<Value *> args;
					args.push_back(subcompositionStateValueDst);
					args.push_back(inputPortNameValue);
					args.push_back(dataValue);
					args.push_back(falseValue);
					CallInst::Create(setPublishedInputPortValueFunctionDst, args, "", block);
				}
			}

			// Call recursively for the subcomposition.

			Function *subcompositionFunctionDst = VuoCompilerModule::declareFunctionInModule(module, subcompositionFunctionSrc);
			CallInst::Create(subcompositionFunctionDst, subcompositionStateValueDst, "", block);

			VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, subcompositionStateValue);
			VuoCompilerCodeGenUtilities::generateFreeCall(module, block, subcompositionIdentifierValue);
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the `compositionReleasePortData()` function.
 *
 * \eg{void compositionReleasePortData(void *portData, unsigned long typeIndex);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionReleasePortDataFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionReleasePortDataFunction(module);
	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *portAddressAsVoidPointer = args++;
	portAddressAsVoidPointer->setName("portData");
	Value *typeIndexValue = args++;
	typeIndexValue->setName("typeIndex");

	vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex;

	for (vector<VuoCompilerType *>::iterator i = orderedTypes.begin(); i != orderedTypes.end(); ++i)
	{
		VuoCompilerType *type = *i;

		BasicBlock *block = BasicBlock::Create(module->getContext(), type->getBase()->getModuleKey(), function, NULL);
		Value *portAddress = new BitCastInst(portAddressAsVoidPointer, PointerType::get(type->getType(), 0), "", block);
		Value *portValue = new LoadInst(portAddress, "", false, block);
		type->generateReleaseCall(module, block, portValue);

		blocksForIndex.push_back(make_pair(block, block));
	}

	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, initialBlock, finalBlock, typeIndexValue, blocksForIndex);

	ReturnInst::Create(module->getContext(), finalBlock);
}

/**
 * Generates code to set the composition's published input port states (data and event) from the corresponding arguments in a node function.
 */
void VuoCompilerBitcodeGenerator::generateSetInputDataFromNodeFunctionArguments(Function *function, BasicBlock *&block, Value *compositionStateValue,
																				map<VuoPort *, size_t> indexOfParameter,
																				map<VuoPort *, size_t> indexOfEventParameter,
																				bool shouldWaitForDataOnlyDownstreamNodes,
																				bool shouldUpdateTriggers, bool shouldSendTelemetry)
{
	VuoCompilerNode *publishedInputNode = graph->getPublishedInputNode();
	if (! publishedInputNode)
		return;

	Value *publishedNodeContextValue = publishedInputNode->generateGetContext(module, block, compositionStateValue);

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoCompilerInputEventPort *> inputEventPorts;

	// Copy data from the function arguments to the published input node's input ports.

	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		VuoPublishedPort *publishedInputPort = publishedInputPorts[i];

		VuoPort *inputPort = graph->getInputPortOnPublishedInputNode(i);
		VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>( inputPort->getCompiler() );
		inputEventPorts.push_back(inputEventPort);

		if (publishedInputPort->getClass()->getPortType() == VuoPortClass::dataAndEventPort)
		{
			size_t dataArgIndex = indexOfParameter[ modelInputPorts[i] ];
			VuoCompilerType *type = static_cast<VuoCompilerPort *>( publishedInputPort->getCompiler() )->getDataVuoType()->getCompiler();

			// If the argument is a struct that would normally be passed "byval", it's not "byval" here.
			// Instead, the Vuo compiler has implemented the "byval" semantics in the caller, which
			// has passed a struct pointer that is effectively passed by value but not marked as such.
			//
			// This is a workaround for a bug where LLVM would sometimes give the node function body
			// an invalid value for a "byval" struct argument. https://b33p.net/kosada/node/11386
			Value *dataArg;
			if (type->getType()->isStructTy() &&
					type->getFunctionParameterAttributes().hasAttrSomewhere(Attribute::ByVal))
			{
				Value *argAsPointerToOtherType = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, dataArgIndex);
				Value *argAsPointer = new BitCastInst(argAsPointerToOtherType, type->getType()->getPointerTo(), "", block);
				dataArg = new LoadInst(argAsPointer, "", block);
			}
			else
			{
				dataArg = VuoCompilerCodeGenUtilities::unlowerArgument(type, function, dataArgIndex, module, block);
			}

			inputEventPort->generateReplaceData(module, block, publishedNodeContextValue, dataArg);
		}
	}

	// Transmit data through the published input node and onward through the published input cables.
	// (This temporarily sets an event on all of the published input node's input ports.)

	generateDataOnlyTransmissionFromNode(function, block, compositionStateValue, publishedInputNode,
										 shouldWaitForDataOnlyDownstreamNodes, shouldUpdateTriggers, shouldSendTelemetry);

	// Copy events from the function arguments to the published input node's input ports.

	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		VuoCompilerInputEventPort *inputEventPort = inputEventPorts[i];

		map<VuoPort *, size_t> *indexMap = (inputEventPort->getBase()->getClass()->getPortType() == VuoPortClass::dataAndEventPort ?
												&indexOfEventParameter :
												&indexOfParameter);

		auto foundIndex = indexMap->find( modelInputPorts[i] );
		if (foundIndex != indexMap->end())
		{
			size_t eventArgIndex = foundIndex->second;
			Value *eventArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, eventArgIndex);

			inputEventPort->generateStoreEvent(module, block, publishedNodeContextValue, eventArg);
		}
	}
}

/**
 * Generates the @c nodeEvent() or @c nodeInstanceEvent() function, which handles each event into the
 * composition's published input ports.
 *
 * \eg{void nodeEvent( VuoCompositionState *compositionState, ...);}
 *
 * \eg{void nodeInstanceEvent( VuoCompositionState *compositionState, VuoInstanceData(InstanceDataType *) instanceData, ... );}
 */
void VuoCompilerBitcodeGenerator::generateNodeEventFunction(bool isStatefulComposition)
{
	vector<VuoPort *> modelPorts;
	modelPorts.insert(modelPorts.end(), modelInputPorts.begin(), modelInputPorts.end());
	modelPorts.insert(modelPorts.end(), modelOutputPorts.begin(), modelOutputPorts.end());

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();
	vector<VuoPublishedPort *> publishedPorts;
	publishedPorts.insert(publishedPorts.end(), publishedInputPorts.begin(), publishedInputPorts.end());
	publishedPorts.insert(publishedPorts.end(), publishedOutputPorts.begin(), publishedOutputPorts.end());

	map<VuoPort *, string> displayNamesForPorts;
	map<VuoPort *, json_object *> detailsForPorts;
	for (size_t i = 0; i < modelPorts.size(); ++i)
	{
		VuoPort *modelPort = modelPorts[i];
		VuoPublishedPort *publishedPort = publishedPorts[i];

		string portName = modelPort->getClass()->getName();
		bool isAllCaps = true;
		for (size_t j = 0; j < portName.length(); ++j)
			if (! isupper(portName[j]))
			{
				isAllCaps = false;
				break;
			}
		if (isAllCaps)
			displayNamesForPorts[modelPort] = portName;

		detailsForPorts[modelPort] = static_cast<VuoCompilerPortClass *>( publishedPort->getClass()->getCompiler() )->getDetails();
	}

	map<VuoPort *, string> defaultValuesForInputPorts;
	map<VuoPort *, VuoPortClass::EventBlocking> eventBlockingForInputPorts;
	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		VuoPublishedPort *publishedInputPort = publishedInputPorts[i];

		string defaultValue = static_cast<VuoCompilerPublishedPort *>( publishedInputPort->getCompiler() )->getInitialValue();
		if (! defaultValue.empty())
			defaultValuesForInputPorts[publishedInputPort] = defaultValue;

		eventBlockingForInputPorts[publishedInputPort] = graph->getPublishedInputEventBlocking(i);
	}

	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, moduleKey, true, isStatefulComposition,
																		   VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																		   modelInputPorts, modelOutputPorts,
																		   detailsForPorts, displayNamesForPorts,
																		   defaultValuesForInputPorts, eventBlockingForInputPorts,
																		   indexOfParameter, indexOfEventParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	VuoCompilerTriggerPort *trigger = graph->getPublishedInputTrigger();
	if (! trigger)
	{
		ReturnInst::Create(module->getContext(), block);
		return;
	}

	VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);

	Value *compositionStateValue = function->arg_begin();
	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, block, compositionStateValue);

	// Get the event ID passed down from the composition containing this subcomposition node.

	Value *eventIdValue = VuoCompilerCodeGenUtilities::generateGetOneExecutingEvent(module, block, compositionContextValue);

	// Claim all necessary downstream nodes.

	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
	generateWaitForNodes(module, function, block, compositionStateValue, triggerWaitNodes, eventIdValue);

	// Set the data and event for each input port on the published input node from the input arguments.

	bool hasClaimedNodesDownstreamOfPublishedInputNode = (triggerWaitNodes.size() > 2);
	generateSetInputDataFromNodeFunctionArguments(function, block, compositionStateValue, indexOfParameter, indexOfEventParameter,
												  ! hasClaimedNodesDownstreamOfPublishedInputNode, true, true);

	// Wait for the event to reach the published output node (if any) and all other leaf nodes — part 1.

	Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, block, compositionContextValue);
	VuoCompilerCodeGenUtilities::generateEnterDispatchGroup(module, block, subcompositionOutputGroupValue);

	// Fire an event from the published input trigger.

	Value *triggerNodeContextValue = triggerNode->generateGetContext(module, block, compositionStateValue);
	Value *triggerFunctionValue = trigger->generateLoadFunction(module, block, triggerNodeContextValue);
	CallInst::Create(triggerFunctionValue, "", block);

	// Wait for the event to reach the published output node (if any) and all other leaf nodes — part 2.

	VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, subcompositionOutputGroupValue);

	// Set each output argument from the published output port values.

	VuoCompilerNode *publishedOutputNode = graph->getPublishedOutputNode();
	Value *publishedOutputNodeContext = publishedOutputNode->generateGetContext(module, block, compositionStateValue);
	for (size_t i = 0; i < modelOutputPorts.size(); ++i)
	{
		VuoPort *modelOutputPort = modelOutputPorts[i];
		VuoPortClass::PortType portType = modelOutputPort->getClass()->getPortType();

		if (portType == VuoPortClass::dataAndEventPort || portType == VuoPortClass::eventOnlyPort)
		{
			VuoPort *inputPort = graph->getInputPortOnPublishedOutputNode(i);
			bool hasEventParameter = false;
			size_t eventIndex = 0;

			if (portType == VuoPortClass::dataAndEventPort)
			{
				size_t index = indexOfParameter[ modelOutputPort ];
				Value *outputArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, index);

				Value *value = static_cast<VuoCompilerEventPort *>( inputPort->getCompiler() )->generateLoadData(module, block, publishedOutputNodeContext);
				new StoreInst(value, outputArg, block);

				map<VuoPort *, size_t>::iterator iter = indexOfEventParameter.find(modelOutputPort);
				if (iter != indexOfEventParameter.end())
				{
					hasEventParameter = true;
					eventIndex = iter->second;
				}
			}
			else
			{
				hasEventParameter = true;
				eventIndex = indexOfParameter[ modelOutputPort ];
			}

			if (hasEventParameter)
			{
				Value *outputArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, eventIndex);

				Value *eventValue = VuoCompilerCodeGenUtilities::generateGetNodeContextOutputEvent(module, block, compositionContextValue, i);
				new StoreInst(eventValue, outputArg, block);
			}
		}
	}

	// Signal the published output node.

	generateSignalForNodes(module, block, compositionStateValue, vector<VuoCompilerNode *>(1, publishedOutputNode));

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceInit() function, which calls the corresponding function for all stateful nodes.
 *
 * \eg{InstanceDataType * nodeInstanceInit( VuoCompositionState *compositionState, ... );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceInitFunction(void)
{
	vector<VuoPort *> inputPorts;
	if (! isTopLevelComposition)
		inputPorts = modelInputPorts;

	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(module, moduleKey, true,
																				  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																				  inputPorts, indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionStateValue = function->arg_begin();

	if (! isTopLevelComposition)
		generateSetInputDataFromNodeFunctionArguments(function, block, compositionStateValue, indexOfParameter, map<VuoPort *, size_t>(),
													  false, false, false);

	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (node->getBase()->getNodeClass()->getCompiler()->isStateful())
		{
			BasicBlock *initBlock = NULL;
			BasicBlock *nextBlock = NULL;
			Value *replacementJsonValue = NULL;
			VuoCompilerCodeGenUtilities::generateIsNodeBeingAddedOrReplacedCheck(module, function, node->getIdentifier(),
																				 compositionStateValue, block, initBlock, nextBlock,
																				 constantStrings, replacementJsonValue);

			node->generateInitFunctionCall(module, initBlock, compositionStateValue);

			VuoCompilerCodeGenUtilities::generateJsonObjectPut(module, initBlock, replacementJsonValue);

			BranchInst::Create(nextBlock, initBlock);
			block = nextBlock;
		}
	}

	PointerType *instanceDataType = static_cast<PointerType *>( function->getReturnType() );
	ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);
	ReturnInst::Create(module->getContext(), nullInstanceDataValue, block);
}

/**
 * Generates the @c nodeInstanceFini() function, which calls the corresponding function for all stateful nodes,
 * except nodes being carried across a live-coding reload.
 *
 * \eg{void nodeInstanceFini( VuoCompositionState *compositionState, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceFiniFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(module, moduleKey,
																				  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																				  constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionStateValue = function->arg_begin();

	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (node->getBase()->getNodeClass()->getCompiler()->isStateful())
		{
			BasicBlock *finiBlock = NULL;
			BasicBlock *nextBlock = NULL;
			Value *replacementJsonValue = NULL;
			VuoCompilerCodeGenUtilities::generateIsNodeBeingRemovedOrReplacedCheck(module, function, node->getIdentifier(),
																				   compositionStateValue, block, finiBlock, nextBlock,
																				   constantStrings, replacementJsonValue);

			node->generateFiniFunctionCall(module, finiBlock, compositionStateValue);

			VuoCompilerCodeGenUtilities::generateJsonObjectPut(module, finiBlock, replacementJsonValue);

			BranchInst::Create(nextBlock, finiBlock);
			block = nextBlock;
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerStart() function, which calls the corresponding function for all stateful nodes.
 *
 * Assumes the trigger function for each node's trigger port has been generated and associated with the port.
 *
 * \eg{void nodeInstanceTriggerStart( VuoCompositionState *compositionState, VuoInstanceData(InstanceDataType *) instanceData, ... );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerStartFunction(void)
{
	vector<VuoPort *> inputPorts;
	if (! isTopLevelComposition)
		inputPorts = modelInputPorts;

	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStartFunction(module, moduleKey,
																						  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																						  inputPorts, indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionStateValue = function->arg_begin();

	if (! isTopLevelComposition)
		generateSetInputDataFromNodeFunctionArguments(function, block, compositionStateValue, indexOfParameter, map<VuoPort *, size_t>(),
													  false, false, false);

	// Since a node's nodeInstanceTriggerStart() function can generate an event,
	// make sure trigger functions wait until all nodes' init functions have completed.
	generateWaitForNodes(module, function, block, compositionStateValue, orderedNodes);

	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (node->getBase()->getNodeClass()->getCompiler()->isStateful())
		{
			// { /* call nodeInstanceTriggerStart() for node */ }
			node->generateCallbackStartFunctionCall(module, block, compositionStateValue);
		}
	}

	generateSignalForNodes(module, block, compositionStateValue, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerStop() function, which calls the corresponding function for all stateful nodes.
 *
 * \eg{void nodeInstanceTriggerStop( VuoCompositionState *compositionState, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerStopFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStopFunction(module, moduleKey,
																						 VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																						 constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionStateValue = function->arg_begin();

	// Stop all triggers from firing events — call nodeInstanceTriggerStop() for each stateful node.
	generateWaitForNodes(module, function, block, compositionStateValue, orderedNodes);
	for (VuoCompilerNode *node : graph->getNodes())
	{
		if (node->getBase()->getNodeClass()->getCompiler()->isStateful())
			node->generateCallbackStopFunctionCall(module, block, compositionStateValue);
	}
	generateSignalForNodes(module, block, compositionStateValue, orderedNodes);

	if (isTopLevelComposition)
	{
		// Wait for any scheduled trigger workers to launch events into the composition.
		Value *triggerWorkersScheduledValue = VuoCompilerCodeGenUtilities::getTriggerWorkersScheduledValue(module, block, compositionStateValue);
		VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, triggerWorkersScheduledValue);

		// Wait for any in-progress events to travel through the composition.
		generateWaitForNodes(module, function, block, compositionStateValue, orderedNodes);
		generateSignalForNodes(module, block, compositionStateValue, orderedNodes);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerUpdate() function, which calls the corresponding function in all stateful nodes.
 *
 * \eg{void nodeInstanceTriggerUpdate( VuoCompositionState *compositionState, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerUpdateFunction(void)
{
	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerUpdateFunction(module, moduleKey,
																						   VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																						   modelInputPorts, indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionStateValue = function->arg_begin();

	VuoCompilerTriggerPort *trigger = graph->getPublishedInputTrigger();
	if (! trigger)
	{
		ReturnInst::Create(module->getContext(), block);
		return;
	}

	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
	generateWaitForNodes(module, function, block, compositionStateValue, triggerWaitNodes);

	bool hasClaimedNodesDownstreamOfPublishedInputNode = (triggerWaitNodes.size() > 2);
	generateSetInputDataFromNodeFunctionArguments(function, block, compositionStateValue, indexOfParameter, map<VuoPort *, size_t>(),
												  ! hasClaimedNodesDownstreamOfPublishedInputNode, true, false);

	generateSignalForNodes(module, block, compositionStateValue, triggerWaitNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to wait on the semaphore of each of the given nodes.
 *
 * If @a shouldBlock is true, the generated code waits indefinitely for each semaphore.
 * Otherwise, if any of the semaphores is not immediately available, the generated code signals any
 * semaphores claimed so far and returns.
 *
 * If @a shouldBlock is false, returns a value that is true if all semaphore were successfully claimed
 * and false if none were claimed. Otherwise, returns null.
 *
 * @eg{
 * // shouldBlock=true
 *
 * compositionWaitForNode(compositionState, 2, eventId, true);  // orderedNodes[2]
 * compositionWaitForNode(compositionState, 4, eventId, true);  // orderedNodes[4]
 * compositionWaitForNode(compositionState, 5, eventId, true);  // orderedNodes[5]
 * }
 *
 * @eg{
 * // shouldBlock=false
 *
 * bool keepTrying;
 *
 * keepTrying = compositionWaitForNode(compositionState, 2, eventId, false);  // orderedNodes[2]
 * if (! keepTrying)
 *    goto SIGNAL0;
 *
 * keepTrying = compositionWaitForNode(compositionState, 4, eventId, false);  // orderedNodes[4]
 * if (! keepTrying)
 *    goto SIGNAL1;
 *
 * keepTrying = compositionWaitForNode(compositionState, 5, eventId, false);  // orderedNodes[5]
 * if (! keepTrying)
 *    goto SIGNAL2:
 *
 * goto SIGNAL0;
 *
 * SIGNAL2:
 * dispatch_semaphore_signal(...semaphore for orderedNodes[4]...);
 * SIGNAL1:
 * dispatch_semaphore_signal(...semaphore for orderedNodes[2]...);
 * SIGNAL0:
 * }
 */
Value * VuoCompilerBitcodeGenerator::generateWaitForNodes(Module *module, Function *function, BasicBlock *&block,
														  Value *compositionStateValue, vector<VuoCompilerNode *> nodes,
														  Value *eventIdValue, bool shouldBlock)
{
	sortNodes(nodes);

	if (! eventIdValue)
		eventIdValue = VuoCompilerCodeGenUtilities::generateGetNextEventId(module, block, compositionStateValue);

	Function *waitForNodeFunction = VuoCompilerCodeGenUtilities::getWaitForNodeFunction(module, moduleKey);
	Type *unsignedLongType = waitForNodeFunction->getFunctionType()->getParamType(1);
	Type *boolType = waitForNodeFunction->getFunctionType()->getParamType(3);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	Constant *trueValue = ConstantInt::get(boolType, 1);

	if (shouldBlock)
	{
		vector<VuoCompilerNode *>::iterator prevNodeIter = orderedNodes.begin();

		for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoCompilerNode *node = *i;

			vector<VuoCompilerNode *>::iterator orderedNodeIter = find(prevNodeIter, orderedNodes.end(), node);
			prevNodeIter = orderedNodeIter;
			size_t orderedNodeIndex = orderedNodeIter - orderedNodes.begin();
			Constant *orderedNodeIndexValue = ConstantInt::get(unsignedLongType, orderedNodeIndex);

			vector<Value *> args;
			args.push_back(compositionStateValue);
			args.push_back(orderedNodeIndexValue);
			args.push_back(eventIdValue);
			args.push_back(trueValue);
			CallInst::Create(waitForNodeFunction, args, "", block);
		}

		return NULL;
	}
	else
	{
		AllocaInst *keepTryingVariable = new AllocaInst(IntegerType::get(module->getContext(), 1), "keepTrying", block);
		new StoreInst(trueValue, keepTryingVariable, block);

		vector<VuoCompilerNode *>::iterator prevNodeIter = orderedNodes.begin();

		vector<BasicBlock *> signalBlocks;
		for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoCompilerNode *node = *i;

			BasicBlock *waitBlock = block;

			vector<VuoCompilerNode *>::iterator orderedNodeIter = find(prevNodeIter, orderedNodes.end(), node);
			prevNodeIter = orderedNodeIter;
			size_t orderedNodeIndex = orderedNodeIter - orderedNodes.begin();
			Value *orderedNodeIndexValue = ConstantInt::get(unsignedLongType, orderedNodeIndex);

			vector<Value *> args;
			args.push_back(compositionStateValue);
			args.push_back(orderedNodeIndexValue);
			args.push_back(eventIdValue);
			args.push_back(falseValue);
			CallInst *keepTryingValue = CallInst::Create(waitForNodeFunction, args, "", waitBlock);
			new StoreInst(keepTryingValue, keepTryingVariable, waitBlock);

			ICmpInst *keepTryingIsFalse = new ICmpInst(*waitBlock, ICmpInst::ICMP_EQ, keepTryingValue, falseValue);
			BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "signal", function);
			BasicBlock *nextNodeBlock = BasicBlock::Create(module->getContext(), "wait", function);
			BranchInst::Create(signalBlock, nextNodeBlock, keepTryingIsFalse, waitBlock);

			signalBlocks.push_back(signalBlock);
			block = nextNodeBlock;
		}

		if (! signalBlocks.empty())
		{
			BranchInst::Create(signalBlocks[0], block);
			block = signalBlocks[0];
		}

		for (size_t i = 1; i < signalBlocks.size(); ++i)
		{
			BasicBlock *signalBlock = signalBlocks[i];
			VuoCompilerNode *nodeToSignal = nodes[i-1];

			generateSignalForNodes(module, signalBlock, compositionStateValue, vector<VuoCompilerNode *>(1, nodeToSignal));

			BranchInst::Create(signalBlocks[i-1], signalBlock);
		}

		Value *keepTryingValue = new LoadInst(keepTryingVariable, "", false, block);

		return keepTryingValue;
	}
}


/**
 * Generates the @c compositionWaitForNode() function, which does a non-blocking wait on the semaphore of the given node.
 *
 * @eg{
 * // When an event reaches a node through just one edge into the node, this code is called just once.
 * // When an event reaches a node through multiple edges (i.e., the node is at the hub of a feedback loop
 * // or at a gather), this code is called for each of those edges. When one of those edges claims the
 * // semaphore, the rest of the edges for the same event need to recognize this and also stop waiting.
 * // Hence the checking of event IDs and the limited-time wait.
 *
 * bool compositionWaitForNode(VuoCompositionState *compositionState, unsigned long indexInOrderedNodes, unsigned long eventId, bool shouldBlock)
 * {
 *    bool keepTrying = true;
 *
 *    int64_t timeoutDelta = (shouldBlock ? ... : 0);
 *    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeoutDelta);
 *
 *    NodeContext *nodeContext = vuoGetNodeContext(compositionState, indexInOrderedNodes);
 *
 *    while (nodeContext->claimingEventId != eventId && keepTrying)
 *    {
 *       int ret = dispatch_semaphore_wait(nodeContext->semaphore, timeout);
 *       if (ret == 0)
 *          nodeContext->claimingEventId = eventId;
 *       else if (! shouldBlock)
 *          keepTrying = false;
 *    }
 *
 *    return keepTrying;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateCompositionWaitForNodeFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getWaitForNodeFunction(module, moduleKey);
	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *indexInOrderedNodesValue = args++;
	indexInOrderedNodesValue->setName("indexInOrderedNodes");
	Value *eventIdValue = args++;
	eventIdValue->setName("eventId");
	Value *shouldBlockValue = args++;
	shouldBlockValue->setName("shouldBlock");


	// bool keepTrying = true;

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	AllocaInst *keepTryingVariable = new AllocaInst(IntegerType::get(module->getContext(), 1), "keepTrying", initialBlock);
	ConstantInt *trueValue = ConstantInt::get(module->getContext(), APInt(1, 1));
	new StoreInst(trueValue, keepTryingVariable, initialBlock);


	// int64_t timeoutDelta = (shouldBlock ? ... : 0);
	// dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeoutDelta);

	Type *timeoutDeltaType = IntegerType::get(module->getContext(), 64);
	AllocaInst *timeoutDeltaVariable = new AllocaInst(timeoutDeltaType, "timeoutDelta", initialBlock);
	ICmpInst *shouldBlockIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_EQ, shouldBlockValue, trueValue);
	BasicBlock *nonZeroTimeoutBlock = BasicBlock::Create(module->getContext(), "nonZeroTimeout", function);
	BasicBlock *zeroTimeoutBlock = BasicBlock::Create(module->getContext(), "zeroTimeout", function);
	BranchInst::Create(nonZeroTimeoutBlock, zeroTimeoutBlock, shouldBlockIsTrue, initialBlock);

	BasicBlock *checkEventIdBlock = BasicBlock::Create(module->getContext(), "checkEventId", function);

	ConstantInt *nonZeroTimeoutValue = ConstantInt::get(module->getContext(), APInt(64, NSEC_PER_SEC / 1000));  /// @todo (https://b33p.net/kosada/node/6682)
	new StoreInst(nonZeroTimeoutValue, timeoutDeltaVariable, false, nonZeroTimeoutBlock);
	BranchInst::Create(checkEventIdBlock, nonZeroTimeoutBlock);

	ConstantInt *zeroTimeoutValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	new StoreInst(zeroTimeoutValue, timeoutDeltaVariable, false, zeroTimeoutBlock);
	BranchInst::Create(checkEventIdBlock, zeroTimeoutBlock);

	Value *timeoutDeltaValue = new LoadInst(timeoutDeltaVariable, "", false, checkEventIdBlock);
	Value *timeoutValue = VuoCompilerCodeGenUtilities::generateCreateDispatchTime(module, checkEventIdBlock, timeoutDeltaValue);


	// NodeContext *nodeContext = vuoGetNodeContext(compositionState, indexInOrderedNodes);

	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, checkEventIdBlock,
																				  compositionStateValue, indexInOrderedNodesValue);


	// while (nodeContext->claimingEventId != eventId && keepTrying)
	// {
	//    int ret = dispatch_semaphore_wait(nodeContext->semaphore, timeout);
	//    if (ret == 0)
	//       nodeContext->claimingEventId = eventId;
	//    else if (! shouldBlock)
	//       keepTrying = false;
	// }

	Value *claimingEventIdValue = VuoCompilerCodeGenUtilities::generateGetNodeContextClaimingEventId(module, checkEventIdBlock, nodeContextValue);
	ICmpInst *claimingEventIdNotEqualsEventId = new ICmpInst(*checkEventIdBlock, ICmpInst::ICMP_NE, claimingEventIdValue, eventIdValue, "");
	BasicBlock *checkKeepTryingBlock = BasicBlock::Create(module->getContext(), "checkKeepTrying", function);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "doneWaitingNodeSemaphore", function);
	BranchInst::Create(checkKeepTryingBlock, finalBlock, claimingEventIdNotEqualsEventId, checkEventIdBlock);

	Value *keepTryingValue = new LoadInst(keepTryingVariable, "", false, checkKeepTryingBlock);
	ICmpInst *keepTryingIsTrue = new ICmpInst(*checkKeepTryingBlock, ICmpInst::ICMP_EQ, keepTryingValue, trueValue);
	BasicBlock *waitBlock = BasicBlock::Create(module->getContext(), "waitNodeSemaphore", function);
	BranchInst::Create(waitBlock, finalBlock, keepTryingIsTrue, checkKeepTryingBlock);

	Value *semaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeContextSemaphore(module, waitBlock, nodeContextValue);
	Value *retValue = VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, waitBlock, semaphoreValue, timeoutValue);

	Value *zeroValue = ConstantInt::get(retValue->getType(), 0);
	ICmpInst *retEqualsZero = new ICmpInst(*waitBlock, ICmpInst::ICMP_EQ, retValue, zeroValue, "");
	BasicBlock *setEventIdBlock = BasicBlock::Create(module->getContext(), "setEventId", function);
	BasicBlock *checkShouldBlockBlock = BasicBlock::Create(module->getContext(), "checkShouldBlock", function);
	BasicBlock *endWhileBlock = BasicBlock::Create(module->getContext(), "endWhile", function);
	BranchInst::Create(setEventIdBlock, checkShouldBlockBlock, retEqualsZero, waitBlock);

	VuoCompilerCodeGenUtilities::generateSetNodeContextClaimingEventId(module, setEventIdBlock, nodeContextValue, eventIdValue);
	BranchInst::Create(endWhileBlock, setEventIdBlock);

	BasicBlock *setKeepTryingBlock = BasicBlock::Create(module->getContext(), "setKeepTrying", function);
	BranchInst::Create(endWhileBlock, setKeepTryingBlock, shouldBlockIsTrue, checkShouldBlockBlock);

	Value *falseValue = ConstantInt::get(keepTryingValue->getType(), 0);
	new StoreInst(falseValue, keepTryingVariable, setKeepTryingBlock);
	BranchInst::Create(endWhileBlock, setKeepTryingBlock);

	BranchInst::Create(checkEventIdBlock, endWhileBlock);


	// return keepTrying;

	keepTryingValue = new LoadInst(keepTryingVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), keepTryingValue, finalBlock);
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
void VuoCompilerBitcodeGenerator::generateSignalForNodes(Module *module, BasicBlock *block, Value *compositionStateValue,
														 vector<VuoCompilerNode *> nodes)
{
	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		Value *nodeContextValue = node->generateGetContext(module, block, compositionStateValue);

		ConstantInt *noEventIdConstant = VuoCompilerCodeGenUtilities::generateNoEventIdConstant(module);
		VuoCompilerCodeGenUtilities::generateSetNodeContextClaimingEventId(module, block, nodeContextValue, noEventIdConstant);

		Value *semaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeContextSemaphore(module, block, nodeContextValue);
		VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, block, semaphoreValue);
	}
}

/**
 * Generates the `compositionGetPortValue()` function, which returns a string representation
 * (summary or full serialization) of the value of a data-and-event port.
 *
 * When called with the `isThreadSafe` argument, this function synchronizes against other
 * accesses of the node's port values. Otherwise, this function makes synchronization the
 * responsibility of the caller.
 *
 * The caller of this function is responsible for freeing the return value.
 *
 * Assumes the semaphore for each node has been initialized.
 *
 * Example:
 *
 * @eg{
 * // serializationType=0 : getSummary
 * // serializationType=1 : getString
 * // serializationType=2 : getInterprocessString
 *
 * char * compositionGetPortValue(VuoCompositionState *compositionState, const char *portIdentifier, int serializationType, bool isThreadSafe)
 * {
 *	 char *ret = NULL;
 *
 *   void *portAddress = vuoGetDataForPort(compositionState, portIdentifier);
 *
 *   if (portAddress != NULL)
 *   {
 *     dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionState, portIdentifier);
 *     unsigned long typeIndex = vuoGetTypeIndexForPort(compositionState, portIdentifier);
 *
 *     if (isThreadSafe)
 *       dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);
 *
 *     if (typeIndex == 0)
 *     {
 *       VuoReal portValue = (VuoReal)(*portAddress);
 *       if (serializationType == 0)
 *         ret = VuoReal_getSummary(portValue);
 *       else
 *         ret = VuoReal_getString(portValue);
 *     }
 *     else if (typeIndex == 1)
 *     {
 *       VuoImage portValue = (VuoImage)(*portAddress);
 *       if (serializationType == 0)
 *         ret = VuoImage_getSummary(portValue);
 *       else if (serializationType == 1)
 *         ret = VuoImage_getString(portValue);
 *       else
 *         ret = VuoImage_getInterprocessString(portValue);
 *     }
 *     else if ...
 *
 *     if (isThreadSafe)
 *       dispatch_semaphore_signal(nodeSemaphore);
 *   }
 *
 *   return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateCompositionGetPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionGetPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *serializationTypeValue = args++;
	serializationTypeValue->setName("serializationType");
	Value *isThreadSafeValue = args++;
	isThreadSafeValue->setName("isThreadSafe");


	// char *ret = NULL;

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	AllocaInst *retVariable = new AllocaInst(pointerToCharType, "ret", initialBlock);
	ConstantPointerNull *nullPointerToChar = ConstantPointerNull::get(pointerToCharType);
	new StoreInst(nullPointerToChar, retVariable, false, initialBlock);

	// void *portAddress = vuoGetDataForPort(compositionState, portIdentifier);

	Value *portAddressAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetDataForPort(module, initialBlock, compositionStateValue, portIdentifierValue);

	// if (portAddress != NULL)

	BasicBlock *checkWaitBlock = BasicBlock::Create(module->getContext(), "checkWait", function, 0);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	ConstantPointerNull *nullPortAddress = ConstantPointerNull::get(static_cast<PointerType *>(portAddressAsVoidPointer->getType()));
	ICmpInst *portAddressNotEqualsNull = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, portAddressAsVoidPointer, nullPortAddress, "");
	BranchInst::Create(checkWaitBlock, finalBlock, portAddressNotEqualsNull, initialBlock);

	// dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionState, portIdentifier);
	// unsigned long typeIndex = vuoGetTypeIndexForPort(compositionState, portIdentifier);

	Value *nodeSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeSemaphoreForPort(module, checkWaitBlock, compositionStateValue, portIdentifierValue);
	Value *typeIndexValue = VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(module, checkWaitBlock, compositionStateValue, portIdentifierValue);

	// if (isThreadSafe)
	//   dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);

	BasicBlock *waitBlock = BasicBlock::Create(module->getContext(), "wait", function, 0);
	BasicBlock *checkTypeIndexBlock = BasicBlock::Create(module->getContext(), "checkTypeIndex", function, 0);
	ConstantInt *falseValue = ConstantInt::get(static_cast<IntegerType *>(isThreadSafeValue->getType()), 0);
	ICmpInst *isThreadSafeEqualsTrue = new ICmpInst(*checkWaitBlock, ICmpInst::ICMP_NE, isThreadSafeValue, falseValue, "");
	BranchInst::Create(waitBlock, checkTypeIndexBlock, isThreadSafeEqualsTrue, checkWaitBlock);

	VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, waitBlock, nodeSemaphoreValue);
	BranchInst::Create(checkTypeIndexBlock, waitBlock);


	// if (typeIndex == 0)
	// {
	//   VuoImage portValue = (VuoImage)(*portAddress);
	//   if (serializationType == 0)
	//     ret = VuoImage_getSummary(portValue);
	//   else if (serializationType == 1)
	//     ret = VuoImage_getString(portValue);
	//   else
	//     ret = VuoImage_getInterprocessString(portValue);
	// }
	// else if (typeIndex == 3)
	// {
	//   VuoReal portValue = (VuoReal)(*portAddress);
	//   if (serializationType == 0)
	//     ret = VuoReal_getSummary(portValue);
	//   else
	//     ret = VuoReal_getString(portValue);
	// }
	// else if ...

	vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex;
	for (size_t i = 0; i < orderedTypes.size(); ++i)
	{
		VuoCompilerType *type = orderedTypes[i];

		string typeName = type->getBase()->getModuleKey();
		bool hasInterprocess = type->hasInterprocessStringFunction();

		BasicBlock *checkSummaryBlock = BasicBlock::Create(module->getContext(), typeName + "_checkSummary", function, 0);
		BasicBlock *summaryBlock = BasicBlock::Create(module->getContext(), typeName + "_summary", function, 0);
		BasicBlock *checkStringBlock = NULL;
		BasicBlock *stringBlock = BasicBlock::Create(module->getContext(), typeName + "_string", function, 0);
		BasicBlock *interprocessBlock = NULL;
		BasicBlock *typeFinalBlock = BasicBlock::Create(module->getContext(), typeName + "_final", function, 0);

		BasicBlock *firstStringBlock = NULL;
		if (hasInterprocess)
		{
			checkStringBlock = BasicBlock::Create(module->getContext(), typeName + "_checkString", function, 0);
			interprocessBlock = BasicBlock::Create(module->getContext(), typeName + "_interprocess", function, 0);
			firstStringBlock = checkStringBlock;
		}
		else
		{
			firstStringBlock = stringBlock;
		}

		PointerType *pointerToType = PointerType::get(type->getType(), 0);
		Value *portAddress = new BitCastInst(portAddressAsVoidPointer, pointerToType, "", checkSummaryBlock);
		Value *portValue = new LoadInst(portAddress, "", false, checkSummaryBlock);

		ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(serializationTypeValue->getType()), 0);
		ICmpInst *serializationTypeEqualsZero = new ICmpInst(*checkSummaryBlock, ICmpInst::ICMP_EQ, serializationTypeValue, zeroValue, "");
		BranchInst::Create(summaryBlock, firstStringBlock, serializationTypeEqualsZero, checkSummaryBlock);

		Value *summaryValue = type->generateSummaryFromValueFunctionCall(module, summaryBlock, portValue);
		new StoreInst(summaryValue, retVariable, summaryBlock);
		BranchInst::Create(typeFinalBlock, summaryBlock);

		if (hasInterprocess)
		{
			ConstantInt *oneValue = ConstantInt::get(static_cast<IntegerType *>(serializationTypeValue->getType()), 1);
			ICmpInst *serializationTypeEqualsOne = new ICmpInst(*checkStringBlock, ICmpInst::ICMP_EQ, serializationTypeValue, oneValue, "");
			BranchInst::Create(stringBlock, interprocessBlock, serializationTypeEqualsOne, checkStringBlock);
		}

		Value *stringValue = type->generateStringFromValueFunctionCall(module, stringBlock, portValue);
		new StoreInst(stringValue, retVariable, stringBlock);
		BranchInst::Create(typeFinalBlock, stringBlock);

		if (hasInterprocess)
		{
			Value *interprocessValue = type->generateInterprocessStringFromValueFunctionCall(module, interprocessBlock, portValue);
			new StoreInst(interprocessValue, retVariable, interprocessBlock);
			BranchInst::Create(typeFinalBlock, interprocessBlock);
		}

		blocksForIndex.push_back( make_pair(checkSummaryBlock, typeFinalBlock) );
	}

	BasicBlock *checkSignalBlock = BasicBlock::Create(module->getContext(), "checkSignal", function, 0);
	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, checkTypeIndexBlock, checkSignalBlock, typeIndexValue, blocksForIndex);


	// if (isThreadSafe)
	//   dispatch_semaphore_signal(nodeSemaphore);

	BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "signal", function, 0);
	BranchInst::Create(signalBlock, finalBlock, isThreadSafeEqualsTrue, checkSignalBlock);

	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, signalBlock, nodeSemaphoreValue);
	BranchInst::Create(finalBlock, signalBlock);


	// return ret;

	LoadInst *retValue = new LoadInst(retVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);
}

/**
 * Generates the `compositionSetPortValue()` function, which provides a way to set each port's value.
 *
 * Each definition of `<Type>_makeFromString()` that returns heap data is responsible for registering
 * its return value (with `VuoRegister`).
 *
 * Assumes the semaphore for each node has been initialized.
 *
 * Example:
 *
 * @eg{
 * void compositionSetPortValue(VuoCompositionState *compositionState, const char *portIdentifier, const char *valueAsString,
 *								bool isThreadSafe, bool shouldUpdateTriggers, bool shouldSendTelemetry, bool hasOldValue, bool hasNewValue)
 * {
 *   void *portAddress = vuoGetDataForPort(compositionState, portIdentifier);
 *
 *   if (portAddress != NULL)
 *   {
 *     dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionState, portIdentifier);
 *     unsigned long typeIndex = vuoGetTypeIndexForPort(compositionState, portIdentifier);
 *     unsigned long nodeIndex = vuoGetNodeIndexForPort(compositionState, portIdentifier);
 *     char *summary = NULL;
 *
 *     if (isThreadSafe)
 *       dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);
 *
 *     if (typeIndex == 0)
 *     {
 *       VuoText oldPortValue;
 *       if (hasOldValue)
 *         oldPortValue = *((VuoText *)portAddress);
 *       if (hasNewValue)
 *       {
 *         VuoText portValue = VuoText_makeFromString(valueAsString);
 *         *((VuoText *)portAddress) = portValue;
 *         VuoRetain(portValue);
 *         if (shouldSendTelemetry)
 *           summary = VuoText_getSummary(portValue);
 *       }
 *       if (hasOldValue)
 *         VuoRelease(oldPortValue);
 *     }
 *     else if (typeIndex == 1)
 *     {
 *       if (hasNewValue)
 *       {
 *         VuoReal portValue = VuoReal_makeFromString(valueAsString);
 *         *((VuoReal *)portAddress) = portValue;
 *         if (shouldSendTelemetry)
 *           summary = VuoReal_getSummary(portValue);
 *       }
 *     }
 *     else if ...
 *
 *     if (shouldUpdateTriggers)
 *     {
 *       if (nodeIndex == 0)
 *       {
 *         Top__FirePeriodically2__fired(...);
 *       }
 *       else if ...
 *     }
 *
 *     if (isThreadSafe)
 *       dispatch_semaphore_signal(nodeSemaphore);
 *
 *     if (shouldSendTelemetry)
 *     {
 *       sendInputPortsUpdated(portIdentifier, false, true, summary);
 *       free(summary);
 *     }
 *   }
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateCompositionSetPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");
	Value *isThreadSafeValue = args++;
	isThreadSafeValue->setName("isThreadSafe");
	Value *shouldUpdateTriggersValue = args++;
	shouldUpdateTriggersValue->setName("shouldUpdateTriggers");
	Value *shouldSendTelemetryValue = args++;
	shouldSendTelemetryValue->setName("shouldSendTelemetry");
	Value *hasOldValue = args++;
	hasOldValue->setName("hasOldValue");
	Value *hasNewValue = args++;
	hasNewValue->setName("hasNewValue");


	// void *portAddress = vuoGetDataForPort(compositionState, portIdentifier);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	Value *portAddressAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetDataForPort(module, initialBlock, compositionStateValue, portIdentifierValue);

	// if (portAddress != NULL)

	BasicBlock *checkWaitBlock = BasicBlock::Create(module->getContext(), "checkWait", function, 0);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	ConstantPointerNull *nullPortAddress = ConstantPointerNull::get(static_cast<PointerType *>(portAddressAsVoidPointer->getType()));
	ICmpInst *portAddressNotEqualsNull = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, portAddressAsVoidPointer, nullPortAddress, "");
	BranchInst::Create(checkWaitBlock, finalBlock, portAddressNotEqualsNull, initialBlock);

	// dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionState, portIdentifier);
	// unsigned long typeIndex = vuoGetTypeIndexForPort(compositionState, portIdentifier);
	// unsigned long nodeIndex = vuoGetNodeIndexForPort(compositionState, portIdentifier);
	// char *summary = NULL;

	Value *nodeSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeSemaphoreForPort(module, checkWaitBlock, compositionStateValue, portIdentifierValue);
	Value *typeIndexValue = VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(module, checkWaitBlock, compositionStateValue, portIdentifierValue);
	Value *nodeIndexValue = VuoCompilerCodeGenUtilities::generateGetNodeIndexForPort(module, checkWaitBlock, compositionStateValue, portIdentifierValue);

	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *summaryVariable = new AllocaInst(pointerToCharType, "summary", checkWaitBlock);
	ConstantPointerNull *nullSummary = ConstantPointerNull::get(pointerToCharType);
	new StoreInst(nullSummary, summaryVariable, false, checkWaitBlock);

	// if (isThreadSafe)
	//   dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);

	BasicBlock *waitBlock = BasicBlock::Create(module->getContext(), "wait", function, 0);
	BasicBlock *checkTypeIndexBlock = BasicBlock::Create(module->getContext(), "checkTypeIndex", function, 0);
	ConstantInt *falseArgValue = ConstantInt::get(static_cast<IntegerType *>(isThreadSafeValue->getType()), 0);
	ICmpInst *isThreadSafeEqualsTrue = new ICmpInst(*checkWaitBlock, ICmpInst::ICMP_NE, isThreadSafeValue, falseArgValue, "");
	BranchInst::Create(waitBlock, checkTypeIndexBlock, isThreadSafeEqualsTrue, checkWaitBlock);

	VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, waitBlock, nodeSemaphoreValue);
	BranchInst::Create(checkTypeIndexBlock, waitBlock);


	// if (typeIndex == 0)
	// {
	//   VuoText oldPortValue;
	//   if (hasOldValue)
	//     oldPortValue = *((VuoText *)portAddress);
	//   if (hasNewValue)
	//   {
	//     VuoText portValue = VuoText_makeFromString(valueAsString);
	//     *((VuoText *)portAddress) = portValue;
	//     VuoRetain(portValue);
	//     if (shouldSendTelemetry)
	//       summary = VuoText_getSummary(portValue);
	//   }
	//   if (hasOldValue)
	//     VuoRelease(oldPortValue);
	// }
	// else if (typeIndex == 1)
	// {
	//   if (hasNewValue)
	//   {
	//     VuoReal portValue = VuoReal_makeFromString(valueAsString);
	//     *((VuoReal *)portAddress) = portValue;
	//     if (shouldSendTelemetry)
	//       summary = VuoReal_getSummary(portValue);
	//   }
	// }
	// else if ...

	ICmpInst *hasOldValueIsTrue = new ICmpInst(*checkTypeIndexBlock, ICmpInst::ICMP_NE, hasOldValue, falseArgValue, "");
	ICmpInst *hasNewValueIsTrue = new ICmpInst(*checkTypeIndexBlock, ICmpInst::ICMP_NE, hasNewValue, falseArgValue, "");
	ICmpInst *shouldSendTelemetryIsTrue = new ICmpInst(*checkTypeIndexBlock, ICmpInst::ICMP_NE, shouldSendTelemetryValue, falseArgValue, "");

	vector< pair<BasicBlock *, BasicBlock *> > blocksForTypeIndex;
	for (size_t i = 0; i < orderedTypes.size(); ++i)
	{
		VuoCompilerType *type = orderedTypes[i];

		BasicBlock *typeInitialBlock = BasicBlock::Create(module->getContext(), type->getBase()->getModuleKey() + "_initial", function, 0);
		Value *portAddress = new BitCastInst(portAddressAsVoidPointer, PointerType::get(type->getType(), 0), "", typeInitialBlock);

		BasicBlock *typeFinalBlock = BasicBlock::Create(module->getContext(), type->getBase()->getModuleKey() + "_final", function, 0);

		BasicBlock *setNewValueBlock = BasicBlock::Create(module->getContext(), "setNewValue", function, 0);
		Value *portValue = type->generateValueFromStringFunctionCall(module, setNewValueBlock, valueAsStringValue);
		new StoreInst(portValue, portAddress, false, setNewValueBlock);
		type->generateRetainCall(module, setNewValueBlock, portValue);

		BasicBlock *summaryBlock = BasicBlock::Create(module->getContext(), "summary", function, 0);
		Value *summaryValue = type->generateSummaryFromValueFunctionCall(module, summaryBlock, portValue);
		new StoreInst(summaryValue, summaryVariable, false, summaryBlock);

		if (VuoCompilerCodeGenUtilities::isRetainOrReleaseNeeded(type->getType()))
		{
			BasicBlock *saveOldValueBlock = BasicBlock::Create(module->getContext(), "saveOldValue", function, 0);
			BasicBlock *checkNewValueBlock = BasicBlock::Create(module->getContext(), "checkNewValue", function, 0);
			BasicBlock *checkOldValueBlock = BasicBlock::Create(module->getContext(), "checkOldValue", function, 0);
			BasicBlock *releaseOldValueBlock = BasicBlock::Create(module->getContext(), "releaseOldValue", function, 0);

			AllocaInst *oldPortValueVariable = new AllocaInst(type->getType(), "oldPortValue", typeInitialBlock);
			BranchInst::Create(saveOldValueBlock, checkNewValueBlock, hasOldValueIsTrue, typeInitialBlock);

			Value *oldPortValue = new LoadInst(portAddress, "", false, saveOldValueBlock);
			new StoreInst(oldPortValue, oldPortValueVariable, false, saveOldValueBlock);
			BranchInst::Create(checkNewValueBlock, saveOldValueBlock);

			BranchInst::Create(setNewValueBlock, checkOldValueBlock, hasNewValueIsTrue, checkNewValueBlock);

			BranchInst::Create(summaryBlock, checkOldValueBlock, shouldSendTelemetryIsTrue, setNewValueBlock);

			BranchInst::Create(checkOldValueBlock, summaryBlock);

			BranchInst::Create(releaseOldValueBlock, typeFinalBlock, hasOldValueIsTrue, checkOldValueBlock);

			oldPortValue = new LoadInst(oldPortValueVariable, "", false, releaseOldValueBlock);
			type->generateReleaseCall(module, releaseOldValueBlock, oldPortValue);
			BranchInst::Create(typeFinalBlock, releaseOldValueBlock);
		}
		else
		{
			BranchInst::Create(setNewValueBlock, typeFinalBlock, hasNewValueIsTrue, typeInitialBlock);

			BranchInst::Create(summaryBlock, typeFinalBlock, shouldSendTelemetryIsTrue, setNewValueBlock);

			BranchInst::Create(typeFinalBlock, summaryBlock);
		}

		blocksForTypeIndex.push_back( make_pair(typeInitialBlock, typeFinalBlock) );
	}

	BasicBlock *checkUpdateBlock = BasicBlock::Create(module->getContext(), "checkUpdate", function, 0);
	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, checkTypeIndexBlock, checkUpdateBlock, typeIndexValue, blocksForTypeIndex);


	// if (shouldUpdateTriggers)
	// {
	//   if (nodeIndex == 0)
	//   {
	//     Top__FirePeriodically2__fired(...);
	//   }
	//   else if ...
	// }

	Constant *zeroValue = ConstantInt::get(shouldUpdateTriggersValue->getType(), 0);
	ICmpInst *shouldUpdateTriggersIsTrue = new ICmpInst(*checkUpdateBlock, ICmpInst::ICMP_NE, shouldUpdateTriggersValue, zeroValue, "");
	BasicBlock *updateTriggersBlock = BasicBlock::Create(module->getContext(), "updateTriggers", function, 0);
	BasicBlock *checkSendBlock = BasicBlock::Create(module->getContext(), "checkSend", function, 0);
	BranchInst::Create(updateTriggersBlock, checkSendBlock, shouldUpdateTriggersIsTrue, checkUpdateBlock);

	vector< pair<BasicBlock *, BasicBlock *> > blocksForNodeIndex;
	for (size_t i = 0; i < orderedNodes.size(); ++i)
	{
		VuoCompilerNode *node = orderedNodes[i];
		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), node->getIdentifier(), function, 0);
		BasicBlock *origCurrentBlock = currentBlock;

		node->generateCallbackUpdateFunctionCall(module, currentBlock, compositionStateValue);

		generateDataOnlyTransmissionFromNode(function, currentBlock, compositionStateValue, node, true, true, true);

		blocksForNodeIndex.push_back( make_pair(origCurrentBlock, currentBlock) );
	}

	BasicBlock *checkSignalBlock = BasicBlock::Create(module->getContext(), "checkSignal", function, 0);
	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, updateTriggersBlock, checkSignalBlock, nodeIndexValue, blocksForNodeIndex);


	// if (isThreadSafe)
	//   dispatch_semaphore_signal(nodeSemaphore);

	BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "signal", function, 0);
	BranchInst::Create(signalBlock, checkSendBlock, isThreadSafeEqualsTrue, checkSignalBlock);

	VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, signalBlock, nodeSemaphoreValue);
	BranchInst::Create(checkSendBlock, signalBlock);


	// if (shouldSendTelemetry)
	// {
	//   sendInputPortsUpdated(portIdentifier, false, true, summary);
	//   free(summary);
	// }

	BasicBlock *sendBlock = BasicBlock::Create(module->getContext(), "send", function, 0);
	BranchInst::Create(sendBlock, finalBlock, shouldSendTelemetryIsTrue, checkSendBlock);

	Value *summaryValue = new LoadInst(summaryVariable, "", false, sendBlock);

	VuoCompilerCodeGenUtilities::generateSendInputPortsUpdated(module, sendBlock, compositionStateValue, portIdentifierValue,
															   false, true, summaryValue);

	VuoCompilerCodeGenUtilities::generateFreeCall(module, sendBlock, summaryValue);
	BranchInst::Create(finalBlock, sendBlock);

	ReturnInst::Create(module->getContext(), finalBlock);
}

/**
 * Generates the `vuoSetInputPortValue()` function.
 *
 * @eg{void vuoSetInputPortValue(const char *portIdentifier, const char *valueAsString);}
 */
void VuoCompilerBitcodeGenerator::generateSetInputPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");

	Function *compositionSetPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);

	Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
	Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

	Value *trueValue = ConstantInt::get(compositionSetPortValueFunction->getFunctionType()->getParamType(3), 1);

	vector<Value *> compositionArgs;
	compositionArgs.push_back(compositionStateValue);
	compositionArgs.push_back(portIdentifierValue);
	compositionArgs.push_back(valueAsStringValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	CallInst::Create(compositionSetPortValueFunction, compositionArgs, "", block);

	VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, compositionStateValue);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the `compositionFireTriggerPortEvent()` function.
 *
 * @eg{
 * void compositionFireTriggerPortEvent(VuoCompositionState *compositionState, const char *portIdentifier)
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
void VuoCompilerBitcodeGenerator::generateCompositionFireTriggerPortEventFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionFireTriggerPortEventFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	for (VuoCompilerTriggerPort *trigger : graph->getTriggerPorts())
	{
		VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);

		string currentPortIdentifier = trigger->getIdentifier();

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
		BasicBlock *origCurrentBlock = currentBlock;

		Value *nodeContextValue = triggerNode->generateGetContext(module, currentBlock, compositionStateValue);
		Value *triggerFunctionValue = trigger->generateLoadFunction(module, currentBlock, nodeContextValue);

		vector<Value *> triggerArgs;
		AttributeSet triggerParamAttributes;
		VuoType *dataType = trigger->getDataVuoType();
		if (dataType)
		{
			generateWaitForNodes(module, function, currentBlock, compositionStateValue, vector<VuoCompilerNode *>(1, triggerNode));

			Value *arg = trigger->generateLoadPreviousData(module, currentBlock, nodeContextValue);

			FunctionType *triggerFunctionType = trigger->getClass()->getFunctionType();

			Type *secondParam = NULL;
			dataType->getCompiler()->getFunctionParameterType(&secondParam);

			triggerParamAttributes = dataType->getCompiler()->getFunctionParameterAttributes();
			bool isByVal = triggerParamAttributes.hasAttrSomewhere(Attribute::ByVal);

			Value *secondArg = NULL;
			Value **secondArgIfNeeded = (secondParam ? &secondArg : NULL);
			arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, triggerFunctionType, 0, isByVal, secondArgIfNeeded, module, currentBlock);

			triggerArgs.push_back(arg);
			if (secondParam)
				triggerArgs.push_back(secondArg);
		}

		CallInst *call = CallInst::Create(triggerFunctionValue, triggerArgs, "", currentBlock);
		if (dataType)
			call->setAttributes(VuoCompilerCodeGenUtilities::copyAttributesToIndex(triggerParamAttributes, 1));

		if (dataType)
			generateSignalForNodes(module, currentBlock, compositionStateValue, vector<VuoCompilerNode *>(1, triggerNode));

		blocksForString[currentPortIdentifier] = make_pair(origCurrentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString, constantStrings);
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
		names.push_back( (*i)->getClass()->getName() );
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
		VuoCompilerPublishedPort *publishedPort = static_cast<VuoCompilerPublishedPort *>((*i)->getCompiler());

		VuoType *type = publishedPort->getDataVuoType();
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
		VuoCompilerPublishedPort *publishedPort = static_cast<VuoCompilerPublishedPort *>((*i)->getCompiler());

		json_object *detailsObj = publishedPort->getDetails(input);
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
 * Generates the firePublishedInputPortEvent function.
 *
 * Assumes the trigger function has been set for each trigger port.
 *
 * @eg{
 * void firePublishedInputPortEvent(const char *const *names, unsigned int count);
 * }
 */
void VuoCompilerBitcodeGenerator::generateFirePublishedInputPortEventFunction(void)
{
	string functionName = "firePublishedInputPortEvent";
	Function *function = module->getFunction(functionName);
	if (! function)
	{
		PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
		PointerType *pointerToPointerToChar = PointerType::get(pointerToChar, 0);
		Type *countType = IntegerType::get(module->getContext(), 32);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToPointerToChar);
		functionParams.push_back(countType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *namesValue = args++;
	namesValue->setName("names");
	Value *countValue = args++;
	countValue->setName("count");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	VuoCompilerTriggerPort *trigger = graph->getPublishedInputTrigger();
	if (! trigger)
	{
		ReturnInst::Create(module->getContext(), initialBlock);
		return;
	}

	VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);

	Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, initialBlock);
	Value *compositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, initialBlock, runtimeStateValue,
																							   compositionIdentifierValue);

	// Create an event ID so the nodes claimed by this function can be used (re-claimed) by the trigger worker.

	Value *eventIdValue = VuoCompilerCodeGenUtilities::generateGetNextEventId(module, initialBlock, compositionStateValue);

	// Claim all necessary downstream nodes —
	// including the published output node to synchronize access to the composition's node context when tracking events started/finished.

	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);

	VuoCompilerNode *publishedOutputNode = graph->getPublishedOutputNode();
	if (find(triggerWaitNodes.begin(), triggerWaitNodes.end(), publishedOutputNode) == triggerWaitNodes.end())
		triggerWaitNodes.push_back(publishedOutputNode);

	generateWaitForNodes(module, function, initialBlock, compositionStateValue, triggerWaitNodes, eventIdValue);

	// Start tracking the event so we can later notify the runner when the event is finished.

	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, initialBlock, compositionStateValue);
	VuoCompilerCodeGenUtilities::generateStartedExecutingEvent(module, initialBlock, compositionContextValue, eventIdValue);

	// Mark the selected input ports on the published input node as hit by an event.
	//
	// int i = 0;
	// while (i < count)
	// {
	//    if (! strcmp(names[i], "firstPort"))
	//    {
	//       ...
	//    }
	//    else if (! strcmp(names[i], "secondPort"))
	//    {
	//       ...
	//    }
	//    i = i + 1;
	// }
	//

	VuoCompilerNode *publishedInputNode = graph->getPublishedInputNode();
	Value *publishedInputNodeContextValue = publishedInputNode->generateGetContext(module, initialBlock, compositionStateValue);

	Value *zeroValue = ConstantInt::get(static_cast<IntegerType *>(countValue->getType()), 0);
	AllocaInst *iterVariable = new AllocaInst(countValue->getType(), "i", initialBlock);
	new StoreInst(zeroValue, iterVariable, false, initialBlock);

	BasicBlock *loopConditionBlock = BasicBlock::Create(module->getContext(), "loopCondition", function, 0);
	BasicBlock *loopBeginBlock = BasicBlock::Create(module->getContext(), "loopBegin", function, 0);
	BasicBlock *loopEndBlock = BasicBlock::Create(module->getContext(), "loopEnd", function, 0);
	BasicBlock *fireBlock = BasicBlock::Create(module->getContext(), "fire", function, 0);

	BranchInst::Create(loopConditionBlock, initialBlock);

	Value *iterValue = new LoadInst(iterVariable, "", false, loopConditionBlock);
	ICmpInst *iterLessThanCount = new ICmpInst(*loopConditionBlock, ICmpInst::ICMP_ULT, iterValue, countValue, "");
	BranchInst::Create(loopBeginBlock, fireBlock, iterLessThanCount, loopConditionBlock);

	Value *iterNameValue = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, loopBeginBlock, namesValue, iterValue);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		string currentName = publishedInputPorts[i]->getClass()->getName();
		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

		VuoPort *port = graph->getInputPortOnPublishedInputNode(i);
		VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>(port->getCompiler());
		inputEventPort->generateStoreEvent(module, currentBlock, publishedInputNodeContextValue, true);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, loopBeginBlock, loopEndBlock, iterNameValue, blocksForString, constantStrings);

	Value *oneValue = ConstantInt::get(static_cast<IntegerType *>(countValue->getType()), 1);
	Value *iterPlusOneValue = BinaryOperator::Create(Instruction::Add, iterValue, oneValue, "", loopEndBlock);
	new StoreInst(iterPlusOneValue, iterVariable, false, loopEndBlock);

	BranchInst::Create(loopConditionBlock, loopEndBlock);

	// Fire an event from the published input trigger.

	Value *triggerNodeContextValue = triggerNode->generateGetContext(module, fireBlock, compositionStateValue);
	Value *triggerFunctionValue = trigger->generateLoadFunction(module, fireBlock, triggerNodeContextValue);
	CallInst::Create(triggerFunctionValue, "", fireBlock);

	ReturnInst::Create(module->getContext(), fireBlock);
}

/**
 * Generates the getPublishedInputPortValue() or getPublishedOutputPortValue() function.
 *
 * @eg{
 * char * getPublishedInputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
 * {
 *	 char *ret = NULL;
 *
 *   if (! strcmp(portIdentifier, "firstName"))
 *   {
 *     ret = vuoGetInputPortString("vuo_in__PublishedInputs__firstName", shouldUseInterprocessSerialization);
 *   }
 *   else if (! strcmp(portIdentifier, "secondName"))
 *   {
 *     ret = vuoGetInputPortString("vuo_in__PublishedInputs__secondName", shouldUseInterprocessSerialization);
 *   }
 *
 *   return ret;
 * }
 * }
 *
 * @eg{
 * char * getPublishedOutputPortValue(char *portIdentifier, int shouldUseInterprocessSerialization)
 * {
 *	 char *ret = NULL;
 *
 *   if (! strcmp(portIdentifier, "firstName"))
 *   {
 *     ret = vuoGetInputPortString("vuo_out__PublishedOutputs__firstName", shouldUseInterprocessSerialization);
 *   }
 *   else if (! strcmp(portIdentifier, "secondName"))
 *   {
 *     ret = vuoGetInputPortString("vuo_out__PublishedOutputs__secondName", shouldUseInterprocessSerialization);
 *   }
 *
 *   return ret;
 * }
 * }
 */
void VuoCompilerBitcodeGenerator::generateGetPublishedPortValueFunction(bool input)
{
	Function *function = (input ?
							  VuoCompilerCodeGenUtilities::getGetPublishedInputPortValueFunction(module) :
							  VuoCompilerCodeGenUtilities::getGetPublishedOutputPortValueFunction(module));

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *shouldUseInterprocessSerializationValue = args++;
	shouldUseInterprocessSerializationValue->setName("shouldUseInterprocessSerialization");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *retVariable = new AllocaInst(pointerToChar, "ret", initialBlock);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToChar);
	new StoreInst(nullValue, retVariable, false, initialBlock);

	Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, initialBlock);
	Value *compositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, initialBlock, runtimeStateValue,
																							   compositionIdentifierValue);

	vector<VuoPort *> inputPortsOnPublishedNode;
	if (input)
	{
		vector<VuoPublishedPort *> publishedPorts = composition->getBase()->getPublishedInputPorts();
		for (size_t i = 0; i < publishedPorts.size(); ++i)
		{
			VuoPort *port = graph->getInputPortOnPublishedInputNode(i);
			inputPortsOnPublishedNode.push_back(port);
		}
	}
	else
	{
		vector<VuoPublishedPort *> publishedPorts = composition->getBase()->getPublishedOutputPorts();
		for (size_t i = 0; i < publishedPorts.size(); ++i)
		{
			VuoPort *port = graph->getInputPortOnPublishedOutputNode(i);
			inputPortsOnPublishedNode.push_back(port);
		}
	}

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	for (VuoPort *port : inputPortsOnPublishedNode)
	{
		string currentName = port->getClass()->getName();
		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

		string currentIdentifier = static_cast<VuoCompilerEventPort *>(port->getCompiler())->getIdentifier();
		Constant *currentIdentifierValue = constantStrings.get(module, currentIdentifier);

		Value *retValue = VuoCompilerCodeGenUtilities::generateGetInputPortString(module, currentBlock, compositionStateValue,
																				  currentIdentifierValue, shouldUseInterprocessSerializationValue);
		new StoreInst(retValue, retVariable, currentBlock);

		blocksForString[currentName] = make_pair(currentBlock, currentBlock);
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, finalBlock, compositionStateValue);

	LoadInst *retValue = new LoadInst(retVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString, constantStrings);
}

/**
 * Generates the `compositionSetPublishedInputPortValue()` function.
 *
 * Calls to this function are enqueued on the same dispatch queues as calls to `firePublishedInputPortEvent()`
 * to make sure that the actions are carried out in the order they were requested.
 *
 * @eg{
 * void compositionSetPublishedInputPortValue(VuoCompositionState *compositionState, const char *publishedInputPortName, const char *valueAsString,
 *                                            bool isCompositionRunning);
 * }
 */
void VuoCompilerBitcodeGenerator::generateCompositionSetPublishedInputPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionSetPublishedInputPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *publishedInputPortNameValue = args++;
	publishedInputPortNameValue->setName("publishedInputPortName");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");
	Value *isCompositionRunningValue = args++;
	isCompositionRunningValue->setName("isCompositionRunning");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	VuoCompilerTriggerPort *trigger = graph->getPublishedInputTrigger();
	if (! trigger)
	{
		ReturnInst::Create(module->getContext(), initialBlock);
		return;
	}

	// const char *inputPortIdentifier = NULL;

	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *inputPortIdentifierVariable = new AllocaInst(pointerToCharType, "inputPortIdentifier", initialBlock);
	ConstantPointerNull *nullInputPortIdentifier = ConstantPointerNull::get(pointerToCharType);
	new StoreInst(nullInputPortIdentifier, inputPortIdentifierVariable, false, initialBlock);

	// if (! strcmp(publishedInputPortName, "firstName"))
	//     inputPortIdentifier = "PublishedInputs__firstName";
	// else if (! strcmp(publishedInputPortName, "secondName"))
	//     inputPortIdentifier = "PublishedInputs__secondName";
	// ...

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		string currPublishedInputPortName = publishedInputPorts[i]->getClass()->getName();
		BasicBlock *currBlock = BasicBlock::Create(module->getContext(), currPublishedInputPortName, function, 0);

		VuoPort *inputPort = graph->getInputPortOnPublishedInputNode(i);
		string inputPortIdentifier = static_cast<VuoCompilerPort *>(inputPort->getCompiler())->getIdentifier();
		Value *inputPortIdentifierValue = constantStrings.get(module, inputPortIdentifier);

		new StoreInst(inputPortIdentifierValue, inputPortIdentifierVariable, false, currBlock);

		blocksForString[currPublishedInputPortName] = make_pair(currBlock, currBlock);
	}

	BasicBlock *checkBlock = BasicBlock::Create(module->getContext(), "check", function, 0);
	BasicBlock *scheduleBlock = BasicBlock::Create(module->getContext(), "schedule", function, 0);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, checkBlock, publishedInputPortNameValue, blocksForString, constantStrings);

	// if (inputPortIdentifier != NULL)
	// {

	Value *inputPortIdentifierValue = new LoadInst(inputPortIdentifierVariable, "", false, checkBlock);
	ICmpInst *portIdentifierNotEqualsNull = new ICmpInst(*checkBlock, ICmpInst::ICMP_NE, inputPortIdentifierValue, nullInputPortIdentifier, "");
	BranchInst::Create(scheduleBlock, finalBlock, portIdentifierNotEqualsNull, checkBlock);

	//     void **context = (void **)malloc(4 * sizeof(void *));
	//     context[0] = (void *)compositionState;
	//     context[1] = (void *)inputPortIdentifier;
	//     context[2] = (void *)valueAsString;
	//     context[3] = (void *)isCompositionRunning;

	Value *contextValue = VuoCompilerCodeGenUtilities::generateCreatePublishedInputWorkerContext(module, scheduleBlock, compositionStateValue,
																								 inputPortIdentifierValue, valueAsStringValue,
																								 isCompositionRunningValue);

	//     dispatch_sync(PublishedInputsTrigger__queue, context, PublishedInputPorts__setWorker);
	// }

	VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);
	Value *nodeContextValue = triggerNode->generateGetContext(module, scheduleBlock, compositionStateValue);
	string workerFunctionName = VuoStringUtilities::transcodeToIdentifier(VuoNodeClass::publishedInputNodeClassName) + "__setWorker";

	Function *workerFunction = trigger->generateSynchronousSubmissionToDispatchQueue(module, scheduleBlock, nodeContextValue,
																					 workerFunctionName, contextValue);

	BranchInst::Create(finalBlock, scheduleBlock);
	ReturnInst::Create(module->getContext(), finalBlock);

	// void PublishedInputPorts__firstName__worker(void *context)
	// {

	Function *compositionSetPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPortValueFunction(module);

	Function::arg_iterator workerArgs = workerFunction->arg_begin();
	Value *contextValueInWorker = workerArgs++;
	contextValueInWorker->setName("context");

	BasicBlock *workerBlock = BasicBlock::Create(module->getContext(), "", workerFunction, 0);

	//     VuoCompositionState *compositionState = (VuoCompositionState *)((void **)context)[0];
	//     char *inputPortIdentifier = (char *)((void **)context)[1];
	//     char *valueAsString = (char *)((void **)context)[2];
	//     bool isCompositionRunning = (bool)((void **)context)[3];

	Type *voidPointerType = contextValueInWorker->getType();
	Type *voidPointerPointerType = PointerType::get(voidPointerType, 0);
	Type *compositionStatePointerType = PointerType::get(VuoCompilerCodeGenUtilities::getCompositionStateType(module), 0);
	Type *boolType = compositionSetPortValueFunction->getFunctionType()->getParamType(3);

	Value *contextValueAsVoidPointerArray = new BitCastInst(contextValueInWorker, voidPointerPointerType, "", workerBlock);
	Value *compositionStateAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, workerBlock, contextValueAsVoidPointerArray, (size_t)0);
	Value *compositionStateValueInWorker = new BitCastInst(compositionStateAsVoidPointer, compositionStatePointerType, "", workerBlock);

	Value *inputPortIdentifierAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, workerBlock, contextValueAsVoidPointerArray, 1);
	Value *inputPortIdentifierValueInWorker = new BitCastInst(inputPortIdentifierAsVoidPointer, pointerToCharType, "", workerBlock);

	Value *valueAsStringValueAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, workerBlock, contextValueAsVoidPointerArray, 2);
	Value *valueAsStringValueInWorker = new BitCastInst(valueAsStringValueAsVoidPointer, pointerToCharType, "", workerBlock);

	Value *isCompositionRunningValueAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetArrayElement(module, workerBlock, contextValueAsVoidPointerArray, 3);
	Value *isCompositionRunningValueInWorker = new PtrToIntInst(isCompositionRunningValueAsVoidPointer, boolType, "", workerBlock);

	//     free(context);

	VuoCompilerCodeGenUtilities::generateFreeCall(module, workerBlock, contextValueInWorker);

	//     compositionSetPortValue(compositionState, portIdentifier, valueAsString, isCompositionRunning, isCompositionRunning, isCompositionRunning, true, true);
	// }

	Value *trueValue = ConstantInt::get(boolType, 1);

	vector<Value *> setValueArgs;
	setValueArgs.push_back(compositionStateValueInWorker);
	setValueArgs.push_back(inputPortIdentifierValueInWorker);
	setValueArgs.push_back(valueAsStringValueInWorker);
	setValueArgs.push_back(isCompositionRunningValueInWorker);
	setValueArgs.push_back(isCompositionRunningValueInWorker);
	setValueArgs.push_back(isCompositionRunningValueInWorker);
	setValueArgs.push_back(trueValue);
	setValueArgs.push_back(trueValue);
	CallInst::Create(compositionSetPortValueFunction, setValueArgs, "", workerBlock);

	ReturnInst::Create(module->getContext(), workerBlock);
}

/**
 * Generates the `setPublishedInputPortValue()` function.
 *
 * @eg{
 * void setPublishedInputPortValue(const char *publishedInputPortname, const char *valueAsString);
 * }
 */
void VuoCompilerBitcodeGenerator::generateSetPublishedInputPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetPublishedInputPortValueFunction(module);

	Function::arg_iterator args = function->arg_begin();
	Value *publishedInputPortNameValue = args++;
	publishedInputPortNameValue->setName("publishedInputPortName");
	Value *valueAsStringValue = args++;
	valueAsStringValue->setName("valueAsString");

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Function *compositionFunction = VuoCompilerCodeGenUtilities::getCompositionSetPublishedInputPortValueFunction(module);

	Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
	Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);
	Value *trueValue = ConstantInt::get(compositionFunction->getFunctionType()->getParamType(3), 1);

	vector<Value *> compositionArgs;
	compositionArgs.push_back(compositionStateValue);
	compositionArgs.push_back(publishedInputPortNameValue);
	compositionArgs.push_back(valueAsStringValue);
	compositionArgs.push_back(trueValue);
	CallInst::Create(compositionFunction, compositionArgs, "", block);

	VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, compositionStateValue);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to transmit an event (if any) and data (if any) from @a outputPort to all connected input ports,
 * and send telemetry indicating that these output and input ports have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock,
																	 Value *compositionStateValue, VuoCompilerPort *outputPort,
																	 Value *eventValue, Value *dataValue,
																	 bool requiresEvent, bool shouldSendTelemetry)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 1);
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	set<VuoCompilerCable *> outgoingCables = graph->getOutgoingCables(outputPort);

	Constant *trueValue = ConstantInt::get(boolType, 1);
	Constant *falseValue = ConstantInt::get(boolType, 0);
	Constant *transmittedEventValue = (requiresEvent ? trueValue : falseValue);

	// char *dataSummary = NULL;
	AllocaInst *dataSummaryVariable = new AllocaInst(pointerToCharType, "dataSummary", currentBlock);
	new StoreInst(ConstantPointerNull::get(pointerToCharType), dataSummaryVariable, currentBlock);

	map<VuoCompilerPort *, ICmpInst *> shouldSummarizeInput;
	if (shouldSendTelemetry)
	{
		// bool sentData = false;
		AllocaInst *sentDataVariable = new AllocaInst(boolType, "sentData", currentBlock);
		new StoreInst(falseValue, sentDataVariable, currentBlock);

		for (set<VuoCompilerCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
		{
			VuoCompilerCable *cable = *i;

			VuoCompilerPort *inputPort = static_cast<VuoCompilerPort *>(cable->getBase()->getToPort()->getCompiler());
			ICmpInst *shouldSendDataForInput = VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(module, currentBlock,
																													  inputPort->getIdentifier(),
																													  compositionStateValue,
																													  constantStrings);
			shouldSummarizeInput[inputPort] = shouldSendDataForInput;
		}

		if (dataValue)
		{
			Value *shouldSummarizeOutput = VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(module, currentBlock,
																												  outputPort->getIdentifier(),
																												  compositionStateValue,
																												  constantStrings);

			for (set<VuoCompilerCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
			{
				VuoCompilerCable *cable = *i;

				if (cable->carriesData())
				{
					VuoCompilerPort *inputPort = static_cast<VuoCompilerPort *>(cable->getBase()->getToPort()->getCompiler());
					shouldSummarizeOutput = BinaryOperator::Create(Instruction::Or, shouldSummarizeOutput, shouldSummarizeInput[inputPort], "", currentBlock);
				}
			}

			BasicBlock *summaryBlock = BasicBlock::Create(module->getContext(), "outputSummary", function, NULL);
			BasicBlock *sendOutputBlock = BasicBlock::Create(module->getContext(), "sendOutput", function, NULL);
			BranchInst::Create(summaryBlock, sendOutputBlock, shouldSummarizeOutput, currentBlock);

			// sentData = true;
			new StoreInst(trueValue, sentDataVariable, summaryBlock);

			// dataSummary = <type>_getSummary(portValue);
			VuoCompilerType *type = outputPort->getDataVuoType()->getCompiler();
			Value *dataSummaryValue = type->generateSummaryFromValueFunctionCall(module, summaryBlock, dataValue);
			new StoreInst(dataSummaryValue, dataSummaryVariable, summaryBlock);

			BranchInst::Create(sendOutputBlock, summaryBlock);
			currentBlock = sendOutputBlock;
		}

		Value *sentDataValue = new LoadInst(sentDataVariable, "", false, currentBlock);
		Value *dataSummaryValue = new LoadInst(dataSummaryVariable, "", false, currentBlock);

		Constant *outputPortIdentifierValue = constantStrings.get(module, outputPort->getIdentifier());

		VuoCompilerCodeGenUtilities::generateSendOutputPortsUpdated(module, currentBlock, compositionStateValue, outputPortIdentifierValue,
																	transmittedEventValue, sentDataValue, dataSummaryValue);
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
		transmissionBlock = BasicBlock::Create(module->getContext(), "transmission", function, NULL);
		noTransmissionBlock = BasicBlock::Create(module->getContext(), "noTransmission", function, NULL);

		ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
		ICmpInst *eventValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
		BranchInst::Create(transmissionBlock, noTransmissionBlock, eventValueIsTrue, currentBlock);
	}

	// ... then transmit the event and data (if any) to each connected input port.
	for (set<VuoCompilerCable *>::iterator i = outgoingCables.begin(); i != outgoingCables.end(); ++i)
	{
		VuoCompilerCable *cable = *i;

		VuoCompilerNode *inputNode = cable->getBase()->getToNode()->getCompiler();
		Value *inputNodeContextValue = inputNode->generateGetContext(module, transmissionBlock, compositionStateValue);
		VuoCompilerInputEventPort *inputPort = static_cast<VuoCompilerInputEventPort *>( cable->getBase()->getToPort()->getCompiler() );
		Value *inputPortContextValue = inputPort->generateGetPortContext(module, transmissionBlock, inputNodeContextValue);

		Value *transmittedDataValue = (cable->carriesData() ? dataValue : NULL);

		cable->generateTransmission(module, transmissionBlock, inputNodeContextValue, inputPortContextValue, transmittedDataValue, requiresEvent);

		if (shouldSendTelemetry)
		{
			// char *inputDataSummary = NULL;
			AllocaInst *inputDataSummaryVariable = new AllocaInst(pointerToCharType, "inputDataSummary", transmissionBlock);
			new StoreInst(ConstantPointerNull::get(pointerToCharType), inputDataSummaryVariable, transmissionBlock);

			// bool receivedData = false;
			AllocaInst *receivedDataVariable = new AllocaInst(boolType, "receivedData", transmissionBlock);
			new StoreInst(falseValue, receivedDataVariable, transmissionBlock);

			VuoType *inputDataType = inputPort->getDataVuoType();
			if (inputDataType)
			{
				BasicBlock *summaryBlock = BasicBlock::Create(module->getContext(), "inputSummary", function, NULL);
				BasicBlock *sendInputBlock = BasicBlock::Create(module->getContext(), "sendInput", function, NULL);
				BranchInst::Create(summaryBlock, sendInputBlock, shouldSummarizeInput[inputPort], transmissionBlock);

				Value *inputDataSummaryValue;
				if (transmittedDataValue)
				{
					// receivedData = true;
					new StoreInst(trueValue, receivedDataVariable, summaryBlock);

					// inputDataSummary = dataSummary;
					inputDataSummaryValue = new LoadInst(dataSummaryVariable, "", false, summaryBlock);
				}
				else
				{
					// inputDataSummary = <Type>_getSummary(inputData);
					Value *inputDataValue = inputPort->generateLoadData(module, summaryBlock, inputNodeContextValue, inputPortContextValue);
					inputDataSummaryValue = inputDataType->getCompiler()->generateSummaryFromValueFunctionCall(module, summaryBlock, inputDataValue);
				}
				new StoreInst(inputDataSummaryValue, inputDataSummaryVariable, summaryBlock);

				BranchInst::Create(sendInputBlock, summaryBlock);
				transmissionBlock = sendInputBlock;
			}

			Value *receivedDataValue = new LoadInst(receivedDataVariable, "", false, transmissionBlock);
			Value *inputDataSummaryValue = new LoadInst(inputDataSummaryVariable, "", false, transmissionBlock);

			Constant *inputPortIdentifierValue = constantStrings.get(module, inputPort->getIdentifier());

			VuoCompilerCodeGenUtilities::generateSendInputPortsUpdated(module, transmissionBlock, compositionStateValue, inputPortIdentifierValue,
																	   transmittedEventValue, receivedDataValue, inputDataSummaryValue);

			if (inputDataType && ! transmittedDataValue)
			{
				// free(inputDataSummary);
				VuoCompilerCodeGenUtilities::generateFreeCall(module, transmissionBlock, inputDataSummaryValue);
			}
		}
	}

	if (alwaysTransmitsEvent)
	{
		currentBlock = transmissionBlock;
	}
	else
	{
		BranchInst::Create(noTransmissionBlock, transmissionBlock);
		currentBlock = noTransmissionBlock;
	}

	if (shouldSendTelemetry && dataValue)
	{
		// free(dataSummary)
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		LoadInst *dataSummaryValue = new LoadInst(dataSummaryVariable, "", false, currentBlock);
		CallInst::Create(freeFunction, dataSummaryValue, "", currentBlock);
	}
}

/**
 * Generates code to transmit an event (if any) and data (if any) through each outgoing cable from @a node,
 * and to send telemetry indicating that the output and input ports on these cables have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTransmissionFromNode(Function *function, BasicBlock *&currentBlock,
															   Value *compositionStateValue, Value *nodeContextValue,
															   VuoCompilerNode *node, bool requiresEvent, bool shouldSendTelemetry)
{
	vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		// If the output port is a trigger port, do nothing.
		VuoCompilerOutputEventPort *outputEventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (! outputEventPort)
			continue;

		Value *portContextValue = outputEventPort->generateGetPortContext(module, currentBlock, nodeContextValue);
		Value *outputEventValue = outputEventPort->generateLoadEvent(module, currentBlock, nodeContextValue, portContextValue);

		BasicBlock *telemetryBlock = NULL;
		BasicBlock *noTelemetryBlock = NULL;
		if (requiresEvent)
		{
			telemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);
			noTelemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);

			// If the output port isn't transmitting an event, do nothing.
			ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(outputEventValue->getType()), 0);
			ICmpInst *eventValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, outputEventValue, zeroValue, "");
			BranchInst::Create(telemetryBlock, noTelemetryBlock, eventValueIsTrue, currentBlock);
		}
		else
		{
			telemetryBlock = currentBlock;
		}

		// Transmit the data through the output port to each connected input port.
		VuoCompilerOutputData *outputData = outputEventPort->getData();
		Value *outputDataValue = (outputData ?
									  outputEventPort->generateLoadData(module, telemetryBlock, nodeContextValue, portContextValue) :
									  NULL);
		generateTransmissionFromOutputPort(function, telemetryBlock, compositionStateValue,
										   outputEventPort, outputEventValue, outputDataValue, requiresEvent, shouldSendTelemetry);

		if (requiresEvent)
		{
			BranchInst::Create(noTelemetryBlock, telemetryBlock);
			currentBlock = noTelemetryBlock;
		}
		else
		{
			currentBlock = telemetryBlock;
		}
	}
}

/**
 * Generates code to send telemetry indicating that the published output ports corresponding to the
 * input ports of @a node (of node class VuoCompilerPublishedOutputNodeClass) have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTelemetryFromPublishedOutputNode(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue,
																		   Value *nodeContextValue, VuoCompilerNode *node)
{
	IntegerType *boolType = IntegerType::get(module->getContext(), 1);
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

	vector<VuoPort *> inputPorts = node->getBase()->getInputPorts();
	for (vector<VuoPort *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
	{
		VuoCompilerInputEventPort *inputEventPort = dynamic_cast<VuoCompilerInputEventPort *>((*i)->getCompiler());

		BasicBlock *telemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);
		BasicBlock *noTelemetryBlock = BasicBlock::Create(module->getContext(), "", function, NULL);

		// If the published output port isn't transmitting an event, do nothing.
		Value *eventValue = inputEventPort->generateLoadEvent(module, currentBlock, nodeContextValue);
		ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
		ICmpInst *eventValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
		BranchInst::Create(telemetryBlock, noTelemetryBlock, eventValueIsTrue, currentBlock);

		VuoCompilerInputData *data = inputEventPort->getData();
		Value *dataValue = data ? inputEventPort->generateLoadData(module, telemetryBlock, nodeContextValue) : NULL;
		Constant *trueValue = ConstantInt::get(boolType, 1);
		Constant *falseValue = ConstantInt::get(boolType, 0);

		Value *sentDataValue = NULL;
		Value *dataSummaryValue = NULL;
		if (dataValue)
		{
			// sentData = true;
			sentDataValue = trueValue;

			// dataSummary = <type>_getSummary(portValue);
			VuoCompilerType *type = inputEventPort->getDataVuoType()->getCompiler();
			dataSummaryValue = type->generateSummaryFromValueFunctionCall(module, telemetryBlock, dataValue);
		}
		else
		{
			// sentData = false;
			sentDataValue = falseValue;

			// dataSummary = NULL;
			dataSummaryValue = ConstantPointerNull::get(pointerToCharType);
		}

		Constant *portIdentifierValue = constantStrings.get(module, inputEventPort->getBase()->getClass()->getName());
		VuoCompilerCodeGenUtilities::generateSendPublishedOutputPortsUpdated(module, telemetryBlock, compositionStateValue, portIdentifierValue,
																			 sentDataValue, dataSummaryValue);

		BranchInst::Create(noTelemetryBlock, telemetryBlock);
		currentBlock = noTelemetryBlock;
	}
}

/**
 * Generates code to transmit data without an event through each data-and-event output port of @a node
 * and onward to all nodes and ports reachable via data-only transmission.
 *
 * Assumes that the semaphore for @a firstNode has already been claimed if necessary.
 */
void VuoCompilerBitcodeGenerator::generateDataOnlyTransmissionFromNode(Function *function, BasicBlock *&currentBlock, Value *compositionStateValue,
																	   VuoCompilerNode *node, bool shouldWaitForDownstreamNodes,
																	   bool shouldUpdateTriggers, bool shouldSendTelemetry)
{
	vector<VuoCompilerNode *> downstreamNodes = graph->getNodesDownstreamViaDataOnlyTransmission(node);
	if (downstreamNodes.empty())
		return;

	if (shouldWaitForDownstreamNodes)
	{
		// Claim the nodes downstream via data-only transmission.
		generateWaitForNodes(module, function, currentBlock, compositionStateValue, downstreamNodes, NULL, true);
	}

	// For this node and each node downstream via data-only transmission...
	vector<VuoCompilerNode *> nodesToVisit = downstreamNodes;
	nodesToVisit.insert(nodesToVisit.begin(), node);
	for (VuoCompilerNode *visitedNode : nodesToVisit)
	{
		if (graph->mayTransmitDataOnly(visitedNode))
		{
			Value *nodeContextValue = visitedNode->generateGetContext(module, currentBlock, compositionStateValue);

			// Simulate an event having just hit all input ports of the node (necessary due to published input node's doors).
			vector<VuoPort *> inputPorts = visitedNode->getBase()->getInputPorts();
			for (size_t i = VuoNodeClass::unreservedInputPortStartIndex; i < inputPorts.size(); ++i)
			{
				VuoCompilerInputEventPort *inputEventPort = static_cast<VuoCompilerInputEventPort *>(inputPorts[i]->getCompiler());
				inputEventPort->generateStoreEvent(module, currentBlock, nodeContextValue, true);
			}

			// Call the node's event function, and send telemetry if needed.
			generateNodeExecution(function, currentBlock, compositionStateValue, visitedNode, false);

			// Transmit data through the node's outgoing cables, and send telemetry for port updates if needed.
			bool shouldSendTelemetryForNode = (shouldSendTelemetry && visitedNode != graph->getPublishedInputNode());
			generateTransmissionFromNode(function, currentBlock, compositionStateValue, nodeContextValue, visitedNode, false, shouldSendTelemetryForNode);

			// Reset the node's event inputs and outputs.
			VuoCompilerCodeGenUtilities::generateResetNodeContextEvents(module, currentBlock, nodeContextValue);
		}

		if (visitedNode != node)
		{
			if (shouldUpdateTriggers)
			{
				// Call the downstream node's trigger update function.
				visitedNode->generateCallbackUpdateFunctionCall(module, currentBlock, compositionStateValue);
			}

			if (shouldWaitForDownstreamNodes)
			{
				// Signal the downstream node.
				generateSignalForNodes(module, currentBlock, compositionStateValue, vector<VuoCompilerNode *>(1, visitedNode));
			}
		}
	}
}

/**
 * Generates code to call the node's event function, sending telemetry indicating that execution has started and finished.
 */
void VuoCompilerBitcodeGenerator::generateNodeExecution(Function *function, BasicBlock *&currentBlock,
														Value *compositionStateValue, VuoCompilerNode *node,
														bool shouldSendTelemetry)
{
	Value *nodeIdentifierValue = constantStrings.get(module, node->getIdentifier());

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has started.
		VuoCompilerCodeGenUtilities::generateSendNodeExecutionStarted(module, currentBlock, compositionStateValue, nodeIdentifierValue);
	}

	// Call the node's event function.
	if (debugMode)
		VuoCompilerCodeGenUtilities::generatePrint(module, currentBlock, node->getBase()->getTitle() + "\n");
	node->generateEventFunctionCall(module, function, currentBlock, compositionStateValue);

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has finished.
		VuoCompilerCodeGenUtilities::generateSendNodeExecutionFinished(module, currentBlock, compositionStateValue, nodeIdentifierValue);
	}
}

/**
 * Generates the allocation of all global variables for the top-level composition.
 */
void VuoCompilerBitcodeGenerator::generateAllocation(void)
{
#ifdef VUO_PRO
	generateAllocation_Pro();
#endif

	{
		Constant *value = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
		new GlobalVariable(*module, value->getType(), true, GlobalValue::ExternalLinkage, value, "vuoTopLevelCompositionIdentifier");
	}
}

/**
 * Generates the `vuoSetup()` function, which allocates and initializes all node and port contexts in the composition.
 *
 * Assumes generateTriggerFunctions() has been called. Stores these function pointers in the trigger port contexts.
 *
 * \eg{void vuoSetup(void);}
 */
void VuoCompilerBitcodeGenerator::generateSetupFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetupFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
	Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

	// Top__compositionAddNodeMetadata(compositionState);

	Function *compositionCreateAndRegisterMetadataFunction = VuoCompilerCodeGenUtilities::getCompositionAddNodeMetadataFunction(module);
	CallInst::Create(compositionCreateAndRegisterMetadataFunction, topLevelCompositionStateValue, "", block);

	// vuoInitContextForTopLevelComposition(compositionState, hasInstanceData, publishedOutputPortCount);

	size_t publishedOutputPortCount = composition->getBase()->getPublishedOutputPorts().size();
	VuoCompilerCodeGenUtilities::generateInitContextForTopLevelComposition(module, block, topLevelCompositionStateValue,
																		   isStatefulComposition, publishedOutputPortCount);

	// Set each published input port to its initial value.

	Function *compositionSetPublishedInputPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionSetPublishedInputPortValueFunction(module);
	Value *falseValue = ConstantInt::get(compositionSetPublishedInputPortValueFunction->getFunctionType()->getParamType(3), 0);

	for (VuoPublishedPort *publishedInputPort : composition->getBase()->getPublishedInputPorts())
	{
		string name = publishedInputPort->getClass()->getName();
		string initialValue = static_cast<VuoCompilerPublishedPort *>(publishedInputPort->getCompiler())->getInitialValue();

		vector<Value *> args;
		args.push_back(topLevelCompositionStateValue);
		args.push_back( constantStrings.get(module, name) );
		args.push_back( constantStrings.get(module, initialValue) );
		args.push_back(falseValue);
		CallInst::Create(compositionSetPublishedInputPortValueFunction, args, "", block);
	}

	// Top__compositionPerformDataOnlyTransmissions(compositionState);

	Function *compositionPerformDataOnlyTransmissionsFunction = VuoCompilerCodeGenUtilities::getCompositionPerformDataOnlyTransmissionsFunction(module);
	CallInst::Create(compositionPerformDataOnlyTransmissionsFunction, topLevelCompositionStateValue, "", block);

	// Update the function pointers for all trigger functions in the top-level composition.

	for (map<VuoCompilerTriggerPort *, Function *>::iterator i = topLevelTriggerFunctions.begin(); i != topLevelTriggerFunctions.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		Function *function = i->second;

		VuoCompilerNode *node = graph->getNodeForTriggerPort(trigger);
		Value *nodeContextValue = node->generateGetContext(module, block, topLevelCompositionStateValue);
		trigger->generateStoreFunction(module, block, nodeContextValue, function);
	}

	// Update the function pointers for all trigger functions in subcompositions.

	for (map<string, map<size_t, map<VuoCompilerTriggerDescription *, Function *> > >::iterator i = subcompositionTriggerFunctions.begin(); i != subcompositionTriggerFunctions.end(); ++i)
	{
		string compositionIdentifier = i->first;

		for (map<size_t, map<VuoCompilerTriggerDescription *, Function *> >::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			size_t nodeIndex = j->first;

			for (map<VuoCompilerTriggerDescription *, Function *>::iterator k = j->second.begin(); k != j->second.end(); ++k)
			{
				VuoCompilerTriggerDescription *trigger = k->first;
				Function *function = k->second;

				Value *compositionIdentifierValue = constantStrings.get(module, compositionIdentifier);
				Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, compositionIdentifierValue);

				int portContextIndex = trigger->getPortContextIndex();
				Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, compositionStateValue, nodeIndex);
				Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, block, nodeContextValue, portContextIndex);
				VuoCompilerCodeGenUtilities::generateSetPortContextTriggerFunction(module, block, portContextValue, function);

				VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, compositionStateValue);
			}
		}
	}

	VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the `vuoCleanup()` function, which deallocates all node and port contexts in the composition
 * except those being carried across a live-coding reload.
 *
 * \eg{void vuoCleanup(void);}
 */
void VuoCompilerBitcodeGenerator::generateCleanupFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCleanupFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
	Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
	Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

	VuoCompilerCodeGenUtilities::generateFiniContextForTopLevelComposition(module, block, topLevelCompositionStateValue);

	VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c vuoInstanceInit() function, which calls @c nodeInstanceInit() if this is a stateful composition.
 *
 * \eg{void vuoInstanceInit(void);}
 */
void VuoCompilerBitcodeGenerator::generateInstanceInitFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getInstanceInitFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	if (isStatefulComposition)
	{
		map<VuoPort *, size_t> indexOfParameter;
		Function *nodeInstanceInitFunction = VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(module, moduleKey, true,
																									  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																									  vector<VuoPort *>(),
																									  indexOfParameter, constantStrings);

		Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
		Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
		Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

		CallInst::Create(nodeInstanceInitFunction, topLevelCompositionStateValue, "", block);

		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c vuoInstanceFini() function, which calls @c nodeInstanceFini() if this is a stateful composition.
 *
 * \eg{void vuoInstanceFini(void);}
 */
void VuoCompilerBitcodeGenerator::generateInstanceFiniFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getInstanceFiniFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	if (isStatefulComposition)
	{
		Function *nodeInstanceFiniFunction = VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(module, moduleKey,
																									  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																									  constantStrings);

		Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
		Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
		Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceFiniFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionStateValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceFiniFunction, args, "", block);

		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c vuoInstanceTriggerStart() function, which calls @c nodeInstanceTriggerStart() if this is a stateful composition.
 *
 * \eg{void vuoInstanceTriggerStart(void);}
 */
void VuoCompilerBitcodeGenerator::generateInstanceTriggerStartFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getInstanceTriggerStartFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	if (isStatefulComposition)
	{
		map<VuoPort *, size_t> indexOfParameter;
		Function *nodeInstanceTriggerStartFunction = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStartFunction(module, moduleKey,
																													  VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																													  vector<VuoPort *>(),
																													  indexOfParameter,
																													  constantStrings);

		Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
		Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
		Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceTriggerStartFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionStateValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceTriggerStartFunction, args, "", block);

		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c vuoInstanceTriggerStop() function, which calls @c nodeInstanceTriggerStop() if this is a stateful composition.
 *
 * \eg{void vuoInstanceTriggerStop(void);}
 */
void VuoCompilerBitcodeGenerator::generateInstanceTriggerStopFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getInstanceTriggerStopFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	{
		Function *nodeInstanceTriggerStopFunction = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStopFunction(module, moduleKey,
																													VuoCompilerCodeGenUtilities::getCompositionInstanceDataType(module),
																													constantStrings);

		Value *topLevelRuntimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, block);
		Value *topLevelCompositionIdentifierValue = constantStrings.get(module, VuoCompilerComposition::topLevelCompositionIdentifier);
		Value *topLevelCompositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, block, topLevelRuntimeStateValue, topLevelCompositionIdentifierValue);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceTriggerStopFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionStateValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceTriggerStopFunction, args, "", block);

		VuoCompilerCodeGenUtilities::generateFreeCompositionState(module, block, topLevelCompositionStateValue);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates a trigger worker function for each trigger in this composition (not including triggers inside of subcomposition nodes).
 * If this is a top-level composition, also generates a trigger scheduler function for each trigger at all levels in this
 * composition (including triggers inside of subcomposition nodes).
 */
void VuoCompilerBitcodeGenerator::generateTriggerFunctions(void)
{
	auto isSpinOffTrigger = [] (const string &nodeClassName)
	{
		return (VuoStringUtilities::beginsWith(nodeClassName, "vuo.event.spinOff") ||
				VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.build") ||
				VuoStringUtilities::beginsWith(nodeClassName, "vuo.list.process"));
	};

	map<VuoCompilerTriggerPort *, Function *> workerFunctionForTrigger;
	for (VuoCompilerTriggerPort *trigger : graph->getTriggerPorts())
	{
		Function *workerFunction = generateTriggerWorkerFunction(trigger);
		workerFunctionForTrigger[trigger] = workerFunction;
	}

	if (isTopLevelComposition)
	{
		for (map<VuoCompilerTriggerPort *, Function *>::iterator i = workerFunctionForTrigger.begin(); i != workerFunctionForTrigger.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			Function *workerFunction = i->second;

			VuoType *dataType = trigger->getDataVuoType();

			VuoCompilerNode *node = graph->getNodeForTriggerPort(trigger);
			size_t triggerNodeIndex = node->getIndexInOrderedNodes();

			string portIdentifier = trigger->getIdentifier();

			int portContextIndex = trigger->getIndexInPortContexts();

			bool canDropEvents = (trigger->getBase()->getEventThrottling() == VuoPortClass::EventThrottling_Drop);

			bool isPublishedTrigger = (trigger == graph->getPublishedInputTrigger());

			bool isSpinOff = isSpinOffTrigger(node->getBase()->getNodeClass()->getClassName());

			int minThreadsNeeded, maxThreadsNeeded;
			graph->getWorkerThreadsNeeded(trigger, minThreadsNeeded, maxThreadsNeeded);

			int chainCount = (int)graph->getChains()[trigger].size();

			Function *function = generateTriggerSchedulerFunction(dataType, VuoCompilerComposition::topLevelCompositionIdentifier, triggerNodeIndex,
																  portIdentifier, portContextIndex, canDropEvents, isPublishedTrigger, isSpinOff,
																  minThreadsNeeded, maxThreadsNeeded, chainCount, workerFunction);
			topLevelTriggerFunctions[trigger] = function;
		}

		for (VuoCompilerNode *node : graph->getNodes())
		{
			VuoCompilerNodeClass *nodeClass = node->getBase()->getNodeClass()->getCompiler();

			string nodeIdentifier = VuoStringUtilities::buildCompositionIdentifier(VuoCompilerComposition::topLevelCompositionIdentifier,
																				  node->getGraphvizIdentifier());

			vector<VuoCompilerTriggerDescription *> triggers = nodeClass->getTriggerDescriptions();
			for (vector<VuoCompilerTriggerDescription *>::iterator j = triggers.begin(); j != triggers.end(); ++j)
			{
				VuoCompilerTriggerDescription *trigger = *j;

				size_t triggerNodeIndex = trigger->getNodeIndex();
				string triggerNodeIdentifier = trigger->getNodeIdentifier();
				string portIdentifier = VuoStringUtilities::buildPortIdentifier(triggerNodeIdentifier, trigger->getPortName());
				int portContextIndex = trigger->getPortContextIndex();
				bool canDropEvents = (trigger->getEventThrottling() == VuoPortClass::EventThrottling_Drop);
				VuoType *dataType = trigger->getDataType();
				string subcompositionNodeClassName = trigger->getSubcompositionNodeClassName();
				VuoCompilerNodeClass *subcompositionNodeClass = (subcompositionNodeClassName.empty() ?
																	 nodeClass :
																	 compiler->getNodeClass(subcompositionNodeClassName));
				string subcompositionNodeIdentifier = trigger->getSubcompositionNodeIdentifier();
				string fullSubcompositionNodeIdentifier = (subcompositionNodeIdentifier.empty() ?
															   nodeIdentifier :
															   VuoStringUtilities::buildCompositionIdentifier(nodeIdentifier, subcompositionNodeIdentifier));
				bool isPublishedTrigger = (triggerNodeIdentifier == graph->getPublishedInputTriggerNodeIdentifier());

				bool isSpinOff = isSpinOffTrigger(trigger->getNodeClassName());

				int minThreadsNeeded, maxThreadsNeeded;
				if (isPublishedTrigger)
					minThreadsNeeded = maxThreadsNeeded = -1;
				else
					trigger->getWorkerThreadsNeeded(minThreadsNeeded, maxThreadsNeeded);

				int chainCount = trigger->getChainCount();

				Function *workerFunctionSrc = subcompositionNodeClass->getTriggerWorkerFunction(portIdentifier);
				Function *workerFunction = VuoCompilerModule::declareFunctionInModule(module, workerFunctionSrc);

				Function *function = generateTriggerSchedulerFunction(dataType, fullSubcompositionNodeIdentifier, triggerNodeIndex,
																	  portIdentifier, portContextIndex, canDropEvents, isPublishedTrigger, isSpinOff,
																	  minThreadsNeeded, maxThreadsNeeded, chainCount, workerFunction);

				subcompositionTriggerFunctions[fullSubcompositionNodeIdentifier][triggerNodeIndex][trigger] = function;
			}
		}
	}
}

/**
 * Generates the trigger scheduler function for the trigger port described by the arguments.
 * This is the function that's called by a node implementation each time the node fires an event.
 * This function schedules the trigger worker function to execute soon after.
 *
 * @eg{
 * // PlayMovie:decodedImage -> TwirlImage:image
 * // PlayMovie:decodedImage -> RippleImage:image
 * // TwirlImage:image -> BlendImages:background
 * // RippleImage:image -> BlendImages:foreground
 *
 * void Top__PlayMovie_decodedImage(VuoImage image)
 * {
 *   dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, 0);
 *   int ret = dispatch_semaphore_wait(PlayMovie_decodedImage_semaphore, t);  // Only checked if the trigger can drop events.
 *   if (ret == 0)
 *   {
 *     VuoRetain(image);
 *     VuoImage *dataCopy = (VuoImage *)malloc(sizeof(VuoImage));
 *     *dataCopy = image;
 *
 *     dispatch_group_enter(vuoGetTriggerWorkersScheduled());
 *
 *     unsigned long eventId = vuoGetNextEventId();
 *     unsigned long *eventIdCopy = (unsigned long *)malloc(sizeof(unsigned long));
 *     *eventIdCopy = eventId;
 *
 *     void **context = (void **)malloc(3 * sizeof(void *));
 *     context[0] = (void *)compositionState;  // {vuoRuntimeState, "Top"}
 *     context[1] = (void *)dataCopy;
 *     context[2] = (void *)eventIdCopy;
 *     vuoScheduleTriggerWorker(PlayMovie_decodedImage_queue, (void *)context, PlayMovie_decodedImage, 1, 2, eventId, compositionIdentifier, 3);
 *   }
 *   else
 *   {
 *     // Drop the event.
 *     VuoRetain(image);
 *     VuoRelease(image);
 *     sendTelemetry(EventDropped, PlayMovie_decodedImage);
 *   }
 * }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateTriggerSchedulerFunction(VuoType *dataType,
																		 string compositionIdentifier, size_t nodeIndex,
																		 string portIdentifier, int portContextIndex,
																		 bool canDropEvents, bool isPublishedInputTrigger, bool isSpinOff,
																		 int minThreadsNeeded, int maxThreadsNeeded, int chainCount,
																		 Function *workerFunction)
{
	string functionName = VuoStringUtilities::prefixSymbolName(workerFunction->getName().str(),
															   VuoStringUtilities::transcodeToIdentifier(compositionIdentifier));
	FunctionType *functionType = VuoCompilerCodeGenUtilities::getFunctionType(module, dataType);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	if (dataType)
	{
		AttributeSet paramAttributes = dataType->getCompiler()->getFunctionParameterAttributes();
		function->setAttributes(paramAttributes);
	}

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);
	BasicBlock *scheduleBlock = BasicBlock::Create(module->getContext(), "schedule", function, NULL);

	Value *runtimeStateValue = VuoCompilerCodeGenUtilities::generateRuntimeStateValue(module, initialBlock);
	Value *compositionIdentifierValue = constantStrings.get(module, compositionIdentifier);

	Value *compositionStateValue = VuoCompilerCodeGenUtilities::generateCreateCompositionState(module, initialBlock, runtimeStateValue, compositionIdentifierValue);
	VuoCompilerCodeGenUtilities::generateRegisterCall(module, initialBlock, compositionStateValue, VuoCompilerCodeGenUtilities::getFreeFunction(module));
	VuoCompilerCodeGenUtilities::generateRetainCall(module, initialBlock, compositionStateValue);

	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, initialBlock, compositionStateValue, nodeIndex);
	Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, initialBlock, nodeContextValue, portContextIndex);

	if (canDropEvents)
	{
		BasicBlock *checkEventDropBlock = BasicBlock::Create(module->getContext(), "checkEventDrop", function, NULL);
		BranchInst::Create(checkEventDropBlock, initialBlock);

		// Do a non-blocking wait on the trigger's semaphore to check if it's already claimed. If so...
		Value *retValue = VuoCompilerTriggerPort::generateNonBlockingWaitForSemaphore(module, checkEventDropBlock, portContextValue);
		Constant *zeroValue = ConstantInt::get(retValue->getType(), 0);
		ICmpInst *isTriggerAvailableValue = new ICmpInst(*checkEventDropBlock, ICmpInst::ICMP_EQ, retValue, zeroValue, "");
		BasicBlock *dropEventBlock = BasicBlock::Create(module->getContext(), "dropEvent", function, NULL);
		BranchInst::Create(scheduleBlock, dropEventBlock, isTriggerAvailableValue, checkEventDropBlock);

		// Release the data value.
		if (dataType)
			VuoCompilerTriggerPort::generateDataValueDiscardFromScheduler(module, function, dropEventBlock, dataType);

		// Send telemetry that the event has been dropped.
		Constant *portIdentifierValue = constantStrings.get(module, portIdentifier);
		VuoCompilerCodeGenUtilities::generateSendEventDropped(module, dropEventBlock, compositionStateValue, portIdentifierValue);

		VuoCompilerCodeGenUtilities::generateReleaseCall(module, dropEventBlock, compositionStateValue);

		BranchInst::Create(finalBlock, dropEventBlock);
	}
	else
	{
		BranchInst::Create(scheduleBlock, initialBlock);
	}

	// Enter the trigger's dispatch group for tracking workers scheduled.
	Value *triggerWorkersScheduledValue = VuoCompilerCodeGenUtilities::getTriggerWorkersScheduledValue(module, scheduleBlock, compositionStateValue);
	VuoCompilerCodeGenUtilities::generateEnterDispatchGroup(module, scheduleBlock, triggerWorkersScheduledValue);

	Value *eventIdValue;
	if (! isPublishedInputTrigger)
	{
		// Get a unique ID for this event.
		eventIdValue = VuoCompilerCodeGenUtilities::generateGetNextEventId(module, scheduleBlock, compositionStateValue);

		// If this trigger fires in response to an input event, and the original event came from the published input trigger,
		// associate the new (spun off) event's ID with the original event.
		if (isSpinOff)
		{
			Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, scheduleBlock, compositionStateValue);
			VuoCompilerCodeGenUtilities::generateSpunOffExecutingEvent(module, scheduleBlock, compositionContextValue, eventIdValue);
		}
	}
	else
	{
		// Use the event ID from the parent composition or `firePublishedInputTrigger()`.
		Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, scheduleBlock, compositionStateValue);
		eventIdValue = VuoCompilerCodeGenUtilities::generateGetOneExecutingEvent(module, scheduleBlock, compositionContextValue);
	}

	// Schedule the trigger's worker function via `vuoScheduleTriggerWorker()`.
	VuoCompilerTriggerPort::generateScheduleWorker(module, function, scheduleBlock,
												   compositionStateValue, eventIdValue, portContextValue, dataType,
												   minThreadsNeeded, maxThreadsNeeded, chainCount, workerFunction);
	BranchInst::Create(finalBlock, scheduleBlock);

	ReturnInst::Create(module->getContext(), finalBlock);

	return function;
}

/**
 * Generates the trigger worker function, which schedules downstream nodes for execution.
 *
 * void PlayMovie_decodedImage(void *context)
 * {
 *   VuoCompositionState *compositionState = (VuoCompositionState *)((void **)context)[0];
 *   VuoImage dataCopy = (VuoImage)((void **)context)[1];
 *   unsigned long *eventIdCopy = (unsigned long *)((void **)context)[2];
 *   unsigned long eventId = *eventIdCopy;
 *
 *   // If paused, ignore this event. Not checked for the published input trigger of a subcomposition.
 *   if (vuoIsPaused())
 *   {
 *     free(dataCopy);
 *     free(context);
 *     vuoReturnThreadsForTriggerWorker(eventId);
 *     dispatch_semaphore_signal(PlayMovie_decodedImage_semaphore);
 *     dispatch_group_leave(vuoGetTriggerWorkersScheduled());
 *     return;
 *   }
 *   // Otherwise...
 *
 *   // Wait for the nodes directly downstream of the trigger port.
 *   waitForNodeSemaphore(PlayMovie, eventId);
 *   waitForNodeSemaphore(TwirlImage, eventId);
 *   waitForNodeSemaphore(RippleImage, eventId);
 *
 *   // Handle the trigger port value having changed.
 *   VuoRelease(PlayMovie_decodedImage__previousData);
 *   PlayMovie_decodedImage__previousData = (VuoImage)(*context);
 *   free(dataCopy);
 *   free(eventIdCopy);
 *   free(context);
 *   signalNodeSemaphore(PlayMovie);
 *   dispatch_semaphore_signal(PlayMovie_decodedImage_semaphore);
 *   dispatch_group_leave(vuoGetTriggerWorkersScheduled());
 *
 *   // Send telemetry indicating that the trigger port value may have changed.
 *   sendTelemetry(PortHadEvent, PlayMovie_decodedImage, TwirlImage_image);
 *
 *   // Transmit data and events along each of the trigger's cables.
 *   transmitDataAndEvent(PlayMovie_decodedImage__previousData, TwirlImage_image);  // retains new TwirlImage_image, releases old
 *
 *
 *   // Schedule the chains immediately downstream of the trigger.
 *
 *   unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long));
 *   *eventIdPtr = eventId;
 *
 *   size_t contextBytes = 2 * sizeof(void *);
 *   void **chainContext = (void **)malloc(contextBytes);
 *   chainContext[0] = (void *)compositionIdentifier;
 *   chainContext[1] = (void *)eventIdPtr;
 *   VuoRegister(chainContext, vuoFreeChainWorkerContext);
 *
 *   dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
 *   vuoScheduleChainWorker(globalQueue, chainContext, PlayMovie_decodedImage__TwirlImage__worker, 1, 1, eventId, compositionIdentifier, 0);
 *   vuoScheduleChainWorker(globalQueue, chainContext, PlayMovie_decodedImage__RippleImage__worker, 1, 1, eventId, compositionIdentifier, 1);
 *
 *   VuoRetain(chainContext);
 *   VuoRetain(chainContext);
 * }
 *
 * void vuoFreeChainWorkerContext(void *context)
 * {
 *   VuoRelease(((void **)context)[0]);
 *   free(((void **)context)[1]);
 *   free(context);
 * }
 *
 * void PlayMovie_decodedImage__TwirlImage__worker(void *context)
 * {
 *   unsigned long *eventIdPtr = (unsigned long *)((void **)context)[1];
 *   unsigned long eventId = *eventIdPtr;
 *
 *   // For each node in the chain...
 *   // If the node received an event, then...
 *
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
 *
 *   // Schedule the chains immediately downstream of this chain.
 *
 *   VuoRetain(context);
 *   vuoScheduleChainWorker(globalQueue, context, PlayMovie_decodedImage__BlendImages__worker, 1, 1, eventId, compositionIdentifier, 3);
 *
 *   // Clean up.
 *
 *   vuoReturnThreadsForChainWorker(eventId, compositionIdentifier, 0);
 *   VuoRelease(context);
 * }
 *
 * void PlayMovie_decodedImage__RippleImage__worker(void *context)
 * {
 *   unsigned long *eventIdPtr = (unsigned long *)((void **)context)[1];
 *   unsigned long eventId = *eventIdPtr;
 *
 *   // For each node in the chain...
 *   // If the node received an event, then...
 *
 *   ...
 *
 *   // Don't schedule any chains; this is handled by PlayMovie_decodedImage__TwirlImage__worker().
 *
 *   // Clean up.
 *
 *   vuoReturnThreadsForChainWorker(eventId, compositionIdentifier, 1);
 *   VuoRelease(context);
 * }
 *
 * void PlayMovie_decodedImage__BlendImages__worker(void *context)
 * {
 *   // VuoThreadManager waits to call this function until the chains upstream have completed.
 *
 *   ...
 *
 *   // Clean up.
 *
 *   vuoReturnThreadsForChainWorker(eventId, compositionIdentifier, 2);
 *   VuoRelease(context);
 * }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateTriggerWorkerFunction(VuoCompilerTriggerPort *trigger)
{
	string functionName = VuoStringUtilities::prefixSymbolName(VuoStringUtilities::transcodeToIdentifier(trigger->getIdentifier()), moduleKey);
	Function *function = trigger->getWorkerFunction(module, functionName, true);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *triggerBlock = BasicBlock::Create(module->getContext(), "trigger", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Value *compositionStateValue = trigger->generateCompositionStateValue(module, initialBlock, function);
	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, initialBlock, compositionStateValue);
	Value *eventIdValue = trigger->generateEventIdValue(module, initialBlock, function);

	VuoCompilerNode *triggerNode = graph->getNodeForTriggerPort(trigger);
	Value *triggerNodeContextValue = triggerNode->generateGetContext(module, initialBlock, compositionStateValue);
	Value *triggerWorkersScheduledValue = VuoCompilerCodeGenUtilities::getTriggerWorkersScheduledValue(module, initialBlock, compositionStateValue);
	bool canDropEvents = (trigger->getBase()->getEventThrottling() == VuoPortClass::EventThrottling_Drop);

	bool isPublishedInputTrigger = (trigger == graph->getPublishedInputTrigger());
	bool isNodeEventForSubcomposition = (! isTopLevelComposition && isPublishedInputTrigger);

	if (! isNodeEventForSubcomposition)
	{
		// Check if `isPaused` is true. If so...
		BasicBlock *isPausedBlock = BasicBlock::Create(module->getContext(), "isPaused", function, NULL);
		ICmpInst *isPausedValueIsTrue = VuoCompilerCodeGenUtilities::generateIsPausedComparison(module, initialBlock, compositionStateValue);
		BranchInst::Create(isPausedBlock, triggerBlock, isPausedValueIsTrue, initialBlock);

		// Release the data value.
		trigger->generateDataValueDiscardFromWorker(module, isPausedBlock, function);

		// Wait for the published output node's semaphore, if not already claimed in `firePublishedInputPortEvent()`.
		VuoCompilerNode *publishedOutputNode = graph->getPublishedOutputNode();
		vector<VuoCompilerNode *> publishedOutputNodeVector(1, publishedOutputNode);
		if (publishedOutputNode && ! isPublishedInputTrigger)
			generateWaitForNodes(module, function, isPausedBlock, compositionStateValue, publishedOutputNodeVector);

		// Call `vuoSendEventFinished()`.
		VuoCompilerCodeGenUtilities::generateSendEventFinished(module, isPausedBlock, compositionStateValue, eventIdValue);

		if (isPublishedInputTrigger)
		{
			// Signal the semaphores claimed in `firePublishedInputPortEvent()`.
			vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
			generateSignalForNodes(module, isPausedBlock, compositionStateValue, triggerWaitNodes);
		}

		// Signal the published output node's semaphore.
		if (publishedOutputNode)
			generateSignalForNodes(module, isPausedBlock, compositionStateValue, publishedOutputNodeVector);

		if (canDropEvents)
			// Signal the trigger's semaphore for event dropping.
			trigger->generateSignalForSemaphore(module, isPausedBlock, triggerNodeContextValue);

		// Leave the trigger's dispatch group for tracking workers scheduled.
		VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, isPausedBlock, triggerWorkersScheduledValue);

		// Call `vuoReturnThreadsForTriggerWorker()`.
		VuoCompilerCodeGenUtilities::generateReturnThreadsForTriggerWorker(module, isPausedBlock, eventIdValue, compositionStateValue);

		BranchInst::Create(finalBlock, isPausedBlock);
		// Otherwise...
	}
	else
	{
		BranchInst::Create(triggerBlock, initialBlock);
	}

	if (isPublishedInputTrigger)
	{
		if (! isNodeEventForSubcomposition)
		{
			// Signal the published output node if it was waited on in `firePublishedInputPortEvent()` just to track event start/finished.
			vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
			VuoCompilerNode *publishedOutputNode = graph->getPublishedOutputNode();
			if (find(triggerWaitNodes.begin(), triggerWaitNodes.end(), publishedOutputNode) == triggerWaitNodes.end())
			{
				vector<VuoCompilerNode *> publishedOutputNodeVector(1, publishedOutputNode);
				generateSignalForNodes(module, triggerBlock, compositionStateValue, publishedOutputNodeVector);
			}
		}
	}
	else
	{
		// Claim the semaphores of all necessary downstream nodes.
		vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
		generateWaitForNodes(module, function, triggerBlock, compositionStateValue, triggerWaitNodes, eventIdValue);

		// Update the trigger's data value.
		Value *triggerDataValue = trigger->generateDataValueUpdate(module, triggerBlock, function, triggerNodeContextValue);

		// Transmit events and data (if any) out of the trigger port, and send telemetry for port updates.
		generateTransmissionFromOutputPort(function, triggerBlock, compositionStateValue, trigger, NULL, triggerDataValue);
	}

	// If the trigger node isn't downstream of the trigger, signal the trigger node's semaphore.
	vector<VuoCompilerNode *> downstreamNodes = graph->getNodesDownstream(trigger);
	if (find(downstreamNodes.begin(), downstreamNodes.end(), triggerNode) == downstreamNodes.end())
		generateSignalForNodes(module, triggerBlock, compositionStateValue, vector<VuoCompilerNode *>(1, triggerNode));

	if (canDropEvents)
		// Signal the trigger's semaphore for event dropping.
		trigger->generateSignalForSemaphore(module, triggerBlock, triggerNodeContextValue);

	// Leave the trigger's dispatch group for tracking workers scheduled.
	VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, triggerBlock, triggerWorkersScheduledValue);


	// Schedule the chain worker function for each chain immediately downstream of the trigger.

	map<VuoCompilerChain *, vector<VuoCompilerChain *> > chainsImmediatelyDownstream;
	map<VuoCompilerChain *, vector<VuoCompilerChain *> > chainsImmediatelyUpstream;
	set<VuoCompilerChain *> chainsScheduled;

	vector<VuoCompilerChain *> allChains = chainsForTrigger[trigger];

	if (! allChains.empty())
	{
		// Organize the chains so it's easy to look up what's downstream/upstream of what.
		for (vector<VuoCompilerChain *>::iterator i = allChains.begin(); i != allChains.end(); ++i)
		{
			VuoCompilerChain *chain = *i;
			VuoCompilerNode *firstNodeInThisChain = chain->getNodes().front();

			for (vector<VuoCompilerChain *>::iterator j = allChains.begin(); j != allChains.end(); ++j)
			{
				VuoCompilerChain *otherChain = *j;

				if (chain == otherChain)
					break;  // Any chains after this are downstream.

				VuoCompilerNode *lastNodeInOtherChain = otherChain->getNodes().back();

				if (graph->mayTransmit(lastNodeInOtherChain, firstNodeInThisChain, trigger))
				{
					chainsImmediatelyUpstream[chain].push_back(otherChain);
					chainsImmediatelyDownstream[otherChain].push_back(chain);
				}
			}
		}

		// Create the context to pass to the chain workers.
		Value *contextValue = VuoCompilerChain::generateMakeContext(module, triggerBlock, compositionStateValue, eventIdValue);

		// Find all chains immediately downstream of the trigger (i.e., chains that have no other chains upstream).
		vector<VuoCompilerChain *> firstChains;
		for (vector<VuoCompilerChain *>::iterator i = allChains.begin(); i != allChains.end(); ++i)
		{
			VuoCompilerChain *chain = *i;
			map<VuoCompilerChain *, vector<VuoCompilerChain *> >::iterator upstreamIter = chainsImmediatelyUpstream.find(chain);
			if (upstreamIter == chainsImmediatelyUpstream.end())
				firstChains.push_back(chain);
		}

		// Choose one chain to execute in the trigger worker, to reduce overhead creating/destroying threads.
		VuoCompilerChain *chainToExecute = firstChains.back();
		firstChains.pop_back();
		chainsScheduled.insert(chainToExecute);
		size_t chainIndex = find(allChains.begin(), allChains.end(), chainToExecute) - allChains.begin();
		VuoCompilerCodeGenUtilities::generateRetainCall(module, triggerBlock, contextValue);

		// Call `vuoGrantThreadsToChain()` for the chosen chain.
		int minThreadsNeeded, maxThreadsNeeded;
		graph->getWorkerThreadsNeeded(chainToExecute, minThreadsNeeded, maxThreadsNeeded);
		VuoCompilerCodeGenUtilities::generateGrantThreadsToChain(module, triggerBlock, minThreadsNeeded, maxThreadsNeeded,
																 eventIdValue, compositionStateValue, chainIndex);

		// Schedule the rest of the chains immediately downstream of the trigger.
		generateAndScheduleChainWorkerFunctions(triggerBlock, compositionStateValue, contextValue, firstChains, trigger,
												allChains, chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled);

		// Execute the chosen chain.
		generateChainExecution(function, triggerBlock, compositionStateValue, contextValue, eventIdValue, chainToExecute, trigger,
							   allChains, chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled);
		VuoCompilerCodeGenUtilities::generateReleaseCall(module, triggerBlock, contextValue);
	}
	else
	{
		// Call `vuoReturnThreadsForTriggerWorker()`.
		VuoCompilerCodeGenUtilities::generateReturnThreadsForTriggerWorker(module, triggerBlock, eventIdValue, compositionStateValue);

		if (isNodeEventForSubcomposition)
		{
			// Leave the dispatch group waited on by `nodeEvent()`/`nodeInstanceEvent()`.
			Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, triggerBlock, compositionContextValue);
			VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, triggerBlock, subcompositionOutputGroupValue);
		}
	}

	BranchInst::Create(finalBlock, triggerBlock);

	// Free the trigger worker's context.
	trigger->generateFreeContext(module, finalBlock, function);
	VuoCompilerCodeGenUtilities::generateReleaseCall(module, finalBlock, compositionStateValue);

	ReturnInst::Create(module->getContext(), finalBlock);

	return function;
}

/**
 * Generates and schedules a chain worker function for each of @a chainsToSchedule.
 */
void VuoCompilerBitcodeGenerator::generateAndScheduleChainWorkerFunctions(BasicBlock *schedulerBlock,
																		  Value *compositionStateValueInScheduler, Value *contextValueInScheduler,
																		  const vector<VuoCompilerChain *> &chainsToSchedule, VuoCompilerTriggerPort *trigger,
																		  const vector<VuoCompilerChain *> &allChains,
																		  const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream,
																		  const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
																		  set<VuoCompilerChain *> &chainsScheduled)
{
	// Find the chains in chainsToSchedule that haven't already been scheduled.
	vector<VuoCompilerChain *> uniqueChainsToSchedule;
	for (vector<VuoCompilerChain *>::const_iterator i = chainsToSchedule.begin(); i != chainsToSchedule.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		if (chainsScheduled.find(chain) == chainsScheduled.end())
		{
			uniqueChainsToSchedule.push_back(chain);
			chainsScheduled.insert(chain);
		}
	}

	// Retain the context once for each chain to be scheduled.
	for (vector<VuoCompilerChain *>::const_iterator i = uniqueChainsToSchedule.begin(); i != uniqueChainsToSchedule.end(); ++i)
		VuoCompilerCodeGenUtilities::generateRetainCall(module, schedulerBlock, contextValueInScheduler);

	// Schedule each chain.
	for (vector<VuoCompilerChain *>::const_iterator i = uniqueChainsToSchedule.begin(); i != uniqueChainsToSchedule.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		generateAndScheduleChainWorkerFunction(schedulerBlock, compositionStateValueInScheduler, contextValueInScheduler,
											   chain, trigger, allChains, chainsImmediatelyDownstream, chainsImmediatelyUpstream,
											   chainsScheduled);
	}
}

/**
 * Generates and schedules a chain worker function.
 */
void VuoCompilerBitcodeGenerator::generateAndScheduleChainWorkerFunction(BasicBlock *schedulerBlock,
																		 Value *compositionStateValueInScheduler, Value *contextValueInScheduler,
																		 VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger,
																		 const vector<VuoCompilerChain *> &allChains,
																		 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream,
																		 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
																		 set<VuoCompilerChain *> &chainsScheduled)
{
	int minThreadsNeeded, maxThreadsNeeded;
	graph->getWorkerThreadsNeeded(chain, minThreadsNeeded, maxThreadsNeeded);

	size_t chainIndex = find(allChains.begin(), allChains.end(), chain) - allChains.begin();

	vector<VuoCompilerChain *> upstreamChains;
	map<VuoCompilerChain *, vector<VuoCompilerChain *> >::const_iterator upstreamChainsIter = chainsImmediatelyUpstream.find(chain);
	if (upstreamChainsIter != chainsImmediatelyUpstream.end())
		upstreamChains = upstreamChainsIter->second;

	vector<size_t> upstreamChainIndices;
	for (vector<VuoCompilerChain *>::iterator i = upstreamChains.begin(); i != upstreamChains.end(); ++i)
		upstreamChainIndices.push_back( find(allChains.begin(), allChains.end(), *i) - allChains.begin() );

	// Call `vuoScheduleChainWorker` for the worker function implemented below.
	Function *chainWorker = chain->generateScheduleWorker(module, schedulerBlock, compositionStateValueInScheduler, contextValueInScheduler,
														  trigger->getIdentifier(), minThreadsNeeded, maxThreadsNeeded, chainIndex,
														  upstreamChainIndices);

	BasicBlock *chainBlock = BasicBlock::Create(module->getContext(), "", chainWorker, 0);
	Value *contextValueInChainWorker = chainWorker->arg_begin();
	Value *compositionStateValueInChainWorker = chain->generateCompositionStateValue(module, chainBlock, contextValueInChainWorker);
	Value *eventIdValue = chain->generateEventIdValue(module, chainBlock, contextValueInChainWorker);

	// Execute the chain.
	generateChainExecution(chainWorker, chainBlock, compositionStateValueInChainWorker, contextValueInChainWorker, eventIdValue, chain, trigger,
						   allChains, chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled);

	// Release the chain worker's context.
	VuoCompilerCodeGenUtilities::generateReleaseCall(module, chainBlock, contextValueInChainWorker);

	ReturnInst::Create(module->getContext(), chainBlock);
}

/**
 * Generates code to execute each node that got an event in the given chain.
 */
void VuoCompilerBitcodeGenerator::generateChainExecution(Function *function, BasicBlock *&block,
														 Value *compositionStateValue, Value *contextValue,
														 Value *eventIdValue, VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger,
														 const vector<VuoCompilerChain *> &allChains,
														 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream,
														 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
														 set<VuoCompilerChain *> &chainsScheduled)
{
	size_t chainIndex = find(allChains.begin(), allChains.end(), chain) - allChains.begin();
	Value *chainIndexValue = ConstantInt::get(eventIdValue->getType(), chainIndex);

	// For each node in the chain...
	vector<VuoCompilerNode *> chainNodes = chain->getNodes();
	for (vector<VuoCompilerNode *>::iterator i = chainNodes.begin(); i != chainNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		Function *nodeExecutionFunction = executionFunctionForNode[node];
		if (! nodeExecutionFunction)
		{
			nodeExecutionFunction = generateNodeExecutionFunction(module, node);
			executionFunctionForNode[node] = nodeExecutionFunction;
		}

		Function *nodeTransmissionFunction = transmissionFunctionForNode[node];
		if (! nodeTransmissionFunction)
		{
			nodeTransmissionFunction = generateNodeTransmissionFunction(module, node);
			transmissionFunctionForNode[node] = nodeTransmissionFunction;
		}

		// If the event hit the node, call its event function and send telemetry.
		vector<Value *> nodeExecutionArgs;
		nodeExecutionArgs.push_back(compositionStateValue);
		nodeExecutionArgs.push_back(eventIdValue);
		nodeExecutionArgs.push_back(chainIndexValue);
		CallInst *isHitValue = CallInst::Create(nodeExecutionFunction, nodeExecutionArgs, "", block);

		// Whether or not the event hit the node, wait on any necessary downstream nodes.
		if (! (graph->isRepeatedInFeedbackLoop(node, trigger) && chain->isLastNodeInLoop()))
		{
			vector<VuoCompilerNode *> outputNodes = getNodesToWaitOnBeforeTransmission(trigger, node);
			generateWaitForNodes(module, function, block, compositionStateValue, outputNodes, eventIdValue);
		}

		// If the event hit the node, transmit events and data through its output cables and send telemetry.
		vector<Value *> nodeTransmissionArgs;
		nodeTransmissionArgs.push_back(compositionStateValue);
		nodeTransmissionArgs.push_back(isHitValue);
		CallInst::Create(nodeTransmissionFunction, nodeTransmissionArgs, "", block);

		// Whether or not the event hit the node, if this was the last time this event could reach the node,
		// signal the node's semaphore.
		if (! (graph->isRepeatedInFeedbackLoop(node, trigger) && ! chain->isLastNodeInLoop()))
		{
			// Special case: If this is the published output node in a subcomposition,
			// the node's semaphore is signaled in the node's execution function or the subcomposition's nodeEvent()/nodeInstanceEvent().
			if (! (! isTopLevelComposition && node == graph->getPublishedOutputNode()))
				generateSignalForNodes(module, block, compositionStateValue, vector<VuoCompilerNode *>(1, node));
		}
	}

	// Schedule any chains immediately downstream, if this chain is the one responsible for doing so.
	vector<VuoCompilerChain *> downstreamChains;
	map<VuoCompilerChain *, vector<VuoCompilerChain *> >::const_iterator downstreamChainsIter = chainsImmediatelyDownstream.find(chain);
	if (downstreamChainsIter != chainsImmediatelyDownstream.end())
		downstreamChains = downstreamChainsIter->second;
	if (! downstreamChains.empty())
	{
		vector<size_t> downstreamChainIndices;
		for (vector<VuoCompilerChain *>::iterator i = downstreamChains.begin(); i != downstreamChains.end(); ++i)
			downstreamChainIndices.push_back( find(allChains.begin(), allChains.end(), *i) - allChains.begin() );

		vector<VuoCompilerChain *> nextChains;
		for (vector<VuoCompilerChain *>::iterator i = downstreamChains.begin(); i != downstreamChains.end(); ++i)
		{
			VuoCompilerChain *downstreamChain = *i;
			nextChains.push_back(downstreamChain);
		}

		generateAndScheduleChainWorkerFunctions(block, compositionStateValue, contextValue, nextChains, trigger, allChains,
												chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled);
	}

	// Return the threads used by this chain to the thread pool.
	VuoCompilerCodeGenUtilities::generateReturnThreadsForChainWorker(module, block, eventIdValue, compositionStateValue, chainIndexValue);
}

/**
 * Generates a function that executes the node if the node received an event.
 *
 * @eg{
 * bool vuo_math_subtract__Subtract__execute(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex)
 * {
 *   bool isHit = vuo_math_subtract__Subtract__refresh_event ||
 *                vuo_math_subtract__Subtract__a_event ||
 *                vuo_math_subtract__Subtract__b_event;
 *
 *   if (isHit)
 *   {
 *     sendNodeExecutionStarted("vuo_math_subtract__Subtract");
 *     vuo_math_subtract__Subtract__nodeEvent(vuo_math_subtract__Subtract__a,
 *                                            vuo_math_subtract__Subtract__b,
 *                                            &vuo_math_subtract__Subtract__difference);
 *     sendNodeExecutionFinished("vuo_math_subtract__Subtract");
 *   }
 *
 *   return isHit;
 * }
 *
 * bool my_subcomposition__Subcomposition__execute(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex)
 * {
 *   ...
 *   if (isHit)
 *   {
 *     vuoSetNodeContextExecutingEventId(nodeContext, eventId);
 *     vuoGrantThreadsToSubcomposition(runtimeState, eventId, compositionIdentifier, chainIndex, "my_subcomposition__Subcomposition");
 *     ...
 *   }
 *   return isHit
 * }
 *
 * bool vuo_out__PublishedOutputs__execute(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex)
 * {
 *   bool isHit = vuo_out__PublishedOutputs__a_event;
 *
 *   if (eventId == subcompositionEventId)
 *     dispatch_group_leave(subcompositionExecuted__group);
 *
 *   return isHit;
 * }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateNodeExecutionFunction(Module *module, VuoCompilerNode *node)
{
	string functionName = node->getIdentifier() + "__execute";
	PointerType *pointerToCompositionStateType = PointerType::get(VuoCompilerCodeGenUtilities::getCompositionStateType(module), 0);
	Type *boolType = IntegerType::get(module->getContext(), 1);
	Type *eventIdType = VuoCompilerCodeGenUtilities::generateNoEventIdConstant(module)->getType();
	vector<Type *> params;
	params.push_back(pointerToCompositionStateType);
	params.push_back(eventIdType);
	params.push_back(eventIdType);
	FunctionType *functionType = FunctionType::get(boolType, params, false);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *eventIdValue = args++;
	eventIdValue->setName("eventId");
	Value *chainIndexValue = args++;
	chainIndexValue->setName("chainIndex");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Value *nodeContextValue = node->generateGetContext(module, initialBlock, compositionStateValue);
	Value *isHitValue = node->generateReceivedEventCondition(module, initialBlock, nodeContextValue);

	if (node == graph->getPublishedOutputNode())
	{
		if (isTopLevelComposition)
		{
			VuoCompilerCodeGenUtilities::generateSendEventFinished(module, initialBlock, compositionStateValue, eventIdValue);

			BranchInst::Create(finalBlock, initialBlock);
		}
		else
		{
			// Call the trigger functions for any published trigger ports that the event has hit.

			BasicBlock *currBlock = initialBlock;

			Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetCompositionContext(module, currBlock, compositionStateValue);

			vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
			vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();

			set<string> publishedOutputTriggerNames = graph->getPublishedOutputTriggers();

			for (size_t publishedPortIndex = 0; publishedPortIndex < publishedOutputPorts.size(); ++publishedPortIndex)
			{
				VuoPort *port = graph->getInputPortOnPublishedOutputNode(publishedPortIndex);

				Value *isPortHitValue = node->generateReceivedEventCondition(module, currBlock, nodeContextValue, vector<VuoPort *>(1, port));

				if (publishedOutputTriggerNames.find( port->getClass()->getName() ) != publishedOutputTriggerNames.end())
				{
					BasicBlock *triggerBlock = BasicBlock::Create(module->getContext(), "trigger", function, NULL);
					BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "next", function, NULL);
					BranchInst::Create(triggerBlock, nextBlock, isPortHitValue, currBlock);

					VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>( port->getCompiler() );
					VuoCompilerInputEventPortClass *eventPortClass = static_cast<VuoCompilerInputEventPortClass *>( port->getClass()->getCompiler() );
					VuoType *dataType = eventPort->getDataVuoType();

					FunctionType *triggerFunctionType = VuoCompilerCodeGenUtilities::getFunctionType(module, dataType);

					vector<Value *> args;
					if (dataType)
					{
						Value *dataValue = eventPort->generateLoadData(module, triggerBlock, nodeContextValue);
						bool isPassedByValue = dataType->getCompiler()->getFunctionParameterAttributes().hasAttrSomewhere(Attribute::ByVal);
						bool isLoweredToTwoParameters = eventPortClass->getDataClass()->isLoweredToTwoParameters();
						Value *secondArg = NULL;
						Value **secondArgIfNeeded = (isLoweredToTwoParameters ? &secondArg : NULL);
						Value *arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(dataValue, triggerFunctionType, 0, isPassedByValue,
																								 secondArgIfNeeded, module, triggerBlock);
						args.push_back(arg);
						if (secondArg)
							args.push_back(secondArg);
					}

					int indexInSubcompositionPorts = VuoNodeClass::unreservedInputPortStartIndex + publishedInputPorts.size() +
													 VuoNodeClass::unreservedOutputPortStartIndex + publishedPortIndex;

					Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, triggerBlock, compositionContextValue, indexInSubcompositionPorts);
					Value *triggerFunction = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerFunction(module, triggerBlock, portContextValue, triggerFunctionType);

					CallInst::Create(triggerFunction, args, "", triggerBlock);
					BranchInst::Create(nextBlock, triggerBlock);

					currBlock = nextBlock;
				}
				else
				{
					VuoCompilerCodeGenUtilities::generateSetNodeContextOutputEvent(module, currBlock, compositionContextValue, publishedPortIndex, isPortHitValue);
				}
			}

			// If this event (which may or may not have actually hit the published output node) is from
			// nodeEvent()/nodeInstanceEvent() or an event spun off of it, and is the final one to complete, then...

			Value *subcompositionFinishedValue = VuoCompilerCodeGenUtilities::generateFinishedExecutingEvent(module, currBlock, compositionContextValue, eventIdValue);
			ConstantInt *falseValue = ConstantInt::get(static_cast<IntegerType *>(subcompositionFinishedValue->getType()), 0);
			ICmpInst *subcompositionFinishedIsTrue = new ICmpInst(*currBlock, ICmpInst::ICMP_NE, subcompositionFinishedValue, falseValue, "");
			BasicBlock *leaveBlock = BasicBlock::Create(module->getContext(), "leave", function, NULL);
			BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "signal", function, NULL);
			BranchInst::Create(leaveBlock, signalBlock, subcompositionFinishedIsTrue, currBlock);

			// Leave the dispatch group waited on by nodeEvent()/nodeInstanceEvent().

			Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, leaveBlock, compositionContextValue);
			VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, leaveBlock, subcompositionOutputGroupValue);
			BranchInst::Create(finalBlock, leaveBlock);

			// Otherwise, signal the published output node's semaphore so that the remaining events can claim it.
			// (For the final event, the published output node's semaphore is signaled in nodeEvent()/nodeInstanceEvent().)

			generateSignalForNodes(module, signalBlock, compositionStateValue, vector<VuoCompilerNode *>(1, node));
			BranchInst::Create(finalBlock, signalBlock);
		}
	}
	else
	{
		// If the node received an event, then...
		BasicBlock *executeBlock = BasicBlock::Create(module->getContext(), "execute", function, NULL);
		BranchInst::Create(executeBlock, finalBlock, isHitValue, initialBlock);

		if (node->getBase()->getNodeClass()->getCompiler()->isSubcomposition())
		{
			// Pass the event ID to the subcomposition.
			VuoCompilerCodeGenUtilities::generateStartedExecutingEvent(module, executeBlock, nodeContextValue, eventIdValue);

			// Pass the chain's reserved threads to the subcomposition.
			Value *compositionIdentifierValue = VuoCompilerCodeGenUtilities::generateGetCompositionStateCompositionIdentifier(module, executeBlock, compositionStateValue);
			Value *subcompositionIdentifierValue = node->generateSubcompositionIdentifierValue(module, executeBlock, compositionIdentifierValue);
			VuoCompilerCodeGenUtilities::generateGrantThreadsToSubcomposition(module, executeBlock,
																			  eventIdValue, compositionStateValue, chainIndexValue,
																			  subcompositionIdentifierValue);
			VuoCompilerCodeGenUtilities::generateFreeCall(module, executeBlock, subcompositionIdentifierValue);
		}

		// Call the node's event function, and send telemetry that the node's execution has started and finished.
		bool shouldSendTelemetry = (node != graph->getPublishedInputNode());
		generateNodeExecution(function, executeBlock, compositionStateValue, node, shouldSendTelemetry);
		BranchInst::Create(finalBlock, executeBlock);
	}

	ReturnInst::Create(module->getContext(), isHitValue, finalBlock);

	return function;
}

/**
 * Generates a function that transmits data and events from the node's output ports after the node has executed.
 *
 * @eg{
 * void vuo_math_subtract__Subtract__transmit(VuoCompositionState *compositionState, bool isHit)
 * {
 *   if (isHit)
 *   {
 *     sendOutputPortsUpdated("vuo_math_subtract__Subtract__difference", ...);
 *
 *     if (vuo_math_subtract__Subtract__difference_event)
 *     {
 *       vuo_math_divide__Divide__a = vuo_math_subtract__Subtract__difference;
 *       sendInputPortsUpdated("vuo_math_divide__Divide__a", ...);
 *     }
 *
 *     vuo_math_subtract__Subtract__refresh_event = false;
 *     vuo_math_subtract__Subtract__a_event = false;
 *     vuo_math_subtract__Subtract__b_event = false;
 *     vuo_math_subtract__Subtract__difference = false;
 *   }
 * }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateNodeTransmissionFunction(Module *module, VuoCompilerNode *node)
{
	string functionName = node->getIdentifier() + "__transmit";
	PointerType *pointerToCompositionStateType = PointerType::get(VuoCompilerCodeGenUtilities::getCompositionStateType(module), 0);
	Type *boolType = IntegerType::get(module->getContext(), 1);
	vector<Type *> params;
	params.push_back(pointerToCompositionStateType);
	params.push_back(boolType);
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionStateValue = args++;
	compositionStateValue->setName("compositionState");
	Value *isHitValue = args++;
	isHitValue->setName("isHit");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *transmitBlock = BasicBlock::Create(module->getContext(), "transmit", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	// If the node received an event, then...
	BranchInst::Create(transmitBlock, finalBlock, isHitValue, initialBlock);

	Value *nodeContextValue = node->generateGetContext(module, transmitBlock, compositionStateValue);

	if (node == graph->getPublishedOutputNode())
	{
		if (isTopLevelComposition)
			generateTelemetryFromPublishedOutputNode(function, transmitBlock, compositionStateValue, nodeContextValue, node);
	}
	else
	{
		// Transmit events and data through the node's outgoing cables, and send telemetry for port updates.
		generateTransmissionFromNode(function, transmitBlock, compositionStateValue, nodeContextValue, node);
	}

	// Reset the node's event inputs and outputs.
	VuoCompilerCodeGenUtilities::generateResetNodeContextEvents(module, transmitBlock, nodeContextValue);

	BranchInst::Create(finalBlock, transmitBlock);
	ReturnInst::Create(module->getContext(), finalBlock);

	return function;
}

/**
 * Turn debug mode on/off. In debug mode, print statements are inserted into the generated bitcode.
 */
void VuoCompilerBitcodeGenerator::setDebugMode(bool debugMode)
{
	this->debugMode = debugMode;
}
