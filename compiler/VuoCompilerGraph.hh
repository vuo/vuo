/**
 * @file
 * VuoCompilerGraph interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERGRAPH_HH
#define VUOCOMPILERGRAPH_HH

#include "VuoCable.hh"
#include "VuoCompilerChain.hh"
#include "VuoCompilerTriggerPort.hh"
class VuoCompilerComposition;

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
	VuoCompilerGraph(VuoCompilerComposition *composition, set<VuoCompilerCable *> potentialCables = set<VuoCompilerCable *>());
	bool mayTransmit(VuoCompilerNode *fromNode, VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger);
	bool mayTransmitEventlessly(VuoCompilerNode *node);
	vector<VuoCompilerTriggerPort *> getTriggerPorts(void);
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> getNodesForTriggerPorts(void);
	map<VuoCompilerTriggerPort *, vector<VuoCompilerChain *> > getChains(void);
	vector<VuoCompilerNode *> getNodesImmediatelyDownstream(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesImmediatelyDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesDownstream(VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesDownstream(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	vector<VuoCompilerNode *> getNodesEventlesslyDownstream(VuoCompilerNode *node);
	set<VuoCompilerCable *> getCablesImmediatelyDownstream(VuoCompilerPort *outputPort);
	set<VuoCompilerCable *> getCablesImmediatelyEventlesslyDownstream(VuoCompilerPort *outputPort);
	bool isRepeatedInFeedbackLoop(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool hasGatherDownstream(VuoCompilerTriggerPort *trigger);
	void checkForInfiniteFeedback(void);
	void checkForDeadlockedFeedback(void);

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
		void print(void) const;
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
		void print(void) const;
	};

	friend bool operator==(const Vertex &lhs, const Vertex &rhs);
	friend bool operator!=(const Vertex &lhs, const Vertex &rhs);
	friend bool operator<(const Vertex &lhs, const Vertex &rhs);
	friend bool operator<(const Edge &lhs, const Edge &rhs);

	/// The vertices reachable from each trigger, including those for all published input ports (including @c vuoSimultaneous),
	/// but excluding those for published output port. The vertices are listed in topological order.
	map<VuoCompilerTriggerPort *, vector<Vertex> > vertices;

	/// The edges reachable from each trigger, including those for all published input ports (including @c vuoSimultaneous),
	/// but excluding those for published output port.
	map<VuoCompilerTriggerPort *, set<Edge> > edges;

	/// For each trigger and vertex, the vertices that are downstream of it (i.e., an event can flow from one to the other).
	map<VuoCompilerTriggerPort *, map<Vertex, set<Vertex> > > downstreamVertices;

	/// The vertices encountered more than once while traversing the graph. Each is within an infinite feedback loop.
	map<VuoCompilerTriggerPort *, set<Vertex> > repeatedVertices;

	/// All trigger ports, including those for published input ports (including vuoSimultaneous).
	vector<VuoCompilerTriggerPort *> triggers;

	/// For each trigger port, the node that contains it.
	map<VuoCompilerTriggerPort *, VuoCompilerNode *> nodeForTrigger;

	/// For each node that can transmit data through their output cables without an event, the nodes they can transmit to
	/// in that way, in topological order. The eventlessly downstream nodes may include a mix of eventlessly-transmitting and
	/// not-eventlessly-transmitting nodes.
	map<VuoCompilerNode *, vector<VuoCompilerNode *> > eventlesslyDownstreamNodes;

	void makeTriggers(set<VuoNode *> nonPublishedNodes, VuoNode *publishedInputNode);
	void makeVerticesAndEdges(set<VuoCable *> nonPublishedCables, set<VuoCable *> publishedInputCables,
							  set<VuoCompilerCable *> potentialCables, VuoNode *publishedInputNode);
	void makeDownstreamVertices(void);
	void sortVertices(void);
	void makeEventlesslyDownstreamNodes(set<VuoNode *> nonPublishedNodes, set<VuoCable *> nonPublishedCables);
	static bool mayTransmit(const set<VuoCompilerCable *> &fromCables, const set<VuoCompilerCable *> &toCables);
	bool mayTransmit(Vertex vertex, VuoCompilerTriggerPort *trigger);
	bool mayTransmit(VuoCompilerNode *node, VuoCompilerTriggerPort *trigger);
	bool mayTransmit(VuoCompilerNode *fromNode, VuoCompilerNode *toNode);
	set<Edge> getEdgesFromVertex(Vertex vertex, VuoCompilerTriggerPort *trigger);
	size_t getNumVerticesWithFromNode(VuoCompilerNode *fromNode, VuoCompilerTriggerPort *trigger);
	size_t getNumVerticesWithToNode(VuoCompilerNode *toNode, VuoCompilerTriggerPort *trigger);
	vector<Vertex> getVerticesInTopologicalOrder(VuoCompilerTriggerPort *trigger);

	friend class TestVuoCompilerBitcodeGenerator;
};

#endif
