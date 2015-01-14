/**
 * @file
 * VuoCompilerBitcodeGenerator implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <queue>
#include <stack>
#include "VuoCompilerBitcodeGenerator.hh"
#include "VuoCompilerCodeGenUtilities.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerOutputEventPort.hh"
#include "VuoCompilerPublishedInputPort.hh"
#include "VuoFileUtilities.hh"

#include "VuoPort.hh"

/**
 * Creates a bitcode generator from the specified composition.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromComposition(VuoCompilerComposition *composition, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator;
	cg->compiler = compiler;
	cg->composition = composition;
	cg->initialize();
	return cg;
}

/**
 * Creates a bitcode generator from the specified composition string @c composition.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionString(const string &composition, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator;
	cg->compiler = compiler;
	cg->composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(composition, compiler);
	cg->initialize();
	return cg;
}

/**
 * Creates a bitcode generator from the specified .vuo file @c compositionPath.
 */
VuoCompilerBitcodeGenerator * VuoCompilerBitcodeGenerator::newBitcodeGeneratorFromCompositionFile(const string &compositionPath, VuoCompiler *compiler)
{
	VuoCompilerBitcodeGenerator * cg = new VuoCompilerBitcodeGenerator;
	cg->compiler = compiler;
	VuoCompilerGraphvizParser * parser = new VuoCompilerGraphvizParser(compositionPath, compiler);
	cg->composition = new VuoCompilerComposition(new VuoComposition(), parser);
	delete parser;
	cg->initialize();
	return cg;
}

/**
 * Private constructor.
 */
VuoCompilerBitcodeGenerator::VuoCompilerBitcodeGenerator(void)
{
	composition = NULL;
	compiler = NULL;
	module = NULL;
	debugMode = false;
}

/**
 * Helper for factory methods.
 */
void VuoCompilerBitcodeGenerator::initialize(void)
{
	makeEdgesForNode();
	makeDownstreamEdges();
	makeOrderedNodes();
	makeNodeForTrigger();
	makeTriggerObjects();

	for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		makeEdgesForTrigger(trigger);
		makeChainsForTrigger(trigger);
	}

	debugMode = false;
}

/**
 * Creates an edge for each bundle of cables connecting a node's trigger port or set of output ports
 * to another node's inputs.
 */
void VuoCompilerBitcodeGenerator::makeEdgesForNode(void)
{
	typedef pair<VuoCompilerNode *, VuoCompilerNode *> nodePair;
	typedef pair<VuoCompilerOutputEventPort *, VuoCompilerInputEventPort *> portPair;
	map<pair<nodePair, VuoCompilerTriggerPort *>, set<VuoCompilerInputEventPort *> > triggerEdgeParts;
	map<nodePair, set<portPair> > passiveEdgeParts;

	// Collect cables into edge-like structures.
	set<VuoCable *> cables = composition->getBase()->getCables();
	set<VuoCable *> publishedInputCables = composition->getBase()->getPublishedInputCables();
	cables.insert(publishedInputCables.begin(), publishedInputCables.end());
	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		VuoCable *cable = *i;
		nodePair fromToNodes = make_pair(cable->getFromNode()->getCompiler(), cable->getToNode()->getCompiler());
		VuoCompilerInputEventPort *toPort = static_cast<VuoCompilerInputEventPort *>(cable->getToPort()->getCompiler());
		VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(cable->getFromPort()->getCompiler());
		if (fromTrigger)
		{
			triggerEdgeParts[ make_pair(fromToNodes, fromTrigger) ].insert(toPort);
		}
		else
		{
			VuoCompilerOutputEventPort *fromPort = dynamic_cast<VuoCompilerOutputEventPort *>(cable->getFromPort()->getCompiler());
			passiveEdgeParts[fromToNodes].insert( make_pair(fromPort, toPort) );
		}
	}

	// Create the trigger edges.
	for (map<pair<nodePair, VuoCompilerTriggerPort *>, set<VuoCompilerInputEventPort *> >::iterator i = triggerEdgeParts.begin(); i != triggerEdgeParts.end(); ++i)
	{
		VuoCompilerNode *fromNode = i->first.first.first;
		VuoCompilerNode *toNode = i->first.first.second;
		VuoCompilerTriggerPort *fromTrigger = i->first.second;
		set<VuoCompilerInputEventPort *> toPorts = i->second;
		VuoCompilerTriggerEdge *edge = new VuoCompilerTriggerEdge(fromNode, toNode, fromTrigger, toPorts);
		triggerInEdgesForNode[toNode].insert(edge);
	}

	// Create the passive edges.
	for (map<nodePair, set<portPair> >::iterator i = passiveEdgeParts.begin(); i != passiveEdgeParts.end(); ++i)
	{
		VuoCompilerNode *fromNode = i->first.first;
		VuoCompilerNode *toNode = i->first.second;
		set<portPair> fromToPorts = i->second;
		VuoCompilerPassiveEdge *edge = new VuoCompilerPassiveEdge(fromNode, toNode, fromToPorts);
		passiveInEdgesForNode[toNode].insert(edge);
		passiveOutEdgesForNode[fromNode].insert(edge);
	}
}

/**
 * Creates the mapping of each trigger to the node that contains it.
 */
void VuoCompilerBitcodeGenerator::makeNodeForTrigger(void)
{
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	VuoNode *publishedInputNode = composition->getPublishedInputNode();
	if (publishedInputNode)
		nodes.insert(publishedInputNode);
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for (vector<VuoPort *>::iterator j = outputPorts.begin(); j != outputPorts.end(); ++j)
		{
			VuoCompilerTriggerPort *trigger = dynamic_cast<VuoCompilerTriggerPort *>((*j)->getCompiler());
			if (trigger)
				nodeForTrigger[trigger] = node->getCompiler();
		}
	}
}

/**
 * Returns the set of outgoing edges from a node that may transmit an event from the given incoming edge to the node.
 *
 * The returned set always includes all outgoing edges from the node's done port.
 */
set<VuoCompilerPassiveEdge *> VuoCompilerBitcodeGenerator::outEdgesThatMayTransmitFromInEdge(VuoCompilerEdge *inEdge)
{
	set<VuoCompilerPassiveEdge *> transmittingOutEdges;

	VuoCompilerNode *node = inEdge->getToNode();
	set<VuoCompilerPassiveEdge *> passiveOutEdges = passiveOutEdgesForNode[node];
	if (inEdge->mayTransmitThroughNode())
	{
		transmittingOutEdges = passiveOutEdges;
	}
	else
	{
		VuoCompilerOutputEventPort *donePort = static_cast<VuoCompilerOutputEventPort *>(node->getBase()->getDonePort()->getCompiler());
		for (set<VuoCompilerPassiveEdge *>::iterator i = passiveOutEdges.begin(); i != passiveOutEdges.end(); ++i)
		{
			VuoCompilerPassiveEdge *outEdge = *i;
			set<VuoCompilerOutputEventPort *> outputPorts = outEdge->getOutputPorts();
			for (set<VuoCompilerOutputEventPort *>::iterator j = outputPorts.begin(); j != outputPorts.end(); ++j)
				if (*j == donePort)
					transmittingOutEdges.insert(outEdge);
		}
	}

	return transmittingOutEdges;
}

/**
 * Creates the lists of edges that may be pushed by the trigger.
 */
