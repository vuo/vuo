/**
 * @file
 * VuoCompilerBitcodeGenerator implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stack>
#include "VuoCompiler.hh"
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerConstantStringCache.hh"
#include "VuoCompilerDataClass.hh"
#include "VuoCompilerEventPortClass.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerInputData.hh"
#include "VuoCompilerInputDataClass.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerPublishedPort.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoCompilerType.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"

const string VuoCompilerBitcodeGenerator::topLevelCompositionIdentifier = "Top";

/**
 * Creates a bitcode generator from the specified composition.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition,
																							  bool isTopLevelComposition, bool isLiveCodeable,
																							  string moduleKey, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator(composition, isTopLevelComposition, isLiveCodeable, moduleKey, compiler);
	return cg;
}

/**
 * Private constructor.
 */
VuoCompilerBitcodeGenerator::VuoCompilerBitcodeGenerator(VuoCompilerComposition *composition,
														 bool isTopLevelComposition, bool isLiveCodeable,
														 string moduleKey, VuoCompiler *compiler)
{
	module = NULL;
	debugMode = false;

	this->composition = composition;
	this->isTopLevelComposition = isTopLevelComposition;
	this->isLiveCodeable = isLiveCodeable;
	this->moduleKey = moduleKey;
	this->compiler = compiler;

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedOutputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedInputPorts();
	if (isTopLevelComposition && publishedInputPorts.empty() && publishedOutputPorts.empty())
	{
		publishedInputNode = NULL;
		publishedOutputNode = NULL;
		graph = new VuoCompilerGraph(composition, NULL, NULL);
	}
	else
	{
		publishedInputNode = compiler->createPublishedInputNode(composition->getBase()->getPublishedInputPorts());
		publishedOutputNode = compiler->createPublishedOutputNode(composition->getBase()->getPublishedOutputPorts());
		graph = new VuoCompilerGraph(composition, publishedInputNode->getCompiler(), publishedOutputNode->getCompiler());
	}
	chainsForTrigger = graph->getChains();  // store in a data member, rather than calling getChains() multiple times, to preserve order of chains
	makeOrderedNodes();
	makeOrderedTypes();
	makePublishedOutputTriggers();
	makePortContextInfo();
	makeSubcompositionModelPorts();
}

/**
 * Destructor.
 */
VuoCompilerBitcodeGenerator::~VuoCompilerBitcodeGenerator(void)
{
	delete graph;

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

	// For each node that can transmit eventlessly, put it and its downstream nodes in with the triggers' downstream nodes.
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();

		if (graph->mayTransmitEventlessly(node))
		{
			vector<VuoCompilerNode *> eventlesslyDownstreamNodes = graph->getNodesEventlesslyDownstream(node);
			vector<VuoCompilerNode *> nodesInProgress;
			nodesInProgress.push_back(node);
			nodesInProgress.insert(nodesInProgress.end(), eventlesslyDownstreamNodes.begin(), eventlesslyDownstreamNodes.end());
			orderedNodesPerTrigger.push_back(nodesInProgress);
		}
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

	// Add (at the end) any remaining nodes in the composition.
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		if (find(orderedNodes.begin(), orderedNodes.end(), node) == orderedNodes.end())
			orderedNodes.push_back(node);
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
	// Does the trigger port have a scatter with a gather downstream, and another trigger port overlaps with some branches
	// of the scatter but not others?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents deadlock where the other trigger's event gets into its branch while this trigger's event gets into
	// the other branch and claims the gather node (https://b33p.net/kosada/node/6696).

	bool hasGatherOverlappedByAnotherTrigger = graph->hasGatherOverlappedByAnotherTrigger(trigger);


	// Would the trigger port wait on nodes in a different order than orderedNodes?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents deadlock where the events from two different trigger ports reach the downstream nodes in a different order
	// (https://b33p.net/kosada/node/7924).

	vector<VuoCompilerNode *> downstreamNodesAndTriggerNode;
	vector<VuoCompilerChain *> chains = chainsForTrigger[trigger];
	for (vector<VuoCompilerChain *>::iterator i = chains.begin(); i != chains.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		vector<VuoCompilerNode *> chainNodes = chain->getNodes();
		for (vector<VuoCompilerNode *>::iterator j = chainNodes.begin(); j != chainNodes.end(); ++j)
			if (find(downstreamNodesAndTriggerNode.begin(), downstreamNodesAndTriggerNode.end(), *j) == downstreamNodesAndTriggerNode.end())
				downstreamNodesAndTriggerNode.push_back(*j);
	}

	VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];
	vector<VuoCompilerNode *>::iterator triggerNodeIter = find(downstreamNodesAndTriggerNode.begin(), downstreamNodesAndTriggerNode.end(), triggerNode);
	if (triggerNodeIter != downstreamNodesAndTriggerNode.end())
		downstreamNodesAndTriggerNode.erase(triggerNodeIter);

	// Wait for the node containing the trigger port, to avoid firing corrupted data if the trigger fires
	// on its own at the same time that it's being manually fired (https://b33p.net/kosada/node/6497).
	downstreamNodesAndTriggerNode.insert(downstreamNodesAndTriggerNode.begin(), triggerNode);

	vector<VuoCompilerNode *> sortedDownstreamNodes = downstreamNodesAndTriggerNode;
	sortNodes(sortedDownstreamNodes);
	bool hasOutOfOrderDownstreamNodes = (downstreamNodesAndTriggerNode != sortedDownstreamNodes);

	// Wait for either all nodes downstream of the trigger or the nodes directly connected to the trigger.
	vector<VuoCompilerNode *> nodesToWaitOn;
	if (hasGatherOverlappedByAnotherTrigger || hasOutOfOrderDownstreamNodes)
		nodesToWaitOn = downstreamNodesAndTriggerNode;
	else
	{
		nodesToWaitOn = graph->getNodesImmediatelyDownstream(trigger);
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
	// Does the node have a scatter with a gather downstream, and another trigger port overlaps with some branches
	// of the scatter but not others?
	// If so, then all downstream nodes will be locked before the event can proceed.
	// This prevents deadlock where the other trigger's event gets into its branch while this trigger's event gets into
	// the other branch and claims the gather node (https://b33p.net/kosada/node/6696).

	bool hasGatherOverlappedByAnotherTrigger = graph->hasGatherOverlappedByAnotherTrigger(node, trigger);

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
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;

		vector<VuoPort *> inputPorts = node->getInputPorts();
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		vector<VuoPort *> ports;
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());

		for (vector<VuoPort *>::iterator j = ports.begin(); j != ports.end(); ++j)
		{
			VuoType *dataType = static_cast<VuoCompilerPort *>((*j)->getCompiler())->getDataVuoType();

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
 * Sets up VuoCompilerBitcodeGenerator::publishedOutputTriggerNames.
 */
void VuoCompilerBitcodeGenerator::makePublishedOutputTriggers(void)
{
	if (publishedInputNode)
		publishedOutputTriggerNames = graph->getPublishedOutputTriggers();
}

/**
 * Sets up the node identifier and port context index for each port in the composition.
 */
void VuoCompilerBitcodeGenerator::makePortContextInfo(void)
{
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;

		vector<VuoPort *> inputPorts = node->getInputPorts();
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		vector<VuoPort *> ports;
		ports.insert(ports.end(), inputPorts.begin(), inputPorts.end());
		ports.insert(ports.end(), outputPorts.begin(), outputPorts.end());
		for (size_t i = 0; i < ports.size(); ++i)
		{
			VuoCompilerPort *port = static_cast<VuoCompilerPort *>( ports[i]->getCompiler() );

			port->setNodeIdentifier( node->getCompiler()->getIdentifier() );
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
 * Returns the trigger port on the published input node.
 */
VuoCompilerTriggerPort * VuoCompilerBitcodeGenerator::getPublishedInputTrigger(void)
{
	VuoPort *publishedTriggerPort = publishedInputNode->getOutputPorts().at( VuoNodeClass::unreservedOutputPortStartIndex );
	return static_cast<VuoCompilerTriggerPort *>( publishedTriggerPort->getCompiler() );
}

#ifdef PREMIUM_NODE_LOADER_ENABLED
#include "VuoPremium.hh"
#endif

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
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		if ((*i)->getNodeClass()->getCompiler()->isStateful())
		{
			isStatefulComposition = true;
			break;
		}
	}

	constantStrings.clear();

	module = new Module(moduleKey, getGlobalContext());

	generateCompositionMetadata();

	generateCompositionContextInitFunction(isStatefulComposition);
	generateCompositionContextFiniFunction();

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

	generateCompositionGetPortValueFunction();
	generateCompositionSetPortValueFunction();

	generateCompositionWaitForNodeFunction();

	if (isLiveCodeable)
	{
		generateCompositionSerializeFunction();
		generateCompositionUnserializeFunction();
	}

	if (isTopLevelComposition)
	{
		generateAllocation();
		generateSetupFunction();
		generateCleanupFunction();

		generateInstanceInitFunction(isStatefulComposition);
		generateInstanceFiniFunction(isStatefulComposition);
		generateInstanceTriggerStartFunction(isStatefulComposition);
		generateInstanceTriggerStopFunction(isStatefulComposition);

		generateGetPortValueFunction();
		generateSetInputPortValueFunction();
		generateFireTriggerPortEventFunction();

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

		if (isLiveCodeable)
		{
			generateSerializeFunction();
			generateUnserializeFunction();
		}
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

	string nodeClassNamePart = VuoStringUtilities::split(moduleKey, '.').back();
	string title = VuoStringUtilities::expandCamelCase(nodeClassNamePart);
	json_object_object_add(metadataJson, "title", json_object_new_string(title.c_str()));

	string description = composition->getBase()->getDescription();
	json_object_object_add(metadataJson, "description", json_object_new_string(description.c_str()));

	json_object *dependenciesJson = json_object_new_array();
	set<string> dependenciesSeen;
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		string dependencyName = (*i)->getNodeClass()->getCompiler()->getDependencyName();
		dependenciesSeen.insert(dependencyName);
	}
	for (set<string>::iterator i = dependenciesSeen.begin(); i != dependenciesSeen.end(); ++i)
	{
		string dependency = *i;
		json_object_array_add(dependenciesJson, json_object_new_string(dependency.c_str()));
	}
	json_object_object_add(metadataJson, "dependencies", dependenciesJson);

	bool isInterface = false;
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		if ((*i)->getNodeClass()->isInterface())
		{
			isInterface = true;
			break;
		}
	}
	json_object_object_add(nodeMetadataJson, "isInterface", json_object_new_boolean(isInterface));

	if (! isTopLevelComposition)
	{
		json_object *triggersJson = json_object_new_array();
		vector<VuoCompilerTriggerPort *> triggersInComposition = graph->getTriggerPorts();
		for (vector<VuoCompilerTriggerPort *>::iterator i = triggersInComposition.begin(); i != triggersInComposition.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = *i;
			VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];
			json_object *t = VuoCompilerTriggerDescription::getJson(triggerNode, trigger);
			json_object_array_add(triggersJson, t);
		}
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoNode *node = *i;
			VuoCompilerNodeClass *nodeClass = node->getNodeClass()->getCompiler();
			vector<VuoCompilerTriggerDescription *> triggersInSubcomposition = nodeClass->getTriggerDescriptions();
			for (vector<VuoCompilerTriggerDescription *>::iterator j = triggersInSubcomposition.begin(); j != triggersInSubcomposition.end(); ++j)
			{
				json_object *t = (*j)->getJsonWithinSubcomposition(node->getCompiler());
				json_object_array_add(triggersJson, t);
			}
		}
		json_object_object_add(nodeMetadataJson, "triggers", triggersJson);
	}

	json_object_object_add(metadataJson, "node", nodeMetadataJson);

	string metadata = json_object_to_json_string_ext(metadataJson, JSON_C_TO_STRING_PLAIN);
	json_object_put(metadataJson);

	VuoCompilerCodeGenUtilities::generateModuleMetadata(module, metadata, moduleKey);
}

