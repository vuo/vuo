/**
 * @file
 * VuoCompilerGraph implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerGraph.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include <sstream>
#include <stdexcept>
#include <stack>

/**
 * Creates a graph representation of the (current state of) the composition, to be used for checking the validity
 * of the composition.
 *
 * @param composition The composition to represent. The graph representation is a snapshot of the composition passed
 *		into this constructor, and does not update if the composition is modified.
 * @param potentialCables Cables that are not yet in @a composition but should be added to the graph representation.
 */
VuoCompilerGraph::VuoCompilerGraph(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables)
{
	initialize(composition, potentialCables, NULL, NULL);
}

/**
 * Creates a graph representation of the (current state of) the composition, to be used for generating code for
 * the composition.
 *
 * @param composition The composition to represent. The graph representation is a snapshot of the composition passed
 *		into this constructor, and does not update if the composition is modified.
 * @param publishedInputNode A node instantiated from VuoCompilerPublishedInputNodeClass, used to add a
 *		representation of the composition's published input ports and published input trigger to the graph representation.
 * @param publishedOutputNode A node instantiated from VuoCompilerPublishedOutputNodeClass, used to add a
 *		representation of the composition's published output ports to the graph representation.
 */
VuoCompilerGraph::VuoCompilerGraph(VuoCompilerComposition *composition, VuoCompilerNode *publishedInputNode, VuoCompilerNode *publishedOutputNode)
{
	initialize(composition, set<VuoCompilerCable *>(), publishedInputNode, publishedOutputNode);
}

/**
 * Shared by constructors.
 */
void VuoCompilerGraph::initialize(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables,
								  VuoCompilerNode *publishedInputNode, VuoCompilerNode *publishedOutputNode)
{
	publishedInputTrigger = NULL;
	this->publishedInputNode = publishedInputNode;
	this->publishedOutputNode = publishedOutputNode;

	set<VuoNode *> nodesIncludingInvalid = composition->getBase()->getNodes();
	set<VuoNode *> nodes;
	for (set<VuoNode *>::iterator i = nodesIncludingInvalid.begin(); i != nodesIncludingInvalid.end(); ++i)
		if ((*i)->hasCompiler())  // Ignore nodes with missing node classes.
			nodes.insert(*i);

	set<VuoCable *> cablesIncludingInvalidAndPublished = composition->getBase()->getCables();
	set<VuoCable *> nonPublishedCables;
	for (set<VuoCable *>::iterator i = cablesIncludingInvalidAndPublished.begin(); i != cablesIncludingInvalidAndPublished.end(); ++i)
		if ((*i)->getFromNode() && (*i)->getToNode() && ! (*i)->isPublished())  // Ignore disconnected cables and published cables.
			nonPublishedCables.insert(*i);

	set<VuoCable *> validPotentialCables;
	for (set<VuoCompilerCable *>::iterator i = potentialCables.begin(); i != potentialCables.end(); ++i)
		if (! (*i)->getBase()->isPublished())  // Ignore published cables.
			validPotentialCables.insert( (*i)->getBase() );

	makeTriggers(nodes);
	makeVerticesAndEdges(nodes, nonPublishedCables, validPotentialCables,
						 composition->getBase()->getPublishedInputPorts(), composition->getBase()->getPublishedOutputPorts());
	makeDownstreamVertices();
	sortVertices();
	makeVertexDistances();
	makeEventlesslyDownstreamNodes(nodes, nonPublishedCables);
}

/**
 * Sets up VuoCompilerGraph::triggers and VuoCompilerGraph::publishedInputTrigger.
 */
void VuoCompilerGraph::makeTriggers(set<VuoNode *> nodes)
{
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoNode *node = *i;
		vector<VuoPort *> outputPorts = node->getOutputPorts();
		for (vector<VuoPort *>::iterator j = outputPorts.begin(); j != outputPorts.end(); ++j)
		{
			VuoCompilerTriggerPort *trigger = dynamic_cast<VuoCompilerTriggerPort *>((*j)->getCompiler());
			if (trigger)
			{
				nodeForTrigger[trigger] = node->getCompiler();
				triggers.push_back(trigger);
			}
		}
	}

	if (publishedInputNode)
	{
		VuoPort *port = publishedInputNode->getBase()->getOutputPorts().at( VuoNodeClass::unreservedOutputPortStartIndex );
		publishedInputTrigger = static_cast<VuoCompilerTriggerPort *>(port->getCompiler());
		nodeForTrigger[publishedInputTrigger] = publishedInputNode;
		triggers.push_back(publishedInputTrigger);
	}
}

/**
 * Sets up VuoCompilerGraph::vertices (not yet in topological order), VuoCompilerGraph::edges, and
 * VuoCompilerGraph::verticesNonBlocking.
 */
