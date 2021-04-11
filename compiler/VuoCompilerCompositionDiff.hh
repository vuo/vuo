/**
 * @file
 * VuoCompilerCompositionDiff interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoCompilerComposition;
class VuoCompilerNode;
class VuoCompilerNodeClass;

/**
 * Computes the diff between composition versions before and after a live-editing reload.
 */
class VuoCompilerCompositionDiff
{
public:
	string diff(const string &oldCompositionGraphvizDeclaration, VuoCompilerComposition *newComposition, VuoCompiler *compiler);

	void addNodeReplacement(const string &compositionIdentifier, const string &oldNodeIdentifier, const string &newNodeIdentifier);
	void addNodeReplacement(const string &compositionIdentifier, const string &oldNodeIdentifier, const string &newNodeIdentifier, const map<string, string> &oldAndNewPortNames);
	void addNodeClassReplacement(VuoCompilerNodeClass *oldNodeClass, VuoCompilerNodeClass *newNodeClass);
	void addModuleReplacement(const string &moduleKey);
	void addRefactoring(const string &compositionIdentifier, const set<VuoCompilerNode *> &nodesMoved, VuoCompilerNode *subcompositionMovedTo);
	set<string> getModuleKeysReplaced(void) const;

private:
	void diff(VuoCompilerComposition *oldComposition, VuoCompilerComposition *newComposition, const string &parentCompositionIdentifier, const string &parentCompositionPath, const string &unqualifiedCompositionIdentifier, VuoCompiler *compiler, json_object *diffJson);

	void addNodeReplacementInTopLevelComposition(const string &oldNodeIdentifier, const string &newNodeIdentifier);
	void addNodeReplacementInTopLevelComposition(const string &oldNodeIdentifier, const string &newNodeIdentifier, const map<string, string> &oldAndNewPortNames);
	bool isNodeBeingReplaced(const string &compositionIdentifier, const string &oldNodeIdentifier) const;
	bool isNodeReplacingAnother(const string &compositionIdentifier, const string &newNodeIdentifier) const;
	bool isNodeBeingRefactored(const string &parentCompositionIdentifier, const string &compositionIdentifier, const string &nodeIdentifier) const;

	/**
	 * A mapping from a node in the old composition to its replacement in the new composition.
	 */
	class NodeReplacement
	{
	public:
		string compositionIdentifier;  ///< The identifier of the (sub)composition in which a node is being replaced.
		string oldNodeIdentifier;  ///< The Graphviz identifier of the original node.
		string newNodeIdentifier;  ///< The Graphviz identifier of the replacement node.
		map<string, string> oldAndNewPortNames;  ///< A mapping of equivalent port class names from the original node to the replacement node.
		bool shouldMapIdenticalPortNames;  ///< If true, ports with identical names in original and replacement node are automatically added to the mapping.
	};
	friend bool operator<(const NodeReplacement &lhs, const NodeReplacement &rhs);

	/**
	 * A mapping from a node class in the old composition to its replacement in the new composition.
	 */
	class NodeClassReplacement
	{
	public:
		string nodeClassName;
		map<string, string> oldAndNewPortNames;
		string oldSubcompositionSourceCode;
	};
	friend bool operator<(const NodeClassReplacement &lhs, const NodeClassReplacement &rhs);

	/**
	 * A mapping from nodes in the old composition to their new location after being refactored into a subcomposition in the new composition.
	 */
	class Refactoring
	{
	public:
		string compositionIdentifier;
		string unqualifiedSubcompositionIdentifier;
		set<string> nodeIdentifiers;
	};
	friend bool operator<(const Refactoring &lhs, const Refactoring &rhs);

	set<NodeReplacement> nodeReplacements;  ///< For each node replaced, the mapping from the old to new node.
	set<NodeClassReplacement> nodeClassReplacements;  ///< For each node class replaced, the mapping from the old to new node class.
	set<string> moduleReplacements;  ///< For each module replaced that is not a node class, the module key.
	set<Refactoring> refactorings;  ///< For each subcomposition created by refactoring, the mapping from the old to new locations of the nodes contained in the subcomposition.

	friend class TestVuoCompilerComposition;
};