/**
 * Generates the `compositionContextInit()` function, which returns a data structure containing all information
 * specific to a composition instance (node).
 *
 * The generated code registers the subcomposition instance's context and each contained node's context with the runtime.
 *
 * \eg{struct NodeContext * compositionContextInit(const char *compositionIdentifier);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionContextInitFunction(bool isStatefulComposition)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionContextInitFunction(module, moduleKey);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *compositionIdentifierValue = function->arg_begin();

	size_t publishedOutputPortCount = (publishedInputNode ?
										   publishedOutputNode->getInputPorts().size() - VuoNodeClass::unreservedInputPortStartIndex : 0);

	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateCreateNodeContext(module, block, isStatefulComposition, true, publishedOutputPortCount);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();

		vector<VuoCompilerNode *>::const_iterator nodeIter = find(orderedNodes.begin(), orderedNodes.end(), node);
		size_t nodeIndex = nodeIter - orderedNodes.begin();

		node->setConstantStringCache(&constantStrings);
		node->generateContextInit(module, block, compositionIdentifierValue, nodeIndex, orderedTypes);
	}

	VuoCompilerCodeGenUtilities::generateAddNodeContext(module, block, compositionIdentifierValue, compositionContextValue);

	generateInitialEventlessTransmissions(function, block, compositionIdentifierValue);

	ReturnInst::Create(module->getContext(), compositionContextValue, block);
}

/**
 * Generates the `compositionContextFini()` function, which deallocates the data structure created by `compositionContextInit()`.
 *
 * \eg{void compositionContextFini(const char *compositionIdentifier);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionContextFiniFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCompositionContextFiniFunction(module, moduleKey);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *compositionIdentifierValue = function->arg_begin();

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();

		Value *nodeIdentifierValue = node->generateIdentifierValue(module, block, compositionIdentifierValue);
		Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);

		BasicBlock *finiBlock = NULL;
		BasicBlock *nextBlock = NULL;
		if (isTopLevelComposition)
			VuoCompilerCodeGenUtilities::generateIsNodeInBothCompositionsCheck(module, function, node->getGraphvizIdentifier(), block, nextBlock, finiBlock, constantStrings);
		else
			finiBlock = nextBlock = block;

		node->generateContextFini(module, nextBlock, finiBlock, compositionIdentifierValue, nodeIdentifierValue, nodeContextValue);

		if (isTopLevelComposition)
		{
			BranchInst::Create(nextBlock, finiBlock);
			block = nextBlock;
		}

		VuoCompilerCodeGenUtilities::generateFreeCall(module, block, nodeIdentifierValue);
	}

	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, compositionIdentifierValue);
	VuoCompilerCodeGenUtilities::generateFreeNodeContext(module, block, compositionContextValue, 0);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to set the composition's published input port values from the corresponding arguments in a node function.
 */
void VuoCompilerBitcodeGenerator::generateSetInputDataFromNodeFunctionArguments(Function *function, BasicBlock *block, Value *compositionIdentifierValue,
																				map<VuoPort *, size_t> indexOfParameter, bool shouldUpdateTriggers)
{
	Value *publishedNodeContextValue = publishedInputNode->getCompiler()->generateGetContext(module, block, compositionIdentifierValue);

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	for (size_t i = 0; i < publishedInputPorts.size(); ++i)
	{
		VuoPublishedPort *publishedInputPort = publishedInputPorts[i];
		if (publishedInputPort->getClass()->getPortType() == VuoPortClass::dataAndEventPort)
		{
			size_t index = indexOfParameter[ modelInputPorts[i] ];
			VuoCompilerType *type = static_cast<VuoCompilerPort *>( publishedInputPort->getCompiler() )->getDataVuoType()->getCompiler();

			// If the argument is a struct that would normally be passed "byval", it's not "byval" here.
			// Instead, the Vuo compiler has implemented the "byval" semantics in the caller, which
			// has passed a struct pointer that is effectively passed by value but not marked as such.
			//
			// This is a workaround for a bug where LLVM would sometimes give the node function body
			// an invalid value for a "byval" struct argument. https://b33p.net/kosada/node/11386
			Value *arg;
			if (type->getType()->isStructTy() &&
					type->getFunctionParameterAttributes().hasAttribute(Attributes::ByVal))
			{
				Function::arg_iterator argIter = function->arg_begin();
				for (int i = 0; i < index; ++i)
					++argIter;
				Value *argAsPointerToOtherType = argIter;
				Value *argAsPointer = new BitCastInst(argAsPointerToOtherType, type->getType()->getPointerTo(), "", block);
				arg = new LoadInst(argAsPointer, "", block);
			}
			else
			{
				arg = VuoCompilerCodeGenUtilities::unlowerArgument(type, function, index, module, block);
			}

			VuoPort *publishedInputPortOnNode = publishedInputNode->getInputPorts().at( VuoNodeClass::unreservedInputPortStartIndex + i );
			VuoCompilerInputEventPort *publishedEventPort = static_cast<VuoCompilerInputEventPort *>( publishedInputPortOnNode->getCompiler() );
			publishedEventPort->generateReplaceData(module, block, publishedNodeContextValue, arg);

			vector<VuoCable *> connectedCables = publishedInputPort->getConnectedCables();
			for (vector<VuoCable *>::iterator j = connectedCables.begin(); j != connectedCables.end(); ++j)
			{
				VuoCable *connectedCable = *j;
				if (connectedCable->getCompiler()->carriesData())
				{
					VuoCompilerInputEventPort *eventPort = static_cast<VuoCompilerInputEventPort *>( connectedCable->getToPort()->getCompiler() );
					Value *nodeContextValue = connectedCable->getToNode()->getCompiler()->generateGetContext(module, block, compositionIdentifierValue);

					eventPort->generateReplaceData(module, block, nodeContextValue, arg);

					if (shouldUpdateTriggers)
						connectedCable->getToNode()->getCompiler()->generateCallbackUpdateFunctionCall(module, block, compositionIdentifierValue);
				}
			}
		}
	}
}

