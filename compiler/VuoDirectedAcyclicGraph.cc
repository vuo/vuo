/**
 * @file
 * VuoDirectedAcyclicGraph implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDirectedAcyclicGraph.hh"
#include <sstream>

VuoDirectedAcyclicGraph::Vertex::~Vertex(void)
{
}

/**
 * Constructs an empty graph.
 */
VuoDirectedAcyclicGraph::VuoDirectedAcyclicGraph(void)
{
	cycleVerticesCacheReady = false;
}

/**
 * Destroys all vertex instances in the graph.
 */
VuoDirectedAcyclicGraph::~VuoDirectedAcyclicGraph(void)
{
	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
		delete i->first;
}

/**
 * Adds a vertex to the graph. The graph takes ownership of the vertex object and is responsible for destroying it.
 *
 * If this function is called multiple times for the same vertex object, the vertex is only added once.
 */
void VuoDirectedAcyclicGraph::addVertex(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	edges[vertex];
}

/**
 * Removes a vertex and all of its incoming and outgoing edges from the graph.
 */
void VuoDirectedAcyclicGraph::removeVertex(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		for (vector<Vertex *>::iterator j = i->second.begin(); j != i->second.end(); )
		{
			if (*j == vertex)
				j = i->second.erase(j);
			else
				++j;
		}
	}

	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); )
	{
		if (i->first == vertex)
			edges.erase(i++);
		else
			++i;
	}

	delete vertex;

	downstreamVerticesCache.clear();
	upstreamVerticesCache.clear();
	cycleVerticesCacheReady = false;
	longestDownstreamPathsCache.clear();
}

/**
 * Finds the vertex in the graph whose @ref Vertex::key function returns @a key, or null if there is no such vertex.
 */
VuoDirectedAcyclicGraph::Vertex * VuoDirectedAcyclicGraph::findVertex(const string &key)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
		if (i->first->key() == key)
			return i->first;

	return NULL;
}

/**
 * Adds a directed edge to the graph. The @ref addVertex function can be called first on each vertex but doesn't have to be.
 * The graph takes ownership of the vertex objects and is responsible for destroying them.
 *
 * If this function is called multiple times for the same edge endpoints, the edge is only added once.
 */
void VuoDirectedAcyclicGraph::addEdge(Vertex *fromVertex, Vertex *toVertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	if (find(edges[fromVertex].begin(), edges[fromVertex].end(), toVertex) == edges[fromVertex].end())
	{
		edges[fromVertex].push_back(toVertex);
		edges[toVertex];
	}

	downstreamVerticesCache.clear();
	upstreamVerticesCache.clear();
	cycleVerticesCacheReady = false;
	longestDownstreamPathsCache.clear();
}

/**
 * Removes a directed edge from the graph.
 */
void VuoDirectedAcyclicGraph::removeEdge(Vertex *fromVertex, Vertex *toVertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	auto iter = find(edges[fromVertex].begin(), edges[fromVertex].end(), toVertex);
	if (iter != edges[fromVertex].end())
		edges[fromVertex].erase(iter);

	downstreamVerticesCache.clear();
	upstreamVerticesCache.clear();
	cycleVerticesCacheReady = false;
	longestDownstreamPathsCache.clear();
}