void VuoCompilerGraph::makeVerticesAndEdges(set<VuoNode *> nodes, set<VuoCable *> cables, set<VuoCable *> potentialCables,
											vector<VuoPublishedPort *> publishedInputPorts, vector<VuoPublishedPort *> publishedOutputPorts)
{
	// Create vertices to visit for all internal cables in the composition.

	set<VuoCable *> cablesInComposition;
	cablesInComposition.insert(cables.begin(), cables.end());
	cablesInComposition.insert(potentialCables.begin(), potentialCables.end());

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


	// Create cables and vertices to visit for all published input cables in the composition.
	// For each published input cable in the composition, the graph gets a cable and vertex from publishedInputNode.
	if (publishedInputNode)
	{
		for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
		{
			vector<VuoCable *> publishedInputCables = (*i)->getConnectedCables();
			for (vector<VuoCable *>::iterator j = publishedInputCables.begin(); j != publishedInputCables.end(); ++j)
			{
				VuoCable *publishedInputCable = *j;
				int fromPortIndex = VuoNodeClass::unreservedOutputPortStartIndex;

				VuoCompilerNode *fromNode = publishedInputNode;
				VuoCompilerPort *fromPort = static_cast<VuoCompilerPort *>( publishedInputNode->getBase()->getOutputPorts().at(fromPortIndex)->getCompiler() );
				VuoCompilerNode *toNode = publishedInputCable->getToNode()->getCompiler();
				VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>( publishedInputCable->getToPort()->getCompiler() );
				VuoCompilerCable *cable = new VuoCompilerCable(fromNode, fromPort, toNode, toPort);
				cable->setAlwaysEventOnly(publishedInputCable->getCompiler()->getAlwaysEventOnly());

				VuoCompilerTriggerPort *fromTrigger = static_cast<VuoCompilerTriggerPort *>(fromPort);
				Vertex vertex(fromTrigger, toNode);
				set<Vertex>::iterator vertexIter = allVertices.find(vertex);
				if (vertexIter != allVertices.end())
				{
					vertex.cableBundle = (*vertexIter).cableBundle;
					allVertices.erase(vertexIter);
				}
				vertex.cableBundle.insert(cable);
				allVertices.insert(vertex);
			}
		}
	}


	// Create cables and vertices to visit for all published output cables in the composition.
	// For each published output cable in the composition, the graph gets a cable and vertex to publishedOutputNode.

	if (publishedOutputNode)
	{
		for (size_t i = 0; i < publishedOutputPorts.size(); ++i)
		{
			vector<VuoCable *> publishedOutputCables = publishedOutputPorts[i]->getConnectedCables();
			for (vector<VuoCable *>::iterator j = publishedOutputCables.begin(); j != publishedOutputCables.end(); ++j)
			{
				VuoCable *publishedOutputCable = *j;
				int toPortIndex = VuoNodeClass::unreservedInputPortStartIndex + i;

				VuoCompilerNode *fromNode = publishedOutputCable->getFromNode()->getCompiler();
				VuoCompilerPort *fromPort = static_cast<VuoCompilerPort *>( publishedOutputCable->getFromPort()->getCompiler() );
				VuoCompilerNode *toNode = publishedOutputNode;
				VuoCompilerPort *toPort = static_cast<VuoCompilerPort *>( publishedOutputNode->getBase()->getInputPorts().at(toPortIndex)->getCompiler() );
				VuoCompilerCable *cable = new VuoCompilerCable(fromNode, fromPort, toNode, toPort);
				cable->setAlwaysEventOnly(publishedOutputCable->getCompiler()->getAlwaysEventOnly());
				VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(fromPort);

				Vertex vertex = (fromTrigger ? Vertex(fromTrigger, toNode) : Vertex(fromNode, toNode));
				set<Vertex>::iterator vertexIter = allVertices.find(vertex);
				if (vertexIter != allVertices.end())
				{
					vertex.cableBundle = (*vertexIter).cableBundle;
					allVertices.erase(vertexIter);
				}
				vertex.cableBundle.insert(cable);
				allVertices.insert(vertex);
			}
		}
	}


	// Create cable-less vertices to visit between each leaf node in the composition and the published output node.
	// These are used to ensure that events from the published input trigger gather at the published output node.

	set<Vertex> publishedOutputVertices;
	if (publishedOutputNode)
	{
		map<VuoCompilerNode *, bool> nodeHasIncomingVertices;
		map<VuoCompilerNode *, bool> nodeHasOutgoingVertices;
		map<VuoCompilerTriggerPort *, bool> triggerHasOutgoingVertices;
		for (set<Vertex>::iterator i = allVertices.begin(); i != allVertices.end(); ++i)
		{
			Vertex vertex = *i;

			nodeHasIncomingVertices[vertex.toNode] = true;
			if (vertex.fromTrigger)
				triggerHasOutgoingVertices[vertex.fromTrigger] = true;
			else
				nodeHasOutgoingVertices[vertex.fromNode] = true;
		}

		if (! triggerHasOutgoingVertices[publishedInputTrigger])
		{
			Vertex gatherVertex(publishedInputTrigger, publishedOutputNode);
			publishedOutputVertices.insert(gatherVertex);
		}

		for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
		{
			if (! (*i)->hasCompiler())
				continue;
			VuoCompilerNode *node = (*i)->getCompiler();

			if (nodeHasIncomingVertices[node] && ! nodeHasOutgoingVertices[node])
			{
				Vertex gatherVertex(node, publishedOutputNode);
				publishedOutputVertices.insert(gatherVertex);
			}
		}
	}

	allVertices.insert(publishedOutputVertices.begin(), publishedOutputVertices.end());


	// For each trigger, add all vertices reachable from it.

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		set<Vertex> verticesToVisit;
		set<Edge> edgesVisited;

		for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
			if ((*j).fromTrigger == trigger)
				verticesToVisit.insert(*j);

		map<VuoCompilerNode *, set<Vertex> > outgoingVerticesForNode;  // Cached data to speed up search for outgoing vertices.
		for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
			outgoingVerticesForNode[(*j).fromNode].insert(*j);

		while (! verticesToVisit.empty())
		{
			Vertex vertex = *verticesToVisit.begin();
			if (find(vertices[trigger].begin(), vertices[trigger].end(), vertex) == vertices[trigger].end())
				vertices[trigger].push_back(vertex);
			verticesToVisit.erase(verticesToVisit.begin());

			set<Vertex> potentialOutgoingVertices = outgoingVerticesForNode[vertex.toNode];
			for (set<Vertex>::iterator j = potentialOutgoingVertices.begin(); j != potentialOutgoingVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;

				if (mayTransmit(vertex.cableBundle, outgoingVertex.cableBundle) ||
						(trigger == publishedInputTrigger && publishedOutputVertices.find(outgoingVertex) != publishedOutputVertices.end()))
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


	// For the published input trigger, add all vertices that are reached by every event from it.

	if (publishedInputNode)
	{
		set<Vertex> verticesToVisit;
		set<Edge> edgesVisited;

		for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.fromTrigger == publishedInputTrigger && ! vertex.cableBundle.empty())
				verticesToVisit.insert(vertex);
		}

		while (! verticesToVisit.empty())
		{
			Vertex vertex = *verticesToVisit.begin();
			if (find(verticesNonBlocking.begin(), verticesNonBlocking.end(), vertex) == verticesNonBlocking.end())
				verticesNonBlocking.push_back(vertex);
			verticesToVisit.erase(verticesToVisit.begin());

			for (set<Vertex>::iterator j = allVertices.begin(); j != allVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;

				if (vertex.toNode == outgoingVertex.fromNode && mustTransmit(vertex.cableBundle, outgoingVertex.cableBundle))
				{
					Edge outgoingEdge(vertex, outgoingVertex);
					if (edgesVisited.find(outgoingEdge) == edgesVisited.end())  // Avoid infinite feedback loops.
					{
						edgesVisited.insert(outgoingEdge);

						verticesToVisit.insert(outgoingVertex);
					}
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

		map<Vertex, set<Vertex> > outgoingVerticesFromVertex;  // Cached data to speed up search for outgoing vertices.
		for (set<Edge>::iterator j = edges[trigger].begin(); j != edges[trigger].end(); ++j)
			outgoingVerticesFromVertex[(*j).fromVertex].insert((*j).toVertex);

		while (! verticesToVisit.empty())
		{
			// Visit the vertex at the top of the stack.
			Vertex currentVertex = verticesToVisit.back();

			set<Vertex> currentDownstreamVertices;
			bool areDownstreamVerticesComplete = true;

			// Consider each vertex immediately downstream of this vertex.
			set<Vertex> outgoingVertices = outgoingVerticesFromVertex[currentVertex];
			for (set<Vertex>::iterator j = outgoingVertices.begin(); j != outgoingVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;
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

	// For the published input trigger, add a cable-less vertex from the repeated node in each feedback loop to the
	// published output node. This ensures that events from the published input trigger gather at the published output node.
	if (publishedOutputNode)
	{
		vector<Vertex> verticesCopy = vertices[publishedInputTrigger];
		for (vector<Vertex>::iterator i = verticesCopy.begin(); i != verticesCopy.end(); ++i)
		{
			Vertex vertex = *i;

			if (vertex.toNode != publishedOutputNode && downstreamVertices[publishedInputTrigger][vertex].empty())
			{
				Vertex gatherVertex(vertex.toNode, publishedOutputNode);
				vertices[publishedInputTrigger].push_back(gatherVertex);

				for (map<Vertex, set<Vertex> >::iterator j = downstreamVertices[publishedInputTrigger].begin(); j != downstreamVertices[publishedInputTrigger].end(); ++j)
				{
					Vertex potentialUpstreamVertex = j->first;
					set<Vertex> downstreamVerticesForVertex = j->second;

					if (find(downstreamVerticesForVertex.begin(), downstreamVerticesForVertex.end(), vertex) != downstreamVerticesForVertex.end())
						downstreamVertices[publishedInputTrigger][potentialUpstreamVertex].insert(gatherVertex);
				}
			}
		}
	}
}

/**
 * Puts VuoCompilerGraph::vertices in topological order.
 */
void VuoCompilerGraph::sortVertices(void)
{
	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		vector<Vertex> verticesToSort = i->second;

		map<Vertex, set<Vertex> > dependentVertices;
		list<Vertex> verticesToVisit;  // Used as a stack, except for a call to find().
		map<Vertex, bool> verticesCompleted;

		for (vector<Vertex>::iterator j = verticesToSort.begin(); j != verticesToSort.end(); ++j)
			if ((*j).fromTrigger == trigger)
				verticesToVisit.push_back(*j);

		map<VuoCompilerNode *, set<Vertex> > outgoingVerticesForNode;  // Cached data to speed up topological sort.
		for (vector<Vertex>::iterator j = verticesToSort.begin(); j != verticesToSort.end(); ++j)
			outgoingVerticesForNode[(*j).fromNode].insert(*j);

		while (! verticesToVisit.empty())
		{
			// Visit the vertex at the top of the stack.
			Vertex currentVertex = verticesToVisit.back();

			set<Vertex> currentDependentVertices;
			bool areDependentVerticesComplete = true;

			// Form a list of vertices immediately dependent on this vertex's to-node.
			// This includes vertices that are not downstream of this vertex because of a wall.
			// But, if this vertex is at the end of a feedback loop, it doesn't include vertices
			// beyond the end of the feedback loop.
			set<Vertex> outgoingVertices;
			set<Vertex> potentialOutgoingVertices = outgoingVerticesForNode[currentVertex.toNode];
			for (set<Vertex>::iterator j = potentialOutgoingVertices.begin(); j != potentialOutgoingVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;
				if (downstreamVertices[trigger][outgoingVertex].find(currentVertex) == downstreamVertices[trigger][outgoingVertex].end())
					outgoingVertices.insert(outgoingVertex);
				else
				{
					outgoingVertices.clear();
					break;
				}
			}

			for (set<Vertex>::iterator j = outgoingVertices.begin(); j != outgoingVertices.end(); ++j)
			{
				Vertex outgoingVertex = *j;
				currentDependentVertices.insert(outgoingVertex);

				if (verticesCompleted[outgoingVertex])
				{
					// The dependent vertex has already been visited, so add its dependent vertices to this vertex's.
					currentDependentVertices.insert( dependentVertices[outgoingVertex].begin(), dependentVertices[outgoingVertex].end() );
				}
				else if (find(verticesToVisit.begin(), verticesToVisit.end(), outgoingVertex) == verticesToVisit.end())
				{
					// The dependent vertex has not yet been visited, so add it to the stack.
					verticesToVisit.push_back(outgoingVertex);
					areDependentVerticesComplete = false;
				}
			}

			if (areDependentVerticesComplete)
			{
				dependentVertices[currentVertex] = currentDependentVertices;
				verticesToVisit.pop_back();
				verticesCompleted[currentVertex] = true;
			}
		}

		// Put the vertices in descending order of the number of vertices that can't be reached until after the vertex.
		vector< pair<size_t, Vertex> > verticesAndDependents;
		for (map<Vertex, set<Vertex> >::iterator j = dependentVertices.begin(); j != dependentVertices.end(); ++j)
			verticesAndDependents.push_back( make_pair(j->second.size(), j->first) );
		sort(verticesAndDependents.begin(), verticesAndDependents.end());

		vector<Vertex> sortedVertices;
		for (vector< pair<size_t, Vertex> >::reverse_iterator j = verticesAndDependents.rbegin(); j != verticesAndDependents.rend(); ++j)
			sortedVertices.push_back((*j).second);

		vertices[trigger] = sortedVertices;
	}
}

/**
 * Sets up VuoCompilerGraph::vertexDistanceFromTrigger and VuoCompilerGraph::vertexMustTransmitFromTrigger.
 */
void VuoCompilerGraph::makeVertexDistances(void)
{
	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		vector<Vertex> verticesDownstream = i->second;

		// Handle vertices immediately downstream of the trigger.
		for (vector<Vertex>::iterator i = verticesDownstream.begin(); i != verticesDownstream.end(); ++i)
		{
			Vertex vertex = *i;
			if (vertex.fromTrigger == trigger)
			{
				vertexDistanceFromTrigger[trigger][vertex] = 1;
				triggerMustTransmitToVertex[trigger][vertex] = true;
			}
		}

		// Handle vertices further downstream.
		for (vector<Vertex>::iterator i = verticesDownstream.begin(); i != verticesDownstream.end(); ++i)
		{
			Vertex vertex = *i;
			if (vertex.fromTrigger != trigger)
			{
				size_t minDistance = verticesDownstream.size();
				bool anyMustTransmit = false;
				for (vector<Vertex>::iterator j = verticesDownstream.begin(); j != i; ++j)
				{
					Vertex upstreamVertex = *j;
					if (upstreamVertex.toNode == vertex.fromNode)
					{
						minDistance = min(vertexDistanceFromTrigger[trigger][upstreamVertex], minDistance);
						bool currMustTransmit = triggerMustTransmitToVertex[trigger][upstreamVertex] && mustTransmit(upstreamVertex, trigger);
						anyMustTransmit = currMustTransmit || anyMustTransmit;
					}
				}

				vertexDistanceFromTrigger[trigger][vertex] = minDistance + 1;
				triggerMustTransmitToVertex[trigger][vertex] = anyMustTransmit;
			}
		}
	}
}

/**
 * Sets up VuoCompilerGraph::eventlesslyDownstreamNodes.
 */
void VuoCompilerGraph::makeEventlesslyDownstreamNodes(set<VuoNode *> nodes, set<VuoCable *> cables)
{
	// Find all nodes that can transmit through their output data cables without an event.

	set<VuoCompilerNode *> eventlesslyTransmittingNodes;
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
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
		for (set<VuoCable *>::iterator j = cables.begin(); j != cables.end(); ++j)
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
 * Returns true if an event coming into a node through @a fromCables must always transmit to outgoing @a toCables, based
 * on the event-blocking behavior of the ports in the node.
 *
 * Assumes that the @c toNode of all @a fromCables and the @a fromNode of all @a toCables are the same.
 */
bool VuoCompilerGraph::mustTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables)
{
	bool fromCablesMustTransmit = false;
	for (set<VuoCompilerCable *>::const_iterator i = fromCables.begin(); i != fromCables.end(); ++i)
	{
		VuoPort *inputPort = (*i)->getBase()->getToPort();
		if (inputPort->getClass()->getEventBlocking() == VuoPortClass::EventBlocking_None)
		{
			fromCablesMustTransmit = true;
			break;
		}
	}

	bool toCablesMayTransmit = false;
	for (set<VuoCompilerCable *>::const_iterator i = toCables.begin(); i != toCables.end(); ++i)
	{
		VuoPort *outputPort = (*i)->getBase()->getFromPort();
		if (outputPort->getClass()->getPortType() != VuoPortClass::triggerPort)
		{
			toCablesMayTransmit = true;
			break;
		}
	}

	return fromCablesMustTransmit && toCablesMayTransmit;
}

/**
 * Returns true if an event through the vertex must always transmit to outgoing cables from @c vertex.toNode .
 */
bool VuoCompilerGraph::mustTransmit(Vertex vertex, VuoCompilerTriggerPort *trigger)
{
	set<VuoCompilerCable *> cablesBetweenFromNodeAndToNode = vertex.cableBundle;

	set<VuoCompilerCable *> cablesOutOfToNode;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
	{
		Vertex otherVertex = *i;
		if (vertex.toNode == otherVertex.fromNode)
			cablesOutOfToNode.insert(otherVertex.cableBundle.begin(), otherVertex.cableBundle.end());
	}

	return mustTransmit(cablesBetweenFromNodeAndToNode, cablesOutOfToNode);
}


/**
 * Returns true if an event coming into a node through @a fromCables may transmit to outgoing @a toCables, based
 * on the event-blocking behavior of the ports in the node.
 *
 * Assumes that the @c toNode of all @a fromCables and the @a fromNode of all @a toCables are the same.
 */
bool VuoCompilerGraph::mayTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables)
{
	bool fromCablesMayTransmit = false;
	for (set<VuoCompilerCable *>::const_iterator i = fromCables.begin(); i != fromCables.end(); ++i)
	{
		VuoPort *inputPort = (*i)->getBase()->getToPort();
		if (inputPort->getClass()->getEventBlocking() != VuoPortClass::EventBlocking_Wall)
		{
			fromCablesMayTransmit = true;
			break;
		}
	}

	bool toCablesMayTransmit = false;
	for (set<VuoCompilerCable *>::const_iterator i = toCables.begin(); i != toCables.end(); ++i)
	{
		VuoPort *outputPort = (*i)->getBase()->getFromPort();
		if (outputPort->getClass()->getPortType() != VuoPortClass::triggerPort)
		{
			toCablesMayTransmit = true;
			break;
		}
	}

	return fromCablesMayTransmit && toCablesMayTransmit;
}

/**
 * Returns true if an event through the vertex may transmit to outgoing cables from @c vertex.toNode .
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
	if (vertexMayTransmit[trigger].empty() && ! vertices[trigger].empty())
		for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
			vertexMayTransmit[trigger][(*i).fromNode][(*i).toNode] = true;

	return vertexMayTransmit[trigger][fromNode][toNode];
}

/**
 * Returns true if @a node belongs to a node class that can transmit data through output cables without an event.
 */
bool VuoCompilerGraph::mayTransmitEventlessly(VuoCompilerNode *node)
{
	return node->getBase()->getNodeClass()->isDrawerNodeClass();
}

/**
 * Returns all trigger ports, including those for published input ports.
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
	map<VuoCompilerTriggerPort *, map<VuoCompilerNode *, size_t> >::iterator triggerIter = numVerticesWithToNode.find(trigger);
	if (triggerIter != numVerticesWithToNode.end())
	{
		map<VuoCompilerNode *, size_t>::iterator nodeIter = triggerIter->second.find(toNode);
		if (nodeIter != triggerIter->second.end())
			return nodeIter->second;
	}

	size_t numVertices = 0;
	for (vector<Vertex>::iterator i = vertices[trigger].begin(); i != vertices[trigger].end(); ++i)
		if ((*i).toNode == toNode)
			++numVertices;

	numVerticesWithToNode[trigger][toNode] = numVertices;
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
	if (! chains.empty())
		return chains;

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;

		// Visit the vertices in topological order, and put the nodes into lists (chains-in-progress).
		vector< vector<VuoCompilerNode *> > chainsInProgress;
		set<VuoCompilerNode *> nodesAdded;
		bool skippedPublishedOutputNode = false;
		for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
		{
			Vertex vertex = *j;
			bool addedToChain = false;

			if (nodesAdded.find(vertex.toNode) != nodesAdded.end())
				continue;  // Avoid creating duplicate chains for nodes with gathers (multiple incoming vertices).

			if (vertex.toNode == publishedOutputNode)
			{
				skippedPublishedOutputNode = true;
				continue;  // Save the published output node for last.
			}

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

		// Create a new chain for the published output node.
		if (skippedPublishedOutputNode)
		{
			VuoCompilerChain *chain = new VuoCompilerChain(vector<VuoCompilerNode *>(1, publishedOutputNode), false);
			chains[trigger].push_back(chain);
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
	map<VuoCompilerTriggerPort *, vector<VuoCompilerNode *> >::iterator triggerIter = downstreamNodesForTrigger.find(trigger);
	if (triggerIter != downstreamNodesForTrigger.end())
		return triggerIter->second;

	set<VuoCompilerNode *> downstreamNodes;
	vector<VuoCompilerNode *> immediatelyDownstreamNodes = getNodesImmediatelyDownstream(trigger);
	downstreamNodes.insert(immediatelyDownstreamNodes.begin(), immediatelyDownstreamNodes.end());

	for (vector<VuoCompilerNode *>::iterator i = immediatelyDownstreamNodes.begin(); i != immediatelyDownstreamNodes.end(); ++i)
	{
		vector<VuoCompilerNode *> furtherDownstreamNodes = getNodesDownstream(*i, trigger);
		downstreamNodes.insert(furtherDownstreamNodes.begin(), furtherDownstreamNodes.end());
	}

	vector<VuoCompilerNode *> downstreamNodesVector(downstreamNodes.begin(), downstreamNodes.end());
	downstreamNodesForTrigger[trigger] = downstreamNodesVector;
	return downstreamNodesVector;
}

/**
 * Returns the nodes that can be reached by an event from @a trigger that has passed through @a node.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	map<VuoCompilerTriggerPort *, map<VuoCompilerNode *, vector<VuoCompilerNode *> > >::iterator triggerIter = downstreamNodesForNode.find(trigger);
	if (triggerIter != downstreamNodesForNode.end())
	{
		map<VuoCompilerNode *, vector<VuoCompilerNode *> >::iterator nodeIter = triggerIter->second.find(node);
		if (nodeIter != triggerIter->second.end())
			return nodeIter->second;
	}

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

	vector<VuoCompilerNode *> downstreamNodesVector(downstreamNodes.begin(), downstreamNodes.end());
	downstreamNodesForNode[trigger][node] = downstreamNodesVector;
	return downstreamNodesVector;
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
 * composition) for published ports.
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

	vector<VuoCable *> downstreamCablesList = outputPort->getBase()->getConnectedCables();
	for (vector<VuoCable *>::iterator i = downstreamCablesList.begin(); i != downstreamCablesList.end(); ++i)
		if (! (*i)->isPublished())
			downstreamCables.insert( (*i)->getCompiler() );

	return downstreamCables;
}

/**
 * Calculates the minimum and maximum number of threads that may be simultaneously waiting on dispatch semaphores,
 * queues, or groups (i.e., counted toward the Dispatch Thread Soft Limit) while executing the chain of nodes.
 */
void VuoCompilerGraph::getWorkerThreadsNeeded(VuoCompilerChain *chain, int &minThreadsNeeded, int &maxThreadsNeeded)
{
	// Non-subcomposition nodes need 1 thread.
	minThreadsNeeded = 1;
	maxThreadsNeeded = 1;

	vector<VuoCompilerNode *> nodes = chain->getNodes();
	for (vector<VuoCompilerNode *>::iterator j = nodes.begin(); j != nodes.end(); ++j)
	{
		VuoCompilerNodeClass *nodeClass = (*j)->getBase()->getNodeClass()->getCompiler();
		vector<VuoCompilerTriggerDescription *> triggerDescriptions = nodeClass->getTriggerDescriptions();

		for (vector<VuoCompilerTriggerDescription *>::iterator k = triggerDescriptions.begin(); k != triggerDescriptions.end(); ++k)
		{
			if ((*k)->getNodeIdentifier() == VuoNodeClass::publishedInputNodeIdentifier)
			{
				// Subcomposition nodes need 1 thread plus those needed within the subcomposition.
				int minThreadsNeededForSubcomposition, maxThreadsNeededForSubcomposition;
				(*k)->getWorkerThreadsNeeded(minThreadsNeededForSubcomposition, maxThreadsNeededForSubcomposition);
				minThreadsNeeded = max(minThreadsNeeded, minThreadsNeededForSubcomposition + 1);
				maxThreadsNeeded = max(maxThreadsNeeded, maxThreadsNeededForSubcomposition + 1);
				break;
			}
		}
	}
}

/**
 * Calculates the minimum and maximum number of threads that may be simultaneously waiting on dispatch semaphores,
 * queues, or groups (i.e., counted toward the Dispatch Thread Soft Limit) while an event fired from this trigger
 * is propagating through the composition.
 */
void VuoCompilerGraph::getWorkerThreadsNeeded(VuoCompilerTriggerPort *trigger, int &minThreadsNeeded, int &maxThreadsNeeded)
{
	// Make a data structure of the number of threads needed for each chain.
	vector<VuoCompilerChain *> chainsForTrigger = getChains()[trigger];
	map<VuoCompilerChain *, pair<int, int> > threadsNeededForChain;
	for (vector<VuoCompilerChain *>::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
	{
		VuoCompilerChain *chain = *i;

		int minThreadsNeededForChain, maxThreadsNeededForChain;
		getWorkerThreadsNeeded(chain, minThreadsNeededForChain, maxThreadsNeededForChain);

		threadsNeededForChain[chain] = make_pair(minThreadsNeededForChain, maxThreadsNeededForChain);
	}

	// Make a data structure of which chains are downstream of which.
	map<VuoCompilerChain *, set<VuoCompilerChain *> > chainsDownstream;
	for (vector<VuoCompilerChain *>::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		VuoCompilerNode *lastNodeInThisChain = chain->getNodes().back();

		vector<VuoCompilerNode *> nodesDownstream = getNodesDownstream(lastNodeInThisChain, trigger);

		for (vector<VuoCompilerChain *>::iterator j = i+1; j != chainsForTrigger.end(); ++j)
		{
			VuoCompilerChain *otherChain = *j;
			VuoCompilerNode *firstNodeInOtherChain = otherChain->getNodes().front();

			if (find(nodesDownstream.begin(), nodesDownstream.end(), firstNodeInOtherChain) != nodesDownstream.end())
				chainsDownstream[chain].insert(otherChain);
		}
	}

	// Make a data structure of which chains are immediately downstream and upstream of which.
	map<VuoCompilerChain *, set<VuoCompilerChain *> > chainsImmediatelyDownstream;
	map<VuoCompilerChain *, set<VuoCompilerChain *> > chainsImmediatelyUpstream;
	for (vector<VuoCompilerChain *>::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
	{
		VuoCompilerChain *chain = *i;
		VuoCompilerNode *lastNodeInThisChain = chain->getNodes().back();

		vector<VuoCompilerNode *> nodesDownstream = getNodesImmediatelyDownstream(lastNodeInThisChain, trigger);

		for (vector<VuoCompilerChain *>::iterator j = i+1; j != chainsForTrigger.end(); ++j)
		{
			VuoCompilerChain *otherChain = *j;
			VuoCompilerNode *firstNodeInOtherChain = otherChain->getNodes().front();

			if (find(nodesDownstream.begin(), nodesDownstream.end(), firstNodeInOtherChain) != nodesDownstream.end())
			{
				chainsImmediatelyDownstream[chain].insert(otherChain);
				chainsImmediatelyUpstream[otherChain].insert(chain);
			}
		}
	}

	// Every trigger worker needs at least 1 thread, since it waits on the trigger node's semaphore.
	minThreadsNeeded = 1;
	maxThreadsNeeded = 1;


	// Find the maximum number of threads required — an approximation. Ideally, this is the thread count if
	// all chains that can possibly execute concurrently do so. This is approximated by adding up the thread
	// count for all scatters, scatters of scatters, etc., without taking into account any gathers. It may
	// be an overestimate of the ideal thread count.

	// Collect the chains immediately downstream of the trigger.
	vector< pair<VuoCompilerChain *, vector<VuoCompilerChain *> > > potentialScatters;
	vector<VuoCompilerChain *> scatterForTrigger;
	for (vector<VuoCompilerChain *>::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
		if (chainsImmediatelyUpstream.find(*i) == chainsImmediatelyUpstream.end())
			scatterForTrigger.push_back(*i);
	potentialScatters.push_back( make_pair((VuoCompilerChain *)NULL, scatterForTrigger) );

	// Add in the chains immediately downstream of each chain.
	for (vector<VuoCompilerChain *>::iterator i = chainsForTrigger.begin(); i != chainsForTrigger.end(); ++i)
	{
		vector<VuoCompilerChain *> scatterForChain(chainsImmediatelyDownstream[*i].begin(), chainsImmediatelyDownstream[*i].end());
		potentialScatters.push_back( make_pair(*i, scatterForChain) );
	}

	map<VuoCompilerChain *, int> threadsNeededForScatters;
	for (vector< pair<VuoCompilerChain *, vector<VuoCompilerChain *> > >::reverse_iterator i = potentialScatters.rbegin(); i != potentialScatters.rend(); ++i)
	{
		VuoCompilerChain *chain = (*i).first;
		vector<VuoCompilerChain *> scatterChains = (*i).second;

		// Discard any chains that are downstream of another in the collection,
		// leaving a group of chains that may all execute concurrently.
		for (size_t j = 0; j < scatterChains.size(); ++j)
		{
			VuoCompilerChain *outer = scatterChains[j];

			for (size_t k = j+1; k < scatterChains.size(); ++k)
			{
				VuoCompilerChain *inner = scatterChains[k];

				if (chainsDownstream[inner].find(outer) != chainsDownstream[inner].end())
				{
					scatterChains.erase( scatterChains.begin() + j-- );
					break;
				}

				if (chainsDownstream[outer].find(inner) != chainsDownstream[outer].end())
					scatterChains.erase( scatterChains.begin() + k-- );
			}
		}

		// Add up the threads needed for the chains in the scatter.
		int threadsNeededForScatter = 0;
		for (vector<VuoCompilerChain *>::iterator j = scatterChains.begin(); j != scatterChains.end(); ++j)
			threadsNeededForScatter += threadsNeededForScatters[*j];

		// Factor in the threads needed for this chain itself.
		threadsNeededForScatter = max(threadsNeededForScatter, threadsNeededForChain[chain].second);

		threadsNeededForScatters[chain] = threadsNeededForScatter;
	}

	// Find the maximum threads needed for any chain and its downstream scatters.
	for (map<VuoCompilerChain *, int>::iterator i = threadsNeededForScatters.begin(); i != threadsNeededForScatters.end(); ++i)
		maxThreadsNeeded = max(maxThreadsNeeded, i->second);


	// Find the minimum number of threads required. This is the thread count if all chains execute sequentially,
	// which is the maximum of the minimum threads needed across all chains in the composition. If the trigger
	// has no downstream chains, the trigger worker still needs 1 thread to wait on the trigger node's semaphore.

	for (map<VuoCompilerChain *, pair<int, int> >::iterator i = threadsNeededForChain.begin(); i != threadsNeededForChain.end(); ++i)
		minThreadsNeeded = max(minThreadsNeeded, i->second.first);
}

/**
 * Returns the trigger port that is nearest upstream to @a node, or null if no trigger is upstream.
 *
 * The "nearest" trigger is based on two criteria: How many cables are between the trigger and the node,
 * and are there any intervening doors? If there are any triggers that must always transmit to the node
 * (no intervening doors), then the one with the fewest intervening cables is chosen. Otherwise, the trigger
 * (with intervening doors) with the fewest intervening cables is chosen.
 *
 * If there's a tie between two nearest triggers, the one returned is consistent across calls of this function.
 */
VuoCompilerTriggerPort * VuoCompilerGraph::findNearestUpstreamTrigger(VuoCompilerNode *node)
{
	vector< pair<VuoCompilerTriggerPort *, size_t> > distancesForMusts;
	vector< pair<VuoCompilerTriggerPort *, size_t> > distancesForMays;

	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = i->first;
		vector<Vertex> verticesDownstream = i->second;

		for (vector<Vertex>::iterator j = verticesDownstream.begin(); j != verticesDownstream.end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.toNode == node)
			{
				pair<VuoCompilerTriggerPort *, size_t> p = make_pair(trigger, vertexDistanceFromTrigger[trigger][vertex]);
				if (triggerMustTransmitToVertex[trigger][vertex])
					distancesForMusts.push_back(p);
				else
					distancesForMays.push_back(p);
			}
		}
	}

	if (! distancesForMusts.empty())
	{
		sort(distancesForMusts.begin(), distancesForMusts.end(), compareTriggers);
		return distancesForMusts[0].first;
	}
	else if (! distancesForMays.empty())
	{
		sort(distancesForMays.begin(), distancesForMays.end(), compareTriggers);
		return distancesForMays[0].first;
	}
	else
		return NULL;
}

/**
 * Returns true if the index for @a lhs is before the index for @a rhs, with ties broken by lexicographic order of the
 * unique identifiers for @a lhs and @a rhs.
 */
bool VuoCompilerGraph::compareTriggers(const pair<VuoCompilerTriggerPort *, size_t> &lhs, const pair<VuoCompilerTriggerPort *, size_t> &rhs)
{
	if (lhs.second == rhs.second)
		return lhs.first->getIdentifier() < rhs.first->getIdentifier();

	return lhs.second < rhs.second;
}

/**
 * Returns the type of event blocking for the published input ports, based on whether an event from the
 * published input trigger always (none), never (wall), or sometimes (door) reaches the published output ports.
 * If there are no event-only or data-and-event published output ports to reach, returns "none".
 *
 * For certain compositions that contain nodes with door input ports, the analysis is imprecise.
 * For example, if both the False Option and True Option outputs of a `Select Output` node have cables
 * to published output ports, then all events into the node will reach the published output ports.
 * But this function can't reason about the implementation of the node. It can only reason about the
 * fact that the node's input ports are marked as doors. So it returns "door" instead of "none".
 */
VuoPortClass::EventBlocking VuoCompilerGraph::getPublishedInputEventBlocking(void)
{
	size_t publishedOutputCount = publishedOutputNode->getBase()->getInputPorts().size() - VuoNodeClass::unreservedInputPortStartIndex;
	size_t publishedOutputTriggerCount = getPublishedOutputTriggers().size();
	if (publishedOutputCount - publishedOutputTriggerCount == 0)
		return VuoPortClass::EventBlocking_None;

	bool mayReachOutput = false;
	for (vector<Vertex>::iterator i = vertices[publishedInputTrigger].begin(); i != vertices[publishedInputTrigger].end(); ++i)
	{
		Vertex vertex = *i;
		if (vertex.toNode == publishedOutputNode && ! vertex.cableBundle.empty())
		{
			mayReachOutput = true;
			break;
		}
	}

	bool mustReachOutput = false;
	for (vector<Vertex>::iterator i = verticesNonBlocking.begin(); i != verticesNonBlocking.end(); ++i)
	{
		Vertex vertex = *i;
		if (vertex.toNode == publishedOutputNode)
		{
			mustReachOutput = true;
			break;
		}
	}

	if (mustReachOutput)
		return VuoPortClass::EventBlocking_None;
	else if (mayReachOutput)
		return VuoPortClass::EventBlocking_Door;
	else
		return VuoPortClass::EventBlocking_Wall;
}

/**
 * Returns the name of each published output port that may be reached by an event from a trigger other than the
 * published input trigger.
 */
set<string> VuoCompilerGraph::getPublishedOutputTriggers(void)
{
	set<string> publishedOutputPortNames;

	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *trigger = *i;
		if (trigger == publishedInputTrigger)
			continue;

		for (vector<Vertex>::iterator j = vertices[trigger].begin(); j != vertices[trigger].end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.toNode == publishedOutputNode)
			{
				for (set<VuoCompilerCable *>::iterator j = vertex.cableBundle.begin(); j != vertex.cableBundle.end(); ++j)
				{
					VuoCompilerCable *cable = *j;
					string toPortName = cable->getBase()->getToPort()->getClass()->getName();
					publishedOutputPortNames.insert(toPortName);
				}
			}
		}
	}

	return publishedOutputPortNames;
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
bool VuoCompilerGraph::hasGatherDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	vector<VuoCompilerNode *> downstreamNodes = getNodesDownstream(node, trigger);
	for (vector<VuoCompilerNode *>::iterator i = downstreamNodes.begin(); i != downstreamNodes.end(); ++i)
		if (getNumVerticesWithToNode(*i, trigger) > 1)
			return true;

	return false;
}

/**
 * Returns true if there's a scatter originating at @a trigger, and some but not all nodes
 * downstream of @a trigger are also downstream of another trigger.
 */
bool VuoCompilerGraph::hasScatterPartiallyOverlappedByAnotherTrigger(VuoCompilerTriggerPort *trigger)
{
	bool isScatterAtTrigger = getNodesImmediatelyDownstream(trigger).size() > 1;
	if (isScatterAtTrigger)
	{
		vector<VuoCompilerNode *> downstreamNodes = getNodesDownstream(trigger);
		return areNodesPartiallyOverlappedByAnotherTrigger(downstreamNodes, trigger);
	}

	return false;
}

/**
 * Returns true if there's a scatter originating at @a node for events from @a trigger, and some but not all nodes
 * downstream of @a node are also downstream of another trigger.
 */
bool VuoCompilerGraph::hasScatterPartiallyOverlappedByAnotherTrigger(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger)
{
	bool isScatterAtNode = getNodesImmediatelyDownstream(node, trigger).size() > 1;
	if (isScatterAtNode)
	{
		vector<VuoCompilerNode *> downstreamNodes = getNodesDownstream(node, trigger);
		return areNodesPartiallyOverlappedByAnotherTrigger(downstreamNodes, trigger);
	}

	return false;
}

/**
 * Returns true if there is a trigger that is upstream of some but not all of @a nodes.
 */
bool VuoCompilerGraph::areNodesPartiallyOverlappedByAnotherTrigger(const vector<VuoCompilerNode *> &nodes, VuoCompilerTriggerPort *trigger)
{
	for (vector<VuoCompilerTriggerPort *>::iterator i = triggers.begin(); i != triggers.end(); ++i)
	{
		VuoCompilerTriggerPort *otherTrigger = *i;
		if (otherTrigger == trigger)
			continue;

		vector<VuoCompilerNode *> nodesDownstreamOfOtherTrigger = getNodesDownstream(otherTrigger);

		bool hasOverlappedNode = false;
		bool hasNonOverlappedNode = false;
		for (vector<VuoCompilerNode *>::const_iterator j = nodes.begin(); j != nodes.end(); ++j)
		{
			VuoCompilerNode *downstreamNode = *j;

			if (find(nodesDownstreamOfOtherTrigger.begin(), nodesDownstreamOfOtherTrigger.end(), downstreamNode) != nodesDownstreamOfOtherTrigger.end())
				hasOverlappedNode = true;
			else
				hasNonOverlappedNode = true;
		}

		if (hasOverlappedNode && hasNonOverlappedNode)
			return true;
	}

	return false;
}

/**
 * Returns true if this trigger port causes a `Spin Off` node to fire, and this trigger and the `Spin Off` trigger
 * have downstream nodes in common.
 */
bool VuoCompilerGraph::hasOverlapWithSpinOff(VuoCompilerTriggerPort *trigger)
{
	vector<VuoCompilerNode *> nodesDownstreamOfTrigger = getNodesDownstream(trigger);
	vector<VuoCompilerNode *> nodesDownstreamOfSpinOffs;

	// Collect all nodes downstream of Spin Off nodes downstream of this trigger
	// (including Spin Offs downstream of other Spin Offs downstream of this trigger).
	vector<VuoCompilerNode *> nodesToCheck = nodesDownstreamOfTrigger;
	map<VuoCompilerNode *, bool> nodesChecked;
	while (! nodesToCheck.empty())
	{
		VuoCompilerNode *node = nodesToCheck.back();
		nodesToCheck.pop_back();

		if (nodesChecked[node])
			continue;
		nodesChecked[node] = true;

		string nodeClassName = node->getBase()->getNodeClass()->getClassName();
		if (nodeClassName == "vuo.event.spinOffEvent" || nodeClassName == "vuo.event.spinOffEvents" ||
				VuoStringUtilities::beginsWith(nodeClassName, "vuo.event.spinOffValue."))
		{
			VuoPort *spinOffOutput = node->getBase()->getOutputPorts().at(VuoNodeClass::unreservedOutputPortStartIndex);
			VuoCompilerTriggerPort *spinOffTrigger = static_cast<VuoCompilerTriggerPort *>( spinOffOutput->getCompiler() );
			vector<VuoCompilerNode *> nodesDownstream = getNodesDownstream(spinOffTrigger);

			nodesDownstreamOfSpinOffs.insert(nodesDownstreamOfSpinOffs.end(), nodesDownstream.begin(), nodesDownstream.end());
			nodesToCheck.insert(nodesToCheck.end(), nodesDownstream.begin(), nodesDownstream.end());
		}
	}

	sort(nodesDownstreamOfTrigger.begin(), nodesDownstreamOfTrigger.end());
	sort(nodesDownstreamOfSpinOffs.begin(), nodesDownstreamOfSpinOffs.end());

	// Do this trigger and the Spin Offs have any downstream nodes in common?
	set<VuoCompilerNode *> nodesDownstreamOfBoth;
	set_intersection(nodesDownstreamOfTrigger.begin(), nodesDownstreamOfTrigger.end(),
					 nodesDownstreamOfSpinOffs.begin(), nodesDownstreamOfSpinOffs.end(),
					 std::inserter(nodesDownstreamOfBoth, nodesDownstreamOfBoth.begin()));
	return ! nodesDownstreamOfBoth.empty();
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
 * Generates a unique, consistent hash value for the elements of composition structure analyzed by this class.
 */
long VuoCompilerGraph::getHash(VuoCompilerComposition *composition)
{
	ostringstream s;

	// nodes — pointers, identifiers
	s << "[";
	set<VuoNode *> nodes = composition->getBase()->getNodes();
	for (set<VuoNode *>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		VuoCompilerNode *compilerNode = NULL;
		string identifier;
		if ((*i)->hasCompiler())
		{
			compilerNode = (*i)->getCompiler();
			identifier = compilerNode->getIdentifier();
		}
		s << "{" << compilerNode << "," << identifier << "},";
	}
	s << "]";

	// cables — pointers, node identifiers, port identifiers
	s << "[";
	set<VuoCable *> cables = composition->getBase()->getCables();
	for (set<VuoCable *>::iterator i = cables.begin(); i != cables.end(); ++i)
	{
		/// @todo Check for change in to-port's event blocking (https://b33p.net/kosada/node/8197)
		string fromNode = ((*i)->getFromNode() && (*i)->getFromNode()->hasCompiler()) ? (*i)->getFromNode()->getCompiler()->getIdentifier() : "";
		string fromPort = (*i)->getFromPort() ? (*i)->getFromPort()->getClass()->getName() : "";
		string toNode = ((*i)->getToNode() && (*i)->getToNode()->hasCompiler()) ? (*i)->getToNode()->getCompiler()->getIdentifier() : "";
		string toPort = (*i)->getToPort() ? (*i)->getToPort()->getClass()->getName() : "";
		s << "{" << (*i)->getCompiler() << "," << fromNode << "," << fromPort << "," << toNode << "," << toPort << "},";
	}
	s << "]";

	// published ports — pointers, identifiers
	s << "[";
	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	for (vector<VuoPublishedPort *>::iterator i = publishedInputPorts.begin(); i != publishedInputPorts.end(); ++i)
		s << "{" << (*i)->getCompiler() << "," << (*i)->getClass()->getName() << "},";
	s << "]";
	s << "[";
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();
	for (vector<VuoPublishedPort *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
		s << "{" << (*i)->getCompiler() << "," << (*i)->getClass()->getName() << "},";
	s << "]";

	return VuoStringUtilities::hash(s.str());
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
