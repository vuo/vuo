/**
 * @file
 * VuoCompilerGraph implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCable.hh"
#include "VuoCompiler.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerCable.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerInputEventPortClass.hh"
#include "VuoCompilerIssue.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerPublishedInputNodeClass.hh"
#include "VuoCompilerPublishedOutputNodeClass.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoCompilerTriggerPortClass.hh"
#include "VuoComposition.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"
#include <sstream>

/**
 * Creates a graph representation of (the current state of) the composition, to be used for checking the validity
 * of the composition.
 *
 * @param composition The composition to represent. The graph representation is a snapshot of the composition passed
 *		into this constructor, and does not update if the composition is modified.
 * @param compiler A compiler that may be used to generate published input and output nodes.
 * @param potentialCables Cables that are not yet in @a composition but should be added to the graph representation.
 *      If they would displace existing cables, the potentially displaced cables are omitted from the graph representation.
 */
VuoCompilerGraph::VuoCompilerGraph(VuoCompilerComposition *composition, VuoCompiler *compiler, set<VuoCompilerCable *> potentialCables)
{
	VuoCompilerNode *publishedInputNode = nullptr;
	VuoCompilerNode *publishedOutputNode = nullptr;
	VuoCompilerNode *publishedInputTriggerNode = nullptr;
	bool ownsPublishedNodeClasses = false;

	VuoCompilerNodeClass *triggerNodeClass = nullptr;
	if (compiler)
	{
		triggerNodeClass = compiler->getNodeClass("vuo.event.spinOffEvent2");
	}
	else
	{
		VuoPortClass *refreshPortClass = (new VuoCompilerInputEventPortClass("refresh"))->getBase();
		VuoCompilerTriggerPortClass *triggerPortClass = new VuoCompilerTriggerPortClass("spunOff", nullptr);
		vector<VuoPortClass *> inputPortClasses;
		vector<VuoPortClass *> outputPortClasses(1, triggerPortClass->getBase());
		VuoNodeClass *baseNodeClass = new VuoNodeClass("vuo.event.spinOffEvent2", refreshPortClass, inputPortClasses, outputPortClasses);
		triggerNodeClass = VuoCompilerNodeClass::newNodeClassWithoutImplementation(baseNodeClass)->getCompiler();
	}

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();

	if (! (publishedInputPorts.empty() && publishedOutputPorts.empty()) )
	{
		if (compiler)
		{
			publishedInputNode = compiler->createPublishedInputNode(publishedInputPorts)->getCompiler();
			publishedOutputNode = compiler->createPublishedOutputNode(publishedOutputPorts)->getCompiler();

			ownsPublishedNodeClasses = false;
		}
		else
		{
			VuoNodeClass *publishedInputNodeClass = VuoCompilerPublishedInputNodeClass::newNodeClass(publishedInputPorts);
			VuoNodeClass *publishedOutputNodeClass = VuoCompilerPublishedOutputNodeClass::newNodeClass(publishedOutputPorts);
			publishedInputNode = publishedInputNodeClass->getCompiler()->newNode()->getCompiler();
			publishedOutputNode = publishedOutputNodeClass->getCompiler()->newNode()->getCompiler();

			ownsPublishedNodeClasses = true;
		}

		publishedInputTriggerNode = triggerNodeClass->newNode()->getCompiler();
		publishedInputTriggerNode->setGraphvizIdentifier(getPublishedInputTriggerNodeIdentifier());
	}

	VuoCompilerNode *manuallyFirableTriggerNode = triggerNodeClass->newNode()->getCompiler();
	manuallyFirableTriggerNode->setGraphvizIdentifier(getManuallyFirableTriggerNodeIdentifier());

	initializeInstance(composition, potentialCables, publishedInputNode, publishedOutputNode, publishedInputTriggerNode, ownsPublishedNodeClasses,
					   manuallyFirableTriggerNode);
}

/**
 * Creates an internal representation of the composition that consists of:
 *
 *    - all valid nodes[1] in the composition
 *    - @a publishedInputNode and @a publishedOutputNode
 *    - a `Spin Off Event` node with a cable to @a publishedInputNode
 *    - a `Spin Off Event` node with a cable to @a manuallyFirableInputPort
 *    - all valid internal cables in the composition
 *    - all internal cables in @a potentialCables
 *    - for each valid published input cable in the composition or @a potentialCables,
 *         a cable from the corresponding @a publishedInputNode output port to the published cable's input port
 *    - for each valid published output cable in the composition or @a potentialCables,
 *         a cable from the published cable's output port to the corresponding @a publishedOutputNode input port
 *    - a cable from the `Spin Off Event` node to an input port of @a publishedInputNode
 *    - for each trigger,
 *         a cable from each leaf vertex's to-node[3] or from the trigger itself if it is a leaf[4]
 *         to @a publishedOutputNode's gather input port
 *
 * [1] Valid node — has a known node class.
 * [2] Valid cable — is connected at both ends to a valid node and not displaced by any of @a potentialCables.
 * [3] Leaf vertex's to-node — a node downstream of the trigger in question that either has no outgoing cables
 *         that may transmit an event from the trigger or is the repeated node in a feedback loop.
 * [4] Leaf trigger — a trigger with no outgoing cables, excluding the triggers on the added `Spin Off Event` nodes.
 */
