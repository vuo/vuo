/**
 * @file
 * VuoCompilerGraph implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerGraph.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoPort.hh"
#include "VuoStringUtilities.hh"
#include <stdexcept>

/**
 * Creates a graph representation of the (current state of) the composition.
 *
 * @param composition The composition to represent. The graph representation is a snapshot of the composition passed
 *		into this constructor, and does not update if the composition is modified.
 * @param potentialCables Cables that are not yet in @a composition but should be added to the graph representation.
 */
VuoCompilerGraph::VuoCompilerGraph(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables)
{
	set<VuoNode *> nonPublishedNodesIncludingInvalid = composition->getBase()->getNodes();
	set<VuoNode *> nonPublishedNodes;
	for (set<VuoNode *>::iterator i = nonPublishedNodesIncludingInvalid.begin(); i != nonPublishedNodesIncludingInvalid.end(); ++i)
		if ((*i)->hasCompiler())  // Ignore nodes with missing node classes.
			nonPublishedNodes.insert(*i);

	VuoNode *publishedInputNode = composition->getPublishedInputNode();

	set<VuoCable *> nonPublishedCablesIncludingInvalid = composition->getBase()->getCables();
	set<VuoCable *> nonPublishedCables;
	for (set<VuoCable *>::iterator i = nonPublishedCablesIncludingInvalid.begin(); i != nonPublishedCablesIncludingInvalid.end(); ++i)
		if ((*i)->getFromNode() && (*i)->getToNode())  // Ignore disconnected cables.
			nonPublishedCables.insert(*i);

	set<VuoCable *> publishedInputCables = composition->getBase()->getPublishedInputCables();

	set<VuoCompilerCable *> validPotentialCables;
	for (set<VuoCompilerCable *>::iterator i = potentialCables.begin(); i != potentialCables.end(); ++i)
		if ((*i)->getBase()->getToNode())  // Ignore published output cables.
			validPotentialCables.insert(*i);

	makeTriggers(nonPublishedNodes, publishedInputNode);
	makeVerticesAndEdges(nonPublishedCables, publishedInputCables, validPotentialCables, publishedInputNode);
	makeDownstreamVertices();
	sortVertices();
	makeEventlesslyDownstreamNodes(nonPublishedNodes, nonPublishedCables);
}

/**
 * Sets up VuoCompilerGraph::triggers.
 */
void VuoCompilerGraph::makeTriggers(set<VuoNode *> nonPublishedNodes, VuoNode *publishedInputNode)
{
	vector<VuoNode *> nodes;
	nodes.insert(nodes.end(), nonPublishedNodes.begin(), nonPublishedNodes.end());
	if (publishedInputNode)
		nodes.push_back(publishedInputNode);

	VuoCompilerTriggerPort *simultaneousTrigger = NULL;
	for (vector<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for (vector<VuoPort *>::iterator j = outputPorts.begin(); j != outputPorts.end(); ++j)
		{
			VuoCompilerTriggerPort *trigger = dynamic_cast<VuoCompilerTriggerPort *>((*j)->getCompiler());
			if (trigger)
			{
				nodeForTrigger[trigger] = node->getCompiler();

				if (node == publishedInputNode &&
						trigger->getBase()->getClass()->getName() == VuoNodeClass::publishedInputNodeSimultaneousTriggerName)
					simultaneousTrigger = trigger;
				else
					triggers.push_back(trigger);
			}
		}
	}
	if (simultaneousTrigger)
		triggers.push_back(simultaneousTrigger);
}

/**
 * Sets up VuoCompilerGraph::vertices (not yet in topological order) and VuoCompilerGraph::edges.
 */
