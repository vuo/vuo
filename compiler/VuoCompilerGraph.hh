/**
 * @file
 * VuoCompilerGraph interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoCompilerCable;
class VuoCompilerChain;
class VuoCompilerComposition;
class VuoCompilerInputEventPort;
class VuoCompilerIssues;
class VuoCompilerNode;
class VuoCompilerNodeClass;
class VuoCompilerPort;
class VuoCompilerTriggerPort;
class VuoCable;
class VuoNode;
class VuoPort;
class VuoPublishedPort;

#include "VuoPortClass.hh"

/**
 * Data structure used for performing graph analysis on a composition in order to compile it or check its validity.
 *
 * Like a composition, VuoCompilerGraph is a graph structure consisting of vertices and edges. However, the vertices
 * in VuoCompilerGraph represent edges (sets of cables) in the composition. Although counterintuitive, this turns
 * out to make the graph analysis smoother, since the rules about feedback and event blocking are more simply
 * expressed in terms of VuoCompilerGraph vertices and edges.
 */
class VuoCompilerGraph
{
public:
	VuoCompilerGraph(VuoCompilerComposition *composition, VuoCompiler *compiler = nullptr, set<VuoCompilerCable *> potentialCables = set<VuoCompilerCable *>());
	~VuoCompilerGraph(void);
	bool mayTransmit(VuoCompilerNode *fromNode, VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger);
	bool mayTransmitDataOnly(VuoCompilerNode *node);
	vector<VuoCompilerTriggerPort *> getTriggerPorts(void);
	VuoCompilerNode * getNodeForTriggerPort(VuoCompilerTriggerPort *trigger);
	set<VuoCompilerNode *> getNodes(void);
	VuoCompilerNode * getPublishedInputNode(void);
	VuoCompilerNode * getPublishedOutputNode(void);
	VuoCompilerTriggerPort * getPublishedInputTrigger(void);
	VuoPort * getInputPortOnPublishedInputNode(size_t publishedInputPortIndex);
	VuoPort * getInputPortOnPublishedOutputNode(size_t publishedOutputPortIndex);
	VuoCompilerTriggerPort * getManuallyFirableTrigger(void);
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > getChains(void);
	vector<VuoCompilerNode *> getNodesImmediatelyDownstream(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesImmediatelyDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesDownstream(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesDownstreamViaDataOnlyTransmission(VuoCompilerNode *node);
	set<VuoCompilerCable *> getOutgoingCables(VuoCompilerPort *outputPort);
	set<VuoCompilerNode *> getSourceNodesOfDataOnlyTransmission(void);
	void getWorkerThreadsNeeded(VuoCompilerChain *chain, int &minThreadsNeeded, int &maxThreadsNeeded);
	void getWorkerThreadsNeeded(VuoCompilerTriggerPort *trigger, int &minThreadsNeeded, int &maxThreadsNeeded);
	VuoCompilerTriggerPort * findNearestUpstreamTrigger(VuoCompilerNode *node);
	VuoPortClass::EventBlocking getPublishedInputEventBlocking(size_t publishedInputPortIndex);
	set<string> getPublishedOutputTriggers(void);
	set<string> getPublishedOutputTriggersDownstream(VuoCompilerTriggerPort *trigger);
	set<string> getPublishedOutputPortsDownstream(VuoCompilerTriggerPort *trigger);
	bool mayEventsReachPublishedOutputPorts(void);
	bool isRepeatedInFeedbackLoop(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool hasGatherDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool hasScatterPartiallyOverlappedByAnotherTrigger(VuoCompilerTriggerPort *trigger);
	bool hasScatterPartiallyOverlappedByAnotherTrigger(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool hasOverlapWithSpinOff(VuoCompilerTriggerPort *trigger);
	void checkForInfiniteFeedback(VuoCompilerIssues *issues);
	void checkForDeadlockedFeedback(VuoCompilerIssues *issues);
	static long getHash(VuoCompilerComposition *composition);
	static string getPublishedInputTriggerNodeIdentifier(void);
	static string getManuallyFirableTriggerNodeIdentifier(void);

private:
	/**
	 * A vertex in the graph, representing a set of cables between two nodes or between a trigger and a node
	 * in the composition.
	 */
	class Vertex
	{
	public:
		VuoCompilerTriggerPort *fromTrigger;
		VuoCompilerNode *fromNode;
		VuoCompilerNode *toNode;
		set<VuoCompilerCable *> cableBundle;
		Vertex(VuoCompilerTriggerPort *fromTrigger, VuoCompilerNode *toNode);
		Vertex(VuoCompilerNode *fromNode, VuoCompilerNode *toNode);
		Vertex(void);
		string toString(void) const;
	};

	/**
	 * An edge in the graph, representing a set of cables (Vertex) coming into a node and a set of cables (Vertex)
	 * going out of a node in the composition.
	 */
	class Edge
	{
	public:
		Vertex fromVertex;
		Vertex toVertex;
		Edge(const Vertex &fromVertex, const Vertex &toVertex);
		Edge(void);
		string toString(void) const;
	};

	friend bool operator==(const Vertex &lhs, const Vertex &rhs);
	friend bool operator!=(const Vertex &lhs, const Vertex &rhs);
	friend bool operator<(const Vertex &lhs, const Vertex &rhs);
	friend bool operator<(const Edge &lhs, const Edge &rhs);

	/// The vertices reachable from each trigger, including those for all published input and output ports.
	/// The vertices are listed in topological order.
	map<VuoCompilerTriggerPort *, vector<Vertex> > vertices;

	/// The edges reachable from each trigger, including those for all published input ports,
	/// but excluding those for published output port.
	map<VuoCompilerTriggerPort *, set<Edge> > edges;

	/// For each trigger and vertex, the vertices that are downstream of it (i.e., an event can flow from one to the other).
	map<VuoCompilerTriggerPort *, map<Vertex, set<Vertex> > > downstreamVertices;

	/// The vertices encountered more than once while traversing the graph. Each is within an infinite feedback loop.
	map<VuoCompilerTriggerPort *, set<Vertex> > repeatedVertices;

	/// All trigger ports, including those for published input ports.
	vector<VuoCompilerTriggerPort *> triggers;

	/// For each trigger port, the node that contains it.
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger;

	/// The trigger port that fires into the published input node.
	VuoCompilerTriggerPort *publishedInputTrigger;

	/// The trigger port that fires into VuoCompilerComposition's manually firable input port.
	VuoCompilerTriggerPort *manuallyFirableTrigger;

	/// The nodes in the graph representation (may differ from the set of nodes in the composition).
	set<VuoCompilerNode *> nodes;

	/// The published input node.
	VuoCompilerNode *publishedInputNode;

	/// The published output node.
	VuoCompilerNode *publishedOutputNode;

	/// True if this class created the node classes for @ref publishedInputNode and @ref publishedOutputNode and therefore is
	/// responsible for destroying them.
	bool ownsPublishedNodeClasses;

	/// The published input and output cables in the composition. This class's internal representation of the composition replaces
	/// these cables with ones connected to the published input and output nodes.
	set<VuoCable *> publishedCables;

	/// The minimum number of vertices that must be traversed to get from the trigger to the vertex, counting the vertex itself.
	map<VuoCompilerTriggerPort *, map<Vertex, size_t> > vertexDistanceFromTrigger;

	/// True if an event from the trigger must always reach the vertex's to-node.
	map<VuoCompilerTriggerPort *, map<Vertex, bool> > triggerMustTransmitToVertex;

	/// For each node that can transmit data through their output cables without an event, the nodes they can transmit to
	/// in that way, in topological order. The downstream nodes may include a mix of nodes that may and may not themselves
	/// transmit data without an event.
	map<VuoCompilerNode *, vector<VuoCompilerNode *> > downstreamNodesViaDataOnlyTransmission;

	/// Cached results of getChains().
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > chains;

	/// Cached results of mayTransmit(VuoCompilerNode *, VuoCompilerNode *, VuoCompilerTriggerPort *).
	map<VuoCompilerTriggerPort *, map<VuoCompilerNode *, map<VuoCompilerNode *, bool> > > vertexMayTransmit;

	/// Cached results of getNumVerticesWithToNode().
	map<VuoCompilerTriggerPort *, map<VuoCompilerNode *, size_t> > numVerticesWithToNode;

	/// Cached results of getNodesDownstream(VuoCompilerTriggerPort *).
	map<VuoCompilerTriggerPort *, vector<VuoCompilerNode *> > downstreamNodesForTrigger;

	/// Cached results of getNodesDownstream(VuoCompilerNode *, VuoCompilerTriggerPort *).
	map<VuoCompilerTriggerPort *, map<VuoCompilerNode *, vector<VuoCompilerNode *> > > downstreamNodesForNode;

	/// Cached intermediate data of getPublishedInputEventBlocking().
	map<Vertex, set<Vertex> > downstreamVerticesNonBlocking;

	/// Cached intermediate data of getPublishedInputEventBlocking().
	map<Vertex, set<Vertex> > downstreamVerticesNonBlockingOrDoor;

	/// Cached results of getPublishedOutputPortsDownstream().
	map<VuoCompilerTriggerPort *, set<string> > publishedOutputNames;

	void initializeInstance(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables, VuoCompilerNode *publishedInputNode, VuoCompilerNode *publishedOutputNode, VuoCompilerNode *publishedInputTriggerNode, bool ownsPublishedNodeClasses, VuoCompilerNode *manuallyFirableTriggerNode);
	void makeTriggers(set<VuoNode *> nodes);
	void makeVerticesAndEdges(const set<VuoCable *> &cables);
	void makeDownstreamVerticesWithInclusionRule(VuoCompilerTriggerPort *trigger, std::function<bool(Edge)> includeEdge, map<Vertex, set<Vertex> > &_downstreamVertices, set<Vertex> &_repeatedVertices);
	void makeDownstreamVertices(void);
	void sortVertices(void);
	void makeVertexDistances(void);
	static bool compareTriggers(const pair<VuoCompilerTriggerPort *, size_t> &lhs, const pair<VuoCompilerTriggerPort *, size_t> &rhs);
	void makeDownstreamNodesViaDataOnlyTransmission(set<VuoNode *> nodes, set<VuoCable *> cables);
	static bool mustTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables);
	static bool mayTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables);
	bool mustTransmit(Vertex vertex, VuoCompilerTriggerPort *trigger);
	bool mayTransmit(Vertex vertex, VuoCompilerTriggerPort *trigger);
	bool mayTransmit(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool mayTransmit(VuoCompilerNode *fromNode, VuoCompilerNode *toNode);
	VuoPort * getOutputPortOnPublishedInputNode(size_t publishedInputPortIndex);
	VuoPort * getGatherPortOnPublishedOutputNode(void);
	size_t getNumVerticesWithFromNode(VuoCompilerNode *fromNode, VuoCompilerTriggerPort *trigger);
	size_t getNumVerticesWithToNode(VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger);
	bool areNodesPartiallyOverlappedByAnotherTrigger(const vector<VuoCompilerNode *> &nodes, VuoCompilerTriggerPort *trigger);

	friend class TestVuoCompilerBitcodeGenerator;
};