void VuoCompilerGraph::initializeInstance(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables,
										  VuoCompilerNode *publishedInputNode, VuoCompilerNode *publishedOutputNode,
										  VuoCompilerNode *publishedInputTriggerNode, bool ownsPublishedNodeClasses,
										  VuoCompilerNode *manuallyFirableTriggerNode)
{
	this->publishedInputNode = publishedInputNode;
	this->publishedOutputNode = publishedOutputNode;
	this->ownsPublishedNodeClasses = ownsPublishedNodeClasses;
	this->publishedInputTrigger = nullptr;

	// Nodes in the composition

	set<VuoNode *> nodesIncludingInvalid = composition->getBase()->getNodes();
	set<VuoNode *> nodesToAnalyze;
	for (VuoNode *node : nodesIncludingInvalid)
		if (node->hasCompiler())  // Ignore nodes with missing node classes.
			nodesToAnalyze.insert(node);

	if (publishedInputNode && publishedOutputNode)
	{
		// Published input/output nodes

		nodesToAnalyze.insert(publishedInputNode->getBase());
		nodesToAnalyze.insert(publishedOutputNode->getBase());

		// `Spin Off Event` node for published inputs

		nodesToAnalyze.insert(publishedInputTriggerNode->getBase());

		VuoPort *publishedInputTriggerPort = publishedInputTriggerNode->getBase()->getOutputPorts().at(VuoNodeClass::unreservedOutputPortStartIndex);
		this->publishedInputTrigger = static_cast<VuoCompilerTriggerPort *>(publishedInputTriggerPort->getCompiler());
	}

	// `Spin Off Event` node for manually firable trigger

	nodesToAnalyze.insert(manuallyFirableTriggerNode->getBase());

	VuoPort *manuallyFirableTriggerPort = manuallyFirableTriggerNode->getBase()->getOutputPorts().at(VuoNodeClass::unreservedOutputPortStartIndex);
	this->manuallyFirableTrigger = static_cast<VuoCompilerTriggerPort *>(manuallyFirableTriggerPort->getCompiler());

	// Internal cables in the composition

	set<VuoPort *> potentialCableInputs;
	for (VuoCompilerCable *cable : potentialCables)
		if (cable->carriesData())
			potentialCableInputs.insert(cable->getBase()->getToPort());

	set<VuoCable *> cablesIncludingInvalidAndPublished = composition->getBase()->getCables();
	set<VuoCable *> cablesToAnalyze;
	for (VuoCable *cable : cablesIncludingInvalidAndPublished)
	{
		if ((cable->getFromPort() && cable->getToPort())  // Ignore disconnected cables.
				&& (cable->getFromPort()->hasCompiler() && cable->getToPort()->hasCompiler())  // Ignore cables on nodes with missing node classes.
				&& ! (cable->hasCompiler() && cable->getCompiler()->carriesData() &&
					  potentialCableInputs.find(cable->getToPort()) != potentialCableInputs.end()) )  // Ignore displaced cables.
		{
			if (cable->isPublished())
				publishedCables.insert(cable);
			else
				cablesToAnalyze.insert(cable);
		}
	}

	// Potential internal cables

	for (VuoCompilerCable *cable : potentialCables)
	{
		if (cable->getBase()->isPublished())
			publishedCables.insert(cable->getBase());
		else
			cablesToAnalyze.insert(cable->getBase());
	}

	// Cables connected to published input/output nodes

	vector<VuoPublishedPort *> publishedInputPorts = composition->getBase()->getPublishedInputPorts();
	vector<VuoPublishedPort *> publishedOutputPorts = composition->getBase()->getPublishedOutputPorts();
	for (VuoCable *cable : publishedCables)
	{
		VuoNode *fromNode = nullptr;
		VuoNode *toNode = nullptr;
		VuoPort *fromPort = nullptr;
		VuoPort *toPort = nullptr;

		if (cable->isPublishedInputCable())
		{
			size_t publishedPortIndex = std::distance(publishedInputPorts.begin(), std::find(publishedInputPorts.begin(), publishedInputPorts.end(), cable->getFromPort()));

			fromNode = publishedInputNode->getBase();
			fromPort = getOutputPortOnPublishedInputNode(publishedPortIndex);
			toNode = cable->getToNode();
			toPort = cable->getToPort();
		}
		else
		{
			size_t publishedPortIndex = std::distance(publishedOutputPorts.begin(), std::find(publishedOutputPorts.begin(), publishedOutputPorts.end(), cable->getToPort()));

			fromNode = cable->getFromNode();
			fromPort = cable->getFromPort();
			toNode = publishedOutputNode->getBase();
			toPort = getInputPortOnPublishedOutputNode(publishedPortIndex);
		}

		VuoCompilerCable *replacement = new VuoCompilerCable(fromNode->getCompiler(), static_cast<VuoCompilerPort *>(fromPort->getCompiler()),
															 toNode->getCompiler(), static_cast<VuoCompilerPort *>(toPort->getCompiler()), false);
		replacement->setAlwaysEventOnly(cable->getCompiler()->getAlwaysEventOnly());
		cablesToAnalyze.insert(replacement->getBase());
	}

	// Cable from `Spin Off Event` to published input node

	if (publishedInputTrigger)
	{
		VuoPort *toPort = publishedInputNode->getBase()->getInputPorts().at(0);
		VuoCompilerCable *spinOffCable = new VuoCompilerCable(publishedInputTriggerNode, publishedInputTrigger,
															  publishedInputNode, static_cast<VuoCompilerPort *>(toPort->getCompiler()), false);
		cablesToAnalyze.insert(spinOffCable->getBase());
	}

	// Cable from `Spin Off Event` to manually firable input port

	{
		VuoNode *toNode = composition->getManuallyFirableInputNode();
		VuoPort *toPort = composition->getManuallyFirableInputPort();
		if (toNode && toPort)
		{
			VuoCompilerCable *spinOffCable = new VuoCompilerCable(manuallyFirableTriggerNode, manuallyFirableTrigger,
																  toNode->getCompiler(), static_cast<VuoCompilerPort *>(toPort->getCompiler()), false);
			cablesToAnalyze.insert(spinOffCable->getBase());
		}
	}

	for (VuoNode *node : nodesToAnalyze)
		nodes.insert(node->getCompiler());

	makeTriggers(nodesToAnalyze);
	makeVerticesAndEdges(cablesToAnalyze);
	makeDownstreamVertices();
	sortVertices();
	makeVertexDistances();
	makeDownstreamNodesViaDataOnlyTransmission(nodesToAnalyze, cablesToAnalyze);
}

/**
 * Destructor.
 */
VuoCompilerGraph::~VuoCompilerGraph(void)
{
	VuoCompilerNode *publishedInputTriggerNode = nodeForTrigger[publishedInputTrigger];

	if (ownsPublishedNodeClasses)
	{
		delete publishedInputNode->getBase()->getNodeClass()->getCompiler();
		delete publishedOutputNode->getBase()->getNodeClass()->getCompiler();
		delete publishedInputTriggerNode->getBase()->getNodeClass()->getCompiler();
	}

	delete publishedInputNode;
	delete publishedOutputNode;
	delete publishedInputTriggerNode;
}