void VuoCompilerGraph::makeVerticesAndEdges(set<VuoCable *> nonPublishedCables, set<VuoCable *> publishedInputCables,
											set<VuoCompilerCable *> potentialCables, VuoNode *publishedInputNode)
{
	// Create vertices to visit for all internal and published input cables in the composition (except for vuoSimultaneous).

	set<VuoCable *> cablesInComposition;
	cablesInComposition.insert(nonPublishedCables.begin(), nonPublishedCables.end());
	cablesInComposition.insert(publishedInputCables.begin(), publishedInputCables.end());
	for (set<VuoCompilerCable *>::iterator i = potentialCables.begin(); i != potentialCables.end(); ++i)
		cablesInComposition.insert((*i)->getBase());

	set<Vertex> allVertices;
	for (set<VuoCable *>::iterator i = cablesInComposition.begin(); i != cablesInComposition.end(); ++i)
	{
		VuoCable *cable = *i;

		VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(cable->getFromPort()->getCompiler());
		VuoCompilerNode *fromNode = cable->getFromNode()->getCompiler();
		VuoCompilerNode *toNode = cable->getToNode()->getCompiler();
		Vertex vertex = (fromTrigger ? Vertex(fromTrigger, toNode) : Vertex(fromNode, toNode));
		set<Vertex>::iterator vertexIter = allVertices.find(vertex);
		if (vertexIter != allVertices.end())
		{
			vertex.cableBundle = (*vertexIter).cableBundle;
			allVertices.erase(vertexIter);
		}
		vertex.cableBundle.insert(cable->getCompiler());
		allVertices.insert(vertex);
	}


	// For each trigger, add all vertices reachable from it.

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		set<Vertex> verticesToVisit;
		set<Edge> edgesVisited;

		for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.fromTrigger == trigger)
				verticesToVisit.insert(vertex);
		}

		while (! verticesToVisit.empty())
		{
			Vertex vertex = *verticesToVisit.begin();
			if (find(vertices[trigger].begin(), vertices[trigger].end(), vertex) == vertices[trigger].end())
				vertices[trigger].push_back(vertex);
			verticesToVisit.erase(verticesToVisit.begin());

			for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;

				if (vertex.toNode == outgoingVertex.fromNode && mayTransmit(vertex.cableBundle, outgoingVertex.cableBundle))
				{
					Edge outgoingEdge(vertex, outgoingVertex);
					if (edgesVisited.find(outgoingEdge) == edgesVisited.end())  // Avoid infinite feedback loops.
					{
						edges[trigger].insert(outgoingEdge);
						edgesVisited.insert(outgoingEdge);

						verticesToVisit.insert(outgoingVertex);
					}
				}
			}
		}
	}


	// Add cables, vertices, and edges for the vuoSimultaneous trigger (copying from the other published input ports).
	// This creates additional VuoCompilerCable objects that are not in the composition.

	if (publishedInputNode)
	{
		VuoPort *simultaneousPort = publishedInputNode->getOutputPortWithName(VuoNodeClass::publishedInputNodeSimultaneousTriggerName);
		VuoCompilerTriggerPort *simultaneousTrigger = static_cast<VuoCompilerTriggerPort *>(simultaneousPort->getCompiler());

		for (map<VuoCompilerTriggerPort *, VuoCompilerNode *>::iterator i = nodeForTrigger.begin(); i != nodeForTrigger.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			VuoCompilerNode *node = i->second;

			if (node->getBase() == publishedInputNode && trigger != simultaneousTrigger)
			{
				for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
				{
					Vertex vertex = *j;

					Vertex vertexCopy;
					if (vertex.fromTrigger)
					{
						vertexCopy = Vertex(simultaneousTrigger, vertex.toNode);
						for (set<VuoCompilerCable *>::iterator k = vertex.cableBundle.begin(); k != vertex.cableBundle.end(); ++k)
						{
							VuoCompilerCable *cable = *k;
							VuoCompilerNode *toNode = cable->getBase()->getToNode()->getCompiler();
							VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>(cable->getBase()->getToPort()->getCompiler());
							VuoCompilerCable *cableCopy = new VuoCompilerCable(publishedInputNode->getCompiler(), simultaneousTrigger,
																			   toNode, toPort);
							simultaneousTrigger->getBase()->removeConnectedCable(cableCopy->getBase());
							toPort->getBase()->removeConnectedCable(cableCopy->getBase());
							vertexCopy.cableBundle.insert(cableCopy);
						}
					}
					else
					{
						vertexCopy = vertex;
					}

					vector<Vertex>::iterator existingVertexIter =
							find(vertices[simultaneousTrigger].begin(), vertices[simultaneousTrigger].end(), vertexCopy);
					if (existingVertexIter != vertices[simultaneousTrigger].end())
					{
						vertexCopy.cableBundle.insert( existingVertexIter->cableBundle.begin(), existingVertexIter->cableBundle.end() );
						vertices[simultaneousTrigger].erase(existingVertexIter);
					}

					vertices[simultaneousTrigger].push_back(vertexCopy);
				}

				for (set<Edge>::iterator j = edges[trigger].begin(); j != edges[trigger].end(); ++j)
				{
					Edge edge = *j;
					Edge edgeCopy = (edge.fromVertex.fromTrigger ?
										 Edge(Vertex(simultaneousTrigger, edge.fromVertex.toNode), edge.toVertex) :
										 edge);
					edges[simultaneousTrigger].insert(edgeCopy);
				}
			}
		}
	}
}

/**
 * Sets up VuoCompilerGraph::downstreamVertices and VuoCompilerGraph::repeatedVertices.
 */
