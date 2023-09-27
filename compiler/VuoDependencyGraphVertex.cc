/**
 * @file
 * VuoDependencyGraphVertex implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoDependencyGraphVertex.hh"

/**
 * Finds the vertex for @a dependency in @a graph if the vertex exists, otherwise creates it (but does not add it to @a graph).
 *
 * @param dependency The module key of the module/subcomposition.
 * @param graph The dependency graph in which to search.
 */
VuoDependencyGraphVertex * VuoDependencyGraphVertex::vertexForDependency(const string &dependency, VuoDirectedAcyclicGraph *graph)
{
	VuoDirectedAcyclicGraph::Vertex *v = graph->findVertex(dependency);
	if (v)
		return dynamic_cast<VuoDependencyGraphVertex *>(v);

	VuoDependencyGraphVertex *vv = new VuoDependencyGraphVertex();
	vv->dependency = dependency;
	vv->environment = NULL;
	vv->compatible = true;
	return vv;
}

/**
 * Returns the module key of this module/subcomposition.
 */
string VuoDependencyGraphVertex::getDependency(void)
{
	return dependency;
}

/**
 * Returns the value set by VuoDependencyGraphVertex::setEnvironment().
 */
VuoCompilerEnvironment * VuoDependencyGraphVertex::getEnvironment(void)
{
	return environment;
}

/**
 * Stores the environment that this module/subcomposition belongs to.
 */
void VuoDependencyGraphVertex::setEnvironment(VuoCompilerEnvironment *environment)
{
	this->environment = environment;
}

/**
 * Returns the module key of this module/subcomposition (as the unique identifier used by VuoDirectedAcyclicGraph).
 */
string VuoDependencyGraphVertex::key(void)
{
	return dependency;
}