/**
 * Generates the @c nodeEvent() or @c nodeInstanceEvent() function, which handles each event into the
 * composition's published input ports.
 *
 * \eg{void nodeEvent( const char *compositionIdentifier, ...);}
 *
 * \eg{void nodeInstanceEvent( const char *compositionIdentifier, VuoInstanceData(InstanceDataType *) instanceData, ... );}
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
	for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
	{
		VuoPublishedPort *publishedInputPort = *i;

		string defaultValue = static_cast<VuoCompilerPublishedPort *>( publishedInputPort->getCompiler() )->getInitialValue();
		if (! defaultValue.empty())
			defaultValuesForInputPorts[publishedInputPort] = defaultValue;

		eventBlockingForInputPorts[publishedInputPort] = graph->getPublishedInputEventBlocking();
	}

	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, moduleKey, true, isStatefulComposition,
																		   modelInputPorts, modelOutputPorts,
																		   detailsForPorts, displayNamesForPorts,
																		   defaultValuesForInputPorts, eventBlockingForInputPorts,
																		   indexOfParameter, indexOfEventParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();
	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, compositionIdentifierValue);

	// Get a unique ID for this event.

	Value *eventIdValue = generateGetNextEventID(module, block);
	VuoCompilerCodeGenUtilities::generateSetNodeContextExecutingEventId(module, block, compositionContextValue, eventIdValue);

	// Claim all necessary downstream nodes.

	VuoCompilerTriggerPort *trigger = getPublishedInputTrigger();
	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
	generateWaitForNodes(module, function, block, compositionIdentifierValue, triggerWaitNodes, eventIdValue);

	// Set each published input port value from the input arguments.

	generateSetInputDataFromNodeFunctionArguments(function, block, compositionIdentifierValue, indexOfParameter, false);

	// Wait for the event to reach the published output node (if any) and all other leaf nodes — part 1.

	Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, block, compositionContextValue);
	VuoCompilerCodeGenUtilities::generateEnterDispatchGroup(module, block, subcompositionOutputGroupValue);

	// Fire an event from the published input node's trigger.

	Value *publishedInputNodeContextValue = publishedInputNode->getCompiler()->generateGetContext(module, block, compositionIdentifierValue);
	Value *triggerFunctionValue = trigger->generateLoadFunction(module, block, publishedInputNodeContextValue);
	CallInst::Create(triggerFunctionValue, "", block);

	// Wait for the event to reach the published output node (if any) and all other leaf nodes — part 2.

	VuoCompilerCodeGenUtilities::generateWaitForDispatchGroup(module, block, subcompositionOutputGroupValue);

	// Set each output argument from the published output port values.

	Value *publishedOutputNodeContext = publishedOutputNode->getCompiler()->generateGetContext(module, block, compositionIdentifierValue);
	vector<VuoPort *> publishedOutputPortsOnNode = publishedOutputNode->getInputPorts();
	for (size_t i = 0; i < modelOutputPorts.size(); ++i)
	{
		VuoPort *modelOutputPort = modelOutputPorts[i];

		if (modelOutputPort->getClass()->getPortType() != VuoPortClass::triggerPort)
		{
			VuoPort *publishedOutputPortOnNode = publishedOutputPortsOnNode[VuoNodeClass::unreservedInputPortStartIndex + i];
			bool hasEventParameter = false;
			size_t eventIndex;

			if (modelOutputPort->getClass()->getPortType() == VuoPortClass::dataAndEventPort)
			{
				size_t index = indexOfParameter[ modelOutputPort ];
				Function::arg_iterator argIter = function->arg_begin();
				for (int j = 0; j < index; ++j)
					++argIter;
				Value *outputArg = argIter;

				Value *value = static_cast<VuoCompilerEventPort *>( publishedOutputPortOnNode->getCompiler() )->generateLoadData(module, block, publishedOutputNodeContext);
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
				Function::arg_iterator argIter = function->arg_begin();
				for (int j = 0; j < eventIndex; ++j)
					++argIter;
				Value *outputArg = argIter;

				Value *eventValue = VuoCompilerCodeGenUtilities::generateGetNodeContextOutputEvent(module, block, compositionContextValue, i);
				new StoreInst(eventValue, outputArg, block);
			}
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceInit() function, which calls the corresponding function for all stateful nodes.
 *
 * \eg{InstanceDataType * nodeInstanceInit( const char *compositionIdentifier, ... );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceInitFunction(void)
{
	vector<VuoPort *> inputPorts;
	if (! isTopLevelComposition)
		inputPorts = modelInputPorts;

	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(module, moduleKey, inputPorts,
																				  indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();

	if (! isTopLevelComposition)
		generateSetInputDataFromNodeFunctionArguments(function, block, compositionIdentifierValue, indexOfParameter, false);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->isStateful())
		{
			BasicBlock *initBlock = NULL;
			BasicBlock *nextBlock = NULL;
			if (isTopLevelComposition)
				VuoCompilerCodeGenUtilities::generateIsNodeInBothCompositionsCheck(module, function, node->getCompiler()->getGraphvizIdentifier(), block, nextBlock, initBlock, constantStrings);
			else
				initBlock = block;

			node->getCompiler()->generateInitFunctionCall(module, initBlock, compositionIdentifierValue);

			if (isTopLevelComposition)
			{
				BranchInst::Create(nextBlock, initBlock);
				block = nextBlock;
			}
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
 * \eg{void nodeInstanceFini( const char *compositionIdentifier, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceFiniFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(module, moduleKey, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->isStateful())
		{
			BasicBlock *finiBlock = NULL;
			BasicBlock *nextBlock = NULL;
			if (isTopLevelComposition)
				VuoCompilerCodeGenUtilities::generateIsNodeInBothCompositionsCheck(module, function, node->getCompiler()->getGraphvizIdentifier(), block, nextBlock, finiBlock, constantStrings);
			else
				finiBlock = block;

			node->getCompiler()->generateFiniFunctionCall(module, finiBlock, compositionIdentifierValue);

			if (isTopLevelComposition)
			{
				BranchInst::Create(nextBlock, finiBlock);
				block = nextBlock;
			}
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerStart() function, which calls the corresponding function for all stateful nodes.
 *
 * Assumes the trigger function for each node's trigger port has been generated and associated with the port.
 *
 * \eg{void nodeInstanceTriggerStart( const char *compositionIdentifier, VuoInstanceData(InstanceDataType *) instanceData, ... );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerStartFunction(void)
{
	vector<VuoPort *> inputPorts;
	if (! isTopLevelComposition)
		inputPorts = modelInputPorts;

	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStartFunction(module, moduleKey, inputPorts,
																						  indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();

	if (! isTopLevelComposition)
		generateSetInputDataFromNodeFunctionArguments(function, block, compositionIdentifierValue, indexOfParameter, false);

	// Since a node's nodeInstanceTriggerStart() function can generate an event,
	// make sure trigger functions wait until all nodes' init functions have completed.
	generateWaitForNodes(module, function, block, compositionIdentifierValue, orderedNodes);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->isStateful())
		{
			// { /* call nodeInstanceTriggerStart() for node */ }
			node->getCompiler()->generateCallbackStartFunctionCall(module, block, compositionIdentifierValue);
		}
	}

	generateSignalForNodes(module, block, compositionIdentifierValue, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerStop() function, which calls the corresponding function for all stateful nodes.
 *
 * \eg{void nodeInstanceTriggerStop( const char *compositionIdentifier, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerStopFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerStopFunction(module, moduleKey, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();

	// Wait for any in-progress events to complete.
	generateWaitForNodes(module, function, block, compositionIdentifierValue, orderedNodes);

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		if (node->getNodeClass()->getCompiler()->isStateful())
		{
			// { /* call nodeInstanceTriggerStop() for node */ }
			node->getCompiler()->generateCallbackStopFunctionCall(module, block, compositionIdentifierValue);
		}
	}

	// Signal semaphores so they can be safely released.
	generateSignalForNodes(module, block, compositionIdentifierValue, orderedNodes);

	// Flush any pending events.
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = graph->getNodesForTriggerPorts();
	for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		VuoCompilerNode *node = i->second;

		Value *nodeContextValue = node->generateGetContext(module, block, compositionIdentifierValue);
		Function *barrierWorkerFunction = trigger->generateSynchronousSubmissionToDispatchQueue(module, block, nodeContextValue, trigger->getIdentifier() + "__barrier");
		BasicBlock *barrierBlock = BasicBlock::Create(module->getContext(), "", barrierWorkerFunction, NULL);
		ReturnInst::Create(module->getContext(), barrierBlock);
	}
	generateWaitForNodes(module, function, block, compositionIdentifierValue, orderedNodes);
	generateSignalForNodes(module, block, compositionIdentifierValue, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c nodeInstanceTriggerUpdate() function, which calls the corresponding function in all stateful nodes.
 *
 * \eg{void nodeInstanceTriggerUpdate( const char *compositionIdentifier, VuoInstanceData(InstanceDataType *) instanceData );}
 */
void VuoCompilerBitcodeGenerator::generateNodeInstanceTriggerUpdateFunction(void)
{
	map<VuoPort *, size_t> indexOfParameter;
	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceTriggerUpdateFunction(module, moduleKey, modelInputPorts,
																						   indexOfParameter, constantStrings);
	BasicBlock *block = &(function->getEntryBlock());

	Value *compositionIdentifierValue = function->arg_begin();

	VuoCompilerTriggerPort *trigger = getPublishedInputTrigger();
	vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
	generateWaitForNodes(module, function, block, compositionIdentifierValue, triggerWaitNodes);

	generateSetInputDataFromNodeFunctionArguments(function, block, compositionIdentifierValue, indexOfParameter, true);

	generateSignalForNodes(module, block, compositionIdentifierValue, triggerWaitNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to get a unique event ID.
 *
 * @eg{
 * unsigned long eventId = vuoGetNextEventId();
 * }
 */
Value * VuoCompilerBitcodeGenerator::generateGetNextEventID(Module *module, BasicBlock *block)
{
	Function *getLastEventIdFunction = VuoCompilerCodeGenUtilities::getGetNextEventIdFunction(module);
	return CallInst::Create(getLastEventIdFunction, "", block);
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
 * compositionWaitForNode(compositionIdentifier, 2, eventId, true);  // orderedNodes[2]
 * compositionWaitForNode(compositionIdentifier, 4, eventId, true);  // orderedNodes[4]
 * compositionWaitForNode(compositionIdentifier, 5, eventId, true);  // orderedNodes[5]
 * }
 *
 * @eg{
 * // shouldBlock=false
 *
 * bool keepTrying;
 *
 * keepTrying = compositionWaitForNode(compositionIdentifier, 2, eventId, false);  // orderedNodes[2]
 * if (! keepTrying)
 *    goto SIGNAL0;
 *
 * keepTrying = compositionWaitForNode(compositionIdentifier, 4, eventId, false);  // orderedNodes[4]
 * if (! keepTrying)
 *    goto SIGNAL1;
 *
 * keepTrying = compositionWaitForNode(compositionIdentifier, 5, eventId, false);  // orderedNodes[5]
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
														  Value *compositionIdentifierValue, vector<VuoCompilerNode *> nodes,
														  Value *eventIdValue, bool shouldBlock)
{
	sortNodes(nodes);

	if (! eventIdValue)
		eventIdValue = generateGetNextEventID(module, block);

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
			args.push_back(compositionIdentifierValue);
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
			args.push_back(compositionIdentifierValue);
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

			generateSignalForNodes(module, signalBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, nodeToSignal));

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
 * bool compositionWaitForNode(char *compositionIdentifier, unsigned long indexInOrderedNodes, unsigned long eventId, bool shouldBlock)
 * {
 *    char *baseNodeIdentifier = NULL;
 *    bool keepTrying = true;
 *
 *    if (indexInOrderedNodes == 0)  // orderedNodes[0]
 *       baseNodeIdentifier = "Add";
 *    else if (indexInOrderedNodes == 1)  // orderedNodes[1]
 *       baseNodeIdentifier = "Count3";
 *    else if (...)
 *       ...
 *
 *    int64_t timeoutDelta = (shouldBlock ? ... : 0);
 *    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeoutDelta);
 *
 *    char *nodeIdentifier = ...;  // compositionIdentifier__baseNodeIdentifier
 *    NodeContext *nodeContext = vuoGetNodeContext(nodeIdentifier);
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
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
	Value *indexInOrderedNodesValue = args++;
	indexInOrderedNodesValue->setName("indexInOrderedNodes");
	Value *eventIdValue = args++;
	eventIdValue->setName("eventId");
	Value *shouldBlockValue = args++;
	shouldBlockValue->setName("shouldBlock");


	// char *baseNodeIdentifier = NULL;
	// bool keepTrying = true;

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *baseNodeIdentifierVariable = new AllocaInst(pointerToCharType, "baseNodeIdentifier", initialBlock);
	ConstantPointerNull *nullIdentifier = ConstantPointerNull::get(pointerToCharType);
	new StoreInst(nullIdentifier, baseNodeIdentifierVariable, false, initialBlock);

	AllocaInst *keepTryingVariable = new AllocaInst(IntegerType::get(module->getContext(), 1), "keepTrying", initialBlock);
	ConstantInt *trueValue = ConstantInt::get(module->getContext(), APInt(1, 1));
	new StoreInst(trueValue, keepTryingVariable, initialBlock);


	// if (indexInOrderedNodes == 0)
	//    baseNodeIdentifier = "Add";
	// else ...

	vector< pair<BasicBlock *, BasicBlock *> > blocksForIndex;
	for (size_t i = 0; i < orderedNodes.size(); ++i)
	{
		VuoCompilerNode *node = orderedNodes[i];

		string baseNodeIdentifier = node->getGraphvizIdentifier();
		BasicBlock *block = BasicBlock::Create(module->getContext(), baseNodeIdentifier, function, 0);

		Value *baseNodeIdentifierValue = constantStrings.get(module, baseNodeIdentifier);
		new StoreInst(baseNodeIdentifierValue, baseNodeIdentifierVariable, false, block);

		blocksForIndex.push_back( make_pair(block, block) );
	}
	BasicBlock *timeoutBlock = BasicBlock::Create(module->getContext(), "timeout", function, 0);
	VuoCompilerCodeGenUtilities::generateIndexMatchingCode(module, function, initialBlock, timeoutBlock, indexInOrderedNodesValue, blocksForIndex);


	// int64_t timeoutDelta = (shouldBlock ? ... : 0);
	// dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeoutDelta);

	Type *timeoutDeltaType = IntegerType::get(module->getContext(), 64);
	AllocaInst *timeoutDeltaVariable = new AllocaInst(timeoutDeltaType, "timeoutDelta", timeoutBlock);
	ICmpInst *shouldBlockIsTrue = new ICmpInst(*timeoutBlock, ICmpInst::ICMP_EQ, shouldBlockValue, trueValue);
	BasicBlock *nonZeroTimeoutBlock = BasicBlock::Create(module->getContext(), "nonZeroTimeout", function);
	BasicBlock *zeroTimeoutBlock = BasicBlock::Create(module->getContext(), "zeroTimeout", function);
	BranchInst::Create(nonZeroTimeoutBlock, zeroTimeoutBlock, shouldBlockIsTrue, timeoutBlock);

	BasicBlock *checkEventIdBlock = BasicBlock::Create(module->getContext(), "checkEventId", function);

	ConstantInt *nonZeroTimeoutValue = ConstantInt::get(module->getContext(), APInt(64, NSEC_PER_SEC / 1000));  /// @todo (https://b33p.net/kosada/node/6682)
	new StoreInst(nonZeroTimeoutValue, timeoutDeltaVariable, false, nonZeroTimeoutBlock);
	BranchInst::Create(checkEventIdBlock, nonZeroTimeoutBlock);

	ConstantInt *zeroTimeoutValue = ConstantInt::get(module->getContext(), APInt(64, 0));
	new StoreInst(zeroTimeoutValue, timeoutDeltaVariable, false, zeroTimeoutBlock);
	BranchInst::Create(checkEventIdBlock, zeroTimeoutBlock);

	Value *timeoutDeltaValue = new LoadInst(timeoutDeltaVariable, "", false, checkEventIdBlock);
	Value *timeoutValue = VuoCompilerCodeGenUtilities::generateCreateDispatchTime(module, checkEventIdBlock, timeoutDeltaValue);


	// char *nodeIdentifier = ...;  // compositionIdentifier__baseNodeIdentifier
	// NodeContext *nodeContext = vuoGetNodeContext(nodeIdentifier);

	Value *baseNodeIdentifierValue = new LoadInst(baseNodeIdentifierVariable, "", false, checkEventIdBlock);

	vector<Value *> identifierParts;
	identifierParts.push_back(compositionIdentifierValue);
	identifierParts.push_back(constantStrings.get(module, "__"));
	identifierParts.push_back(baseNodeIdentifierValue);
	Value *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generateStringConcatenation(module, checkEventIdBlock, identifierParts, constantStrings);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, checkEventIdBlock, nodeIdentifierValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, checkEventIdBlock, nodeIdentifierValue);


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
void VuoCompilerBitcodeGenerator::generateSignalForNodes(Module *module, BasicBlock *block, Value *compositionIdentifierValue,
														 vector<VuoCompilerNode *> nodes)
{
	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		Value *nodeContextValue = node->generateGetContext(module, block, compositionIdentifierValue);

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
 * char * compositionGetPortValue(const char *compositionIdentifier, const char *portIdentifier, int serializationType, bool isThreadSafe)
 * {
 *	 char *ret = NULL;
 *
 *   void *portAddress = vuoGetDataForPort(compositionIdentifier, portIdentifier);
 *
 *   if (portAddress != NULL)
 *   {
 *     dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionIdentifier, portIdentifier);
 *     unsigned long typeIndex = vuoGetTypeIndexForPort(compositionIdentifier, portIdentifier);
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
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
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

	// void *portAddress = vuoGetDataForPort(compositionIdentifier, portIdentifier);

	Value *portAddressAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetDataForPort(module, initialBlock, compositionIdentifierValue, portIdentifierValue);

	// if (portAddress != NULL)

	BasicBlock *checkWaitBlock = BasicBlock::Create(module->getContext(), "checkWait", function, 0);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	ConstantPointerNull *nullPortAddress = ConstantPointerNull::get(static_cast<PointerType *>(portAddressAsVoidPointer->getType()));
	ICmpInst *portAddressNotEqualsNull = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, portAddressAsVoidPointer, nullPortAddress, "");
	BranchInst::Create(checkWaitBlock, finalBlock, portAddressNotEqualsNull, initialBlock);

	// dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionIdentifier, portIdentifier);
	// unsigned long typeIndex = vuoGetTypeIndexForPort(compositionIdentifier, portIdentifier);

	Value *nodeSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeSemaphoreForPort(module, checkWaitBlock, compositionIdentifierValue, portIdentifierValue);
	Value *typeIndexValue = VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(module, checkWaitBlock, compositionIdentifierValue, portIdentifierValue);

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
 * void compositionSetPortValue(const char *compositionIdentifier, const char *portIdentifier, const char *valueAsString,
 *								bool isThreadSafe, bool shouldUpdateTriggers, bool shouldSendTelemetry, bool hasOldValue, bool hasNewValue)
 * {
 *   void *portAddress = vuoGetDataForPort(compositionIdentifier, portIdentifier);
 *
 *   if (portAddress != NULL)
 *   {
 *     dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionIdentifier, portIdentifier);
 *     unsigned long typeIndex = vuoGetTypeIndexForPort(compositionIdentifier, portIdentifier);
 *     unsigned long nodeIndex = vuoGetNodeIndexForPort(compositionIdentifier, portIdentifier);
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
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
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


	// void *portAddress = vuoGetDataForPort(compositionIdentifier, portIdentifier);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	Value *portAddressAsVoidPointer = VuoCompilerCodeGenUtilities::generateGetDataForPort(module, initialBlock, compositionIdentifierValue, portIdentifierValue);

	// if (portAddress != NULL)

	BasicBlock *checkWaitBlock = BasicBlock::Create(module->getContext(), "checkWait", function, 0);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);

	ConstantPointerNull *nullPortAddress = ConstantPointerNull::get(static_cast<PointerType *>(portAddressAsVoidPointer->getType()));
	ICmpInst *portAddressNotEqualsNull = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, portAddressAsVoidPointer, nullPortAddress, "");
	BranchInst::Create(checkWaitBlock, finalBlock, portAddressNotEqualsNull, initialBlock);

	// dispatch_semaphore_t nodeSemaphore = vuoGetNodeSemaphoreForPort(compositionIdentifier, portIdentifier);
	// unsigned long typeIndex = vuoGetTypeIndexForPort(compositionIdentifier, portIdentifier);
	// unsigned long nodeIndex = vuoGetNodeIndexForPort(compositionIdentifier, portIdentifier);
	// char *summary = NULL;

	Value *nodeSemaphoreValue = VuoCompilerCodeGenUtilities::generateGetNodeSemaphoreForPort(module, checkWaitBlock, compositionIdentifierValue, portIdentifierValue);
	Value *typeIndexValue = VuoCompilerCodeGenUtilities::generateGetTypeIndexForPort(module, checkWaitBlock, compositionIdentifierValue, portIdentifierValue);
	Value *nodeIndexValue = VuoCompilerCodeGenUtilities::generateGetNodeIndexForPort(module, checkWaitBlock, compositionIdentifierValue, portIdentifierValue);

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

		node->generateCallbackUpdateFunctionCall(module, currentBlock, compositionIdentifierValue);

		generateEventlessTransmission(function, currentBlock, compositionIdentifierValue, node, true);

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

	if (isTopLevelComposition)  // sendInputPortsUpdated() is only available to the top-level composition
	{
		Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);
		Type *boolType = sendInputPortsUpdatedFunction->getFunctionType()->getParamType(1);
		Constant *falseValue = ConstantInt::get(boolType, 0);
		Constant *trueValue = ConstantInt::get(boolType, 1);

		vector<Value *> sendInputPortsUpdatedArgs;
		sendInputPortsUpdatedArgs.push_back(portIdentifierValue);
		sendInputPortsUpdatedArgs.push_back(falseValue);
		sendInputPortsUpdatedArgs.push_back(trueValue);
		sendInputPortsUpdatedArgs.push_back(summaryValue);
		CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", sendBlock);
	}

	VuoCompilerCodeGenUtilities::generateFreeCall(module, sendBlock, summaryValue);
	BranchInst::Create(finalBlock, sendBlock);

	ReturnInst::Create(module->getContext(), finalBlock);
}

/**
 * Generates the `vuoGetPortValue()` function.
 *
 * @eg{char * vuoGetPortValue(const char *portIdentifier, int serializationType);}
 */
void VuoCompilerBitcodeGenerator::generateGetPortValueFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getGetPortValueFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");
	Value *serializationTypeValue = args++;
	serializationTypeValue->setName("serializationType");

	Function *compositionGetPortValueFunction = VuoCompilerCodeGenUtilities::getCompositionGetPortValueFunction(module);

	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

	Value *trueValue = ConstantInt::get(compositionGetPortValueFunction->getFunctionType()->getParamType(3), 1);

	vector<Value *> compositionArgs;
	compositionArgs.push_back(topLevelCompositionIdentifierValue);
	compositionArgs.push_back(portIdentifierValue);
	compositionArgs.push_back(serializationTypeValue);
	compositionArgs.push_back(trueValue);
	Value *ret = CallInst::Create(compositionGetPortValueFunction, compositionArgs, "", block);

	ReturnInst::Create(module->getContext(), ret, block);
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

	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

	Value *trueValue = ConstantInt::get(compositionSetPortValueFunction->getFunctionType()->getParamType(3), 1);

	vector<Value *> compositionArgs;
	compositionArgs.push_back(topLevelCompositionIdentifierValue);
	compositionArgs.push_back(portIdentifierValue);
	compositionArgs.push_back(valueAsStringValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	compositionArgs.push_back(trueValue);
	CallInst::Create(compositionSetPortValueFunction, compositionArgs, "", block);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates code to initialize each input and output data-and-event port reachable by eventless transmission.
 *
 * Assumes all other input ports' values have already been initialized.
 */
void VuoCompilerBitcodeGenerator::generateInitialEventlessTransmissions(Function *function, BasicBlock *&block, Value *compositionIdentifierValue)
{
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		generateEventlessTransmission(function, block, compositionIdentifierValue, node, false);
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
		Type *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);

		vector<Type *> functionParams;
		functionParams.push_back(pointerToCharType);
		FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), functionParams, false);
		function = Function::Create(functionType, GlobalValue::ExternalLinkage, functionName, module);
	}

	Function::arg_iterator args = function->arg_begin();
	Value *portIdentifierValue = args++;
	portIdentifierValue->setName("portIdentifier");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);

	Value *compositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *compositionIdentifierValue = new LoadInst(compositionIdentifierVariable, "", false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger = graph->getNodesForTriggerPorts();
	for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		VuoCompilerNode *triggerNode = i->second;

		string currentPortIdentifier = trigger->getIdentifier();
		Function *triggerFunction = topLevelTriggerFunctions[trigger];

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
		BasicBlock *origCurrentBlock = currentBlock;

		vector<Value *> triggerArgs;
		bool hasData = trigger->getDataVuoType();
		if (hasData)
		{
			generateWaitForNodes(module, function, currentBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, triggerNode));

			Value *nodeContextValue = triggerNode->generateGetContext(module, currentBlock, compositionIdentifierValue);

			Value *arg = trigger->generateLoadPreviousData(module, currentBlock, nodeContextValue);
			Value *secondArg = NULL;
			Value **secondArgIfNeeded = (triggerFunction->getFunctionType()->getNumParams() == 2 ? &secondArg : NULL);
			arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(arg, triggerFunction, 0, secondArgIfNeeded, module, currentBlock);
			triggerArgs.push_back(arg);
			if (secondArg)
				triggerArgs.push_back(secondArg);
		}

		CallInst::Create(triggerFunction, triggerArgs, "", currentBlock);

		if (hasData)
			generateSignalForNodes(module, currentBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, triggerNode));

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
 * Example:
 *
 * @eg{
 * void firePublishedInputPortEvent(char *name)
 * {
 *		vuo_in__PublishedInputPorts__fired();
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

	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, 0);

	if (! composition->getBase()->getPublishedInputPorts().empty())
	{
		VuoCompilerTriggerPort *trigger = getPublishedInputTrigger();
		Function *triggerFunction = topLevelTriggerFunctions[trigger];
		CallInst::Create(triggerFunction, "", block);
	}

	ReturnInst::Create(module->getContext(), block);
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
 *     ret = getInputPortString("vuo_in__PublishedInputs__firstName", shouldUseInterprocessSerialization);
 *   }
 *   else if (! strcmp(portIdentifier, "secondName"))
 *   {
 *     ret = getInputPortString("vuo_in__PublishedInputs__secondName", shouldUseInterprocessSerialization);
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
 *     ret = getInputPortString("vuo_out__PublishedOutputs__firstName", shouldUseInterprocessSerialization);
 *   }
 *   else if (! strcmp(portIdentifier, "secondName"))
 *   {
 *     ret = getInputPortString("vuo_out__PublishedOutputs__secondName", shouldUseInterprocessSerialization);
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

	Function *getValueFunction = VuoCompilerCodeGenUtilities::getGetInputPortStringFunction(module);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	PointerType *pointerToChar = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	AllocaInst *retVariable = new AllocaInst(pointerToChar, "ret", initialBlock);
	ConstantPointerNull *nullValue = ConstantPointerNull::get(pointerToChar);
	new StoreInst(nullValue, retVariable, false, initialBlock);

	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;
	if (publishedInputNode)
	{
		vector<VuoPort *> ports = (input ?
									   publishedInputNode->getInputPorts() :
									   publishedOutputNode->getInputPorts());
		for (size_t i = VuoNodeClass::unreservedInputPortStartIndex; i < ports.size(); ++i)
		{
			VuoPort *port = ports[i];
			string currentName = port->getClass()->getName();
			BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

			string currentIdentifier = static_cast<VuoCompilerEventPort *>(port->getCompiler())->getIdentifier();
			Constant *currentIdentifierValue = constantStrings.get(module, currentIdentifier);

			vector<Value *> args;
			args.push_back(currentIdentifierValue);
			args.push_back(shouldUseInterprocessSerializationValue);
			CallInst *getValueResult = CallInst::Create(getValueFunction, args, "", currentBlock);
			new StoreInst(getValueResult, retVariable, currentBlock);

			blocksForString[currentName] = make_pair(currentBlock, currentBlock);
		}
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	LoadInst *retValue = new LoadInst(retVariable, "", false, finalBlock);
	ReturnInst::Create(module->getContext(), retValue, finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString, constantStrings);
}

/**
 * Generates the setPublishedInputPortValue() function.
 *
 * Calls to setPublishedInputPortValue() are enqueued on the same dispatch queues as calls to firePublishedInputPortEvent()
 * to make sure that the actions are carried out in the order they were requested.
 *
 * @eg{
 * void setPublishedInputPortValue(char *portIdentifier, char *valueAsString)
 * {
 *   if (! strcmp(portIdentifier, "firstName"))
 *   {
 *     dispatch_sync(vuo_in__PublishedInputs__firstName__queue, (void *)valueAsString, vuo_in__PublishedInputPorts__firstName__set);
 *   }
 *   else if (! strcmp(portIdentifier, "secondName"))
 *   {
 *     dispatch_sync(vuo_in__PublishedInputs__secondName__queue, (void *)valueAsString, vuo_in__PublishedInputPorts__secondName__set);
 *   }
 *   else if (! strcmp(portIdentifier, "thirdName"))
 *   {
 *     dispatch_sync(vuo_in__PublishedInputs__thirdName__queue, (void *)valueAsString, vuo_in__PublishedInputPorts__thirdName__set);
 *   }
 * }
 *
 * void vuo_in__firstName__set__worker(void *context)
 * {
 *   char *valueAsString = (char *)context;
 *   vuoSetInputPortValue("vuo_in__PublishedInputs__firstName", valueAsString);
 *   vuoSetInputPortValue("vuo_math_subtract__Subtract__a", valueAsString);
 * }
 *
 * void vuo_in__secondName__set__worker(void *context)
 * {
 *   char *valueAsString = (char *)context;
 *   vuoSetInputPortValue("vuo_in__PublishedInputs__secondName", valueAsString);
 *   vuoSetInputPortValue("vuo_math_subtract__Subtract__b", valueAsString);
 *   vuoSetInputPortValue("vuo_math_subtract__Subtract2__b", valueAsString);
 * }
 *
 * void vuo_in__thirdName__set__worker(void *context)
 * {
 *   char *valueAsString = (char *)context;
 *   vuoSetInputPortValue("vuo_in__PublishedInputs__thirdName", valueAsString);
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

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, 0);
	map<string, pair<BasicBlock *, BasicBlock *> > blocksForString;

	if (publishedInputNode)
	{
		Value *compositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
		Value *compositionIdentifierValue = new LoadInst(compositionIdentifierVariable, "", false, initialBlock);

		VuoCompilerTriggerPort *trigger = getPublishedInputTrigger();

		Function *setValueFunction = VuoCompilerCodeGenUtilities::getSetInputPortValueFunction(module);

		Value *nodeContextValue = publishedInputNode->getCompiler()->generateGetContext(module, initialBlock, compositionIdentifierValue);

		vector<VuoPublishedPort *> publishedPorts = composition->getBase()->getPublishedInputPorts();
		for (size_t i = 0; i < publishedPorts.size(); ++i)
		{
			VuoPublishedPort *publishedPort = publishedPorts[i];
			string currentName = publishedPort->getClass()->getName();
			BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentName, function, 0);

			string currentSetFunctionName = VuoNodeClass::publishedInputNodeClassName + "__" + currentName + "__set";
			Function *currentSetFunction = trigger->generateSynchronousSubmissionToDispatchQueue(module, currentBlock, nodeContextValue,
																								 currentSetFunctionName, valueAsStringValue);

			Function::arg_iterator args = currentSetFunction->arg_begin();
			Value *context = args++;
			context->setName("valueAsString");

			BasicBlock *setBlock = BasicBlock::Create(module->getContext(), "", currentSetFunction, 0);
			Value *valueAsStringValueInSetFunction = new BitCastInst(context, valueAsStringValue->getType(), "", setBlock);

			VuoPort *port = publishedInputNode->getInputPorts().at( VuoNodeClass::unreservedInputPortStartIndex + i );
			Value *underlyingPortIdentifierValue = constantStrings.get(module, static_cast<VuoCompilerPort *>( port->getCompiler() )->getIdentifier());

			vector<Value *> setValueArgs;
			setValueArgs.push_back(underlyingPortIdentifierValue);
			setValueArgs.push_back(valueAsStringValueInSetFunction);
			CallInst::Create(setValueFunction, setValueArgs, "", setBlock);

			vector<VuoCable *> connectedCables = publishedPort->getConnectedCables();
			for (vector<VuoCable *>::iterator j = connectedCables.begin(); j != connectedCables.end(); ++j)
			{
				VuoCable *cable = *j;
				if (cable->getCompiler()->carriesData())
				{
					VuoCompilerPort *connectedPort = static_cast<VuoCompilerPort *>( cable->getToPort()->getCompiler() );
					Value *connectedPortIdentifierValue = constantStrings.get(module, connectedPort->getIdentifier());

					vector<Value *> setValueArgs;
					setValueArgs.push_back(connectedPortIdentifierValue);
					setValueArgs.push_back(valueAsStringValueInSetFunction);
					CallInst::Create(setValueFunction, setValueArgs, "", setBlock);
				}
			}
			ReturnInst::Create(module->getContext(), setBlock);

			blocksForString[currentName] = make_pair(currentBlock, currentBlock);
		}
	}

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, portIdentifierValue, blocksForString, constantStrings);
}

/**
 * Generates code to transmit an event (if any) and data (if any) from @a outputPort to all connected input ports,
 * and send telemetry indicating that these output and input ports have been updated.
 */
void VuoCompilerBitcodeGenerator::generateTransmissionFromOutputPort(Function *function, BasicBlock *&currentBlock,
																	 Value *compositionIdentifierValue, VuoCompilerPort *outputPort,
																	 Value *eventValue, Value *dataValue,
																	 bool requiresEvent, bool shouldSendTelemetry)
{
	Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);
	Type *boolType = sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	PointerType *pointerToCharType = static_cast<PointerType *>(sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(2));

	set<VuoCompilerCable *> outgoingCables = (requiresEvent ?
												  graph->getCablesImmediatelyDownstream(outputPort) :
												  graph->getCablesImmediatelyEventlesslyDownstream(outputPort));

	shouldSendTelemetry = shouldSendTelemetry && isTopLevelComposition;

	Constant *trueValue = ConstantInt::get(boolType, 1);
	Constant *falseValue = ConstantInt::get(boolType, 0);

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
			ICmpInst *shouldSendDataForInput = VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(module, currentBlock, inputPort->getIdentifier(), constantStrings);
			shouldSummarizeInput[inputPort] = shouldSendDataForInput;
		}

		if (dataValue)
		{
			Value *shouldSummarizeOutput = VuoCompilerCodeGenUtilities::generateShouldSendDataTelemetryComparison(module, currentBlock, outputPort->getIdentifier(), constantStrings);

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
		Value *inputNodeContextValue = inputNode->generateGetContext(module, transmissionBlock, compositionIdentifierValue);
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
			generateSendInputPortUpdated(transmissionBlock, inputPort, receivedDataValue, inputDataSummaryValue);

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
															   Value *compositionIdentifierValue, Value *nodeContextValue,
															   VuoCompilerNode *node, bool requiresEvent, bool shouldSendTelemetry)
{
	shouldSendTelemetry = shouldSendTelemetry && isTopLevelComposition;

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
		generateTransmissionFromOutputPort(function, telemetryBlock, compositionIdentifierValue,
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
void VuoCompilerBitcodeGenerator::generateTelemetryFromPublishedOutputNode(Function *function, BasicBlock *&currentBlock,
																		   Value *nodeContextValue, VuoCompilerNode *node)
{
	Function *sendPublishedOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendPublishedOutputPortsUpdatedFunction(module);
	Type *boolType = sendPublishedOutputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	PointerType *pointerToCharType = static_cast<PointerType *>(sendPublishedOutputPortsUpdatedFunction->getFunctionType()->getParamType(2));

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

		generateSendPublishedOutputPortUpdated(telemetryBlock, inputEventPort, sentDataValue, dataSummaryValue);

		BranchInst::Create(noTelemetryBlock, telemetryBlock);
		currentBlock = noTelemetryBlock;
	}
}

/**
 * Generates code to transmit data without an event through each data-and-event output port of @a firstNode
 * and onward to all data-and-event ports reachable via eventless transmission.
 *
 * If @a isCompositionStarted is true, assumes that the semaphore for @a firstNode has already been claimed.
 */
void VuoCompilerBitcodeGenerator::generateEventlessTransmission(Function *function, BasicBlock *&currentBlock, Value *compositionIdentifierValue,
																VuoCompilerNode *firstNode, bool isCompositionStarted)
{
	if (! graph->mayTransmitEventlessly(firstNode))
		return;
	vector<VuoCompilerNode *> downstreamNodes = graph->getNodesEventlesslyDownstream(firstNode);

	if (isCompositionStarted)
	{
		// Claim the nodes downstream via eventless transmission.
		generateWaitForNodes(module, function, currentBlock, compositionIdentifierValue, downstreamNodes, NULL, true);
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
			generateNodeExecution(function, currentBlock, compositionIdentifierValue, node, isCompositionStarted);

			// Transmit data through the node's outgoing cables, and send telemetry for port updates if needed.
			Value *nodeContextValue = node->generateGetContext(module, currentBlock, compositionIdentifierValue);
			generateTransmissionFromNode(function, currentBlock, compositionIdentifierValue, nodeContextValue, node, false, isCompositionStarted);
		}

		if (isCompositionStarted && node != firstNode)
			generateSignalForNodes(module, currentBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, node));
	}
}