void VuoCompilerGraph::makeDownstreamVertices(void)
{
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		list<Vertex> verticesToVisit;  // Used as a stack, except for a call to find().

		for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
			if ((*j).fromTrigger == trigger)
				verticesToVisit.push_back(*j);

		while (! verticesToVisit.empty())
		{
			// Visit the vertex at the top of the stack.
			Vertex currentVertex = verticesToVisit.back();

			set<Vertex> currentDownstreamVertices;
			bool areDownstreamVerticesComplete = true;

			// Consider each vertex immediately downstream of this vertex.
			set<Edge> outgoingEdges = getEdgesFromVertex(currentVertex, trigger);
			for (set<Edge>::iterator j = outgoingEdges.begin(); j != outgoingEdges.end(); ++j)
			{
				Vertex outgoingVertex = (*j).toVertex;
				currentDownstreamVertices.insert(outgoingVertex);

				if (downstreamVertices[trigger].find(outgoingVertex) != downstreamVertices[trigger].end())
				{
					// The downstream vertex has already been visited, so add its downstream vertices to this vertex's.
					set<Vertex> furtherDownstreamVertices = downstreamVertices[trigger][outgoingVertex];
					currentDownstreamVertices.insert( furtherDownstreamVertices.begin(), furtherDownstreamVertices.end() );
				}
				else
				{
					if (find(verticesToVisit.begin(), verticesToVisit.end(), outgoingVertex) != verticesToVisit.end())
					{
						// The downstream vertex is already on the stack, so it's an infinite feedback loop.
						repeatedVertices[trigger].insert(outgoingVertex);
					}
					else
					{
						// The downstream vertex has not yet been visited, so add it to the stack.
						verticesToVisit.push_back(outgoingVertex);
						areDownstreamVerticesComplete = false;
					}
				}
			}

			if (areDownstreamVerticesComplete)
			{
				downstreamVertices[trigger][currentVertex] = currentDownstreamVertices;
				verticesToVisit.pop_back();
			}
		}

		// Clean up repeatedVertices so that it only contains vertices within an infinite feedback loop,
		// not other outgoing vertices from nodes in the infinite feedback loop.
		if (repeatedVertices.find(trigger) != repeatedVertices.end())
		{
			set<Vertex> repeatedVerticesCopy = repeatedVertices[trigger];
			for (set<Vertex>::iterator j = repeatedVerticesCopy.begin(); j != repeatedVerticesCopy.end(); ++j)
			{
				Vertex vertex = *j;
				if (downstreamVertices[trigger][vertex].find(vertex) == downstreamVertices[trigger][vertex].end())
					repeatedVertices[trigger].erase(vertex);
			}
		}
	}
}

/**
 * Puts VuoCompilerGraph::vertices in topological order.
 */
void VuoCompilerGraph::sortVertices(void)
{
	map<VuoCompilerTriggerPort *, map<Vertex, set<Vertex> > > dependentVertices = downstreamVertices;

	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		vector<Vertex> verticesToSort = i->second;

		// For each vertex, create a list of all vertices that can't be reached until after the vertex.
		// This includes all vertices downstream of the vertex, plus all vertices downstream of gathers
		// involving the vertex (even if the vertex itself hits a wall).

		for (vector<Vertex>::iterator j = verticesToSort.begin(); j != verticesToSort.end(); ++j)
		{
			Vertex vertex = *j;

			if (mayTransmit(vertex.toNode, trigger))
			{
				vector<VuoCompilerNode *> downstreamNodes = getNodesDownstream(vertex.toNode, trigger);
				if (find(downstreamNodes.begin(), downstreamNodes.end(), vertex.fromNode) == downstreamNodes.end())
				{
					set<Vertex> dependentVerticesForToNode;
					for (vector<Vertex>::iterator k = vertices[trigger].begin(); k != vertices[trigger].end(); ++k)
					{
						Vertex otherVertex = *k;

						if (vertex.toNode == otherVertex.fromNode)
						{
							dependentVerticesForToNode.insert( otherVertex );
							dependentVerticesForToNode.insert( dependentVertices[trigger][otherVertex].begin(),
															   dependentVertices[trigger][otherVertex].end());
						}
					}

					for (vector<Vertex>::iterator k = vertices[trigger].begin(); k != vertices[trigger].end(); ++k)
					{
						Vertex otherVertex = *k;

						if (otherVertex == vertex ||
								dependentVertices[trigger][otherVertex].find(vertex) != dependentVertices[trigger][otherVertex].end())
						{
							dependentVertices[trigger][otherVertex].insert( dependentVerticesForToNode.begin(),
																			dependentVerticesForToNode.end() );
						}
					}
				}
			}
		}

		// Put the vertices in descending order of the number of vertices that can't be reached until after the vertex.

		vector<Vertex> sortedVertices;
		while (! verticesToSort.empty())
		{
			size_t maxSize = 0;
			size_t maxIndex = 0;
			for (size_t j = 0; j < verticesToSort.size(); ++j)
			{
				size_t currSize = dependentVertices[trigger][verticesToSort[j]].size();
				if (currSize > maxSize)
				{
					maxSize = currSize;
					maxIndex = j;
				}
			}
			sortedVertices.push_back(verticesToSort[maxIndex]);
			verticesToSort.erase(maxIndex + verticesToSort.begin());
		}

		vertices[trigger] = sortedVertices;
	}
}

/**
 * Sets up VuoCompilerGraph::eventlesslyDownstreamNodes.
 */