void VuoCompilerBitcodeGenerator::makeEdgesForTrigger(VuoCompilerTriggerPort *trigger)
{
	// This is a depth-first search of the graph edges, starting from the trigger.

	stack<VuoCompilerEdge *> edgesToVisit;
	map<VuoCompilerEdge *, bool> edgesSeen;

	// Add each out-edge of the trigger to...
	set<VuoCompilerTriggerEdge *> triggerEdges = getTriggerEdges();
	for (set<VuoCompilerTriggerEdge *>::iterator i = triggerEdges.begin(); i != triggerEdges.end(); ++i)
	{
		VuoCompilerTriggerEdge *edge = *i;
		if (edge->getTrigger() == trigger)
		{
			// ... the list of reachable trigger edges.
			triggerEdgesForTrigger[trigger].insert(edge);

			// ... the list of edges to visit.
			edgesToVisit.push(edge);
		}
	}

	// Visit each edge reachable from the trigger.
	while (! edgesToVisit.empty())
	{
		VuoCompilerEdge *edge = edgesToVisit.top();
		edgesToVisit.pop();

		// Plan to visit each out-edge that may get an event from the current edge.
		set<VuoCompilerPassiveEdge *> outEdges = outEdgesThatMayTransmitFromInEdge(edge);
		for (set<VuoCompilerPassiveEdge *>::iterator i = outEdges.begin(); i != outEdges.end(); ++i)
		{
			// If the out-edge has not yet been visited or planned to visit, add it to...
			VuoCompilerPassiveEdge *outEdge = *i;
			if (! edgesSeen[outEdge])
			{
				// ... the list of reachable passive edges.
				passiveEdgesForTrigger[trigger].insert(outEdge);

				// ... the list of edges to visit.
				edgesToVisit.push(outEdge);
				edgesSeen[outEdge] = true;
			}
		}
	}

	// In case the trigger has no out-edges, add empty lists of edges.
	triggerEdgesForTrigger[trigger];
	passiveEdgesForTrigger[trigger];
}

/**
 * Creates the list of edges downstream of each edge.
 *
 * Reports an error if the composition contains an infinite feedback loop or a deadlocked feedback loop.
 */
void VuoCompilerBitcodeGenerator::makeDownstreamEdges(void)
{
	list<VuoCompilerEdge *> edgesToVisit;  // Used as a stack, except for a call to find().

	// Prepare to visit each trigger edge.
	set<VuoCompilerTriggerEdge *> triggerEdges = getTriggerEdges();
	for (set<VuoCompilerTriggerEdge *>::iterator i = triggerEdges.begin(); i != triggerEdges.end(); ++i)
	{
		VuoCompilerTriggerEdge *triggerEdge = *i;
		edgesToVisit.push_back(triggerEdge);
	}

	// Use dynamic programming to find the edges downstream of each edge.
	while (! edgesToVisit.empty())
	{
		// Visit an edge.
		VuoCompilerEdge *edge = edgesToVisit.back();

		set<VuoCompilerPassiveEdge *> downstreamEdges;
		bool areDownstreamEdgesComplete = true;

		// Prepare to visit any out-edges that have not yet been visited.
		set<VuoCompilerPassiveEdge *> outEdges = outEdgesThatMayTransmitFromInEdge(edge);
		for (set<VuoCompilerPassiveEdge *>::iterator i = outEdges.begin(); i != outEdges.end(); ++i)
		{
			VuoCompilerPassiveEdge *outEdge = *i;
			downstreamEdges.insert(outEdge);

			map<VuoCompilerEdge *, set<VuoCompilerPassiveEdge *> >::iterator downstreamEdgesIter = downstreamEdgesForEdge.find(outEdge);
			if (downstreamEdgesIter != downstreamEdgesForEdge.end())
			{
				// The out-edge has been visited, so add its downstream edges to this edge's.
				set<VuoCompilerPassiveEdge *> downstreamEdgesForOutEdge = downstreamEdgesIter->second;
				downstreamEdges.insert(downstreamEdgesForOutEdge.begin(), downstreamEdgesForOutEdge.end());
			}
			else
			{
				// Check for an infinite feedback loop.
				if (find(edgesToVisit.begin(), edgesToVisit.end(), outEdge) != edgesToVisit.end())
				{
					fprintf(stderr, "Infinite feedback loop involving cables from %s to %s\n", outEdge->getFromNode()->getBase()->getTitle().c_str(), outEdge->getToNode()->getBase()->getTitle().c_str());
					downstreamEdgesForEdge.clear();
					return;
				}

				// The out-edge has not been visited. It needs to be visited before this edge's downstream edges can be determined.
				edgesToVisit.push_back(outEdge);
				areDownstreamEdgesComplete = false;
			}
		}

		if (areDownstreamEdgesComplete)
		{
			// All out-edges have been visited. These out-edges and their downstream edges become this edge's downstream edges.
			downstreamEdgesForEdge[edge] = downstreamEdges;
			edgesToVisit.pop_back();
		}
	}

	checkForDeadlockedFeedbackLoops();
}

/**
 * Check for a deadlocked feedback loop (where node A must execute before node B, and node B must execute before node A).
 */