/**
 * Generates code to call the node's event function, sending telemetry indicating that execution has started and finished.
 */
void VuoCompilerBitcodeGenerator::generateNodeExecution(Function *function, BasicBlock *&currentBlock,
														Value *compositionIdentifierValue, VuoCompilerNode *node,
														bool shouldSendTelemetry)
{
	shouldSendTelemetry = shouldSendTelemetry && isTopLevelComposition;

	Value *nodeIdentifierValue = constantStrings.get(module, node->getIdentifier());

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has started.
		Function *sendNodeExecutionStartedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionStartedFunction(module);
		CallInst::Create(sendNodeExecutionStartedFunction, nodeIdentifierValue, "", currentBlock);
	}

	// Call the node's event function.
	if (debugMode)
		VuoCompilerCodeGenUtilities::generatePrint(module, currentBlock, node->getBase()->getTitle() + "\n");
	Value *fullyQualifiedNodeIdentifierValue = node->generateIdentifierValue(module, currentBlock, compositionIdentifierValue);
	node->generateEventFunctionCall(module, function, currentBlock, fullyQualifiedNodeIdentifierValue);
	VuoCompilerCodeGenUtilities::generateFreeCall(module, currentBlock, fullyQualifiedNodeIdentifierValue);

	if (shouldSendTelemetry)
	{
		// Send telemetry indicating that the node's execution has finished.
		Function *sendNodeExecutionFinishedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionFinishedFunction(module);
		CallInst::Create(sendNodeExecutionFinishedFunction, nodeIdentifierValue, "", currentBlock);
	}
}