void VuoCompilerGraph::makeEventlesslyDownstreamNodes(set<VuoNode *> nonPublishedNodes, set<VuoCable *> nonPublishedCables)
{
	// Find all nodes that can transmit through their output data cables without an event.

	set<VuoCompilerNode *> eventlesslyTransmittingNodes;
	for (set<VuoNode *>::iterator i = nonPublishedNodes.begin(); i != nonPublishedNodes.end(); ++i)
	{
		VuoCompilerNode *node = (*i)->getCompiler();
		if (mayTransmitEventlessly(node))
			eventlesslyTransmittingNodes.insert(node);
	}


	// Put those nodes in topological order.
	// (Assumes the nodes don't have walled ports, so they can't have different topological orders for different triggers.)

	map<VuoCompilerNode *, set<VuoCompilerNode *> > remainingIncomingNodes;
	map<VuoCompilerNode *, set<VuoCompilerNode *> > remainingOutgoingNodes;
	for (set<VuoCompilerNode *>::iterator i = eventlesslyTransmittingNodes.begin(); i != eventlesslyTransmittingNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;
		for (set<VuoCable *>::iterator j = nonPublishedCables.begin(); j != nonPublishedCables.end(); ++j)
		{
			VuoCable *cable = *j;

			VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(cable->getFromPort()->getCompiler());
			if (fromTrigger || ! cable->getCompiler()->carriesData())
				continue;  // Ignore cables that can't transmit without an event.

			VuoCompilerNode *fromNode = cable->getFromNode()->getCompiler();
			VuoCompilerNode *toNode = cable->getToNode()->getCompiler();
			if (toNode == node)
				remainingIncomingNodes[node].insert(fromNode);
			if (fromNode == node)
				remainingOutgoingNodes[node].insert(toNode);
		}
	}
	map<VuoCompilerNode *, set<VuoCompilerNode *> > outgoingNodes = remainingOutgoingNodes;

	set<VuoCompilerNode *> nodesToVisit;
	for (set<VuoCompilerNode *>::iterator i = eventlesslyTransmittingNodes.begin(); i != eventlesslyTransmittingNodes.end(); ++i)
	{
		VuoCompilerNode *node = *i;
		if (remainingIncomingNodes.find(node) == remainingIncomingNodes.end())
			nodesToVisit.insert(node);
	}

	vector<VuoCompilerNode *> sortedEventlesslyTransmittingNodes;
	while (! nodesToVisit.empty())
	{
		VuoCompilerNode *node = *nodesToVisit.begin();
		nodesToVisit.erase(nodesToVisit.begin());

		sortedEventlesslyTransmittingNodes.push_back(node);

		set<VuoCompilerNode *> outgoingNodesCopy = remainingOutgoingNodes[node];
		for (set<VuoCompilerNode *>::iterator i = outgoingNodesCopy.begin(); i != outgoingNodesCopy.end(); ++i)
		{
			VuoCompilerNode *outgoingNode = *i;

			remainingIncomingNodes[outgoingNode].erase(node);
			remainingOutgoingNodes[node].erase(outgoingNode);

			if (remainingIncomingNodes[outgoingNode].empty())
				nodesToVisit.insert(outgoingNode);
		}
	}


	// For each of those nodes, find the nodes that are downstream of it via eventless transmission, in topological order.

	for (int firstIndex = sortedEventlesslyTransmittingNodes.size() - 1; firstIndex >= 0; --firstIndex)
	{
		VuoCompilerNode *firstNode = sortedEventlesslyTransmittingNodes[firstIndex];

		for (size_t possiblyDownstreamIndex = firstIndex + 1; possiblyDownstreamIndex < sortedEventlesslyTransmittingNodes.size(); ++possiblyDownstreamIndex)
		{
			VuoCompilerNode *possiblyDownstreamNode = sortedEventlesslyTransmittingNodes[possiblyDownstreamIndex];

			if (outgoingNodes[firstNode].find(possiblyDownstreamNode) != outgoingNodes[firstNode].end())
			{
				vector<VuoCompilerNode *> nodesToAdd = eventlesslyDownstreamNodes[possiblyDownstreamNode];
				nodesToAdd.insert(nodesToAdd.begin(), possiblyDownstreamNode);

				for (vector<VuoCompilerNode *>::iterator j = nodesToAdd.begin(); j != nodesToAdd.end(); ++j)
				{
					VuoCompilerNode *nodeToAdd = *j;
					if (find(eventlesslyDownstreamNodes[firstNode].begin(), eventlesslyDownstreamNodes[firstNode].end(), nodeToAdd) == eventlesslyDownstreamNodes[firstNode].end())
						eventlesslyDownstreamNodes[firstNode].push_back(nodeToAdd);
				}
			}
		}
	}
}

/**
 * Returns true if an event coming into a node through @a fromCables may transmit to outgoing @a toCables, based
 * on the event-blocking behavior of the ports in the node.
 *
 * Assumes that the @c toNode of all @a fromCables and the @a fromNode of all @a toCables are the same.
 */
bool VuoCompilerGraph::mayTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables)
{
	for (set<VuoCompilerCable *>::const_iterator i = fromCables.begin(); i != fromCables.end(); ++i)
	{
		VuoPort *inputPort = (*i)->getBase()->getToPort();
		if (inputPort->getClass()->getEventBlocking() != VuoPortClass::EventBlocking_Wall)
			return true;
	}

	return false;
}

/**
 * Returns true if an event through the vertex will transmit to any outgoing cables from @c vertex.toNode .
 */
bool VuoCompilerGraph::mayTransmit(Vertex vertex, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerCable *> cablesBetweenFromNodeAndToNode = vertex.cableBundle;

	set<VuoCompilerCable *> cablesOutOfToNode;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
	{
		Vertex otherVertex = *i;
		if (vertex.toNode == otherVertex.fromNode)
			cablesOutOfToNode.insert(otherVertex.cableBundle.begin(), otherVertex.cableBundle.end());
	}

	return mayTransmit(cablesBetweenFromNodeAndToNode, cablesOutOfToNode);
}

/**
 * Returns true if an event coming into @a node from @a trigger may transmit to any outgoing cables from @a node.
 */
