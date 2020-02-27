/**
 * @file
 * VuoCompilerComposition interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"

class VuoCompiler;
class VuoCompilerCable;
class VuoCompilerGraph;
class VuoCompilerGraphvizParser;
class VuoCompilerIssues;
class VuoCompilerNodeClass;
class VuoCompilerPort;
class VuoCompilerTargetSet;
class VuoComposition;
class VuoCable;
class VuoComment;
class VuoNode;
class VuoPort;
class VuoPortClass;
class VuoProtocol;
class VuoPublishedPort;

/**
 * A collection of nodes and the cables connecting them.
 */
class VuoCompilerComposition : public VuoBaseDetail<VuoComposition>
{
public:
	VuoCompilerComposition(VuoComposition *baseComposition, VuoCompilerGraphvizParser *parser);
	~VuoCompilerComposition(void);
	static VuoCompilerComposition * newCompositionFromGraphvizDeclaration(const string &compositionGraphvizDeclaration, VuoCompiler *compiler);
	void getChangesToReplaceNode(VuoNode *oldNode, VuoNode *newNode, map<VuoCable *, VuoPort *> &cablesToTransferFromPort, map<VuoCable *, VuoPort *> &cablesToTransferToPort, set<VuoCable *> &cablesToRemove) const;
	void replaceNode(VuoNode *oldNode, VuoNode *newNode);
	VuoCompilerGraph * getCachedGraph(VuoCompiler *compiler = nullptr);
	void invalidateCachedGraph(void);
	void check(VuoCompilerIssues *issues);
	void checkForMissingNodeClasses(VuoCompilerIssues *issues);
	void checkForEventFlowIssues(VuoCompilerIssues *issues);
	void checkForEventFlowIssues(set<VuoCompilerCable *> potentialCables, VuoCompilerIssues *issues);
	void updateGenericPortTypes(void);
	set<VuoPort *> getCorrelatedGenericPorts(VuoNode *entryNode, VuoPort *entryPort, bool useOriginalType);
	VuoPort * findNearestUpstreamTriggerPort(VuoNode *node);
	void setModule(Module *module);
	Module * takeModule(void);
	void setUniqueGraphvizIdentifierForNode(VuoNode *node, const string &preferredIdentifier = "", const string &identifierPrefix = "");
	void clearGraphvizNodeIdentifierHistory();
	void setUniqueGraphvizIdentifierForComment(VuoComment *comment);
	void clearGraphvizCommentIdentifierHistory();
	void setManuallyFirableInputPort(VuoNode *nodeContainingPort, VuoPort *portFiredInto);
	VuoNode * getManuallyFirableInputNode(void);
	VuoPort * getManuallyFirableInputPort(void);
	string getGraphvizDeclaration(VuoProtocol *activeProtocol = NULL, string header = "", string footer = "");
	string getGraphvizDeclarationForComponents(set<VuoNode *> nodeSet, set<VuoCable *> cableSet, set<VuoComment *> commentSet,
											   vector<VuoPublishedPort *> publishedInputPorts, vector<VuoPublishedPort *> publishedOutputPorts,
											   string header="", string footer="", double xPositionOffset=0, double yPositionOffset=0);
	VuoCompilerTargetSet getCompatibleTargets(void);

	static bool portsMatch(VuoPort *oldPort, VuoPort *newPort);
	static bool portClassesMatch(VuoPortClass *oldPortClass, VuoPortClass *newPortClass);

	static const string defaultGraphDeclaration; ///< The default graph type and ID to be generated for new .vuo (Graphviz dot format) composition files.
	static const string topLevelCompositionIdentifier;  ///< The key of the top-level composition when looking up node contexts.

	friend class TestCompilingAndLinking;

private:
	VuoCompilerGraph *graph;
	long graphHash;
	map<unsigned int, bool> genericTypeSuffixUsed;
	map<string, VuoNode *> nodeGraphvizIdentifierUsed;
	map<string, VuoComment *> commentGraphvizIdentifierUsed;
	VuoNode *manuallyFirableInputNode;
	VuoPort *manuallyFirableInputPort;
	Module *module;

	void checkForMissingTypes(VuoCompilerIssues *issues);
	set< set<VuoCompilerPort *> > groupGenericPortsByType(void);
	string createFreshGenericTypeName(void);
	static bool compareGraphvizIdentifiersOfNodes(VuoNode *lhs, VuoNode *rhs);
	static bool compareGraphvizIdentifiersOfCables(VuoCable *lhs, VuoCable *rhs);
	static bool compareGraphvizIdentifiersOfComments(VuoComment *lhs, VuoComment *rhs);
};
