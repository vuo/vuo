/**
 * @file
 * VuoDirectedAcyclicGraph interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <mutex>

/**
 * A directed acyclic graph (DAG) data structure with informative error reporting for cycles.
 */
class VuoDirectedAcyclicGraph
{
public:
	/**
	 * Abstract base class for the items contained in the DAG.
	 */
	class Vertex
	{
	public:
		/**
		 * Used for looking up vertices by key (e.g. @ref findVertex).
		 */
		virtual string key(void) = 0;

		virtual ~Vertex(void);
	};

	VuoDirectedAcyclicGraph(void);
	~VuoDirectedAcyclicGraph(void);
	void addVertex(Vertex *vertex);
	void removeVertex(Vertex *vertex);
	Vertex * findVertex(const string &key);
	void addEdge(Vertex *fromVertex, Vertex *toVertex);
	void removeEdge(Vertex *fromVertex, Vertex *toVertex);
	vector<Vertex *> getImmediatelyDownstreamVertices(Vertex *vertex);
	vector<Vertex *> getDownstreamVertices(Vertex *vertex);
	vector<Vertex *> getImmediatelyUpstreamVertices(Vertex *vertex);
	vector<Vertex *> getUpstreamVertices(Vertex *vertex);
	set<Vertex *> getCycleVertices(void);
	int getLongestDownstreamPath(Vertex *vertex);
	string toString(bool showVertexAddresses=false);

private:
	vector<Vertex *> getDownstreamVerticesInternal(Vertex *vertex);
	int getLongestDownstreamPathInternal(Vertex *vertex);
	static vector<Vertex *> getReachableVertices(Vertex *vertex, const map< Vertex *, vector<Vertex *> > &edges, set<Vertex *> &cycleVertices);

	map< Vertex *, vector<Vertex *> > edges;  ///< Adjacency list. Every vertex has a key in this map, even if it doesn't have any outgoing edges.
	map< Vertex *, vector<Vertex *> > downstreamVerticesCache;
	map< Vertex *, vector<Vertex *> > upstreamVerticesCache;
	bool cycleVerticesCacheReady;
	set<Vertex *> cycleVerticesCache;
	map< Vertex *, int > longestDownstreamPathsCache;
	std::mutex graphMutex;  ///< Synchronizes access to the graph data structures.

	friend class VuoDirectedAcyclicNetwork;
};

/**
 * A data structure that ties together multiple directed acyclic graphs. If a VuoDirectedAcyclicGraph::Vertex
 * with the same key appears in two graphs, and there is an edge from the first graph to the second graph, then
 * this data structure effectively adds an edge from the vertex in the first graph to the vertex in the second graph:
 * vertices downstream of the vertex in the second graph are also downstream of the vertex in the first graph.
 */
class VuoDirectedAcyclicNetwork
{
public:
	vector<VuoDirectedAcyclicGraph::Vertex *> findVertex(const string &key);
	void addEdge(VuoDirectedAcyclicGraph *fromGraph, VuoDirectedAcyclicGraph *toGraph);
	vector<VuoDirectedAcyclicGraph::Vertex *> getImmediatelyDownstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex);
	vector<VuoDirectedAcyclicGraph::Vertex *> getDownstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex);
	vector<VuoDirectedAcyclicGraph::Vertex *> getImmediatelyUpstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex);
	vector<VuoDirectedAcyclicGraph::Vertex *> getUpstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex);
	string toString(bool showVertexAddresses=false);

private:
	vector<VuoDirectedAcyclicGraph::Vertex *> getUpstreamVerticesInternal(VuoDirectedAcyclicGraph::Vertex *vertex, bool isImmediate);
	static vector<VuoDirectedAcyclicGraph::Vertex *> getReachableVertices(VuoDirectedAcyclicGraph::Vertex *vertex, const map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> > &edges, bool isDownstream, bool isImmediate);

	map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> > edges;
};