bool VuoCompilerGraph::mayTransmit(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerCable *> incomingCables;
	set<VuoCompilerCable *> outgoingCables;

	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
	{
		Vertex vertex = *i;

		if (vertex.toNode == node)
			incomingCables.insert( vertex.cableBundle.begin(), vertex.cableBundle.end() );
		if (vertex.fromNode == node)
			outgoingCables.insert( vertex.cableBundle.begin(), vertex.cableBundle.end() );
	}

	return mayTransmit(incomingCables, outgoingCables);
}

/**
 * Returns true if an event from @a trigger may transmit along cables that directly connect @a fromNode to @a toNode.
 */
bool VuoCompilerGraph::mayTransmit(VuoCompilerNode *fromNode, VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger)
{
	return find(vertices[trigger].begin(), vertices[trigger].end(), Vertex(fromNode, toNode)) != vertices[trigger].end();
}

/**
 * Returns true if @a node belongs to a node class that can transmit data through output cables without an event.
 */
bool VuoCompilerGraph::mayTransmitEventlessly(VuoCompilerNode *node)
{
	set<string> prefixes;
	prefixes.insert("vuo.list.make.");
	prefixes.insert("vuo.dictionary.make.");

	string nodeClassName = node->getBase()->getNodeClass()->getClassName();
	for (set<string>::iterator i = prefixes.begin(); i != prefixes.end(); ++i)
		if (VuoStringUtilities::beginsWith(nodeClassName, *i))
			return true;

	return false;
}

/**
 * Returns all trigger ports, including those for published input ports (including vuoSimultaneous).
 */
vector<VuoCompilerTriggerPort *> VuoCompilerGraph::getTriggerPorts(void)
{
	return triggers;
}

/**
 * Returns a mapping of each trigger port to the node that contains it.
 */
map<VuoCompilerTriggerPort *, VuoCompilerNode *> VuoCompilerGraph::getNodesForTriggerPorts(void)
{
	return nodeForTrigger;
}

/**
 * Returns the outgoing edges from @a vertex that are reachable from @a trigger.
 */
set<VuoCompilerGraph::Edge> VuoCompilerGraph::getEdgesFromVertex(Vertex vertex, VuoCompilerTriggerPort *trigger)
{
	set<Edge> edgesFromVertex;

	for (set<Edge>::iterator i = edges[trigger].begin(); i != edges[trigger].end(); ++i)
		if ((*i).fromVertex == vertex)
			edgesFromVertex.insert(*i);

	return edgesFromVertex;
}

/**
 * Returns a count of the vertices that have the given @a fromNode and are reachable from @a trigger.
 */
size_t VuoCompilerGraph::getNumVerticesWithFromNode(VuoCompilerNode *fromNode, VuoCompilerTriggerPort *trigger)
{
	int numVertices = 0;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if ((*i).fromNode == fromNode)
			++numVertices;

	return numVertices;
}

/**
 * Returns a count of the vertices that have the given @a toNode and are reachable from @a trigger.
 */
size_t VuoCompilerGraph::getNumVerticesWithToNode(VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger)
{
	int numVertices = 0;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if ((*i).toNode == toNode)
			++numVertices;

	return numVertices;
}

/**
 * Returns a data structure that divides the graph up into linear chains of nodes.
 *
 * A chain is a list of nodes where each is connected to the next by a single vertex (set of cables).
 * There is no scattering/branching or gathering/joining within a chain.
 *
 * If the graph has a feedback loop, then the one repeated node in the feedback loop appears in
 * two chains (the second chain consisting of just that node).
 *
 * For each trigger, the chains are listed in topological order.
 */
map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > VuoCompilerGraph::getChains(void)
{
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chains;

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		// Visit the vertices in topological order, and put the nodes into lists (chains-in-progress).
		vector< vector<VuoCompilerNode *> > chainsInProgress;
		set<VuoCompilerNode *> nodesAdded;
		for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
		{
			Vertex vertex = *j;
			bool addedToChain = false;

			if (nodesAdded.find(vertex.toNode) != nodesAdded.end())
				continue;  // Avoid creating duplicate chains for nodes with gathers (multiple incoming vertices).
			nodesAdded.insert(vertex.toNode);

			if (vertex.fromNode)
			{
				if (getNumVerticesWithFromNode(vertex.fromNode, trigger) == 1 &&
						getNumVerticesWithToNode(vertex.toNode, trigger) == 1)
				{
					for (int k = 0; k < chainsInProgress.size(); ++k)
					{
						VuoCompilerNode *lastNodeInChain = chainsInProgress[k].back();
						if (lastNodeInChain == vertex.fromNode)
						{
							// Add vertex.toNode to an existing chain-in-progress.
							chainsInProgress[k].push_back(vertex.toNode);
							addedToChain = true;
							break;
						}
					}
				}
			}

			if (! addedToChain)
			{
				// Create a new chain-in-progress that starts with vertex.toNode.
				chainsInProgress.push_back( vector<VuoCompilerNode *>(1, vertex.toNode) );
			}
		}

		// Turn the chains-in-progress into actual chains.
		for (vector< vector<VuoCompilerNode *> >::iterator j = chainsInProgress.begin(); j != chainsInProgress.end(); ++j)
		{
			vector<VuoCompilerNode *> chainNodes = *j;
			VuoCompilerChain *chain = new VuoCompilerChain(chainNodes, false);
			chains[trigger].push_back(chain);
		}

		// Create a new chain for each node that is the repeated node in a feedback loop.
		map<VuoCompilerNode *, bool> nodesSeen;
		for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
		{
			VuoCompilerNode *node = (*j).toNode;
			if (nodesSeen[node])
				continue;
			nodesSeen[node] = true;

			if (isRepeatedInFeedbackLoop(node, trigger))
			{
				VuoCompilerChain *chain = new VuoCompilerChain(vector<VuoCompilerNode *>(1, node), true);
				chains[trigger].push_back(chain);
			}
		}
	}

	return chains;
}