/**
 * Returns all vertices that are directly connected to @a vertex by a directed edge from @a vertex to the other vertex.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getImmediatelyDownstreamVertices(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	return edges[vertex];
}

/**
 * Returns all vertices that can be reached from @a vertex via a path of directed edges.
 *
 * The returned vertices are not in any particular order. In the future, this function may be modified
 * to return them in topological order.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getDownstreamVertices(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	return getDownstreamVerticesInternal(vertex);
}

/**
 * Thread-unsafe version of VuoDirectedAcyclicGraph::getDownstreamVertices().
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getDownstreamVerticesInternal(Vertex *vertex)
{
	map< Vertex *, vector<Vertex *> >::iterator iter = downstreamVerticesCache.find(vertex);
	if (iter != downstreamVerticesCache.end())
		return iter->second;

	set<Vertex *> unused;
	vector<Vertex *> downstreamVertices = getReachableVertices(vertex, edges, unused);

	downstreamVerticesCache[vertex] = downstreamVertices;

	return downstreamVertices;
}

/**
 * Returns all vertices that are directly connected to @a vertex by a directed edge from the other vertex to @a vertex.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getImmediatelyUpstreamVertices(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	vector<VuoDirectedAcyclicGraph::Vertex *> upstreamVertices;

	for (auto i : edges)
	{
		Vertex *fromVertex = i.first;
		for (Vertex *toVertex : i.second)
			if (toVertex == vertex && find(upstreamVertices.begin(), upstreamVertices.end(), fromVertex) == upstreamVertices.end())
				upstreamVertices.push_back(fromVertex);
	}

	return upstreamVertices;
}

/**
 * Returns all vertices that can reach @a vertex via a path of directed edges.
 *
 * The returned vertices are not in any particular order. In the future, this function may be modified
 * to return them in topological order.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getUpstreamVertices(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	map< Vertex *, vector<Vertex *> >::iterator iter = upstreamVerticesCache.find(vertex);
	if (iter != upstreamVerticesCache.end())
		return iter->second;

	map< Vertex *, vector<Vertex *> > backEdges;
	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		for (vector<Vertex *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			if (find(backEdges[*j].begin(), backEdges[*j].end(), i->first) == backEdges[*j].end())
				backEdges[*j].push_back(i->first);

		backEdges[i->first];
	}

	set<Vertex *> unused;
	vector<Vertex *> upstreamVertices = getReachableVertices(vertex, backEdges, unused);

	upstreamVerticesCache[vertex] = upstreamVertices;

	return upstreamVertices;
}

/**
 * Returns any vertices that are part of a cycle (making this not actually a directed acyclic graph).
 */
set<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getCycleVertices(void)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	if (cycleVerticesCacheReady)
		return cycleVerticesCache;

	set<Vertex *> cycleVertices;
	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		set<Vertex *> c;
		getReachableVertices(i->first, edges, c);
		cycleVertices.insert(c.begin(), c.end());
	}

	cycleVerticesCacheReady = true;
	cycleVerticesCache = cycleVertices;

	return cycleVertices;
}

/**
 * Returns the number of vertices in the longest path downstream of @a vertex (not counting @a vertex itself).
 */
int VuoDirectedAcyclicGraph::getLongestDownstreamPath(Vertex *vertex)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	return getLongestDownstreamPathInternal(vertex);
}

/**
 * Thread-unsafe version of @ref VuoDirectedAcyclicGraph::getLongestDownstreamPath().
 */
int VuoDirectedAcyclicGraph::getLongestDownstreamPathInternal(Vertex *vertex)
{
	auto iter = longestDownstreamPathsCache.find(vertex);
	if (iter != longestDownstreamPathsCache.end())
		return iter->second;

	int longest = -1;
	for (Vertex *v : getDownstreamVerticesInternal(vertex))
	{
		int curr = getLongestDownstreamPathInternal(v);
		longest = std::max(longest, curr);
	}

	return longest + 1;
}

/**
 * Helper function.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicGraph::getReachableVertices(Vertex *vertex,
																						const map< Vertex *, vector<Vertex *> > &edges,
																						set<Vertex *> &cycleVertices)
{
	map<Vertex *, vector<Vertex *> > reachableVertices;

	if (edges.find(vertex) == edges.end())
		return vector<Vertex *>();

	list<Vertex *> toVisit;  // Used as a stack, except for a call to find().
	toVisit.push_back(vertex);

	while (! toVisit.empty())
	{
		// Visit the vertex at the top of the stack.
		Vertex *currVertex = toVisit.back();

		set<Vertex *> nextVertices;
		bool areCurrVerticesComplete = true;

		// Consider each vertex immediately downstream of this vertex.
		map< Vertex *, vector<Vertex *> >::const_iterator edgesIter = edges.find(currVertex);
		if (edgesIter != edges.end())
		{
			for (vector<Vertex *>::const_iterator j = edgesIter->second.begin(); j != edgesIter->second.end(); ++j)
			{
				Vertex *nextVertex = *j;
				nextVertices.insert(nextVertex);

				map<Vertex *, vector<Vertex *> >::iterator nextNextVerticesIter = reachableVertices.find(nextVertex);
				if (nextNextVerticesIter != reachableVertices.end())
				{
					// The downstream vertex has already been visited, so add its downstream vertices to this vertex's.
					nextVertices.insert( nextNextVerticesIter->second.begin(), nextNextVerticesIter->second.end() );
				}
				else
				{
					if (find(toVisit.begin(), toVisit.end(), nextVertex) != toVisit.end())
					{
						// The downstream vertex is already on the stack, so there's a cycle.
						cycleVertices.insert(nextVertex);
					}
					else
					{
						// The downstream vertex has not yet been visited, so add it to the stack.
						toVisit.push_back(nextVertex);
						areCurrVerticesComplete = false;
					}
				}
			}
		}

		if (areCurrVerticesComplete)
		{
			reachableVertices[currVertex].insert(reachableVertices[currVertex].end(), nextVertices.begin(), nextVertices.end());
			toVisit.pop_back();
		}
	}

	return reachableVertices[vertex];
}

/**
 * For debugging.
 */