/**
 * Generates a call to @c sendInputPortsUpdated() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendInputPortUpdated(BasicBlock *block, VuoCompilerPort *inputPort,
															   Value *receivedDataValue, Value *dataSummaryValue)
{
	Constant *inputPortIdentifierValue = constantStrings.get(module, inputPort->getIdentifier());
	Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);

	// sendInputPortsUpdated(inputPortIdentifier, true, sentData, inputDataSummary);
	Type *boolType = sendInputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	Constant *trueValue = ConstantInt::get(boolType, 1);
	vector<Value *> sendInputPortsUpdatedArgs;
	sendInputPortsUpdatedArgs.push_back(inputPortIdentifierValue);
	sendInputPortsUpdatedArgs.push_back(trueValue);
	sendInputPortsUpdatedArgs.push_back(receivedDataValue);
	sendInputPortsUpdatedArgs.push_back(dataSummaryValue);
	CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", block);
}

/**
 * Generates a call to @c sendOutputPortsUpdated() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendOutputPortUpdated(BasicBlock *block, VuoCompilerPort *outputPort,
																Value *sentDataValue, Value *dataSummaryValue)
{
	Constant *outputPortIdentifierValue = constantStrings.get(module, outputPort->getIdentifier());
	Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);

	// sendOutputPortsUpdated(outputPortIdentifier, sentData, outputDataSummary);
	vector<Value *> sendOutputPortsUpdatedArgs;
	sendOutputPortsUpdatedArgs.push_back(outputPortIdentifierValue);
	sendOutputPortsUpdatedArgs.push_back(sentDataValue);
	sendOutputPortsUpdatedArgs.push_back(dataSummaryValue);
	CallInst::Create(sendOutputPortsUpdatedFunction, sendOutputPortsUpdatedArgs, "", block);
}

/**
 * Generates a call to @c sendPublishedOutputPortsUpdated() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendPublishedOutputPortUpdated(BasicBlock *block, VuoCompilerPort *port,
																		 Value *sentDataValue, Value *dataSummaryValue)
{
	Constant *portIdentifierValue = constantStrings.get(module, port->getBase()->getClass()->getName());
	Function *sendPublishedOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendPublishedOutputPortsUpdatedFunction(module);

	// sendPublishedOutputPortsUpdated(outputPortIdentifier, sentData, outputDataSummary);
	vector<Value *> sendPublishedOutputPortsUpdatedArgs;
	sendPublishedOutputPortsUpdatedArgs.push_back(portIdentifierValue);
	sendPublishedOutputPortsUpdatedArgs.push_back(sentDataValue);
	sendPublishedOutputPortsUpdatedArgs.push_back(dataSummaryValue);
	CallInst::Create(sendPublishedOutputPortsUpdatedFunction, sendPublishedOutputPortsUpdatedArgs, "", block);
}

/**
 * Generates a call to @c sendEventDropped() to send telemetry.
 */
void VuoCompilerBitcodeGenerator::generateSendEventDropped(BasicBlock *block, string portIdentifier)
{
	Constant *portIdentifierValue = constantStrings.get(module, portIdentifier);

	// sendEventDropped(triggerPortIdentifier);
	Function *sendEventDroppedFunction = VuoCompilerCodeGenUtilities::getSendEventDroppedFunction(module);
	vector<Value *> sendEventDroppedArgs;
	sendEventDroppedArgs.push_back(portIdentifierValue);
	CallInst::Create(sendEventDroppedFunction, sendEventDroppedArgs, "", block);
}

/**
 * Generates the @c compositionSerialize() function, which returns a string representation of the current state
 * of the running composition.
 *
 * The generated function assumes that no events are firing or executing (e.g., the composition is paused),
 * and that the composition's @c nodeInstanceInit() function has run.
 *
 * @eg{char * compositionSerialize(char *compositionIdentifier);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionSerializeFunction(void)
{
	Function *serializeFunction = VuoCompilerCodeGenUtilities::getCompositionSerializeFunction(module, moduleKey);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", serializeFunction, NULL);

	Value *compositionIdentifierValue = serializeFunction->arg_begin();
	compositionIdentifierValue->setName("compositionIdentifier");

	vector<Value *> serializedComponentValues;

	if (isTopLevelComposition)
	{
		Value *headerValue = constantStrings.get(module, "digraph G\n{\n");
		serializedComponentValues.push_back(headerValue);
	}

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;

		Value *serializedNodeValue = node->getCompiler()->generateSerializedString(module, block, compositionIdentifierValue);
		serializedComponentValues.push_back(serializedNodeValue);
	}

	if (isTopLevelComposition)
	{
		Value *footerValue = constantStrings.get(module, "}\n");
		serializedComponentValues.push_back(footerValue);
	}

	Value *serializedCompositionValue = VuoCompilerCodeGenUtilities::generateStringConcatenation(module, block, serializedComponentValues, constantStrings);
	ReturnInst::Create(module->getContext(), serializedCompositionValue, block);
}

/**
 * Generates the @c compositionUnserialize() function, which parses the string representation of a composition
 * and sets the state of the running composition accordingly.
 *
 * The generated function assumes that no events are triggering or executing (e.g., the composition is paused),
 * and that the composition's @c nodeInstanceInit() function has run.
 *
 * @eg{void compositionUnserialize(char *compositionIdentifier, char *serializedComposition, graph_t *graph);}
 */