/**
 * Returns the nodes that are directly connected to @a trigger through its outgoing cables.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesImmediatelyDownstream(VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerNode *> downstreamNodes;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if ((*i).fromTrigger == trigger)
			downstreamNodes.insert((*i).toNode);

	return vector<VuoCompilerNode *>(downstreamNodes.begin(), downstreamNodes.end());
}

/**
 * Returns the nodes that are directly connected to @a node through its outgoing cables and are reachable from @a trigger.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesImmediatelyDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerNode *> downstreamNodes;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if ((*i).fromNode == node)
			downstreamNodes.insert((*i).toNode);

	return vector<VuoCompilerNode *>(downstreamNodes.begin(), downstreamNodes.end());
}

/**
 * Returns the nodes that can be reached by an event from @a trigger.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesDownstream(VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerNode *> downstreamNodes;
	vector<VuoCompilerNode *> immediatelyDownstreamNodes = getNodesImmediatelyDownstream(trigger);
	downstreamNodes.insert(immediatelyDownstreamNodes.begin(), immediatelyDownstreamNodes.end());

	for (vector<VuoCompilerNode *>::iterator i = immediatelyDownstreamNodes.begin(); i != immediatelyDownstreamNodes.end(); ++i)
	{
		vector<VuoCompilerNode *> furtherDownstreamNodes = getNodesDownstream(*i, trigger);
		downstreamNodes.insert(furtherDownstreamNodes.begin(), furtherDownstreamNodes.end());
	}

	return vector<VuoCompilerNode *>(downstreamNodes.begin(), downstreamNodes.end());
}

/**
 * Returns the nodes that can be reached by an event from @a trigger that has passed through @a node.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	set<Vertex> verticesWithDownstreamNodes;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
	{
		Vertex vertex = *i;
		if (vertex.fromNode == node)
		{
			verticesWithDownstreamNodes.insert(vertex);
			set<Vertex> downstream = downstreamVertices[trigger][vertex];
			verticesWithDownstreamNodes.insert(downstream.begin(), downstream.end());
		}
	}

	set<VuoCompilerNode *> downstreamNodes;
	for (set<Vertex>::iterator i = verticesWithDownstreamNodes.begin(); i != verticesWithDownstreamNodes.end(); ++i)
		downstreamNodes.insert((*i).toNode);

	return vector<VuoCompilerNode *>(downstreamNodes.begin(), downstreamNodes.end());
}

/**
 * Returns the nodes that can be reached by data eventlessly transmitted from the output cables of @a node,
 * in topological order.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesEventlesslyDownstream(VuoCompilerNode *node)
{
	return eventlesslyDownstreamNodes[node];
}

/**
 * Returns the outgoing cables from @a outputPort, including any that were created by this class (but not added to the
 * composition) for the @c vuoSimultaneous port.
 */
set<VuoCompilerCable *> VuoCompilerGraph::getCablesImmediatelyDownstream(VuoCompilerPort *outputPort)
{
	set<VuoCompilerCable *> downstreamCables;

	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		for (vector<Vertex>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			for (set<VuoCompilerCable *>::iterator k = (*j).cableBundle.begin(); k != (*j).cableBundle.end(); ++k)
			{
				VuoCompilerCable *cable = *k;
				if (cable->getBase()->getFromPort() == outputPort->getBase())
					downstreamCables.insert(cable);
			}
		}
	}

	return downstreamCables;
}

/**
 * Returns the outgoing cables from @a outputPort that can transmit data eventlessly.
 */
set<VuoCompilerCable *> VuoCompilerGraph::getCablesImmediatelyEventlesslyDownstream(VuoCompilerPort *outputPort)
{
	set<VuoCompilerCable *> downstreamCables;

	vector<VuoCable *> downstreamCablesList = outputPort->getBase()->getConnectedCables(false);
	for (vector<VuoCable *>::iterator i = downstreamCablesList.begin(); i != downstreamCablesList.end(); ++i)
		downstreamCables.insert( (*i)->getCompiler() );

	return downstreamCables;
}

/**
 * Returns true if @a node is the one repeated node in a feedback loop downstream of @a trigger.
 */
bool VuoCompilerGraph::isRepeatedInFeedbackLoop(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	vector<VuoCompilerNode *> downstreamNodes = getNodesDownstream(node, trigger);
	return find(downstreamNodes.begin(), downstreamNodes.end(), node) != downstreamNodes.end();
}

/**
 * Returns true if @a trigger has a gather (node with multiple incoming vertices) somewhere downstream.
 */
bool VuoCompilerGraph::hasGatherDownstream(VuoCompilerTriggerPort *trigger)
{
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if (getNumVerticesWithToNode((*i).toNode, trigger) > 1)
			return true;

	return false;
}

