/**
 * @file
 * VuoDependencyGraphVertex interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoDirectedAcyclicGraph.hh"
class VuoCompilerEnvironment;

/**
 * A vertex representing a module or subcomposition in a composition's dependency graph.
 */
class VuoDependencyGraphVertex : public VuoDirectedAcyclicGraph::Vertex
{
public:
	static VuoDependencyGraphVertex * vertexForDependency(const string &dependency, VuoDirectedAcyclicGraph *graph);
	string getDependency(void);
	VuoCompilerEnvironment * getEnvironment(void);
	void setEnvironment(VuoCompilerEnvironment *environment);
	string key(void);

private:
	string dependency;
	VuoCompilerEnvironment *environment;
	bool compatible;
};