void VuoCompilerBitcodeGenerator::generateCompositionUnserializeFunction(void)
{
	Function *unserializeFunction = VuoCompilerCodeGenUtilities::getCompositionUnserializeFunction(module, moduleKey);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", unserializeFunction, NULL);

	Function::arg_iterator args = unserializeFunction->arg_begin();
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
	Value *serializedCompositionValue = args++;
	serializedCompositionValue->setName("serializedComposition");
	Value *graphValue = args++;
	serializedCompositionValue->setName("graph");

	if (isTopLevelComposition)
	{
		// graph_t graph = openGraphvizGraph(serializedComposition);
		Function *openGraphvizGraphFunction = VuoCompilerCodeGenUtilities::getOpenGraphvizGraphFunction(module);
		graphValue = CallInst::Create(openGraphvizGraphFunction, serializedCompositionValue, "graph", block);
	}

	set<VuoNode *> nodes = composition->getBase()->getNodes();
	if (publishedInputNode)
	{
		nodes.insert(publishedInputNode);
		nodes.insert(publishedOutputNode);
	}
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		node->getCompiler()->generateUnserialization(module, unserializeFunction, block, compositionIdentifierValue, graphValue);
	}

	if (isTopLevelComposition)
	{
		// closeGraphvizGraph(graph);
		Function *closeGraphvizGraphFunction = VuoCompilerCodeGenUtilities::getCloseGraphvizGraphFunction(module);
		CallInst::Create(closeGraphvizGraphFunction, graphValue, "", block);
	}

	// If a drawer has been resized in this live-coding update, and if the list input port attached to the drawer was
	// unserialized after the drawer, then then list input port has been reverted to the old (unresized) list value.
	// So re-transmit the value from the drawer to the list input.
	generateInitialEventlessTransmissions(unserializeFunction, block, compositionIdentifierValue);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the allocation of all global variables for the top-level composition.
 */
void VuoCompilerBitcodeGenerator::generateAllocation(void)
{
	bool proEnabled = false;
#ifdef PREMIUM_NODE_LOADER_ENABLED
	proEnabled = VuoPremium::getPremiumAccess();
#endif
	ConstantInt *proEnabledConstant = ConstantInt::get(module->getContext(), APInt(8, proEnabled));
	new GlobalVariable(*module,
					   IntegerType::get(module->getContext(), 8),
					   false,
					   GlobalValue::ExternalLinkage,
					   proEnabledConstant,
					   "VuoProEnabled");
}

/**
 * Generates the `vuoSetup()` function, which allocates and initializes all node and port contexts in the composition.
 *
 * Assumes generateTriggerFunctions() has been called. Stores these function pointers in the trigger port contexts.
 *
 * \eg{void vuoSetup(void);}
 */
void VuoCompilerBitcodeGenerator::generateSetupFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSetupFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *topLevelCompositionIdentifierValue = constantStrings.get(module, topLevelCompositionIdentifier);
	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	new StoreInst(topLevelCompositionIdentifierValue, topLevelCompositionIdentifierVariable, false, block);

	Function *compositionContextInitFunction = VuoCompilerCodeGenUtilities::getCompositionContextInitFunction(module, moduleKey);
	CallInst::Create(compositionContextInitFunction, topLevelCompositionIdentifierValue, "", block);

	for (map<VuoCompilerTriggerPort *, Function *>::iterator i = topLevelTriggerFunctions.begin(); i != topLevelTriggerFunctions.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		Function *function = i->second;

		VuoCompilerNode *node = graph->getNodesForTriggerPorts()[trigger];
		Value *nodeContextValue = node->generateGetContext(module, block, topLevelCompositionIdentifierValue);
		trigger->generateStoreFunction(module, block, nodeContextValue, function);
	}

	for (map<string, map<VuoCompilerTriggerDescription *, Function *> >::iterator i = subcompositionTriggerFunctions.begin(); i != subcompositionTriggerFunctions.end(); ++i)
	{
		string fullTriggerNodeIdentifier = i->first;

		for (map<VuoCompilerTriggerDescription *, Function *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			VuoCompilerTriggerDescription *trigger = j->first;
			Function *function = j->second;

			int portContextIndex = trigger->getPortContextIndex();
			Value *nodeIdentifierValue = constantStrings.get(module, fullTriggerNodeIdentifier);
			Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, block, nodeIdentifierValue);
			Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, block, nodeContextValue, portContextIndex);
			VuoCompilerCodeGenUtilities::generateSetPortContextTriggerFunction(module, block, portContextValue, function);
		}
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the `vuoCleanup()` function, which deallocates all node and port contexts in the composition.
 *
 * \eg{void vuoCleanup(void);}
 */
void VuoCompilerBitcodeGenerator::generateCleanupFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getCleanupFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

	Function *compositionContextFiniFunction = VuoCompilerCodeGenUtilities::getCompositionContextFiniFunction(module, moduleKey);
	CallInst::Create(compositionContextFiniFunction, topLevelCompositionIdentifierValue, "", block);
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
		Function *nodeInstanceInitFunction = VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(module, moduleKey,
																									  vector<VuoPort *>(),
																									  indexOfParameter, constantStrings);

		Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
		Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

		CallInst::Create(nodeInstanceInitFunction, topLevelCompositionIdentifierValue, "", block);
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
		Function *nodeInstanceFiniFunction = VuoCompilerCodeGenUtilities::getNodeInstanceFiniFunction(module, moduleKey, constantStrings);

		Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
		Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceFiniFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionIdentifierValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceFiniFunction, args, "", block);
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
																													  vector<VuoPort *>(),
																													  indexOfParameter,
																													  constantStrings);

		Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
		Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceTriggerStartFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionIdentifierValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceTriggerStartFunction, args, "", block);
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
																													constantStrings);

		Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
		Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

		PointerType *instanceDataType = static_cast<PointerType *>( nodeInstanceTriggerStopFunction->getFunctionType()->getParamType(1) );
		ConstantPointerNull *nullInstanceDataValue = ConstantPointerNull::get(instanceDataType);

		vector<Value *> args;
		args.push_back(topLevelCompositionIdentifierValue);
		args.push_back(nullInstanceDataValue);
		CallInst::Create(nodeInstanceTriggerStopFunction, args, "", block);
	}

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates the @c vuoSerialize() function, which calls @c compositionSerialize().
 *
 * \eg{char * vuoSerialize(void);}
 */
void VuoCompilerBitcodeGenerator::generateSerializeFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getSerializeFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

	Function *compositionSerializeFunction = VuoCompilerCodeGenUtilities::getCompositionSerializeFunction(module, moduleKey);
	CallInst *retValue = CallInst::Create(compositionSerializeFunction, topLevelCompositionIdentifierValue, "", block);
	ReturnInst::Create(module->getContext(), retValue, block);
}

/**
 * Generates the @c vuoUnserialize() function, which calls @c compositionUnserialize().
 *
 * \eg{void vuoUnserialize(char *serializedComposition);}
 */
void VuoCompilerBitcodeGenerator::generateUnserializeFunction(void)
{
	Function *function = VuoCompilerCodeGenUtilities::getUnserializeFunction(module);
	BasicBlock *block = BasicBlock::Create(module->getContext(), "", function, NULL);

	Value *serializedCompositionValue = function->arg_begin();

	Value *topLevelCompositionIdentifierVariable = VuoCompilerCodeGenUtilities::getTopLevelCompositionIdentifierVariable(module);
	Value *topLevelCompositionIdentifierValue = new LoadInst(topLevelCompositionIdentifierVariable, "", false, block);

	Function *compositionUnserializeFunction = VuoCompilerCodeGenUtilities::getCompositionUnserializeFunction(module, moduleKey);
	PointerType *pointerToGraphType = static_cast<PointerType *>( compositionUnserializeFunction->getFunctionType()->getParamType(2) );
	ConstantPointerNull *nullGraphValue = ConstantPointerNull::get(pointerToGraphType);
	vector<Value *> args;
	args.push_back(topLevelCompositionIdentifierValue);
	args.push_back(serializedCompositionValue);
	args.push_back(nullGraphValue);
	CallInst::Create(compositionUnserializeFunction, args, "", block);
	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generates a trigger worker function for each trigger in this composition (not including triggers inside of subcomposition nodes).
 * If this is a top-level composition, also generates a trigger scheduler function for each trigger at all levels in this
 * composition (including triggers inside of subcomposition nodes).
 */
void VuoCompilerBitcodeGenerator::generateTriggerFunctions(void)
{
	map<VuoCompilerTriggerPort *, Function *> workerFunctionForTrigger;

	vector<VuoCompilerTriggerPort *> triggers = graph->getTriggerPorts();
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

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

			VuoCompilerNode *node = graph->getNodesForTriggerPorts()[trigger];
			string triggerNodeIdentifier = node->getGraphvizIdentifier();

			string portIdentifier = trigger->getIdentifier();

			int portContextIndex = trigger->getIndexInPortContexts();

			bool canDropEvents = (trigger->getBase()->getEventThrottling() == VuoPortClass::EventThrottling_Drop);

			Function *function = generateTriggerSchedulerFunction(dataType, topLevelCompositionIdentifier, triggerNodeIdentifier,
																  portIdentifier, portContextIndex, canDropEvents, workerFunction);
			topLevelTriggerFunctions[trigger] = function;
		}

		set<VuoNode *> nodes = composition->getBase()->getNodes();
		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			VuoNode *node = *i;
			VuoCompilerNodeClass *nodeClass = node->getNodeClass()->getCompiler();

			string nodeIdentifier = topLevelCompositionIdentifier + "__" + node->getCompiler()->getGraphvizIdentifier();

			vector<VuoCompilerTriggerDescription *> triggers = nodeClass->getTriggerDescriptions();
			for (vector<VuoCompilerTriggerDescription *>::iterator j = triggers.begin(); j != triggers.end(); ++j)
			{
				VuoCompilerTriggerDescription *trigger = *j;

				string triggerNodeIdentifier = trigger->getNodeIdentifier();
				string portIdentifier = triggerNodeIdentifier + "__" + trigger->getPortName();
				int portContextIndex = trigger->getPortContextIndex();
				bool canDropEvents = (trigger->getEventThrottling() == VuoPortClass::EventThrottling_Drop);
				VuoType *dataType = trigger->getDataType();
				string subcompositionNodeClassName = trigger->getSubcompositionNodeClassName();
				VuoCompilerNodeClass *subcompositionNodeClass = (subcompositionNodeClassName.empty() ?
																	 nodeClass :
																	 compiler->getNodeClass(subcompositionNodeClassName));
				string subcompositionNodeIdentifier = trigger->getSubcompositionNodeIdentifier();
				string fullSubcompositionNodeIdentifier = nodeIdentifier +
														  (subcompositionNodeIdentifier.empty() ? "" : ("__" + subcompositionNodeIdentifier));

				Function *workerFunctionSrc = subcompositionNodeClass->getTriggerWorkerFunction(portIdentifier);
				Function *workerFunction = VuoCompilerModule::declareFunctionInModule(module, workerFunctionSrc);

				Function *function = generateTriggerSchedulerFunction(dataType, fullSubcompositionNodeIdentifier, triggerNodeIdentifier,
																	  portIdentifier, portContextIndex, canDropEvents, workerFunction);

				string fullTriggerNodeIdentifier = fullSubcompositionNodeIdentifier + "__" + triggerNodeIdentifier;
				subcompositionTriggerFunctions[fullTriggerNodeIdentifier][trigger] = function;
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
 *   int ret = waitForTriggerSemaphore(PlayMovie_decodedImage)  // Only checked if the trigger can drop events.
 *   if (ret == 0)
 *   {
 *     VuoRetain(image);
 *     VuoImage *dataCopy = (VuoImage)malloc(sizeof(VuoImage));
 *     *dataCopy = image;
 *
 *     void **context = (void **)malloc(2 * sizeof(void *));
 *     context[0] = (void *)compositionIdentifier;  // "Top"
 *     context[1] = (void *)dataCopy;
 *     dispatch_async_f(PlayMovie_decodedImage_queue, PlayMovie_decodedImage(), (void *)context);
 *   }
 *   else
 *   {
 *     // Drop the event.
 *     VuoRetain(image);
 *     VuoRelease(image);
 *     sendTelemetry(EventDropped, PlayMovie_decodedImage);
 *   }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateTriggerSchedulerFunction(VuoType *dataType,
																		 string compositionIdentifier, string nodeIdentifier,
																		 string portIdentifier, int portContextIndex,
																		 bool canDropEvents, Function *workerFunction)
{
	string functionName = compositionIdentifier + "__" + workerFunction->getName().str();
	FunctionType *functionType = VuoCompilerCodeGenUtilities::getFunctionType(module, dataType);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	if (dataType)
	{
		Attributes paramAttributes = dataType->getCompiler()->getFunctionParameterAttributes();
		function->addAttribute(1, paramAttributes);
	}

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);
	BasicBlock *scheduleBlock = BasicBlock::Create(module->getContext(), "schedule", function, NULL);

	string fullyQualifiedNodeIdentifier = compositionIdentifier + "__" + nodeIdentifier;
	Value *nodeIdentifierValue = constantStrings.get(module, fullyQualifiedNodeIdentifier);
	Value *nodeContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, initialBlock, nodeIdentifierValue);
	Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, initialBlock, nodeContextValue, portContextIndex);

	if (canDropEvents)
	{
		BasicBlock *checkEventDropBlock = BasicBlock::Create(module->getContext(), "checkEventDrop", function, NULL);
		BranchInst::Create(checkEventDropBlock, initialBlock);

		Value *retValue = VuoCompilerTriggerPort::generateNonBlockingWaitForSemaphore(module, checkEventDropBlock, portContextValue);
		Constant *zeroValue = ConstantInt::get(retValue->getType(), 0);
		ICmpInst *isTriggerAvailableValue = new ICmpInst(*checkEventDropBlock, ICmpInst::ICMP_EQ, retValue, zeroValue, "");
		BasicBlock *dropEventBlock = BasicBlock::Create(module->getContext(), "dropEvent", function, NULL);
		BranchInst::Create(scheduleBlock, dropEventBlock, isTriggerAvailableValue, checkEventDropBlock);

		if (dataType)
			VuoCompilerTriggerPort::generateDataValueDiscardFromScheduler(module, function, dropEventBlock, dataType);
		if (isTopLevelComposition)
			generateSendEventDropped(dropEventBlock, portIdentifier);
		BranchInst::Create(finalBlock, dropEventBlock);
	}
	else
	{
		BranchInst::Create(scheduleBlock, initialBlock);
	}

	Value *compositionIdentifierValue = constantStrings.get(module, compositionIdentifier);
	VuoCompilerTriggerPort::generateAsynchronousSubmissionToDispatchQueue(module, function, scheduleBlock, compositionIdentifierValue, portContextValue, dataType, workerFunction);
	BranchInst::Create(finalBlock, scheduleBlock);

	ReturnInst::Create(module->getContext(), finalBlock);

	return function;
}