/**
 * Sets up VuoCompilerGraph::triggers and VuoCompilerGraph::nodeForTrigger.
 */
void VuoCompilerGraph::makeTriggers(set<VuoNode *> nodes)
{
	for (VuoNode *node : nodes)
	{
		for (VuoPort *port : node->getOutputPorts())
		{
			VuoCompilerTriggerPort *trigger = dynamic_cast<VuoCompilerTriggerPort *>(port->getCompiler());
			if (trigger)
			{
				nodeForTrigger[trigger] = node->getCompiler();
				triggers.push_back(trigger);
			}
		}
	}
}

/**
 * Sets up VuoCompilerGraph::vertices (not yet in topological order) and VuoCompilerGraph::edges.
 */
void VuoCompilerGraph::makeVerticesAndEdges(const set<VuoCable *> &cables)
{
	// Create vertices to visit for all cables.

	set<Vertex> allVertices;
	for (VuoCable *cable : cables)
	{
		VuoCompilerPort *fromPort = cable->getFromPort() ? static_cast<VuoCompilerPort *>(cable->getFromPort()->getCompiler()) : nullptr;
		VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(fromPort);
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

	for (VuoCompilerTriggerPort *trigger : triggers)
	{
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

				if (mayTransmit(vertex.cableBundle, outgoingVertex.cableBundle))
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
}

/**
 * Fills in @a downstreamVertices with the vertices that may be reached by an event from @a trigger,
 * with the option to exclude some vertices from analysis using @a includeEdge.
 */
void VuoCompilerGraph::makeDownstreamVerticesWithInclusionRule(VuoCompilerTriggerPort *trigger, std::function<bool(Edge)> includeEdge,
															   map<Vertex, set<Vertex> > &_downstreamVertices,
															   set<Vertex> &_repeatedVertices)
{
	list<Vertex> verticesToVisit;  // Used as a stack, except for a call to find().

	for (Vertex vertex : vertices[trigger])
		if (vertex.fromTrigger == trigger)
			verticesToVisit.push_back(vertex);

	map<Vertex, set<Vertex> > outgoingVerticesFromVertex;  // Cached data to speed up search for outgoing vertices.
	for (Edge edge : edges[trigger])
		if (includeEdge(edge))
			outgoingVerticesFromVertex[edge.fromVertex].insert(edge.toVertex);

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

			if (_downstreamVertices.find(outgoingVertex) != _downstreamVertices.end())
			{
				// The downstream vertex has already been visited, so add its downstream vertices to this vertex's.
				set<Vertex> furtherDownstreamVertices = _downstreamVertices[outgoingVertex];
				currentDownstreamVertices.insert( furtherDownstreamVertices.begin(), furtherDownstreamVertices.end() );
			}
			else
			{
				if (find(verticesToVisit.begin(), verticesToVisit.end(), outgoingVertex) != verticesToVisit.end())
				{
					// The downstream vertex is already on the stack, so it's an infinite feedback loop.
					_repeatedVertices.insert(outgoingVertex);
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
			_downstreamVertices[currentVertex] = currentDownstreamVertices;
			verticesToVisit.pop_back();
		}
	}

	// Clean up repeatedVertices so that it only contains vertices within an infinite feedback loop,
	// not other outgoing vertices from nodes in the infinite feedback loop.
	set<Vertex> repeatedVerticesCopy = _repeatedVertices;
	for (Vertex vertex : repeatedVerticesCopy)
		if (_downstreamVertices[vertex].find(vertex) == _downstreamVertices[vertex].end())
			_repeatedVertices.erase(vertex);
}

/**
 * Sets up VuoCompilerGraph::downstreamVertices and VuoCompilerGraph::repeatedVertices.
 */
void VuoCompilerGraph::makeDownstreamVertices(void)
{
	auto includeAllEdges = [] (Edge edge) { return true; };

	for (VuoCompilerTriggerPort *trigger : triggers)
	{
		makeDownstreamVerticesWithInclusionRule(trigger, includeAllEdges, downstreamVertices[trigger], repeatedVertices[trigger]);

		if (repeatedVertices[trigger].empty())
			repeatedVertices.erase(trigger);
	}

	// For each trigger, add a cable and vertex from each leaf trigger/vertex to the published output node's gather port.
	//    - For the published input trigger and triggers that spin off events from it, this ensures that the
	// composition doesn't notify its runner (for top-level compositions) or parent composition (for subcompositions)
	// until the event has finished propagating through the composition.
	//    - For internal triggers that reach a published output, this ensures that published trigger doesn't fire
	// until the event has finished propagating through the composition.

	if (publishedOutputNode)
	{
		for (VuoCompilerTriggerPort *trigger : triggers)
		{
			set< pair<VuoCompilerTriggerPort *, Vertex> > leaves;
			if (trigger != publishedInputTrigger && trigger != manuallyFirableTrigger && downstreamVertices[trigger].empty())
				leaves.insert({ trigger, Vertex() });
			else
				for (auto i : downstreamVertices[trigger])
					if (i.first.toNode != publishedOutputNode && i.second.empty())
						leaves.insert({ nullptr, i.first });

			for (auto &leaf : leaves)
			{
				VuoCompilerNode *fromNode = (leaf.first ? nodeForTrigger[leaf.first] : leaf.second.toNode);
				VuoCompilerTriggerPort *fromPort = (leaf.first ? leaf.first : nullptr);
				VuoPort *toPort = getGatherPortOnPublishedOutputNode();

				VuoCompilerCable *gather = new VuoCompilerCable(fromNode, fromPort,
																publishedOutputNode, static_cast<VuoCompilerPort *>(toPort->getCompiler()), false);

				Vertex gatherVertex = (leaf.first ? Vertex(fromPort, publishedOutputNode) : Vertex(fromNode, publishedOutputNode));
				gatherVertex.cableBundle.insert(gather);
				vertices[trigger].push_back(gatherVertex);

				if (! leaf.first)
				{
					Vertex leafVertex = leaf.second;
					Edge gatherEdge(leafVertex, gatherVertex);
					edges[trigger].insert(gatherEdge);
					downstreamVertices[trigger][leafVertex].insert(gatherVertex);
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

		map<VuoCompilerNode *, set<Vertex> > incomingVerticesForNode;  // Cached data to speed up search for incoming vertices.
		for (vector<Vertex>::iterator j = verticesDownstream.begin(); j != verticesDownstream.end(); ++j)
			incomingVerticesForNode[(*j).toNode].insert(*j);

		// Handle vertices immediately downstream of the trigger.
		for (vector<Vertex>::iterator j = verticesDownstream.begin(); j != verticesDownstream.end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.fromTrigger == trigger)
			{
				vertexDistanceFromTrigger[trigger][vertex] = 1;
				triggerMustTransmitToVertex[trigger][vertex] = true;
			}
		}

		// Handle vertices further downstream.
		for (vector<Vertex>::iterator j = verticesDownstream.begin(); j != verticesDownstream.end(); ++j)
		{
			Vertex vertex = *j;
			if (vertex.fromTrigger != trigger)
			{
				size_t minDistance = verticesDownstream.size();
				bool anyMustTransmit = false;
				for (set<Vertex>::iterator k = incomingVerticesForNode[vertex.fromNode].begin(); k != incomingVerticesForNode[vertex.fromNode].end(); ++k)
				{
					Vertex upstreamVertex = *k;

					minDistance = min(vertexDistanceFromTrigger[trigger][upstreamVertex], minDistance);
					bool currMustTransmit = triggerMustTransmitToVertex[trigger][upstreamVertex] && mustTransmit(upstreamVertex, trigger);
					anyMustTransmit = currMustTransmit || anyMustTransmit;
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
void VuoCompilerGraph::makeDownstreamNodesViaDataOnlyTransmission(set<VuoNode *> nodes, set<VuoCable *> cables)
{
	// @todo Maybe this could be refactored to call makeDownstreamVerticesWithInclusionRule.

	// Find all nodes that can transmit through their output data cables, and put them in topological order.
	// (Assumes the nodes don't have walled ports, so they can't have different topological orders for different triggers.)

	map<VuoCompilerNode *, set<VuoCompilerNode *> > remainingIncomingNodes;
	map<VuoCompilerNode *, set<VuoCompilerNode *> > remainingOutgoingNodes;
	for (VuoCable *cable : cables)
	{
		VuoCompilerPort *fromPort = cable->getFromPort() ? static_cast<VuoCompilerPort *>(cable->getFromPort()->getCompiler()) : nullptr;
		VuoCompilerTriggerPort *fromTrigger = dynamic_cast<VuoCompilerTriggerPort *>(fromPort);
		if (fromTrigger || ! cable->getCompiler()->carriesData())
			continue;  // Ignore cables that can't transmit without an event.

		VuoCompilerNode *fromNode = cable->getFromNode()->getCompiler();
		VuoCompilerNode *toNode = cable->getToNode()->getCompiler();

		if (mayTransmitDataOnly(fromNode))
		{
			remainingOutgoingNodes[fromNode].insert(toNode);
			if (mayTransmitDataOnly(toNode))
				remainingIncomingNodes[toNode].insert(fromNode);
		}
	}
	map<VuoCompilerNode *, set<VuoCompilerNode *> > outgoingNodes = remainingOutgoingNodes;

	set<VuoCompilerNode *> nodesToVisit;
	for (VuoNode *node : nodes)
	{
		if (mayTransmitDataOnly(node->getCompiler()) &&
				remainingIncomingNodes.find(node->getCompiler()) == remainingIncomingNodes.end() &&
				remainingOutgoingNodes.find(node->getCompiler()) != remainingOutgoingNodes.end())
			nodesToVisit.insert(node->getCompiler());
	}

	vector<VuoCompilerNode *> sortedNodesAllowingDataOnlyTransmission;
	while (! nodesToVisit.empty())
	{
		VuoCompilerNode *node = *nodesToVisit.begin();
		nodesToVisit.erase(nodesToVisit.begin());

		sortedNodesAllowingDataOnlyTransmission.push_back(node);

		set<VuoCompilerNode *> outgoingNodesCopy = remainingOutgoingNodes[node];
		for (VuoCompilerNode *outgoingNode : outgoingNodesCopy)
		{
			remainingIncomingNodes[outgoingNode].erase(node);
			remainingOutgoingNodes[node].erase(outgoingNode);

			if (remainingIncomingNodes[outgoingNode].empty())
				nodesToVisit.insert(outgoingNode);
		}
	}

	// For each of those nodes, find the nodes that are downstream of it via eventless transmission, in topological order.

	for (int firstIndex = sortedNodesAllowingDataOnlyTransmission.size() - 1; firstIndex >= 0; --firstIndex)
	{
		VuoCompilerNode *firstNode = sortedNodesAllowingDataOnlyTransmission[firstIndex];

		for (size_t possiblyDownstreamIndex = firstIndex + 1; possiblyDownstreamIndex < sortedNodesAllowingDataOnlyTransmission.size(); ++possiblyDownstreamIndex)
		{
			VuoCompilerNode *possiblyDownstreamNode = sortedNodesAllowingDataOnlyTransmission[possiblyDownstreamIndex];

			if (outgoingNodes[firstNode].find(possiblyDownstreamNode) != outgoingNodes[firstNode].end())
			{
				vector<VuoCompilerNode *> nodesToAdd = downstreamNodesViaDataOnlyTransmission[possiblyDownstreamNode];
				nodesToAdd.insert(nodesToAdd.begin(), possiblyDownstreamNode);

				for (VuoCompilerNode *nodeToAdd : nodesToAdd)
					if (find(downstreamNodesViaDataOnlyTransmission[firstNode].begin(), downstreamNodesViaDataOnlyTransmission[firstNode].end(), nodeToAdd) == downstreamNodesViaDataOnlyTransmission[firstNode].end())
						downstreamNodesViaDataOnlyTransmission[firstNode].push_back(nodeToAdd);
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
	for (VuoCompilerCable *cable : fromCables)
	{
		VuoPort *inputPort = cable->getBase()->getToPort();
		if (inputPort->getClass()->getEventBlocking() == VuoPortClass::EventBlocking_None)
		{
			fromCablesMustTransmit = true;
			break;
		}
	}

	bool toCablesMayTransmit = false;
	for (VuoCompilerCable *cable : toCables)
	{
		VuoPort *outputPort = cable->getBase()->getFromPort();
		if (! outputPort || outputPort->getClass()->getPortType() != VuoPortClass::triggerPort)
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
	for (VuoCompilerCable *cable : fromCables)
	{
		VuoPort *inputPort = cable->getBase()->getToPort();
		if (inputPort->getClass()->getEventBlocking() != VuoPortClass::EventBlocking_Wall)
		{
			fromCablesMayTransmit = true;
			break;
		}
	}

	bool toCablesMayTransmit = false;
	for (VuoCompilerCable *cable : toCables)
	{
		VuoPort *outputPort = cable->getBase()->getFromPort();
		if (! outputPort || outputPort->getClass()->getPortType() != VuoPortClass::triggerPort)
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
bool VuoCompilerGraph::mayTransmitDataOnly(VuoCompilerNode *node)
{
	return node->getBase()->getNodeClass()->isDrawerNodeClass() || node == publishedInputNode;
}

/**
 * Returns all trigger ports, including those for published input ports.
 */
vector<VuoCompilerTriggerPort *> VuoCompilerGraph::getTriggerPorts(void)
{
	return triggers;
}

/**
 * Returns the node that contains @a trigger.
 */
VuoCompilerNode * VuoCompilerGraph::getNodeForTriggerPort(VuoCompilerTriggerPort *trigger)
{
	auto foundIter = nodeForTrigger.find(trigger);
	if (foundIter != nodeForTrigger.end())
		return foundIter->second;

	return NULL;
}

/**
 * Returns the nodes in the graph, which may exclude some in the composition (those with an unknown node class)
 * and include some nodes that were not in the composition (e.g. published input and output nodes).
 */
set<VuoCompilerNode *> VuoCompilerGraph::getNodes(void)
{
	return nodes;
}

/**
 * Returns the published input node.
 *
 * @see VuoCompilerPublishedInputNodeClass
 */
VuoCompilerNode * VuoCompilerGraph::getPublishedInputNode(void)
{
	return publishedInputNode;
}

/**
 * Returns the published output node.
 *
 * @see VuoCompilerPublishedOutputNodeClass
 */
VuoCompilerNode * VuoCompilerGraph::getPublishedOutputNode(void)
{
	return publishedOutputNode;
}

/**
 * Returns the trigger port that fires into the published input node's input ports.
 */
VuoCompilerTriggerPort * VuoCompilerGraph::getPublishedInputTrigger(void)
{
	return publishedInputTrigger;
}

/**
 * Returns the input port on the published input node that corresponds to the published input port.
 */
VuoPort * VuoCompilerGraph::getInputPortOnPublishedInputNode(size_t publishedInputPortIndex)
{
	VuoCompilerPublishedInputNodeClass *publishedInputNodeClass = static_cast<VuoCompilerPublishedInputNodeClass *>(publishedInputNode->getBase()->getNodeClass()->getCompiler());
	size_t portIndex = publishedInputNodeClass->getInputPortIndexForPublishedInputPort(publishedInputPortIndex);
	return publishedInputNode->getBase()->getInputPorts().at(portIndex);
}

/**
 * Returns the output port on the published input node that corresponds to the published input port.
 */
VuoPort * VuoCompilerGraph::getOutputPortOnPublishedInputNode(size_t publishedInputPortIndex)
{
	VuoCompilerPublishedInputNodeClass *publishedInputNodeClass = static_cast<VuoCompilerPublishedInputNodeClass *>(publishedInputNode->getBase()->getNodeClass()->getCompiler());
	size_t portIndex = publishedInputNodeClass->getOutputPortIndexForPublishedInputPort(publishedInputPortIndex);
	return publishedInputNode->getBase()->getOutputPorts().at(portIndex);
}

/**
 * Returns the input port on the published output node that corresponds to the published output port.
 */
VuoPort * VuoCompilerGraph::getInputPortOnPublishedOutputNode(size_t publishedOutputPortIndex)
{
	VuoCompilerPublishedOutputNodeClass *publishedOutputNodeClass = static_cast<VuoCompilerPublishedOutputNodeClass *>(publishedOutputNode->getBase()->getNodeClass()->getCompiler());
	size_t portIndex = publishedOutputNodeClass->getInputPortIndexForPublishedOutputPort(publishedOutputPortIndex);
	return publishedOutputNode->getBase()->getInputPorts().at(portIndex);
}

/**
 * Returns the gather input port on the published output node.
 */
VuoPort * VuoCompilerGraph::getGatherPortOnPublishedOutputNode(void)
{
	VuoCompilerPublishedOutputNodeClass *publishedOutputNodeClass = static_cast<VuoCompilerPublishedOutputNodeClass *>(publishedOutputNode->getBase()->getNodeClass()->getCompiler());
	size_t portIndex = publishedOutputNodeClass->getGatherInputPortIndex();
	return publishedOutputNode->getBase()->getInputPorts().at(portIndex);
}

/**
 * Returns the trigger port that fires into VuoCompilerComposition's manually firable input port.
 */
VuoCompilerTriggerPort * VuoCompilerGraph::getManuallyFirableTrigger(void)
{
	return manuallyFirableTrigger;
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

	for (VuoCompilerTriggerPort *trigger : triggers)
	{
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
 * Returns the nodes that can be reached by data-only transmission from the output cables of @a node,
 * in topological order.
 */
vector<VuoCompilerNode *> VuoCompilerGraph::getNodesDownstreamViaDataOnlyTransmission(VuoCompilerNode *node)
{
	return downstreamNodesViaDataOnlyTransmission[node];
}

/**
 * Returns the outgoing cables from @a outputPort. This includes cables downstream via an event from any trigger,
 * cables downstream via data-only transmission, cables created for graph analysis, and any other cables from
 * output ports of the node to input ports on a node with a compiler detail.
 */
set<VuoCompilerCable *> VuoCompilerGraph::getOutgoingCables(VuoCompilerPort *outputPort)
{
	set<VuoCompilerCable *> outgoingCables;

	// Add cables from the original composition.

	for (VuoCable *cable : outputPort->getBase()->getConnectedCables())
		if (cable->getToNode() && cable->getToNode()->hasCompiler())
			outgoingCables.insert(cable->getCompiler());

	// Add any other cables created for graph analysis.

	for (map<VuoCompilerTriggerPort *, vector<Vertex> >::value_type kv : vertices)
		for (Vertex vertex : kv.second)
			for (VuoCompilerCable *cable : vertex.cableBundle)
				if (cable->getBase()->getFromPort() == outputPort->getBase())
					outgoingCables.insert(cable);

	return outgoingCables;
}

/**
 * Returns the nodes that can transmit data without an event and don't have any incoming data-only transmissions.
 */
set<VuoCompilerNode *> VuoCompilerGraph::getSourceNodesOfDataOnlyTransmission(void)
{
	set<VuoCompilerNode *> nodesTransmittingDataOnly;
	set<VuoCompilerNode *> nodesDownstreamOfAnother;

	for (const map<VuoCompilerNode *, vector<VuoCompilerNode *> >::value_type &i : downstreamNodesViaDataOnlyTransmission)
	{
		nodesTransmittingDataOnly.insert(i.first);
		nodesDownstreamOfAnother.insert(i.second.begin(), i.second.end());
	}

	set<VuoCompilerNode *> nodesNotDownstream;
	std::set_difference(nodesTransmittingDataOnly.begin(), nodesTransmittingDataOnly.end(),
						nodesDownstreamOfAnother.begin(), nodesDownstreamOfAnother.end(),
						std::inserter(nodesNotDownstream, nodesNotDownstream.end()));
	return nodesNotDownstream;
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

	for (VuoCompilerNode *node : chain->getNodes())
	{
		VuoCompilerNodeClass *nodeClass = node->getBase()->getNodeClass()->getCompiler();
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
 * Returns the type of event blocking for the published input port, based on whether an event through
 * that port always (none), never (wall), or sometimes (door) reaches at least one published output port.
 * If there are no event-only or data-and-event published output ports to reach, returns "none".
 *
 * For certain compositions that contain nodes with door input ports, the analysis is imprecise.
 * For example, if both the False Option and True Option outputs of a `Select Output` node have cables
 * to published output ports, then all events into the node will reach the published output ports.
 * But this function can't reason about the implementation of the node. It can only reason about the
 * fact that the node's input ports are marked as doors. So it returns "door" instead of "none".
 */
VuoPortClass::EventBlocking VuoCompilerGraph::getPublishedInputEventBlocking(size_t publishedInputPortIndex)
{
	if (! publishedInputTrigger)
		return VuoPortClass::EventBlocking_None;

	// If no event-only or data-and-event published output ports, return "none".

	VuoCompilerPublishedOutputNodeClass *publishedOutputNodeClass = static_cast<VuoCompilerPublishedOutputNodeClass *>(publishedOutputNode->getBase()->getNodeClass()->getCompiler());
	size_t publishedOutputCount = publishedOutputNodeClass->getGatherInputPortIndex() - VuoNodeClass::unreservedInputPortStartIndex;
	set<string> publishedOutputTriggers = getPublishedOutputTriggers();
	if (publishedOutputCount - publishedOutputTriggers.size() == 0)
		return VuoPortClass::EventBlocking_None;

	VuoPort *outputPort = getOutputPortOnPublishedInputNode(publishedInputPortIndex);

	set<Vertex> verticesFromPublishedInputNode;
	for (Vertex vertex : vertices[publishedInputTrigger])
		if (vertex.fromNode == publishedInputNode)
			verticesFromPublishedInputNode.insert(vertex);

	// Find the vertices downstream of the published input node with no intervening doors or walls.

	auto includeNonBlocking = [this] (Edge edge)
	{
		return (edge.fromVertex.fromTrigger == publishedInputTrigger ||
				mustTransmit(edge.fromVertex.cableBundle, edge.toVertex.cableBundle));
	};

	if (downstreamVerticesNonBlocking.empty())
	{
		set<Vertex> unused;
		makeDownstreamVerticesWithInclusionRule(publishedInputTrigger, includeNonBlocking, downstreamVerticesNonBlocking, unused);
	}

	// Find the vertices downstream of the published input node with no intervening walls.

	auto includeNonBlockingAndDoor = [this] (Edge edge)
	{
		return (edge.fromVertex.fromTrigger == publishedInputTrigger ||
				mayTransmit(edge.fromVertex.cableBundle, edge.toVertex.cableBundle));
	};

	if (downstreamVerticesNonBlockingOrDoor.empty())
	{
		set<Vertex> unused;
		makeDownstreamVerticesWithInclusionRule(publishedInputTrigger, includeNonBlockingAndDoor, downstreamVerticesNonBlockingOrDoor, unused);
	}

	auto minBlocking = [] (VuoPortClass::EventBlocking b1, VuoPortClass::EventBlocking b2)
	{
		// Returns the one that lets more events through.
		return min(b1, b2);
	};

	auto maxBlocking = [] (VuoPortClass::EventBlocking b1, VuoPortClass::EventBlocking b2)
	{
		// Returns the one that blocks more events.
		return max(b1, b2);
	};

	VuoPortClass::EventBlocking blocking = VuoPortClass::EventBlocking_Wall;
	vector<Vertex> verticesToPublishedOutputNode;
	for (Vertex vertex : verticesFromPublishedInputNode)
	{
		VuoPortClass::EventBlocking blockingForVertex = VuoPortClass::EventBlocking_Wall;

		// Check the event blocking for the exact cables connected to the output port on the published input node.
		for (VuoCompilerCable *cable : vertex.cableBundle)
		{
			if (cable->getBase()->getFromPort() == outputPort)
			{
				VuoPortClass::EventBlocking toPortBlocking = cable->getBase()->getToPort()->getClass()->getEventBlocking();
				blockingForVertex = minBlocking(blockingForVertex, toPortBlocking);
			}
		}

		if (blockingForVertex == VuoPortClass::EventBlocking_Wall)
		{
			// If the event is blocked at the cables from the published input node's output port, it's a wall.
		}
		else
		{
			auto isVertexToPublishedOutputNode = [this] (Vertex downstreamVertex) { return downstreamVertex.toNode == publishedOutputNode; };

			vector<Vertex> verticesNonBlocking;
			std::copy_if(downstreamVerticesNonBlocking[vertex].begin(), downstreamVerticesNonBlocking[vertex].end(),
						 std::back_inserter(verticesNonBlocking), isVertexToPublishedOutputNode);

			verticesToPublishedOutputNode.insert(verticesToPublishedOutputNode.end(), verticesNonBlocking.begin(), verticesNonBlocking.end());

			if (! verticesNonBlocking.empty())
			{
				// If the event can reach the published outputs without any further blocking, it's whatever blocking
				// the cables from the published input node's output port have (none or door).
			}
			else
			{
				vector<Vertex> verticesNonBlockingOrDoor;
				std::copy_if(downstreamVerticesNonBlockingOrDoor[vertex].begin(), downstreamVerticesNonBlockingOrDoor[vertex].end(),
							 std::back_inserter(verticesNonBlockingOrDoor), isVertexToPublishedOutputNode);

				verticesToPublishedOutputNode.insert(verticesToPublishedOutputNode.end(), verticesNonBlocking.begin(), verticesNonBlocking.end());

				if (! verticesNonBlockingOrDoor.empty())
				{
					// If the event can reach the published outputs via door blocking, it's a door.
					blockingForVertex = VuoPortClass::EventBlocking_Door;
				}
				else
				{
					// If the event can't reach the published outputs, it's a wall.
					blockingForVertex = VuoPortClass::EventBlocking_Wall;
				}
			}
		}

		blocking = minBlocking(blocking, blockingForVertex);
	}

	// If the event can reach some but not all of the non-trigger published outputs, it's a door.

	set<VuoPort *> publishedOutputPortsReached;
	for (Vertex vertex : verticesToPublishedOutputNode)
		for (VuoCompilerCable *cable : vertex.cableBundle)
			if (publishedOutputTriggers.find(cable->getBase()->getToPort()->getClass()->getName()) == publishedOutputTriggers.end())
				publishedOutputPortsReached.insert(cable->getBase()->getToPort());

	if (publishedOutputPortsReached.size() < publishedOutputCount - publishedOutputTriggers.size())
		blocking = maxBlocking(blocking, VuoPortClass::EventBlocking_Door);

	return blocking;
}

/**
 * Returns the name of each published output port that may be reached by an event from a trigger
 * within the composition and may not be reached by an event from the published input trigger.
 */
set<string> VuoCompilerGraph::getPublishedOutputTriggers(void)
{
	set<string> triggerNames;
	for (VuoCompilerTriggerPort *trigger : triggers)
	{
		set<string> currTriggerNames = getPublishedOutputTriggersDownstream(trigger);
		triggerNames.insert(currTriggerNames.begin(), currTriggerNames.end());
	}

	return triggerNames;
}

/**
 * Returns the name of each published output port that may be reached by an event from @a trigger
 * and may not be reached by an event from the published input trigger.
 */
set<string> VuoCompilerGraph::getPublishedOutputTriggersDownstream(VuoCompilerTriggerPort *trigger)
{
	if (trigger == publishedInputTrigger)
		return set<string>();

	set<string> outputNames = getPublishedOutputPortsDownstream(trigger);
	set<string> outputNamesForPublishedInputTrigger = getPublishedOutputPortsDownstream(publishedInputTrigger);

	set<string> triggerNames;
	std::set_difference(outputNames.begin(), outputNames.end(),
						outputNamesForPublishedInputTrigger.begin(), outputNamesForPublishedInputTrigger.end(),
						std::inserter(triggerNames, triggerNames.begin()));

	return triggerNames;
}

/**
 * Returns the name of each published output port that may be reached by an event from @a trigger.
 */
set<string> VuoCompilerGraph::getPublishedOutputPortsDownstream(VuoCompilerTriggerPort *trigger)
{
	auto iter = publishedOutputNames.find(trigger);
	if (iter != publishedOutputNames.end())
		return iter->second;

	set<string> names;
	for (Vertex vertex : vertices[trigger])
	{
		if (vertex.toNode == publishedOutputNode)
		{
			for (VuoCompilerCable *cable : vertex.cableBundle)
			{
				VuoPort *toPort = cable->getBase()->getToPort();
				if (toPort != getGatherPortOnPublishedOutputNode())
					names.insert(toPort->getClass()->getName());
			}
		}
	}

	publishedOutputNames[trigger] = names;
	return names;
}

/**
 * Returns true if at least one published output port is downstream of the published input trigger
 * or a trigger within the composition.
 */
bool VuoCompilerGraph::mayEventsReachPublishedOutputPorts(void)
{
	for (VuoCompilerTriggerPort *trigger : triggers)
		if (! getPublishedOutputPortsDownstream(trigger).empty())
			return true;

	return false;
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
		nodesDownstreamOfOtherTrigger.push_back(nodeForTrigger[otherTrigger]);

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

		if (node == nodeForTrigger[publishedInputTrigger])
			continue;

		string nodeClassName = node->getBase()->getNodeClass()->getClassName();
		if (VuoStringUtilities::beginsWith(nodeClassName, "vuo.event.spinOff"))
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
void VuoCompilerGraph::checkForInfiniteFeedback(VuoCompilerIssues *issues)
{
	if (! repeatedVertices.empty())
	{
		// Identify the nodes and cables involved in the infinite feedback loop.
		vector< set<VuoNode *> > nodesInLoops;
		vector< set<VuoCable *> > cablesInLoops;
		for (const auto &i : repeatedVertices)
		{
			VuoCompilerTriggerPort *trigger = i.first;
			for (const Vertex &repeatedVertex : i.second)
			{
				set<VuoNode *> nodesInLoop;
				set<VuoCable *> cablesInLoop;
				for (const Vertex &otherVertex : vertices[trigger])
				{
					if (downstreamVertices[trigger][repeatedVertex].find(otherVertex) != downstreamVertices[trigger][repeatedVertex].end() &&
							downstreamVertices[trigger][otherVertex].find(repeatedVertex) != downstreamVertices[trigger][otherVertex].end())
					{
						nodesInLoop.insert(otherVertex.fromNode->getBase());
						nodesInLoop.insert(otherVertex.toNode->getBase());

						for (set<VuoCompilerCable *>::iterator m = otherVertex.cableBundle.begin(); m != otherVertex.cableBundle.end(); ++m)
							cablesInLoop.insert((*m)->getBase());
					}
				}
				nodesInLoops.push_back(nodesInLoop);
				cablesInLoops.push_back(cablesInLoop);
			}
		}

		// Coalesce the lists of nodes and cables to avoid reporting the same ones multiple times.
		vector< set<VuoNode *> > coalescedNodesInLoops;
		vector< set<VuoCable *> > coalescedCablesInLoops;
		for (size_t i = 0; i < nodesInLoops.size(); ++i)
		{
			bool coalesced = false;
			for (size_t j = 0; j < coalescedNodesInLoops.size(); ++j)
			{
				set<VuoNode *> nodesInBoth;
				std::set_intersection(nodesInLoops[i].begin(), nodesInLoops[i].end(),
									  coalescedNodesInLoops[j].begin(), coalescedNodesInLoops[j].end(),
									  std::inserter(nodesInBoth, nodesInBoth.begin()));

				set<VuoCable *> cablesInBoth;
				std::set_intersection(cablesInLoops[i].begin(), cablesInLoops[i].end(),
									  coalescedCablesInLoops[j].begin(), coalescedCablesInLoops[j].end(),
									  std::inserter(cablesInBoth, cablesInBoth.begin()));

				if (nodesInBoth.size() == nodesInLoops[i].size() && cablesInBoth.size() == cablesInLoops[i].size())
				{
					// One of the coalesced items is a superset of nodesInLoops[i] and cablesInLoops[i]. Already taken care of.
					coalesced = true;
					break;
				}
				else if (nodesInBoth.size() == coalescedNodesInLoops[j].size() && cablesInBoth.size() == coalescedCablesInLoops[j].size())
				{
					// One of the coalesced items is a subset of nodesInLoops[i] and cablesInLoops[i]. Merge in the additional nodes and cables.
					coalescedNodesInLoops[j] = nodesInLoops[i];
					coalescedCablesInLoops[j] = cablesInLoops[i];
					coalesced = true;
					break;
				}
			}

			if (! coalesced)
			{
				// No existing items to coalesce with, so add a new one.
				coalescedNodesInLoops.push_back(nodesInLoops[i]);
				coalescedCablesInLoops.push_back(cablesInLoops[i]);
			}
		}

		auto exceptionIssues = new VuoCompilerIssues;
		for (size_t i = 0; i < coalescedNodesInLoops.size(); ++i)
		{
			VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling composition", "",
								   "Infinite feedback loop",
								   "An event is not allowed to travel through the same cable more than once.");
			issue.setHelpPath("errors-warnings-and-other-issues.html");
			issue.setNodes(coalescedNodesInLoops[i]);
			issue.setCables(coalescedCablesInLoops[i]);
			exceptionIssues->append(issue);
			if (issues)
				issues->append(issue);
		}

		throw VuoCompilerException(exceptionIssues, true);
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
void VuoCompilerGraph::checkForDeadlockedFeedback(VuoCompilerIssues *issues)
{
	auto exceptionIssues = new VuoCompilerIssues;
	for (VuoCompilerTriggerPort *trigger : triggers)
	{
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

			VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling composition", "",
								   "Deadlocked feedback loop",
								   "There's more than one possible path for events to travel through this composition. "
								   "It's unclear which is the correct one.");
			issue.setHelpPath("errors-warnings-and-other-issues.html");
			issue.setNodes(nodesInLoop);
			issue.setCables(cablesInLoop);
			exceptionIssues->append(issue);
			if (issues)
				issues->append(issue);
		}
	}

	if (!exceptionIssues->isEmpty())
		throw VuoCompilerException(exceptionIssues, true);
}

/**
 * Generates a unique, consistent hash value for the elements of composition structure analyzed by this class.
 */
long VuoCompilerGraph::getHash(VuoCompilerComposition *composition)
{
	ostringstream s;

	// nodes — pointers, identifiers
	s << "[";
	for (VuoNode *node : composition->getBase()->getNodes())
	{
		VuoCompilerNode *compilerNode = NULL;
		string identifier;
		if (node->hasCompiler())
		{
			compilerNode = node->getCompiler();
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

	// manually firable input port — pointer, identifier
	s << "[";
	VuoPort *manuallyFirableInputPort = composition->getManuallyFirableInputPort();
	if (manuallyFirableInputPort)
		s << "{" << manuallyFirableInputPort->getCompiler() << "," << manuallyFirableInputPort->getClass()->getName() << "},";
	s << "]";

	return VuoStringUtilities::hash(s.str());
}

/**
 * Returns the node identifier assigned to the node containing the published input trigger.
 */
string VuoCompilerGraph::getPublishedInputTriggerNodeIdentifier(void)
{
	return VuoNodeClass::publishedInputNodeIdentifier + "Trigger";
}

/**
 * Returns the node identifier assigned to the node containing the trigger for manually firing events.
 */
string VuoCompilerGraph::getManuallyFirableTriggerNodeIdentifier(void)
{
	return "ManuallyFirableTrigger";
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
string VuoCompilerGraph::Vertex::toString(void) const
{
	return (fromNode ?
				fromNode->getBase()->getTitle() :
				fromTrigger->getBase()->getClass()->getName()) +
			"->" + toNode->getBase()->getTitle();
}

/**
 * For debugging.
 */
string VuoCompilerGraph::Edge::toString(void) const
{
	return fromVertex.toString() + " -> " + toVertex.toString();
}