/**
 * Checks that the composition does not have any infinite feedback loops.
 *
 * An infinite feedback loop is detected whenever an event may go through a feedback loop more than once.
 * To avoid this possibility, a feedback loop must use a walled port on the feedback loop's final (and
 * only repeated) node to block the event.
 *
 * @throw VuoCompilerException An event may travel through the same cable more than once.
 */
void VuoCompilerGraph::checkForInfiniteFeedback()
{
	// Report any errors, with all nodes and cables involved in the infinite feedback loops.
	if (! repeatedVertices.empty())
	{
		vector<VuoCompilerError> errors;
		for (map<VuoCompilerTriggerPort *, set<Vertex> >::iterator i = repeatedVertices.begin(); i != repeatedVertices.end(); ++i)
		{
			VuoCompilerTriggerPort *trigger = i->first;
			for (set<Vertex>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				Vertex repeatedVertex = *j;

				set<VuoNode *> nodesInLoop;
				set<VuoCable *> cablesInLoop;
				for (vector<Vertex>::iterator k = vertices[trigger].begin(); k != vertices[trigger].end(); ++k)
				{
					Vertex otherVertex = *k;
					if (downstreamVertices[trigger][repeatedVertex].find(otherVertex) != downstreamVertices[trigger][repeatedVertex].end() &&
							downstreamVertices[trigger][otherVertex].find(repeatedVertex) != downstreamVertices[trigger][otherVertex].end())
					{
						nodesInLoop.insert(otherVertex.fromNode->getBase());
						nodesInLoop.insert(otherVertex.toNode->getBase());

						for (set<VuoCompilerCable *>::iterator m = otherVertex.cableBundle.begin(); m != otherVertex.cableBundle.end(); ++m)
							cablesInLoop.insert((*m)->getBase());
					}
				}

				VuoCompilerError error("Infinite feedback loop",
									   "To keep events from getting stuck repeatedly going around a loop of nodes and cables, "
									   "each event is only allowed to travel around a feedback loop once. "
									   "Your composition has a feedback loop that might let an event go around more than once. "
									   "You can fix the feedback loop by inserting a node with an event wall, such as 'Hold Value'.",
									   nodesInLoop, cablesInLoop);
				errors.push_back(error);
			}
		}

		throw VuoCompilerException(errors);
	}
}

/**
 * Checks that the compositon does not have any deadlocked feedback loops.
 *
 * A deadlocked feedback loop makes it impossible to put the composition's nodes into an order in which
 * they can be executed. It's a situation where node A must execute before node B, but node B must execute
 * before node A.
 *
 * @throw VuoCompilerException The composition contains a pair of nodes that are downstream of each other.
 */
void VuoCompilerGraph::checkForDeadlockedFeedback(void)
{
	vector<VuoCompilerError> errors;

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		// For each node, find all nodes downstream of it.
		map<VuoCompilerNode *, set<VuoCompilerNode *> > downstreamNodes;
		for (map<Vertex, set<Vertex> >::iterator j = downstreamVertices[trigger].begin(); j != downstreamVertices[trigger].end(); ++j)
		{
			Vertex upstreamVertex = j->first;
			set<VuoCompilerNode *> downstreamNodesForVertex;
			bool isRepeated = false;

			for (set<Vertex>::iterator k = j->second.begin(); k != j->second.end(); ++k)
			{
				Vertex downstreamVertex = *k;
				if (upstreamVertex.toNode == downstreamVertex.toNode)
				{
					isRepeated = true;
					break;
				}

				downstreamNodesForVertex.insert(downstreamVertex.toNode);
			}

			if (! isRepeated)
				downstreamNodes[upstreamVertex.toNode].insert(downstreamNodesForVertex.begin(), downstreamNodesForVertex.end());
		}

		// Find any pairs of nodes that are mutually downstream.
		set< pair<VuoCompilerNode *, VuoCompilerNode *> > mutuallyDownstreamNodePairs;
		for (map<VuoCompilerNode *, set<VuoCompilerNode *> >::iterator j = downstreamNodes.begin(); j != downstreamNodes.end(); ++j)
		{
			VuoCompilerNode *upstreamNode = j->first;
			for (set<VuoCompilerNode *>::iterator k = j->second.begin(); k != j->second.end(); ++k)
			{
				VuoCompilerNode *downstreamNode = *k;
				if (downstreamNodes[downstreamNode].find(upstreamNode) != downstreamNodes[downstreamNode].end() &&
						mutuallyDownstreamNodePairs.find( make_pair(downstreamNode, upstreamNode) ) == mutuallyDownstreamNodePairs.end())
				{
					mutuallyDownstreamNodePairs.insert( make_pair(upstreamNode, downstreamNode) );
				}
			}
		}

		// Report any errors, with all nodes and cables involved in the deadlocked feedback loops.
		for (set< pair<VuoCompilerNode *, VuoCompilerNode *> >::iterator j = mutuallyDownstreamNodePairs.begin(); j != mutuallyDownstreamNodePairs.end(); ++j)
		{
			VuoCompilerNode *firstNode = j->first;
			VuoCompilerNode *secondNode = j->second;

			set<VuoNode *> nodesInLoop;
			set<VuoCable *> cablesInLoop;
			nodesInLoop.insert(firstNode->getBase());
			nodesInLoop.insert(secondNode->getBase());

			for (set<VuoCompilerNode *>::iterator k = downstreamNodes[firstNode].begin(); k != downstreamNodes[firstNode].end(); ++k)
			{
				VuoCompilerNode *otherNode = *k;
				if (downstreamNodes[otherNode].find(secondNode) != downstreamNodes[otherNode].end())
					nodesInLoop.insert(otherNode->getBase());
			}
			for (set<VuoCompilerNode *>::iterator k = downstreamNodes[secondNode].begin(); k != downstreamNodes[secondNode].end(); ++k)
			{
				VuoCompilerNode *otherNode = *k;
				if (downstreamNodes[otherNode].find(firstNode) != downstreamNodes[otherNode].end())
					nodesInLoop.insert(otherNode->getBase());
			}

			for (vector<Vertex>::iterator k = vertices[trigger].begin(); k != vertices[trigger].end(); ++k)
			{
				Vertex vertex = *k;
				if (vertex.fromNode &&
						nodesInLoop.find(vertex.fromNode->getBase()) != nodesInLoop.end() &&
						nodesInLoop.find(vertex.toNode->getBase()) != nodesInLoop.end())
				{
					for (set<VuoCompilerCable *>::iterator m = vertex.cableBundle.begin(); m != vertex.cableBundle.end(); ++m)
						cablesInLoop.insert((*m)->getBase());
				}
			}

			VuoCompilerError error("Deadlocked feedback loop",
								   "To make sure that nodes execute in the right order, an event needs to travel "
								   "through the cables leading up to a node before it can execute the node. "
								   "Your composition has a situation where an event can't travel through the cables "
								   "leading up to a node until after it executes the node, which is impossible. "
								   "You can fix the problem by deleting some of the cables involved.",
								   nodesInLoop, cablesInLoop);
			errors.push_back(error);
		}
	}

	if (! errors.empty())
		throw VuoCompilerException(errors);
}