/**
 * Generates the trigger worker function, which schedules downstream nodes for execution.
 *
 * void PlayMovie_decodedImage(void *context)
 * {
 *   char *compositionIdentifier = (char *)((void **)context)[0];
 *   VuoImage dataCopy = (VuoImage)((void **)context)[1];
 *
 *   // If paused, ignore this event.
 *   if (isPaused)
 *   {
 *     free(dataCopy);
 *     free(context);
 *     signalTriggerSemaphore(PlayMovie_decodedImage);
 *     return;
 *   }
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
 *   free(dataCopy);
 *   free(context);
 *   signalNodeSemaphore(PlayMovie);
 *   signalTriggerSemaphore(PlayMovie_decodedImage);
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
 *   size_t chainGroupsBytes = 3 * sizeof(dispatch_group_t);
 *   dispatch_group_t *chainGroups = (dispatch_group_t *)malloc(chainGroupsBytes);
 *   chainGroups[0] = dispatch_group_create();  // TwirlImage
 *   chainGroups[1] = dispatch_group_create();  // RippleImage
 *   chainGroups[2] = dispatch_group_create();  // BlendImages
 *   dispatch_group_enter(chainGroups[0]);
 *   dispatch_group_enter(chainGroups[1]);
 *   dispatch_group_enter(chainGroups[2]);
 *   dispatch_retain(chainGroups[0]);
 *   dispatch_retain(chainGroups[1]);
 *
 *   unsigned long *eventIdPtr = (unsigned long *)malloc(sizeof(unsigned long));
 *   *eventIdPtr = eventId;
 *
 *   size_t contextBytes = 3 * sizeof(void *);
 *   void **context = (void **)malloc(contextBytes);
 *   context[0] = (void *)compositionIdentifier;
 *   context[1] = (void *)eventIdPtr;
 *   context[2] = (void *)chainGroups;
 *   VuoRegister(context, vuoFreeChainWorkerContext);
 *
 *   dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
 *
 *   VuoRetain(context);
 *   VuoRetain(context);
 *   dispatch_async_f(globalQueue, (void *)context, PlayMovie_decodedImage__TwirlImage__worker);
 *   dispatch_async_f(globalQueue, (void *)context, PlayMovie_decodedImage__RippleImage__worker);
 * }
 *
 * void vuoFreeChainWorkerContext(void *context)
 * {
 *   free(((void **)context)[1]);
 *   free(((void **)context)[2]);
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
 *   dispatch_group_t *chainGroups = (dispatch_group_t *)((void **)context)[2];
 *
 *   VuoRetain(context);
 *   dispatch_async_f(globalQueue, context, PlayMovie_decodedImage__BlendImages__worker);
 *
 *   // Clean up.
 *
 *   dispatch_group_leave(chainGroups[0]);
 *   dispatch_release(chainGroups[0]);
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
 *   dispatch_group_leave(chainGroups[1]);
 *   dispatch_release(chainGroups[1]);
 *   VuoRelease(context);
 * }
 *
 * void PlayMovie_decodedImage__BlendImages__worker(void *context)
 * {
 *   // Wait for the chains immediately upstream to complete.
 *
 *   dispatch_group_wait(chainGroups[0]);     // TwirlImage
 *   dispatch_release(chainGroups[0]);
 *   dispatch_group_wait(chainGroups[1]);     // RippleImage
 *   dispatch_release(chainGroups[1]);
 *
 *   ...
 *
 *   // Clean up.
 *
 *   dispatch_group_leave(chainGroups[2]);
 *   dispatch_release(chainGroups[2]);
 *   VuoRelease(context);
 * }
 * }
 */
Function * VuoCompilerBitcodeGenerator::generateTriggerWorkerFunction(VuoCompilerTriggerPort *trigger)
{
	Function *function = trigger->getWorkerFunction(module, VuoStringUtilities::prefixSymbolName(trigger->getIdentifier(), moduleKey), true);

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *triggerBlock = BasicBlock::Create(module->getContext(), "trigger", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Value *compositionIdentifierValue = trigger->generateCompositionIdentifierValue(module, initialBlock, function);
	Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, initialBlock, compositionIdentifierValue);

	VuoCompilerNode *triggerNode = graph->getNodesForTriggerPorts()[trigger];
	Value *triggerNodeContextValue = triggerNode->generateGetContext(module, initialBlock, compositionIdentifierValue);

	// If paused, ignore this event.
	BasicBlock *isPausedBlock = BasicBlock::Create(module->getContext(), "isPaused", function, NULL);
	ICmpInst *isPausedValueIsTrue = VuoCompilerCodeGenUtilities::generateIsPausedComparison(module, initialBlock);
	BranchInst::Create(isPausedBlock, triggerBlock, isPausedValueIsTrue, initialBlock);
	trigger->generateDataValueDiscardFromWorker(module, isPausedBlock, function);
	trigger->generateFreeContext(module, isPausedBlock, function);
	trigger->generateSignalForSemaphore(module, isPausedBlock, triggerNodeContextValue);
	BranchInst::Create(finalBlock, isPausedBlock);
	// Otherwise...

	bool isNodeEventForSubcomposition = (! isTopLevelComposition && trigger == getPublishedInputTrigger());

	Value *eventIdValue;
	if (! isNodeEventForSubcomposition)
	{
		// Get a unique ID for this event.
		eventIdValue = generateGetNextEventID(module, triggerBlock);

		// Claim all necessary downstream nodes.
		vector<VuoCompilerNode *> triggerWaitNodes = getNodesToWaitOnBeforeTransmission(trigger);
		generateWaitForNodes(module, function, triggerBlock, compositionIdentifierValue, triggerWaitNodes, eventIdValue);
	}
	else
	{
		// Use the unique ID for this event already created by nodeEvent()/nodeInstanceEvent().
		eventIdValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingEventId(module, triggerBlock, compositionContextValue);

		// All necessary downstream nodes have already been claimed by nodeEvent()/nodeInstanceEvent().
	}

	// Handle the trigger port value having changed.
	Value *triggerDataValue = trigger->generateDataValueUpdate(module, triggerBlock, function, triggerNodeContextValue);
	trigger->generateFreeContext(module, triggerBlock, function);
	vector<VuoCompilerNode *> downstreamNodes = graph->getNodesDownstream(trigger);
	if (find(downstreamNodes.begin(), downstreamNodes.end(), triggerNode) == downstreamNodes.end())
		generateSignalForNodes(module, triggerBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, triggerNode));
	trigger->generateSignalForSemaphore(module, triggerBlock, triggerNodeContextValue);

	// Transmit events and data (if any) out of the trigger port, and send telemetry for port updates.
	generateTransmissionFromOutputPort(function, triggerBlock, compositionIdentifierValue, trigger, NULL, triggerDataValue);


	// Schedule the chain worker function for each chain immediately downstream of the trigger.

	map<VuoCompilerChain *, vector<VuoCompilerChain *> > chainsImmediatelyDownstream;
	map<VuoCompilerChain *, vector<VuoCompilerChain *> > chainsImmediatelyUpstream;
	set<VuoCompilerChain *> chainsScheduled;
	set<VuoCompilerNode *> nodesScheduled;

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

		Value *contextValue = VuoCompilerChain::generateMakeContext(module, triggerBlock, compositionIdentifierValue,
																	eventIdValue, allChains, chainsImmediatelyDownstream);

		vector<VuoCompilerChain *> firstChains;
		for (vector<VuoCompilerChain *>::iterator i = allChains.begin(); i != allChains.end(); ++i)
		{
			VuoCompilerChain *chain = *i;
			map<VuoCompilerChain *, vector<VuoCompilerChain *> >::iterator upstreamIter = chainsImmediatelyUpstream.find(chain);
			if (upstreamIter == chainsImmediatelyUpstream.end())
				firstChains.push_back(chain);
		}

		generateAndScheduleChainWorkerFunctions(triggerBlock, contextValue, firstChains, trigger, allChains,
												chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled, nodesScheduled);
	}
	else if (publishedInputNode && trigger == getPublishedInputTrigger())
	{
		// If this is an event from the published input trigger via nodeEvent()/nodeInstanceEvent(), then...
		Value *subcompositionEventIdValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingEventId(module, triggerBlock, compositionContextValue);
		ICmpInst *isSubcompositionEventValue = new ICmpInst(*triggerBlock, ICmpInst::ICMP_EQ, eventIdValue, subcompositionEventIdValue, "");
		BasicBlock *leaveBlock = BasicBlock::Create(module->getContext(), "leave", function, NULL);
		BranchInst::Create(leaveBlock, finalBlock, isSubcompositionEventValue, triggerBlock);

		// Leave the dispatch group waited on by nodeEvent()/nodeInstanceEvent().
		Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, leaveBlock, compositionContextValue);
		VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, leaveBlock, subcompositionOutputGroupValue);
		triggerBlock = leaveBlock;
	}

	BranchInst::Create(finalBlock, triggerBlock);
	ReturnInst::Create(module->getContext(), finalBlock);

	return function;
}

/**
 * Generates and schedules a chain worker function for each of @a chainsToSchedule.
 */
