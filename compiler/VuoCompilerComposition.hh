/**
 * @file
 * VuoCompilerComposition interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoComposition.hh"

class VuoCompiler;
class VuoCompilerCable;
class VuoCompilerGraph;
class VuoCompilerGraphvizParser;
class VuoCompilerPort;
class VuoCable;
class VuoNode;
class VuoPort;
class VuoPublishedPort;

/**
 * A collection of nodes and the cables connecting them.
 */
class VuoCompilerComposition : public VuoBaseDetail<VuoComposition>
{
public:
	/**
	 * A mapping from a node to its replacement in a composition diff.
	 */
	class NodeReplacement
	{
	public:
		string oldNodeIdentifier;  ///< The Graphviz identifier of the original node.
		string newNodeIdentifier;  ///< The Graphviz identifier of the replacement node.
		map<string, string> oldAndNewPortIdentifiers;  ///< A mapping of equivalent port class names from the original node to the replacement node.
	};
	friend bool operator<(const NodeReplacement &lhs, const NodeReplacement &rhs);

	VuoCompilerComposition(VuoComposition *baseComposition, VuoCompilerGraphvizParser *parser);
	~VuoCompilerComposition(void);
	static VuoCompilerComposition * newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler);
	VuoCompilerGraph * getCachedGraph(void);
	void check(void);
	void checkForMissingNodeClasses(void);
	void checkFeedback(set<VuoCompilerCable *> potentialCables = set<VuoCompilerCable *>());
	void updateGenericPortTypes(void);
	set< set<VuoCompilerPort *> > groupGenericPortsByType(bool useOriginalType);
	set<VuoPort *> getConnectedGenericPorts(VuoPort *port);
	void setUniqueGraphvizIdentifierForNode(VuoNode *node);
	void clearGraphvizNodeIdentifierHistory();
	VuoPort * findNearestUpstreamTriggerPort(VuoNode *node);
	void setModule(Module *module);
	string getGraphvizDeclaration(string header = "", string footer = "");
	string getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet, set<VuoCable *> cableSet, vector<VuoPublishedPort *> publishedInputPorts, vector<VuoPublishedPort *> publishedOutputPorts, string header="", string footer="", double xPositionOffset=0, double yPositionOffset=0);
	string diffAgainstOlderComposition(string oldCompositionGraphvizDeclaration, VuoCompiler *compiler, const set<NodeReplacement> &nodeReplacements);

	static const string defaultGraphDeclaration; ///< The default graph type and ID to be generated for new .vuo (Graphviz dot format) composition files.

	friend class TestCompilingAndLinking;

private:
	VuoCompilerGraph *graph;
	long graphHash;
	map<unsigned int, bool> genericTypeSuffixUsed;
	map<string, VuoNode *> nodeGraphvizIdentifierUsed;
	Module *module;

	string createFreshGenericTypeName(void);
	static bool compareGraphvizIdentifiersOfNodes(VuoNode *lhs, VuoNode *rhs);
	static bool compareGraphvizIdentifiersOfCables(VuoCable *lhs, VuoCable *rhs);
};