string VuoDirectedAcyclicGraph::toString(bool showVertexPointers)
{
	std::lock_guard<std::mutex> lock(graphMutex);

	ostringstream ss;

	for (map< Vertex *, vector<Vertex *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		ss << i->first->key();
		if (showVertexPointers)
			ss << " (" << i->first << ")";
		ss << " ->";

		for (Vertex *vertex : i->second)
		{
			ss << " " << vertex->key();
			if (showVertexPointers)
				ss << " (" << vertex << ")";
		}

		ss << "\n";
	}

	return ss.str();
}

/**
 * Returns all vertices in the network whose @ref VuoDirectedAcyclicGraph::Vertex::key function returns @a key.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::findVertex(const string &key)
{
	vector<VuoDirectedAcyclicGraph::Vertex *> foundVertices;

	for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::const_iterator i = edges.begin(); i != edges.end(); ++i)
	{
		VuoDirectedAcyclicGraph::Vertex *vertex = i->first->findVertex(key);
		if (vertex)
			foundVertices.push_back(vertex);
	}

	return foundVertices;
}

/**
 * Adds a directed edge to the network. The network does not take ownership of the graph objects.
 *
 * If this function is called multiple times for the same edge endpoints, the edge is only added once.
 */
void VuoDirectedAcyclicNetwork::addEdge(VuoDirectedAcyclicGraph *fromGraph, VuoDirectedAcyclicGraph *toGraph)
{
	if (find(edges[fromGraph].begin(), edges[fromGraph].end(), toGraph) == edges[fromGraph].end())
	{
		edges[fromGraph].push_back(toGraph);
		edges[toGraph];
	}
}

/**
 * Returns all vertices that are directly connected to @a vertex by a directed edge from @a vertex to the other vertex
 * within any graph in the network.
 *
 * When different vertex instances with the same key as @a vertex appear in multiple graphs, all downstream
 * instances are returned.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getImmediatelyDownstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex)
{
	return getReachableVertices(vertex, edges, true, true);
}

/**
 * Returns all vertices that can be reached from @a vertex via a path of directed vertex-to-vertex edges
 * within graphs and directed graph-to-graph edges within the network.
 *
 * When different vertex instances with the same key appear in multiple graphs along the paths, all
 * instances are returned.
 *
 * The returned vertices are not in any particular order. In the future, this function may be modified
 * to return them in topological order.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getDownstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex)
{
	return getReachableVertices(vertex, edges, true, false);
}

/**
 * Returns all vertices that are directly connected to @a vertex by a directed edge from the other vertex to @a vertex
 * within any graph in the network.
 *
 * When different vertex instances with the same key as @a vertex appear in multiple graphs, all upstream
 * instances are returned.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getImmediatelyUpstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex)
{
	return getUpstreamVerticesInternal(vertex, true);
}

/**
 * Returns all vertices that can reach @a vertex via a path of directed vertex-to-vertex edges with graphs
 * and directed graph-to-graph edges within the network.
 *
 * When different vertex instances with the same key appear in multiple graphs along the paths, all
 * instances are returned.
 *
 * The returned vertices are not in any particular order. In the future, this function may be modified
 * to return them in topological order.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getUpstreamVertices(VuoDirectedAcyclicGraph::Vertex *vertex)
{
	return getUpstreamVerticesInternal(vertex, false);
}

/**
 * Helper function.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getUpstreamVerticesInternal(VuoDirectedAcyclicGraph::Vertex *vertex,
																								 bool isImmediate)
{
	map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> > backEdges;
	for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		for (vector<VuoDirectedAcyclicGraph *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			if (find(backEdges[*j].begin(), backEdges[*j].end(), i->first) == backEdges[*j].end())
				backEdges[*j].push_back(i->first);

		backEdges[i->first];
	}

	return getReachableVertices(vertex, backEdges, false, isImmediate);
}

/**
 * Helper function.
 */