void VuoCompilerBitcodeGenerator::checkForDeadlockedFeedbackLoops(void)
{
	// Create a set of all nodes downstream of each node, except for the node containing the trigger port.
	map<VuoCompilerNode *, set<VuoCompilerNode *> > downstreamNodesForNode;
	for (map<VuoCompilerEdge *, set<VuoCompilerPassiveEdge *> >::iterator i = downstreamEdgesForEdge.begin(); i != downstreamEdgesForEdge.end(); ++i)
	{
		VuoCompilerEdge *upstreamEdge = i->first;
		VuoCompilerNode *upstreamNode = upstreamEdge->getToNode();
		set<VuoCompilerPassiveEdge *> downstreamEdges = i->second;
		for (set<VuoCompilerPassiveEdge *>::iterator j = downstreamEdges.begin(); j != downstreamEdges.end(); ++j)
		{
			VuoCompilerPassiveEdge *downstreamEdge = *j;
			VuoCompilerNode *downstreamNode = downstreamEdge->getToNode();
			downstreamNodesForNode[upstreamNode].insert(downstreamNode);
		}
	}

	// Find any cases where two nodes are mutually downstream of, and dependent on, each other
	// (excluding the valid case where the repeated node of a feedback loop is both upstream and downstream of each node in the loop).
	for (map<VuoCompilerNode *, set<VuoCompilerNode *> >::iterator i = downstreamNodesForNode.begin(); i != downstreamNodesForNode.end(); ++i)
	{
		VuoCompilerNode *upstreamNode = i->first;
		set<VuoCompilerNode *> downstreamNodesOfUpstreamNode = i->second;
		for (set<VuoCompilerNode *>::iterator j = downstreamNodesOfUpstreamNode.begin(); j != downstreamNodesOfUpstreamNode.end(); ++j)
		{
			VuoCompilerNode *downstreamNode = *j;

			// Are downstreamNode and upstreamNode different nodes?
			if (downstreamNode != upstreamNode)
			{
				set<VuoCompilerNode *> downstreamNodesOfDownstreamNode = downstreamNodesForNode[downstreamNode];

				// Does upstreamNode have at least as many downstream nodes as downstreamNode does?
				// (Avoid reporting an error when upstreamNode is downstream of downstreamNode in a valid feedback loop.)
				if (downstreamNodesOfUpstreamNode.size() >= downstreamNodesOfDownstreamNode.size())
				{
					// Is upstreamNode downstream of downstreamNode?
					if (downstreamNodesOfDownstreamNode.find(upstreamNode) != downstreamNodesOfDownstreamNode.end())
					{
						// Are the nodes downstream of downstreamNode *not* a subset of the nodes downstream of upstreamNode?
						// If so, report an error.
						for (set<VuoCompilerNode *>::iterator k = downstreamNodesOfDownstreamNode.begin(); k != downstreamNodesOfDownstreamNode.end(); ++k)
						{
							VuoCompilerNode *downstreamNodeOfDownstreamNode = *k;
							if (downstreamNodesOfUpstreamNode.find(downstreamNodeOfDownstreamNode) == downstreamNodesOfUpstreamNode.end())
							{
								fprintf(stderr, "Deadlocked feedback loop involving nodes %s and %s\n", upstreamNode->getBase()->getTitle().c_str(), downstreamNode->getBase()->getTitle().c_str());
								downstreamEdgesForEdge.clear();
								return;
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Creates a topologically sorted list of all nodes in the graph.
 *
 * Assumes the list of downstream edges has been created.
 */
void VuoCompilerBitcodeGenerator::makeOrderedNodes(void)
{
	// If the composition has an infinite or deadlocked feedback loop, don't create the node list.
	if (downstreamEdgesForEdge.empty())
		return;

	// This is a topological sort of the graph nodes. (http://en.wikipedia.org/wiki/Topological_sorting)

	// Create a list of edges to visit...
	set<VuoCompilerPassiveEdge *> edgesToVisit;
	set<VuoCompilerPassiveEdge *> backEdges;
	for (map<VuoCompilerEdge *, set<VuoCompilerPassiveEdge *> >::iterator i = downstreamEdgesForEdge.begin(); i != downstreamEdgesForEdge.end(); ++i)
	{
		VuoCompilerEdge *upstreamEdge = i->first;
		set<VuoCompilerPassiveEdge *> downstreamEdges = i->second;
		for (set<VuoCompilerPassiveEdge *>::iterator j = downstreamEdges.begin(); j != downstreamEdges.end(); ++j)
		{
			VuoCompilerPassiveEdge *downstreamEdge = *j;
			if (upstreamEdge->getToNode() == downstreamEdge->getToNode())
			{
				loopEndNodes.insert(upstreamEdge->getToNode());
				backEdges.insert(downstreamEdge);
			}
			else
			{
				edgesToVisit.insert(downstreamEdge);
			}
		}
	}

	// ... excluding back-edges (in feedback loops).
	for (set<VuoCompilerPassiveEdge *>::iterator i = backEdges.begin(); i != backEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *edge = *i;
		edgesToVisit.erase(edge);
	}

	// Create the initial list of nodes to visit: those whose only in-edges come from triggers.
	set<VuoCompilerNode *> nodesToVisit;
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		if (! triggerInEdgesForNode[node].empty())
		{
			bool hasPassiveInEdgesToVisit = false;
			set<VuoCompilerPassiveEdge *> passiveInEdges = passiveInEdgesForNode[node];
			for (set<VuoCompilerPassiveEdge *>::iterator j = passiveInEdges.begin(); j != passiveInEdges.end(); ++j)
			{
				VuoCompilerPassiveEdge *edge = *j;
				if (edgesToVisit.find(edge) != edgesToVisit.end())
				{
					hasPassiveInEdgesToVisit = true;
					break;
				}
			}

			if (! hasPassiveInEdgesToVisit)
				nodesToVisit.insert(node);
		}
	}

	// Visit the nodes in topological order, adding them to the list.
	while (! nodesToVisit.empty())
	{
		// Add this node to the list.
		VuoCompilerNode *node = *nodesToVisit.begin();
		nodesToVisit.erase(node);
		orderedNodes.push_back(node);

		// Find all nodes that are successors to this node, and add them to a list of nodes to possibly visit.
		set<VuoCompilerNode *> nodesToAdd;
		set<VuoCompilerPassiveEdge *> edgesToRemove;
		for (set<VuoCompilerPassiveEdge *>::iterator i = edgesToVisit.begin(); i != edgesToVisit.end(); ++i)
		{
			VuoCompilerPassiveEdge *edge = *i;
			if (edge->getFromNode() == node)
			{
				edgesToRemove.insert(edge);
				nodesToAdd.insert(edge->getToNode());
			}
		}

		// Rrom the list of edges to visit, remove each edge between this node and a successor node.
		for (set<VuoCompilerPassiveEdge *>::iterator i = edgesToRemove.begin(); i != edgesToRemove.end(); ++i)
		{
			VuoCompilerPassiveEdge *edge = *i;
			edgesToVisit.erase(edge);
		}

		// From the list of nodes to possibly visit, remove each node that still has incoming edges.
		for (set<VuoCompilerPassiveEdge *>::iterator i = edgesToVisit.begin(); i != edgesToVisit.end(); ++i)
		{
			VuoCompilerPassiveEdge *edge = *i;
			VuoCompilerNode *toNode = edge->getToNode();
			if (nodesToAdd.find(toNode) != nodesToAdd.end())
				nodesToAdd.erase(toNode);
		}

		// Add nodes to visit.
		nodesToVisit.insert(nodesToAdd.begin(), nodesToAdd.end());
	}
}

/**
 * Creates a topologically sorted list of all linear chains of nodes that may be pushed by the trigger.
 *
 * Assumes the topologically sorted list of nodes has been created.
 */
void VuoCompilerBitcodeGenerator::makeChainsForTrigger(VuoCompilerTriggerPort *trigger)
{
	map<VuoCompilerNode *, bool> chainNodesSeen;
	for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
	{
		if (! isNodeDownstreamOfTrigger(*i, trigger))
			continue;

		if (chainNodesSeen[*i])
			continue;

		// Create the chain starting with node *i and ending when we hit a leaf, scatter, or gather.
		vector<VuoCompilerNode *> chainNodes;
		vector<VuoCompilerNode *>::iterator currNodeIter, nextNodeIter = i;
		do {
			currNodeIter = nextNodeIter;
			chainNodes.push_back(*currNodeIter);
			chainNodesSeen[*currNodeIter] = true;

			// Skip any nodes that belong to other chains.
			for (nextNodeIter = currNodeIter + 1; nextNodeIter != orderedNodes.end(); ++nextNodeIter)
				 if (edgeExists(*currNodeIter, *nextNodeIter, trigger))
					 break;

		} while (nextNodeIter != orderedNodes.end() &&
				 ! isDeadEnd(*currNodeIter, trigger) &&
				 ! isScatter(*currNodeIter, trigger) &&
				 ! isGather(*nextNodeIter, trigger));

		VuoCompilerChain *chain = new VuoCompilerChain(chainNodes, false);
		chainsForTrigger[trigger].push_back(chain);
	}

	// Create a chain for each node that is the last (and only repeated) node in a feedback loop.
	for (set<VuoCompilerNode *>::iterator i = loopEndNodes.begin(); i != loopEndNodes.end(); ++i)
	{
		if (! isNodeDownstreamOfTrigger(*i, trigger))
			continue;

		VuoCompilerNode *loopEndNode = *i;
		vector<VuoCompilerNode *> loopEndNodeList;
		loopEndNodeList.push_back(loopEndNode);
		VuoCompilerChain *chain = new VuoCompilerChain(loopEndNodeList, true);
		chainsForTrigger[trigger].push_back(chain);
	}
}

/**
 * Creates a VuoCompilerTriggerAction for each VuoCompilerTriggerPort.
 *
 * Assumes the mapping of triggers to nodes has been created.
 */
void VuoCompilerBitcodeGenerator::makeTriggerObjects(void)
{
	for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		VuoCompilerNode *node = i->second;
		triggerActionForTrigger[trigger] = new VuoCompilerTriggerAction(trigger, node);
	}
}

/**
 * Returns true if at least one of the node's in-edges (trigger or passive) is reachable from the trigger.
 *
 * Assumes the list of downstream edges for each edge has been created.
 */
bool VuoCompilerBitcodeGenerator::isNodeDownstreamOfTrigger(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerTriggerEdge *> triggerOutEdges = triggerEdgesForTrigger[trigger];
	set<VuoCompilerPassiveEdge *> nodeInEdges = passiveInEdgesForNode[node];

	for (set<VuoCompilerTriggerEdge *>::iterator i = triggerOutEdges.begin(); i != triggerOutEdges.end(); ++i)
	{
		VuoCompilerTriggerEdge *triggerOutEdge = *i;

		// Is the node directly connected to the trigger?
		if (triggerOutEdge->getToNode() == node)
			return true;

		// Is the node further downstream of the trigger?
		for (set<VuoCompilerPassiveEdge *>::iterator j = nodeInEdges.begin(); j != nodeInEdges.end(); ++j)
		{
			VuoCompilerPassiveEdge *nodeInEdge = *j;
			if (downstreamEdgesForEdge[triggerOutEdge].find(nodeInEdge) != downstreamEdgesForEdge[triggerOutEdge].end())
				return true;
		}
	}

	return false;
}

/**
 * Returns true if at least one of @a node's in-edges is reachable from @a upstreamNode.
 *
 * Assumes the list of downstream edges for each edge has been created.
 */
bool VuoCompilerBitcodeGenerator::isNodeDownstreamOfNode(VuoCompilerNode *node, VuoCompilerNode *upstreamNode)
{
	set<VuoCompilerPassiveEdge *> nodeOutEdges = passiveOutEdgesForNode[upstreamNode];
	set<VuoCompilerPassiveEdge *> nodeInEdges = passiveInEdgesForNode[node];

	for (set<VuoCompilerPassiveEdge *>::iterator i = nodeOutEdges.begin(); i != nodeOutEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *nodeOutEdge = *i;

		// Is the node directly connected to upstreamNode?
		if (nodeOutEdge->getToNode() == node)
			return true;

		// Is the node further downstream of upstreamNode?
		for (set<VuoCompilerPassiveEdge *>::iterator j = nodeInEdges.begin(); j != nodeInEdges.end(); ++j)
		{
			VuoCompilerPassiveEdge *nodeInEdge = *j;
			if (downstreamEdgesForEdge[nodeOutEdge].find(nodeInEdge) != downstreamEdgesForEdge[nodeOutEdge].end())
				return true;
		}
	}

	return false;
}

/**
 * Returns true if there is a passive edge from fromNode to toNode that may be pushed by the trigger.
 *
 * Assumes the list of edges reachable from each trigger has been created.
 */
bool VuoCompilerBitcodeGenerator::edgeExists(VuoCompilerNode *fromNode, VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerPassiveEdge *> outEdges = passiveOutEdgesForNode[fromNode];
	for (set<VuoCompilerPassiveEdge *>::iterator i = outEdges.begin(); i != outEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *outEdge = *i;

		// Is fromNode's out-edge an in-edge of toNode, and is it reachable from the trigger?
		if (outEdge->getToNode() == toNode &&
				passiveEdgesForTrigger[trigger].find(outEdge) != passiveEdgesForTrigger[trigger].end())
			return true;
	}
	return false;
}

/**
 * Returns true if the node doesn't have any passive out-edges that may be pushed by the trigger.
 *
 * Assumes the list of edges reachable from each trigger has been created.
 */
bool VuoCompilerBitcodeGenerator::isDeadEnd(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerPassiveEdge *> outEdges = passiveOutEdgesForNode[node];
	for (set<VuoCompilerPassiveEdge *>::iterator i = outEdges.begin(); i != outEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *outEdge = *i;

		// Is the node's out-edge reachable from the trigger?
		if (passiveEdgesForTrigger[trigger].find(outEdge) != passiveEdgesForTrigger[trigger].end())
			return false;
	}
	return true;
}

/**
 * Returns true if more than one of the node's passive out-edges may be pushed by the trigger.
 *
 * Assumes the list of edges reachable from each trigger has been created.
 */
bool VuoCompilerBitcodeGenerator::isScatter(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	int numOutEdges = 0;
	set<VuoCompilerPassiveEdge *> outEdges = passiveOutEdgesForNode[node];
	for (set<VuoCompilerPassiveEdge *>::iterator i = outEdges.begin(); i != outEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *outEdge = *i;

		// Is the node's out-edge reachable from the trigger?
		if (passiveEdgesForTrigger[trigger].find(outEdge) != passiveEdgesForTrigger[trigger].end())
			++numOutEdges;
	}
	return numOutEdges > 1;
}

/**
 * Returns true if more than one of the node's passive in-edges may be pushed by the trigger.
 *
 * Assumes the list of edges reachable from each trigger has been created.
 */
bool VuoCompilerBitcodeGenerator::isGather(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	int numInEdges = 0;
	set<VuoCompilerPassiveEdge *> inEdges = passiveInEdgesForNode[node];
	for (set<VuoCompilerPassiveEdge *>::iterator i = inEdges.begin(); i != inEdges.end(); ++i)
	{
		VuoCompilerPassiveEdge *inEdge = *i;

		// Is the node's in-edge reachable from the trigger?
		if (passiveEdgesForTrigger[trigger].find(inEdge) != passiveEdgesForTrigger[trigger].end())
			++numInEdges;
	}
	return numInEdges > 1;
}

/**
 * Returns a list of all trigger edges in the composition.
 */
set<VuoCompilerTriggerEdge *> VuoCompilerBitcodeGenerator::getTriggerEdges(void)
{
	set<VuoCompilerTriggerEdge *> edges;
	for (map<VuoCompilerNode *, set<VuoCompilerTriggerEdge *> >::iterator i = triggerInEdgesForNode.begin(); i != triggerInEdgesForNode.end(); ++i)
		for (set<VuoCompilerTriggerEdge *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			edges.insert(*j);
	return edges;
}

/**
 * Puts the nodes into the same order as they appear in @ref orderedNodes.
 */
vector<VuoCompilerNode *> VuoCompilerBitcodeGenerator::sortNodes(set<VuoCompilerNode *> originalNodes)
{
	vector<VuoCompilerNode *> sortedNodes;
	for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;
		if (originalNodes.find(node) != originalNodes.end())
			sortedNodes.push_back(node);
	}
	return sortedNodes;
}


/**
 * Generates bitcode that can be read in as a node class.
 */
Module * VuoCompilerBitcodeGenerator::generateBitcode(void)
{
	module = new Module("", getGlobalContext());  /// @todo Set module identifier (https://b33p.net/kosada/node/2639)

	generateMetadata();

	generateAllocation();

	generateGetPortValueOrSummaryFunctions();

	generatePublishedPortGetters();

	generateSerializeFunction();
	generateUnserializeFunction();

	generateSetupFunction();
	generateCleanupFunction();

	// First set the function header for each trigger, then generate the function body for each trigger.
	// This has to be split into 2 steps because the 2nd step assumes that the 1st step is complete for all triggers.
	for (map<VuoCompilerTriggerPort *, set<VuoCompilerTriggerEdge *> >::iterator i = triggerEdgesForTrigger.begin(); i != triggerEdgesForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		Function *f = generateTriggerFunctionHeader(trigger);
		trigger->setFunction(f);
	}
	for (map<VuoCompilerTriggerPort *, set<VuoCompilerTriggerEdge *> >::iterator i = triggerEdgesForTrigger.begin(); i != triggerEdgesForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
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

	for (map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *>::iterator i = triggerActionForTrigger.begin(); i != triggerActionForTrigger.end(); ++i)
		i->second->generateAllocation(module);
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

	generateInitializationForReferenceCounts(block);

	VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(module, block, lastEventIdSemaphoreVariable);

	for (map<VuoCompilerNode *, GlobalVariable *>::iterator i = semaphoreVariableForNode.begin(); i != semaphoreVariableForNode.end(); ++i)
		VuoCompilerCodeGenUtilities::generateInitializationForSemaphore(module, block, i->second);

	generateInitializationForPorts(block, false);
	generateInitializationForPorts(block, true);

	for (map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *>::iterator i = triggerActionForTrigger.begin(); i != triggerActionForTrigger.end(); ++i)
		i->second->generateInitialization(module, block);

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

	for (map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *>::iterator i = triggerActionForTrigger.begin(); i != triggerActionForTrigger.end(); ++i)
		i->second->generateFinalization(module, block);

	generateFinalizationForReferenceCounts(block);

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
	for (map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *>::iterator i = triggerActionForTrigger.begin(); i != triggerActionForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *triggerPort = i->first;
		VuoCompilerTriggerAction *triggerAction = i->second;
		Function *barrierWorkerFunction = triggerAction->generateSynchronousSubmissionToDispatchQueue(module, block, triggerPort->getIdentifier() + "__barrier");
		BasicBlock *barrierBlock = BasicBlock::Create(module->getContext(), "", barrierWorkerFunction, NULL);
		ReturnInst::Create(module->getContext(), barrierBlock);
	}
	generateWaitForNodes(module, function, block, orderedNodes);
	generateSignalForNodes(module, block, orderedNodes);

	ReturnInst::Create(module->getContext(), block);
}

/**
 * Generate the initialization of the referenceCounts map.
 *
 * @eg{
 * VuoHeap_init();
 * }
 */
void VuoCompilerBitcodeGenerator::generateInitializationForReferenceCounts(BasicBlock *block)
{
	Function *initFunction = VuoCompilerCodeGenUtilities::getInitializeReferenceCountsFunction(module);
	CallInst::Create(initFunction, "", block);
}

/**
 * Generate the finalization of the referenceCounts map.
 *
 * @eg{
 * VuoHeap_fini();
 * }
 */
void VuoCompilerBitcodeGenerator::generateFinalizationForReferenceCounts(BasicBlock *block)
{
	Function *finiFunction = VuoCompilerCodeGenUtilities::getFinalizeReferenceCountsFunction(module);
	CallInst::Create(finiFunction, "", block);
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
 * Generates a call to wait on the semaphore of each of the given nodes.
 *
 * Assumes the list of nodes is in the same order as @ref orderedNodes (to avoid deadlock).
 *
 * @eg{
 * // When an event reaches a node through just one edge into the node, this code is generated just once.
 * // When an event reaches a node through multiple edges (i.e., the node is at the hub of a feedback loop
 * // or at a gather), this code is generated for each of those edges. When one of those edges claims the
 * // semaphore, the rest of the edges for the same event need to recognize this and also stop waiting.
 * // Hence the checking of event IDs and the limited-time wait.
 *
 * // For each node:
 * while (vuo_math_count__Count__eventIDClaimingSemaphore != eventId)
 * {
 *    int ret = dispatch_semaphore_wait(vuo_math_count__Count__semaphore, WAIT_TIME);
 *    if (ret == 0)  // got semaphore
 *       vuo_math_count__Count__eventIDClaimingSemaphore = eventId;
 * }
 */
void VuoCompilerBitcodeGenerator::generateWaitForNodes(Module *module, Function *function, BasicBlock *&block,
													   vector<VuoCompilerNode *> nodes, Value *eventIdValue)
{
	if (! eventIdValue)
		eventIdValue = generateGetNextEventID(module, block);

	for (vector<VuoCompilerNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;

		BasicBlock *checkEventIdBlock = BasicBlock::Create(module->getContext(), "checkEventId", function);
		BranchInst::Create(checkEventIdBlock, block);

		GlobalVariable *claimingEventIdVariable = claimingEventIdVariableForNode[node];
		Value *claimingEventIdValue = new LoadInst(claimingEventIdVariable, "", false, checkEventIdBlock);
		ICmpInst *claimingEventIdNotEqualsEventId = new ICmpInst(*checkEventIdBlock, ICmpInst::ICMP_NE, claimingEventIdValue, eventIdValue, "");
		BasicBlock *waitBlock = BasicBlock::Create(module->getContext(), "waitNodeSemaphore", function);
		BasicBlock *nextBlock = BasicBlock::Create(module->getContext(), "gotNodeSemaphore", function);
		BranchInst::Create(waitBlock, nextBlock, claimingEventIdNotEqualsEventId, checkEventIdBlock);

		GlobalVariable *semaphoreVariable = semaphoreVariableForNode[node];
		int64_t timeoutInNanoseconds = NSEC_PER_SEC / 1000;  /// @todo (https://b33p.net/kosada/node/6682)
		Value *retValue = VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, waitBlock, semaphoreVariable, timeoutInNanoseconds);

		ConstantInt *zeroValue = ConstantInt::get(module->getContext(), APInt(64, 0));
		ICmpInst *retEqualsZero = new ICmpInst(*waitBlock, ICmpInst::ICMP_EQ, retValue, zeroValue, "");
		BasicBlock *setEventIdBlock = BasicBlock::Create(module->getContext(), "setEventId", function);
		BranchInst::Create(setEventIdBlock, checkEventIdBlock, retEqualsZero, waitBlock);

		new StoreInst(eventIdValue, claimingEventIdVariable, false, setEventIdBlock);

		BranchInst::Create(checkEventIdBlock, setEventIdBlock);
		block = nextBlock;
	}
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
 * Generates a call to @c <Type>_stringFromValue(...) to get a string representation of the port's current value.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerialization(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, false, false);
}

/**
 * Generates a call to @c <Type>_interprocessStringFromValue(...) to get an interprocess-safe string representation of the port's current value.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerializationInterprocess(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, false, true);
}

/**
 * Generates a call to @c <Type>_summaryFromValue(...) to get a brief description of the port's current value,
 * or a null value if the port is event-only.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSummary(Module *module, BasicBlock *block, VuoCompilerPort *port)
{
	return generatePortSerializationOrSummary(module, block, port, true, false);
}

/**
 * Generates a call to @c <Type>_stringFromValue() or @c <Type>_summaryFromValue() for the port.
 * or a null value if the port is event-only.
 *
 * The generated code assumes that the semaphore has been claimed for the port's node.
 */
Value * VuoCompilerBitcodeGenerator::generatePortSerializationOrSummary(Module *module, BasicBlock *block, VuoCompilerPort *port, bool isSummary, bool isInterprocess)
{
	Value *stringOrSummaryValue = NULL;
	GlobalVariable *dataVariable = NULL;
	VuoCompilerType *type = NULL;

	/// @todo Refactor by replacing with VuoCompilerPort::getDataVariable() and VuoCompilerPort::getDataType() (https://b33p.net/kosada/node/2995)
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
			VuoCompilerTriggerAction *triggerAction = triggerActionForTrigger[triggerPort];
			dataVariable = triggerAction->getPreviousDataVariable();
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

		// <type>_stringFromValue(dataValue)
		stringOrSummaryValue = (isSummary ?
									type->generateSummaryFromValueFunctionCall(module, block, dataValue) :
									(isInterprocess ?
										 type->generateInterprocessStringFromValueFunctionCall(module, block, dataValue) :
										 type->generateStringFromValueFunctionCall(module, block, dataValue)
										 ));

		if (llvmType->isPointerTy())
		{
			// dataValue != NULL ? <type>_stringFromValue(dataValue) : NULL
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
 *     ret = VuoInteger_stringFromValue(vuo_math_add__Add2__sum);
 *     dispatch_semaphore_signal(vuo_math_add__Add2__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_console_read__ReadFromConsole3__string"))
 *   {
 *     dispatch_semaphore_wait(vuo_console_read__ReadFromConsole3__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     if (vuo_console_read__ReadFromConsole3__string != NULL) {
 *       ret = VuoString_stringFromValue(vuo_console_read__ReadFromConsole3__string);
 *     } else {
 *       ret = NULL;
 *     }
 *     dispatch_semaphore_signal(vuo_console_read__ReadFromConsole3__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_mouse__GetMouse__leftPressed"))
 *   {
 *     dispatch_semaphore_wait(vuo_mouse__GetMouse__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     ret = VuoPoint2d_stringFromValue(vuo_mouse__GetMouse__leftPressed__previous);
 *     dispatch_semaphore_signal(vuo_mouse__GetMouse__semaphore);  // not in getOutputPortValueThreadUnsafe()
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_image_filter_ripple__RippleImage__rippledImage"))
 *   {
 *     dispatch_semaphore_wait(vuo_image_filter_ripple__RippleImage__semaphore, DISPATCH_TIME_FOREVER);  // not in getOutputPortValueThreadUnsafe()
 *     if (shouldUseInterprocessSerialization)
 *       ret = VuoImage_interprocessStringFromValue(vuo_image_filter_ripple__RippleImage__rippledImage);
 *     else
 *       ret = VuoImage_stringFromValue(vuo_image_filter_ripple__RippleImage__rippledImage);
 *     dispatch_semaphore_signal(vuo_image_filter_ripple__RippleImage__semaphore);  // not in getOutputPortValueThreadUnsafe()
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

			/// @todo Refactor by replacing with VuoCompilerPort::getIdentifier() and VuoCompilerPort::getDataVariable() (https://b33p.net/kosada/node/2995)
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
					VuoCompilerTriggerAction *triggerAction = triggerActionForTrigger[triggerPort];
					currentPortIdentifier = triggerPort->getIdentifier();
					dataVariable = triggerAction->getPreviousDataVariable();
				}
			}

			if (dataVariable)
			{
				BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);
				BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);

				vector<VuoCompilerNode *> nodeList;
				nodeList.push_back(node->getCompiler());
				if (isThreadSafe)
				{
					// dispatch_semaphore_wait(currentBlock, DISPATCH_TIME_FOREVER);
					generateWaitForNodes(module, function, currentBlock, nodeList);
				}

				// <Type>_summaryFromValue(data) or <Type>_stringFromValue(data) or <Type>_interprocessStringFromValue(data)
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
					//   [type]_stringFromValue(...);
					// else
					//   [type]_interprocessStringFromValue(...);
					ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(shouldUseInterprocessSerializationValue->getType()), 0);
					ICmpInst *shouldUseInterprocessSerializationValueIsTrue = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, shouldUseInterprocessSerializationValue, zeroValue, "");
					BranchInst::Create(interprocessSerializationBlock, serializationBlock, shouldUseInterprocessSerializationValueIsTrue, currentBlock);
				}

				if (isThreadSafe)
				{
					// dispatch_semaphore_signal(nodeSemaphore);
					generateSignalForNodes(module, finalBlock, nodeList);
				}

				blocksForString[currentPortIdentifier] = make_pair(currentBlock, finalBlock);
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
 * Each definition of @c <Type>_valueFromString() that returns heap data is responsible for registering
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
 *     VuoReal value = VuoReal_valueFromString(valueAsString);
 *     dispatch_semaphore_wait(vuo_time_firePeriodically__FirePeriodically__semaphore, DISPATCH_TIME_FOREVER);
 *     vuo_time_firePeriodically__FirePeriodically__seconds = value;
 *     if (shouldUpdateCallbacks)
 *       vuo_time_firePeriodically__FirePeriodically__nodeInstanceCallbackUpdate(...);
 *     dispatch_semaphore_signal(vuo_time_firePeriodically__FirePeriodically__semaphore);
 *     char *summary = VuoReal_summaryFromValue(value);
 *     sendInputPortsUpdated(portIdentifier, false, true, summary);
 *     free(summary);
 *   }
 *   else if (! strcmp(portIdentifier, "vuo_console_write__Write__string"))
 *   {
 *     VuoText value = VuoText_valueFromString(valueAsString);
 *     dispatch_semaphore_wait(vuo_console_write__Write__semaphore, DISPATCH_TIME_FOREVER);
 *     VuoRelease((void *)vuo_console_write__Write__string);
 *     vuo_console_write__Write__string = VuoText_valueFromString(valueAsString);
 *     VuoRetain((void *)vuo_console_write__Write__string);
 *     dispatch_semaphore_signal(vuo_console_write__Write__semaphore);
 *     char *summary = VuoText_summaryFromValue(value);
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

					// <Type> value = <Type>_valueFromString(valueAsString);
					VuoCompilerDataClass *dataClass = static_cast<VuoCompilerDataClass *>(data->getBase()->getClass()->getCompiler());
					VuoCompilerType *type = dataClass->getVuoType()->getCompiler();
					Value *dataValue = type->generateValueFromStringFunctionCall(module, currentBlock, valueAsStringValue);

					// dispatch_semaphore_wait(nodeSemaphore, DISPATCH_TIME_FOREVER);
					GlobalVariable *semaphoreVariable = semaphoreVariableForNode[node->getCompiler()];
					VuoCompilerCodeGenUtilities::generateWaitForSemaphore(module, currentBlock, semaphoreVariable);

					// VuoRelease((void *)data);
					LoadInst *dataValueBefore = data->generateLoad(currentBlock);
					VuoCompilerCodeGenUtilities::generateReleaseCall(module, currentBlock, dataValueBefore);

					// data = value;
					data->generateStore(dataValue, currentBlock);

					// VuoRetain((void *)data);
					LoadInst *dataValueAfter = data->generateLoad(currentBlock);
					VuoCompilerCodeGenUtilities::generateRetainCall(module, currentBlock, dataValueAfter);

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

					// dispatch_semaphore_signal(nodeSemaphore);
					VuoCompilerCodeGenUtilities::generateSignalForSemaphore(module, currentBlock, semaphoreVariable);

					// char *summary = <Type>_summaryFromValue(value);
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
 *
 * @param block The basic block to which the generated code is appended.
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
					VuoCompilerCodeGenUtilities::generateRetainCall(module, block, dataVariableAfter);
				}
			}
		}
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
	for (map<VuoCompilerTriggerPort *, VuoCompilerTriggerAction *>::iterator i = triggerActionForTrigger.begin(); i != triggerActionForTrigger.end(); ++i)
	{
		VuoCompilerTriggerPort *port = i->first;
		VuoCompilerTriggerAction *triggerAction = i->second;
		string currentPortIdentifier = port->getIdentifier();
		Function *triggerFunction = port->getFunction();
		VuoCompilerNode *triggerNode = nodeForTrigger[port];

		BasicBlock *currentBlock = BasicBlock::Create(module->getContext(), currentPortIdentifier, function, 0);

		vector<Value *> triggerArgs;
		GlobalVariable *dataVariable = triggerAction->getPreviousDataVariable();
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

		blocksForString[currentPortIdentifier] = make_pair(currentBlock, currentBlock);
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

	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, 0);
	ReturnInst::Create(module->getContext(), finalBlock);

	VuoCompilerCodeGenUtilities::generateStringMatchingCode(module, function, initialBlock, finalBlock, nameValue, blocksForString);
}

/**
 * Generates a call to sendOutputPortsUpdated() for each output port that is transmitting an event and data,
 * and a call to sendInputPortsUpdated() for each input port that is receiving an event and data.
 *
 * Assumes the node's semaphore is already claimed (in code generated by this method's caller).
 */
void VuoCompilerBitcodeGenerator::generateSendPortsUpdatedCall(BasicBlock *initialBlock, BasicBlock *finalBlock, VuoCompilerNode *node)
{
	vector<VuoPort *> outputPorts = node->getBase()->getOutputPorts();
	for (vector<VuoPort *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
	{
		// If the output port is a trigger port, do nothing.
		VuoCompilerOutputEventPort *outputEventPort = dynamic_cast<VuoCompilerOutputEventPort *>((*i)->getCompiler());
		if (! outputEventPort)
			continue;

		BasicBlock *telemetryBlock = BasicBlock::Create(module->getContext(), "", initialBlock->getParent(), NULL);
		BasicBlock *noTelemetryBlock = BasicBlock::Create(module->getContext(), "", initialBlock->getParent(), NULL);

		// If the output port isn't transmitting an event, do nothing.
		LoadInst *eventValue = outputEventPort->generateLoad(initialBlock);
		ConstantInt *zeroValue = ConstantInt::get(static_cast<IntegerType *>(eventValue->getType()), 0);
		ICmpInst *eventValueIsTrue = new ICmpInst(*initialBlock, ICmpInst::ICMP_NE, eventValue, zeroValue, "");
		BranchInst::Create(telemetryBlock, noTelemetryBlock, eventValueIsTrue, initialBlock);

		Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);
		Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
		Type *boolType = sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(1);
		PointerType *pointerToCharType = static_cast<PointerType *>(sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(2));
		Constant *falseValue = ConstantInt::get(boolType, 0);
		Constant *trueValue = ConstantInt::get(boolType, 1);

		string outputPortIdentifier = outputEventPort->getIdentifier();
		Constant *outputPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, outputPortIdentifier);

		VuoCompilerOutputData *outputData = outputEventPort->getData();
		Value *sentDataValue;
		Value *outputDataSummaryValue;
		if (outputData)
		{
			// sentData = true;
			sentDataValue = trueValue;

			// outputDataSummary = <Type>_summaryFromValue(outputData);
			outputDataSummaryValue = generatePortSummary(module, telemetryBlock, outputEventPort);
		}
		else
		{
			// sentData = false;
			sentDataValue = falseValue;

			// outputDataSummary = NULL;
			outputDataSummaryValue = ConstantPointerNull::get(pointerToCharType);
		}

		// sendOutputPortsUpdated(outputPortIdentifier, sentData, outputDataSummary);
		vector<Value *> sendOutputPortsUpdatedArgs;
		sendOutputPortsUpdatedArgs.push_back(outputPortIdentifierValue);
		sendOutputPortsUpdatedArgs.push_back(sentDataValue);
		sendOutputPortsUpdatedArgs.push_back(outputDataSummaryValue);
		CallInst::Create(sendOutputPortsUpdatedFunction, sendOutputPortsUpdatedArgs, "", telemetryBlock);

		set<VuoCompilerPassiveEdge *> passiveOutEdges = passiveOutEdgesForNode[node];
		for (set<VuoCompilerPassiveEdge *>::iterator i = passiveOutEdges.begin(); i != passiveOutEdges.end(); ++i)
		{
			set<VuoCompilerInputEventPort *> inputPorts = (*i)->getInputPortsConnectedToOutputPort(outputEventPort);
			for (set<VuoCompilerInputEventPort *>::iterator j = inputPorts.begin(); j != inputPorts.end(); ++j)
			{
				VuoCompilerInputEventPort *inputEventPort = *j;

				string inputPortIdentifier = inputEventPort->getIdentifier();
				Constant *inputPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, inputPortIdentifier);

				Value *inputDataSummaryValue;
				if (outputData)
				{
					// inputDataSummary = outputDataSummary;
					inputDataSummaryValue = outputDataSummaryValue;
				}
				else
				{
					// inputDataSummary = <Type>_summaryFromValue(inputData);
					inputDataSummaryValue = generatePortSummary(module, telemetryBlock, inputEventPort);
				}

				// sendInputPortsUpdated(inputPortIdentifier, true, sentData, inputDataSummary);
				Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);
				vector<Value *> sendInputPortsUpdatedArgs;
				sendInputPortsUpdatedArgs.push_back(inputPortIdentifierValue);
				sendInputPortsUpdatedArgs.push_back(trueValue);
				sendInputPortsUpdatedArgs.push_back(sentDataValue);
				sendInputPortsUpdatedArgs.push_back(inputDataSummaryValue);
				CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", telemetryBlock);

				if (! outputData)
				{
					// free(inputDataSummary);
					CallInst::Create(freeFunction, inputDataSummaryValue, "", telemetryBlock);
				}
			}
		}

		if (outputData)
		{
			// free(outputDataSummary);
			CallInst::Create(freeFunction, outputDataSummaryValue, "", telemetryBlock);
		}

		BranchInst::Create(noTelemetryBlock, telemetryBlock);
		initialBlock = noTelemetryBlock;
	}

	BranchInst::Create(finalBlock, initialBlock);
}

/**
 * Generates a call to sendOutputPortsUpdated() for the given trigger port if it carries data.
 *
 * Assumes the node's semaphore is already claimed (in code generated by this method's caller).
 */
void VuoCompilerBitcodeGenerator::generateSendTriggerPortValueChangedCall(BasicBlock *block, VuoCompilerTriggerPort *trigger, Value *triggerDataValue)
{
	Constant *triggerIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, trigger->getIdentifier());

	Function *sendOutputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendOutputPortsUpdatedFunction(module);
	Function *freeFunction = VuoCompilerCodeGenUtilities::getFreeFunction(module);
	Type *boolType = sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(1);
	PointerType *pointerToCharType = static_cast<PointerType *>(sendOutputPortsUpdatedFunction->getFunctionType()->getParamType(2));
	Constant *falseValue = ConstantInt::get(boolType, 0);
	Constant *trueValue = ConstantInt::get(boolType, 1);

	Value *sentDataValue;
	Value *triggerDataSummaryValue;
	if (triggerDataValue)
	{
		// sentData = true;
		sentDataValue = trueValue;

		// triggerDataSummary = <type>_summaryFromValue(portValue);
		VuoCompilerType *type = static_cast<VuoCompilerTriggerPortClass *>(trigger->getBase()->getClass()->getCompiler())->getDataVuoType()->getCompiler();
		triggerDataSummaryValue = type->generateSummaryFromValueFunctionCall(module, block, triggerDataValue);
	}
	else
	{
		// sentData = false;
		sentDataValue = falseValue;

		// triggerDataSummary = NULL;
		triggerDataSummaryValue = ConstantPointerNull::get(pointerToCharType);
	}

	// sendOutputPortsUpdated(triggerPortIdentifier, sentData, triggerDataSummary);
	vector<Value *> sendOutputPortsUpdatedArgs;
	sendOutputPortsUpdatedArgs.push_back(triggerIdentifierValue);
	sendOutputPortsUpdatedArgs.push_back(sentDataValue);
	sendOutputPortsUpdatedArgs.push_back(triggerDataSummaryValue);
	CallInst::Create(sendOutputPortsUpdatedFunction, sendOutputPortsUpdatedArgs, "", block);

	set<VuoCompilerTriggerEdge *> triggerEdges = triggerEdgesForTrigger[trigger];
	for (set<VuoCompilerTriggerEdge *>::iterator i = triggerEdges.begin(); i != triggerEdges.end(); ++i)
	{
		set<VuoCompilerInputEventPort *> inputPorts = (*i)->getInputPorts();
		for (set<VuoCompilerInputEventPort *>::iterator j = inputPorts.begin(); j != inputPorts.end(); ++j)
		{
			VuoCompilerInputEventPort *inputEventPort = *j;

			string inputPortIdentifier = inputEventPort->getIdentifier();
			Constant *inputPortIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, inputPortIdentifier);

			Value *inputDataSummaryValue;
			if (triggerDataValue)
			{
				// inputDataSummary = triggerDataSummary;
				inputDataSummaryValue = triggerDataSummaryValue;
			}
			else
			{
				// inputDataSummary = <Type>_summaryFromValue(inputData);
				inputDataSummaryValue = generatePortSummary(module, block, inputEventPort);
			}

			// sendInputPortsUpdated(inputPortIdentifier, true, sentData, inputDataSummary);
			Function *sendInputPortsUpdatedFunction = VuoCompilerCodeGenUtilities::getSendInputPortsUpdatedFunction(module);
			vector<Value *> sendInputPortsUpdatedArgs;
			sendInputPortsUpdatedArgs.push_back(inputPortIdentifierValue);
			sendInputPortsUpdatedArgs.push_back(trueValue);
			sendInputPortsUpdatedArgs.push_back(sentDataValue);
			sendInputPortsUpdatedArgs.push_back(inputDataSummaryValue);
			CallInst::Create(sendInputPortsUpdatedFunction, sendInputPortsUpdatedArgs, "", block);

			if (! triggerDataValue)
			{
				// free(inputDataSummary)
				CallInst::Create(freeFunction, inputDataSummaryValue, "", block);
			}
		}
	}

	if (triggerDataValue)
	{
		// free(triggerDataSummary)
		CallInst::Create(freeFunction, triggerDataSummaryValue, "", block);
	}
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
	VuoCompilerTriggerAction *triggerAction = triggerActionForTrigger[trigger];

	// Schedule the following:
	BasicBlock *workerSchedulerBlock = BasicBlock::Create(module->getContext(), "workerSchedulerBlock", function, NULL);
	Function *triggerWorker = triggerAction->generateAsynchronousSubmissionToDispatchQueue(module, workerSchedulerBlock, trigger->getIdentifier());
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

	// Does the trigger port have a gather somewhere downstream of it?
	// If so, then when the event reaches its first scatter, all downstream nodes will be locked before the event can proceed.
	// This is an (imprecise) way to prevent deadlock in the situation where one trigger port has scatter and a gather,
	// and another trigger port overlaps with some branches of the scatter but not others (https://b33p.net/kosada/node/6696).
	bool isGatherDownstreamOfTrigger = false;
	for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end() && ! isGatherDownstreamOfTrigger; ++i)
		if (isNodeDownstreamOfTrigger(*i, trigger) && isGather(*i, trigger))
			isGatherDownstreamOfTrigger = true;

	// Wait for the node containing the trigger port and the nodes directly downstream of the trigger —
	// or, if the trigger scatters the event and there's a gather downstream, wait for all nodes downstream.
	set<VuoCompilerNode *> triggerOutputNodes;
	set<VuoCompilerTriggerEdge *> triggerEdges = triggerEdgesForTrigger[trigger];
	if (! (isGatherDownstreamOfTrigger && triggerEdges.size() > 1))
	{
		for (set<VuoCompilerTriggerEdge *>::iterator i = triggerEdges.begin(); i != triggerEdges.end(); ++i)
		{
			VuoCompilerTriggerEdge *edge = *i;
			triggerOutputNodes.insert( edge->getToNode() );
		}
	}
	else
	{
		for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
			if (isNodeDownstreamOfTrigger(*i, trigger))
				triggerOutputNodes.insert(*i);
	}
	VuoCompilerNode *triggerNode = nodeForTrigger[trigger];
	bool isTriggerWaitNeeded = ((triggerNode->getBase() != composition->getPublishedInputNode()) &&
								(triggerNode->getBase() != composition->getPublishedOutputNode()) &&
								(triggerOutputNodes.find(triggerNode) == triggerOutputNodes.end()));
	if (isTriggerWaitNeeded)
		triggerOutputNodes.insert(triggerNode);
	vector<VuoCompilerNode *> orderedTriggerOutputNodes = sortNodes(triggerOutputNodes);
	generateWaitForNodes(module, triggerWorker, triggerBlock, orderedTriggerOutputNodes, eventIdValueInTriggerWorker);

	// Handle the trigger port value having changed.
	Value *triggerDataValue = triggerAction->generateDataValueDidChange(module, triggerBlock, triggerWorker);
	if (isTriggerWaitNeeded)
		generateSignalForNodes(module, triggerBlock, vector<VuoCompilerNode *>(1, triggerNode));

	// Send telemetry indicating that the trigger port value may have changed.
	generateSendTriggerPortValueChangedCall(triggerBlock, trigger, triggerDataValue);

	// Transmit data and events along each of the trigger's cables.
	for (set<VuoCompilerTriggerEdge *>::iterator i = triggerEdges.begin(); i != triggerEdges.end(); ++i)
	{
		VuoCompilerTriggerEdge *edge = *i;
		edge->generateTransmission(module, triggerBlock, triggerDataValue);
	}


	// For each chain downstream of the trigger...
	vector<VuoCompilerChain *> chains = chainsForTrigger[trigger];

	// Create a dispatch group for the chain.
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
			if (edgeExists(lastNodeInOtherChain, firstNodeInThisChain, trigger))
				upstreamChains.push_back(otherChain);
		}


		// Schedule the following:
		Function *chainWorker = chain->generateSubmissionForDispatchGroup(module, triggerBlock, eventIdValueInTriggerWorker, upstreamChains);
		BasicBlock *chainSetupBlock = BasicBlock::Create(module->getContext(), "chainSetupBlock", chainWorker, 0);


		// Wait for any chains directly upstream to complete.
		chain->generateWaitForUpstreamChains(module, chainWorker, chainSetupBlock);

		// For each node in the chain...
		Value *eventIdValueInChainWorker = chain->getEventIdValue(module, chainWorker, chainSetupBlock);
		BasicBlock *prevBlock = chainSetupBlock;
		for (vector<VuoCompilerNode *>::iterator j = chainNodes.begin(); j != chainNodes.end(); ++j)
		{
			VuoCompilerNode *node = *j;

			// If the node received an event, then...
			BasicBlock *nodeExecutionBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BasicBlock *downstreamWaitBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BasicBlock *transmissionBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			BasicBlock *signalBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			Value *nodePushedCondition = node->generateReceivedEventCondition(prevBlock);
			BranchInst::Create(nodeExecutionBlock, downstreamWaitBlock, nodePushedCondition, prevBlock);

			// Send telemetry indicating that the node's execution has started.
			Constant *nodeIdentifierValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, node->getIdentifier());
			Function *sendNodeExecutionStartedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionStartedFunction(module);
			CallInst::Create(sendNodeExecutionStartedFunction, nodeIdentifierValue, "", nodeExecutionBlock);

			// Call the node's event function.
			if (debugMode)
				VuoCompilerCodeGenUtilities::generatePrint(module, nodeExecutionBlock, node->getBase()->getTitle() + "\n");
			BasicBlock *nodeEventFinalBlock = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			node->generateEventFunctionCall(module, chainWorker, nodeExecutionBlock, nodeEventFinalBlock);
			nodeExecutionBlock = nodeEventFinalBlock;

			// Send telemetry indicating that the node's execution has finished.
			Function *sendNodeExecutionFinishedFunction = VuoCompilerCodeGenUtilities::getSendNodeExecutionFinishedFunction(module);
			CallInst::Create(sendNodeExecutionFinishedFunction, nodeIdentifierValue, "", nodeExecutionBlock);


			// Regardless of whether the node received an event...
			BranchInst::Create(downstreamWaitBlock, nodeExecutionBlock);

			// Wait for the nodes directly downstream of the current node —
			// or, if the node scatters the event and there's a gather downstream, wait for all nodes downstream.
			set<VuoCompilerPassiveEdge *> passiveOutEdges = passiveOutEdgesForNode[node];
			set<VuoCompilerPassiveEdge *> passiveEdgesForCurrentTrigger = passiveEdgesForTrigger[trigger];
			if (loopEndNodes.find(node) == loopEndNodes.end() || scheduledNodes.find(node) == scheduledNodes.end())
			{
				vector<VuoCompilerNode *> orderedOutputNodes;
				if (! (isGatherDownstreamOfTrigger && isScatter(node, trigger)))
				{
					set<VuoCompilerNode *> outputNodes;
					for (set<VuoCompilerPassiveEdge *>::iterator j = passiveOutEdges.begin(); j != passiveOutEdges.end(); ++j)
					{
						VuoCompilerPassiveEdge *edge = *j;
						if (passiveEdgesForCurrentTrigger.find(edge) == passiveEdgesForCurrentTrigger.end())
							continue;
						outputNodes.insert( edge->getToNode() );
					}
					orderedOutputNodes = sortNodes(outputNodes);
				}
				else
				{
					for (vector<VuoCompilerNode *>::iterator i = orderedNodes.begin(); i != orderedNodes.end(); ++i)
						if (isNodeDownstreamOfNode(*i, node))
							orderedOutputNodes.push_back(*i);
				}
				generateWaitForNodes(module, chainWorker, downstreamWaitBlock, orderedOutputNodes, eventIdValueInChainWorker);
			}


			// If the node received an event, then...
			BranchInst::Create(transmissionBlock, signalBlock, nodePushedCondition, downstreamWaitBlock);

			// Send telemetry indicating that the node's output port values, and any connected input port values, may have changed.
			BasicBlock *nextnodePushedBlock2 = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
			generateSendPortsUpdatedCall(transmissionBlock, nextnodePushedBlock2, node);
			transmissionBlock = nextnodePushedBlock2;

			// Transmit data and events along the node's output cables.
			set <VuoCompilerPort * > outputPortsInPassiveEdges;
			for (set<VuoCompilerPassiveEdge *>::iterator j = passiveOutEdges.begin(); j != passiveOutEdges.end(); ++j)
			{
				VuoCompilerPassiveEdge *edge = *j;
				if (passiveEdgesForCurrentTrigger.find(edge) == passiveEdgesForCurrentTrigger.end())
					continue;
				set <VuoCompilerOutputEventPort * > outputPortsInThisEdge = edge->getOutputPorts();
				for (set<VuoCompilerOutputEventPort * >::iterator k = outputPortsInThisEdge.begin(); k != outputPortsInThisEdge.end(); ++k)
				{
					VuoCompilerOutputEventPort *outputPort = *k;
					outputPortsInPassiveEdges.insert(outputPort);
				}

				BasicBlock *nextnodePushedBlock2 = BasicBlock::Create(module->getContext(), "", chainWorker, NULL);
				edge->generateTransmission(module, transmissionBlock, nextnodePushedBlock2);
				transmissionBlock = nextnodePushedBlock2;
			}

			// Reset the node's event inputs.
			node->generatePushedReset(transmissionBlock);


			// Regardless of whether the node received an event...
			BranchInst::Create(signalBlock, transmissionBlock);

			// If this was the last time this event could reach the node, signal the node's semaphore.
			if (loopEndNodes.find(node) == loopEndNodes.end() || scheduledNodes.find(node) != scheduledNodes.end())
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