void VuoCompilerBitcodeGenerator::generateAndScheduleChainWorkerFunctions(BasicBlock *schedulerBlock, Value *contextValueInScheduler,
																		  const vector<VuoCompilerChain *> &chainsToSchedule, VuoCompilerTriggerPort *trigger,
																		  const vector<VuoCompilerChain *> &allChains,
																		  const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream,
																		  const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
																		  set<VuoCompilerChain *> &chainsScheduled, set<VuoCompilerNode *> &nodesScheduled)
{
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

	for (vector<VuoCompilerChain *>::const_iterator i = uniqueChainsToSchedule.begin(); i != uniqueChainsToSchedule.end(); ++i)
	{
		// VuoRetain(context);
		VuoCompilerCodeGenUtilities::generateRetainCall(module, schedulerBlock, contextValueInScheduler);
	}

	for (vector<VuoCompilerChain *>::const_iterator i = uniqueChainsToSchedule.begin(); i != uniqueChainsToSchedule.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		generateAndScheduleChainWorkerFunction(schedulerBlock, contextValueInScheduler, chain, trigger, allChains,
											   chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled, nodesScheduled);
	}
}

/**
 * Generates a chain worker function, which executes each node in the chain, and schedules the function to run.
 */
void VuoCompilerBitcodeGenerator::generateAndScheduleChainWorkerFunction(BasicBlock *schedulerBlock, Value *contextValueInScheduler,
																		 VuoCompilerChain *chain, VuoCompilerTriggerPort *trigger,
																		 const vector<VuoCompilerChain *> &allChains,
																		 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyDownstream,
																		 const map<VuoCompilerChain *, vector<VuoCompilerChain *> > &chainsImmediatelyUpstream,
																		 set<VuoCompilerChain *> &chainsScheduled, set<VuoCompilerNode *> &nodesScheduled)
{
	// Schedule the following:

	size_t chainIndex = find(allChains.begin(), allChains.end(), chain) - allChains.begin();
	Function *chainWorker = chain->generateSubmissionForDispatchGroup(module, schedulerBlock, contextValueInScheduler, trigger->getIdentifier());

	BasicBlock *chainBlock = BasicBlock::Create(module->getContext(), "", chainWorker, 0);
	Value *contextValueInChainWorker = chainWorker->arg_begin();
	Value *compositionIdentifierValue = chain->generateCompositionIdentifierValue(module, chainBlock, contextValueInChainWorker);
	Value *eventIdValue = chain->generateEventIdValue(module, chainBlock, contextValueInChainWorker);


	// Wait for any chains immediately upstream to complete.
	vector<VuoCompilerChain *> upstreamChains;
	map<VuoCompilerChain *, vector<VuoCompilerChain *> >::const_iterator upstreamChainsIter = chainsImmediatelyUpstream.find(chain);
	if (upstreamChainsIter != chainsImmediatelyUpstream.end())
		upstreamChains = upstreamChainsIter->second;
	if (! upstreamChains.empty())
	{
		vector<size_t> upstreamChainIndices;
		for (vector<VuoCompilerChain *>::iterator i = upstreamChains.begin(); i != upstreamChains.end(); ++i)
			upstreamChainIndices.push_back( find(allChains.begin(), allChains.end(), *i) - allChains.begin() );
		chain->generateWaitForUpstreamChains(module, chainBlock, contextValueInChainWorker, upstreamChainIndices);
	}

	// For each node in the chain...
	vector<VuoCompilerNode *> chainNodes = chain->getNodes();
	for (vector<VuoCompilerNode *>::iterator j = chainNodes.begin(); j != chainNodes.end(); ++j)
	{
		VuoCompilerNode *node = *j;

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
		nodeExecutionArgs.push_back(compositionIdentifierValue);
		nodeExecutionArgs.push_back(eventIdValue);
		CallInst *isHitValue = CallInst::Create(nodeExecutionFunction, nodeExecutionArgs, "", chainBlock);

		// Whether or not the event hit the node, wait on any necessary downstream nodes.
		if (! (graph->isRepeatedInFeedbackLoop(node, trigger) && chain->isLastNodeInLoop()))
		{
			vector<VuoCompilerNode *> outputNodes = getNodesToWaitOnBeforeTransmission(trigger, node);
			generateWaitForNodes(module, chainWorker, chainBlock, compositionIdentifierValue, outputNodes, eventIdValue);
		}

		// If the event hit the node, transmit events and data through its output cables and send telemetry.
		vector<Value *> nodeTransmissionArgs;
		nodeTransmissionArgs.push_back(compositionIdentifierValue);
		nodeTransmissionArgs.push_back(isHitValue);
		CallInst::Create(nodeTransmissionFunction, nodeTransmissionArgs, "", chainBlock);

		// Whether or not the event hit the node, if this was the last time this event could reach the node,
		// signal the node's semaphore.
		if (! (graph->isRepeatedInFeedbackLoop(node, trigger) && ! chain->isLastNodeInLoop()))
			generateSignalForNodes(module, chainBlock, compositionIdentifierValue, vector<VuoCompilerNode *>(1, node));

		nodesScheduled.insert(node);
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

		generateAndScheduleChainWorkerFunctions(chainBlock, contextValueInChainWorker, nextChains, trigger, allChains,
												chainsImmediatelyDownstream, chainsImmediatelyUpstream, chainsScheduled, nodesScheduled);
	}

	chain->generateCleanupForWorkerFunction(module, chainBlock, contextValueInChainWorker, chainIndex, ! downstreamChains.empty());

	ReturnInst::Create(module->getContext(), chainBlock);
}

/**
 * Generates a function that executes the node if the node received an event.
 *
 * @eg{
 * bool vuo_math_subtract__Subtract__execute(char *compositionIdentifier, unsigned long eventId)
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
 * bool vuo_out__PublishedOutputs__execute(char *compositionIdentifier, unsigned long eventId)
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
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Type *boolType = IntegerType::get(module->getContext(), 1);
	Type *eventIdType = VuoCompilerCodeGenUtilities::generateNoEventIdConstant(module)->getType();
	vector<Type *> params;
	params.push_back(pointerToCharType);
	params.push_back(eventIdType);
	FunctionType *functionType = FunctionType::get(boolType, params, false);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
	Value *eventIdValue = args++;
	eventIdValue->setName("eventIdValue");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	Value *nodeContextValue = node->generateGetContext(module, initialBlock, compositionIdentifierValue);
	Value *isHitValue = node->generateReceivedEventCondition(module, initialBlock, nodeContextValue);

	if (node->getBase() == publishedOutputNode)
	{
		if (! isTopLevelComposition)
		{
			// Call the trigger functions for any published trigger ports that the event has hit.

			BasicBlock *currBlock = initialBlock;

			Value *compositionContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContext(module, currBlock, compositionIdentifierValue);

			vector<VuoPort *> publishedOutputPortsOnNode = publishedOutputNode->getInputPorts();
			for (size_t i = VuoNodeClass::unreservedInputPortStartIndex; i < publishedOutputPortsOnNode.size(); ++i)
			{
				VuoPort *port = publishedOutputPortsOnNode[i];

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
						bool isPassedByValue = dataType->getCompiler()->getFunctionParameterAttributes().hasAttribute(Attributes::ByVal);
						bool isLoweredToTwoParameters = eventPortClass->getDataClass()->isLoweredToTwoParameters();
						Value *secondArg = NULL;
						Value **secondArgIfNeeded = (isLoweredToTwoParameters ? &secondArg : NULL);
						Value *arg = VuoCompilerCodeGenUtilities::convertArgumentToParameterType(dataValue, triggerFunctionType, 0, isPassedByValue,
																								 secondArgIfNeeded, module, triggerBlock);
						args.push_back(arg);
						if (secondArg)
							args.push_back(secondArg);
					}

					int indexInOutputPorts = VuoNodeClass::unreservedOutputPortStartIndex + i - VuoNodeClass::unreservedInputPortStartIndex;
					int indexInPorts = VuoNodeClass::unreservedInputPortStartIndex + composition->getBase()->getPublishedInputPorts().size() + indexInOutputPorts;
					Value *portContextValue = VuoCompilerCodeGenUtilities::generateGetNodeContextPortContext(module, triggerBlock, compositionContextValue, indexInPorts);
					Value *triggerFunction = VuoCompilerCodeGenUtilities::generateGetPortContextTriggerFunction(module, triggerBlock, portContextValue, triggerFunctionType);

					CallInst::Create(triggerFunction, args, "", triggerBlock);
					BranchInst::Create(nextBlock, triggerBlock);

					currBlock = nextBlock;
				}
				else
				{
					int indexInEvents = i - VuoNodeClass::unreservedInputPortStartIndex;
					VuoCompilerCodeGenUtilities::generateSetNodeContextOutputEvent(module, currBlock, compositionContextValue, indexInEvents, isPortHitValue);
				}
			}

			// If this event (which may or may not have actually hit the published output node) is from nodeEvent()/nodeInstanceEvent(), then...

			Value *subcompositionEventIdValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingEventId(module, currBlock, compositionContextValue);
			ICmpInst *isSubcompositionEventValue = new ICmpInst(*currBlock, ICmpInst::ICMP_EQ, eventIdValue, subcompositionEventIdValue, "");
			BasicBlock *leaveBlock = BasicBlock::Create(module->getContext(), "leave", function, NULL);
			BranchInst::Create(leaveBlock, finalBlock, isSubcompositionEventValue, currBlock);

			// Leave the dispatch group waited on by nodeEvent()/nodeInstanceEvent().

			Value *subcompositionOutputGroupValue = VuoCompilerCodeGenUtilities::generateGetNodeContextExecutingGroup(module, leaveBlock, compositionContextValue);
			VuoCompilerCodeGenUtilities::generateLeaveDispatchGroup(module, leaveBlock, subcompositionOutputGroupValue);
			BranchInst::Create(finalBlock, leaveBlock);
		}
		else
		{
			BranchInst::Create(finalBlock, initialBlock);
		}
	}
	else
	{
		// If the node received an event, then...
		BasicBlock *executeBlock = BasicBlock::Create(module->getContext(), "execute", function, NULL);
		BranchInst::Create(executeBlock, finalBlock, isHitValue, initialBlock);

		// Call the node's event function, and send telemetry that the node's execution has started and finished.
		generateNodeExecution(function, executeBlock, compositionIdentifierValue, node);
		BranchInst::Create(finalBlock, executeBlock);
	}

	ReturnInst::Create(module->getContext(), isHitValue, finalBlock);

	return function;
}

/**
 * Generates a function that transmits data and events from the node's output ports after the node has executed.
 *
 * @eg{
 * void vuo_math_subtract__Subtract__transmit(char *compositionIdentifier, bool isHit)
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
	PointerType *pointerToCharType = PointerType::get(IntegerType::get(module->getContext(), 8), 0);
	Type *boolType = IntegerType::get(module->getContext(), 1);
	vector<Type *> params;
	params.push_back(pointerToCharType);
	params.push_back(boolType);
	FunctionType *functionType = FunctionType::get(Type::getVoidTy(module->getContext()), params, false);
	Function *function = Function::Create(functionType, GlobalValue::InternalLinkage, functionName, module);

	Function::arg_iterator args = function->arg_begin();
	Value *compositionIdentifierValue = args++;
	compositionIdentifierValue->setName("compositionIdentifier");
	Value *isHitValue = args++;
	isHitValue->setName("isHit");

	BasicBlock *initialBlock = BasicBlock::Create(module->getContext(), "initial", function, NULL);
	BasicBlock *transmitBlock = BasicBlock::Create(module->getContext(), "transmit", function, NULL);
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, NULL);

	// If the node received an event, then...
	BranchInst::Create(transmitBlock, finalBlock, isHitValue, initialBlock);

	Value *nodeContextValue = node->generateGetContext(module, transmitBlock, compositionIdentifierValue);

	if (node->getBase() == publishedOutputNode)
	{
		if (isTopLevelComposition)
			generateTelemetryFromPublishedOutputNode(function, transmitBlock, nodeContextValue, node);
	}
	else
	{
		// Transmit events and data through the node's outgoing cables, and send telemetry for port updates.
		generateTransmissionFromNode(function, transmitBlock, compositionIdentifierValue, nodeContextValue, node);
	}

	// Reset the node's event inputs and outputs.
	size_t portCount = node->getBase()->getInputPorts().size() + node->getBase()->getOutputPorts().size();
	VuoCompilerCodeGenUtilities::generateResetNodeContextEvents(module, transmitBlock, nodeContextValue, portCount);

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