vector<VuoDirectedAcyclicGraph::Vertex *> VuoDirectedAcyclicNetwork::getReachableVertices(VuoDirectedAcyclicGraph::Vertex *vertex,
																						  const map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> > &edges,
																						  bool isDownstream, bool isImmediate)
{
	vector<VuoDirectedAcyclicGraph::Vertex *> reachableVertices;

	VuoDirectedAcyclicGraph *containingGraph = NULL;
	for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::const_iterator i = edges.begin(); i != edges.end(); ++i)
	{
		if (i->first->edges.find(vertex) != i->first->edges.end())
		{
			containingGraph = i->first;
			break;
		}
	}

	if (! containingGraph)
		return reachableVertices;

	list< pair< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph::Vertex *> > > toVisit;
	toVisit.push_back( make_pair(containingGraph, vector<VuoDirectedAcyclicGraph::Vertex *>(1, vertex)) );

	while (! toVisit.empty())
	{
		VuoDirectedAcyclicGraph *currGraph = toVisit.front().first;
		vector<VuoDirectedAcyclicGraph::Vertex *> currVertices = toVisit.front().second;
		toVisit.pop_front();

		map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::const_iterator edgesIter = edges.find(currGraph);
		if (edgesIter != edges.end())
		{
			map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph::Vertex *> > moreToVisit;

			for (vector<VuoDirectedAcyclicGraph::Vertex *>::iterator i = currVertices.begin(); i != currVertices.end(); ++i)
			{
				VuoDirectedAcyclicGraph::Vertex *currVertex = *i;
				vector<VuoDirectedAcyclicGraph::Vertex *> currReachableVertices = (isDownstream ?
																					   (isImmediate ?
																							currGraph->getImmediatelyDownstreamVertices(currVertex) :
																							currGraph->getDownstreamVertices(currVertex)) :
																					   (isImmediate ?
																							currGraph->getImmediatelyUpstreamVertices(currVertex) :
																							currGraph->getUpstreamVertices(currVertex)));

				for (vector<VuoDirectedAcyclicGraph::Vertex *>::iterator j = currReachableVertices.begin(); j != currReachableVertices.end(); ++j)
				{
					VuoDirectedAcyclicGraph::Vertex *currReachableVertex = *j;

					if (find(reachableVertices.begin(), reachableVertices.end(), currReachableVertex) == reachableVertices.end())
						reachableVertices.push_back(currReachableVertex);
				}

				vector<VuoDirectedAcyclicGraph::Vertex *> candidateVertices;
				if (isImmediate)
				{
					if (currVertex == vertex)
						candidateVertices.push_back(currVertex);
				}
				else
				{
					candidateVertices.push_back(currVertex);
					candidateVertices.insert(candidateVertices.end(), currReachableVertices.begin(), currReachableVertices.end());
				}

				for (vector<VuoDirectedAcyclicGraph::Vertex *>::iterator j = candidateVertices.begin(); j != candidateVertices.end(); ++j)
				{
					VuoDirectedAcyclicGraph::Vertex *candidateVertex = *j;

					for (vector<VuoDirectedAcyclicGraph *>::const_iterator k = edgesIter->second.begin(); k != edgesIter->second.end(); ++k)
					{
						VuoDirectedAcyclicGraph *otherGraph = *k;

						VuoDirectedAcyclicGraph::Vertex *matchingVertexInOtherGraph = otherGraph->findVertex(candidateVertex->key());
						if (matchingVertexInOtherGraph)
						{
							moreToVisit[otherGraph].push_back(matchingVertexInOtherGraph);

							if (find(reachableVertices.begin(), reachableVertices.end(), matchingVertexInOtherGraph) == reachableVertices.end())
								reachableVertices.push_back(matchingVertexInOtherGraph);
						}
					}
				}
			}

			for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph::Vertex *> >::iterator i = moreToVisit.begin(); i != moreToVisit.end(); ++i)
				toVisit.push_back( make_pair(i->first, i->second) );
		}
	}

	return reachableVertices;
}

/**
 * For debugging.
 */
string VuoDirectedAcyclicNetwork::toString(bool showVertexAddresses)
{
	ostringstream ss;

	for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		ss << i->first << " ->";
		for (VuoDirectedAcyclicGraph *graph : i->second)
			ss << " " << graph;
		ss << "\n";
	}

	for (map< VuoDirectedAcyclicGraph *, vector<VuoDirectedAcyclicGraph *> >::iterator i = edges.begin(); i != edges.end(); ++i)
	{
		string graphString = i->first->toString(showVertexAddresses);
		if (! graphString.empty())
			ss << "\n" << i->first << ":\n" << graphString << "\n";
	}

	return ss.str();
}