/**
 * Creates a vertex representing the cables from @a fromTrigger to @a toNode.
 */
VuoCompilerGraph::Vertex::Vertex(VuoCompilerTriggerPort *fromTrigger, VuoCompilerNode *toNode)
{
	this->fromNode = NULL;
	this->fromTrigger = fromTrigger;
	this->toNode = toNode;
}

/**
 * Creates a vertex representing the cables from @a fromNode to @a toNode.
 */
VuoCompilerGraph::Vertex::Vertex(VuoCompilerNode *fromNode, VuoCompilerNode *toNode)
{
	this->fromTrigger = NULL;
	this->fromNode = fromNode;
	this->toNode = toNode;
}

/**
 * Needed so this type can be used in STL containers.
 */
VuoCompilerGraph::Vertex::Vertex(void)
{
	this->fromNode = NULL;
	this->fromTrigger = NULL;
	this->toNode = NULL;
}

/**
 * Creates an edge from @a fromVertex to @a toVertex.
 */
VuoCompilerGraph::Edge::Edge(const VuoCompilerGraph::Vertex &fromVertex, const VuoCompilerGraph::Vertex &toVertex)
	: fromVertex(fromVertex), toVertex(toVertex)
{
}

/**
 * Needed so this type can be used in STL containers.
 */
VuoCompilerGraph::Edge::Edge(void)
{
}

/**
 * Returns true if the vertices represent the same trigger-to-node or node-to-node connection.
 */
bool operator==(const VuoCompilerGraph::Vertex &lhs, const VuoCompilerGraph::Vertex &rhs)
{
	return (lhs.fromTrigger == rhs.fromTrigger && lhs.fromNode == rhs.fromNode && lhs.toNode == rhs.toNode);
}

/**
 * Returns true if the vertices represent different trigger-to-node or node-to-node connections.
 */
bool operator!=(const VuoCompilerGraph::Vertex &lhs, const VuoCompilerGraph::Vertex &rhs)
{
	return ! (lhs == rhs);
}

/**
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerGraph::Vertex &lhs, const VuoCompilerGraph::Vertex &rhs)
{
	return (lhs.fromTrigger != rhs.fromTrigger ?
								   lhs.fromTrigger < rhs.fromTrigger :
								   (lhs.fromNode != rhs.fromNode ?
														lhs.fromNode < rhs.fromNode :
														lhs.toNode < rhs.toNode));
}

/**
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerGraph::Edge &lhs, const VuoCompilerGraph::Edge &rhs)
{
	return (lhs.fromVertex != rhs.fromVertex ?
								  lhs.fromVertex < rhs.fromVertex :
								  lhs.toVertex < rhs.toVertex);
}

/**
 * For debugging.
 */
void VuoCompilerGraph::Vertex::print(void) const
{
	printf("%s->%s\n",
		   (fromNode ?
				fromNode->getBase()->getTitle().c_str() :
				fromTrigger->getBase()->getClass()->getName().c_str()),
		   toNode->getBase()->getTitle().c_str());
}

/**
 * For debugging.
 */
void VuoCompilerGraph::Edge::print(void) const
{
	fromVertex.print();
	printf(" -> ");
	toVertex.print();
	printf("\n");
}
