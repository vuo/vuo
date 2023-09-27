/**
 * @file
 * VuoCompilerCompositionDiff implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCompositionDiff.hh"

#include "VuoCompilerCable.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerGraph.hh"
#include "VuoCompilerNode.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCable.hh"
#include "VuoComposition.hh"
#include "VuoNode.hh"
#include "VuoNodeClass.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoStringUtilities.hh"

/**
 * Returns a JSON-formatted comparison between the old and the new composition, including subcompositions.
 *
 * The comparison is a JSON array loosely based on the [JSON Patch](https://datatracker.ietf.org/doc/html/rfc6902) format.
 * The key used for each node is a path constructed from its Graphviz identifier and the identifiers of the (sub)compositions
 * containing the node. Unlike the examples below (spaced for readability), the returned string contains no whitespace.
 *
 * The comparison follows the compiler's rule that the PublishedInputs and PublishedOutputs pseudo-nodes are either
 * both present or both absent in a compiled composition.
 *
 * @eg{
 * // The new composition contains (added) node FireOnStart. The old composition contains (removed) node Round.
 * [
 *   {"add" : "Top/FireOnStart"},
 *   {"remove" : "Top/Round"}
 * ]
 * }
 *
 * @eg{
 * // From the old composition to the new composition, a drawer has been expanded from 2 to 3 inputs.
 * [
 *   {"replace" : "Top/MakeList1", "with" : "Top/MakeList2",
 *     "ports" : [
 *       {"map" : "1", "to" : "1"},
 *       {"map" : "2", "to" : "2"},
 *       {"map" : "list", "to" : "list}
 *     ]
 *   }
 * ]
 * }
 *
 * @eg{
 * // From the old to the new composition, the ports on a node have changed (some added, some removed)
 * // because its node class has been replaced with a new implementation. Port "keptPort" stays the same.
 * [
 *   {"replace" : "Top/MyNode", "with" : "Top/MyNode",
 *     "ports" : [
 *       {"map" : "keptPort", "to" : "keptPort"}
 *     ]
 *   }
 * ]
 * }
 *
 * @eg{
 * // From the old to the new composition, a published input port has been added.
 * // Published input port "firstInput" and published output port "firstOutput" stay the same.
 * [
 *   {"replace" : "Top/PublishedInputs", "with" : "Top/PublishedInputs",
 *     "ports" : [
 *       {"map" : "firstInput", "to" : "firstInput"}
 *     ]
 *   },
 *   {"replace" : "Top/PublishedOutputs", "with" : "Top/PublishedOutputs",
 *     "ports" : [
 *       {"map" : "firstOutput", "to" : "firstOutput"}
 *     ]
 *   }
 * ]
 * }
 *
 * @eg{
 * // From the old to the new composition, a published input port has been added.
 * // The old composition doesn't have any published ports.
 * [
 *   {"add" : "Top/PublishedInputs"},
 *   {"add" : "Top/PublishedOutputs"}
 * ]
 * }
 *
 * @eg{
 * // From the old to the new composition, nodes have been refactored into a subcomposition.
 * [
 *   {"add" : "Top/Sub"},
 *   {"move" : "Top/Node1", "to" : "Top/Sub/Node1",
 *     "ports" : [
 *       {"copy" : "inputOnNode1", "to" : "Top/Sub:someInput"}
 *     ]
 *   },
 *   {"move" : "Top/Node2", "to" : "Top/Sub/Node2"
 *     "ports" : [
 *       {"copy" : "inputOnNode2", "to" : "Top/Sub:otherInput"}
 *     ]
 *   }
 * }
 *
 * This needs to be kept in sync with @ref VuoCompositionDiff::findNode.
 */
string VuoCompilerCompositionDiff::diff(const string &oldCompositionGraphvizDeclaration, VuoCompilerComposition *newComposition,
										VuoCompiler *compiler)
{
	VuoCompilerComposition *oldComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(oldCompositionGraphvizDeclaration, compiler);
	json_object *diffJson = json_object_new_array();

	diff(oldComposition, newComposition, "", "", VuoCompilerComposition::topLevelCompositionIdentifier, compiler, diffJson);

	delete oldComposition;

	string diffString = json_object_to_json_string_ext(diffJson, JSON_C_TO_STRING_PLAIN);
	VuoStringUtilities::replaceAll(diffString, "\\/", "/");
	json_object_put(diffJson);
	return diffString;
}

/**
 * Helper for @ref diff(const string &, VuoCompilerComposition *, VuoCompiler *), to be called recursively on subcompositions.
 */
void VuoCompilerCompositionDiff::diff(VuoCompilerComposition *oldComposition, VuoCompilerComposition *newComposition,
									  const string &parentCompositionIdentifier, const string &parentCompositionPath,
									  const string &unqualifiedCompositionIdentifier,
									  VuoCompiler *compiler, json_object *diffJson)
{
	string compositionIdentifier = (parentCompositionIdentifier.empty() ?
										unqualifiedCompositionIdentifier :
										parentCompositionIdentifier + "/" + unqualifiedCompositionIdentifier);
	string compositionPath = (parentCompositionPath.empty() ?
								  unqualifiedCompositionIdentifier :
								  parentCompositionPath + "/" + unqualifiedCompositionIdentifier);

	// Diff nodes that have been added or removed with no replacement or refactoring.

	set<VuoNode *> oldNodes;
	set<VuoNode *> newNodes;
	if (oldComposition)
		oldNodes = oldComposition->getBase()->getNodes();
	if (newComposition)
		newNodes = newComposition->getBase()->getNodes();

	map<string, VuoNode *> oldNodeForIdentifier;
	map<string, VuoNode *> newNodeForIdentifier;
	map<string, pair<VuoNode *, VuoNode *> > oldAndNewNodeForIdentifier;
	for (set<VuoNode *>::iterator i = oldNodes.begin(); i != oldNodes.end(); ++i)
	{
		string identifier = (*i)->getCompiler()->getGraphvizIdentifier();
		oldNodeForIdentifier[identifier] = *i;
		oldAndNewNodeForIdentifier[identifier].first = *i;
	}
	for (set<VuoNode *>::iterator i = newNodes.begin(); i != newNodes.end(); ++i)
	{
		string identifier = (*i)->getCompiler()->getGraphvizIdentifier();
		newNodeForIdentifier[identifier] = *i;
		oldAndNewNodeForIdentifier[identifier].second = *i;
	}

	for (map<string, VuoNode *>::iterator oldNodeIter = oldNodeForIdentifier.begin(); oldNodeIter != oldNodeForIdentifier.end(); ++oldNodeIter)
	{
		if (newNodeForIdentifier.find(oldNodeIter->first) == newNodeForIdentifier.end() &&
				! isNodeBeingReplaced(compositionIdentifier, oldNodeIter->first) &&
				! isNodeBeingRefactored(parentCompositionIdentifier, unqualifiedCompositionIdentifier, oldNodeIter->first))
		{
			// { "remove" : "<composition path>/<node identifier>" }
			json_object *remove = json_object_new_object();
			json_object *nodePath = json_object_new_string((compositionPath + "/" + oldNodeIter->first).c_str());
			json_object_object_add(remove, "remove", nodePath);
			json_object_array_add(diffJson, remove);
		}
	}
	for (map<string, VuoNode *>::iterator newNodeIter = newNodeForIdentifier.begin(); newNodeIter != newNodeForIdentifier.end(); ++newNodeIter)
	{
		if (oldNodeForIdentifier.find(newNodeIter->first) == oldNodeForIdentifier.end() &&
				! isNodeReplacingAnother(compositionIdentifier, newNodeIter->first) &&
				! isNodeBeingRefactored(parentCompositionIdentifier, unqualifiedCompositionIdentifier, newNodeIter->first))
		{
			// { "add" : "<composition path>/<node identifier>" }
			json_object *add = json_object_new_object();
			json_object *nodePath = json_object_new_string((compositionPath + "/" + newNodeIter->first).c_str());
			json_object_object_add(add, "add", nodePath);
			json_object_array_add(diffJson, add);
		}
	}

	// If we're inside a subcomposition instance that is being added/removed, add/remove the manually firable trigger node.

	if (! oldComposition)
	{
		json_object *add = json_object_new_object();
		json_object *nodePath = json_object_new_string((compositionPath + "/" + VuoCompilerGraph::getManuallyFirableTriggerNodeIdentifier()).c_str());
		json_object_object_add(add, "add", nodePath);
		json_object_array_add(diffJson, add);
	}
	if (! newComposition)
	{
		json_object *remove = json_object_new_object();
		json_object *nodePath = json_object_new_string((compositionPath + "/" + VuoCompilerGraph::getManuallyFirableTriggerNodeIdentifier()).c_str());
		json_object_object_add(remove, "remove", nodePath);
		json_object_array_add(diffJson, remove);
	}

	// Diff nodes that have been refactored.

	auto buildMovePath = [](const Refactoring &r, const string &nodeIdentifier)
	{
		return r.compositionIdentifier + "/" + nodeIdentifier;
	};
	auto buildToPath = [](const Refactoring &r, const string &nodeIdentifier)
	{
		return r.compositionIdentifier + "/" + r.unqualifiedSubcompositionIdentifier + "/" + nodeIdentifier;
	};

	for (const Refactoring &r : refactorings)
	{
		if (compositionIdentifier == r.compositionIdentifier)
		{
			for (const string &nodeIdentifier : r.nodeIdentifiers)
			{
				// { "move" : "<composition path>/<node identifier>", "to" : "<subcomposition path>/<node identifier>",
				json_object *move = json_object_new_object();
				json_object *oldNodePath = json_object_new_string(buildMovePath(r, nodeIdentifier).c_str());
				json_object_object_add(move, "move", oldNodePath);
				json_object *newNodePath = json_object_new_string(buildToPath(r, nodeIdentifier).c_str());
				json_object_object_add(move, "to", newNodePath);
				json_object_array_add(diffJson, move);
			}
		}
		else if (parentCompositionIdentifier == r.compositionIdentifier && unqualifiedCompositionIdentifier == r.unqualifiedSubcompositionIdentifier)
		{
			for (const string &nodeIdentifier : r.nodeIdentifiers)
			{
				//   "ports" : [
				//          { "copy" : "<port identifier>", "to" : "<composition path>/<node identifier>:<port identifier>" },
				//          { "copy" : "<port identifier>", "to" : "<composition path>/<node identifier>:<port identifier>" },
				//          ... ] }

				json_object *move = nullptr;
				int length = json_object_array_length(diffJson);
				for (int i = 0; i < length; ++i)
				{
					json_object *curr = json_object_array_get_idx(diffJson, i);
					json_object *moveNodePath = nullptr;
					json_object *toNodePath = nullptr;

					if (json_object_object_get_ex(curr, "move", &moveNodePath) && json_object_object_get_ex(curr, "to", &toNodePath) &&
							buildMovePath(r, nodeIdentifier) == json_object_get_string(moveNodePath) &&
							buildToPath(r, nodeIdentifier) == json_object_get_string(toNodePath))
					{
						move = curr;
						break;
					}
				}

				json_object *ports = json_object_new_array();
				for (VuoPort *internalPort : newNodeForIdentifier[nodeIdentifier]->getInputPorts())
				{
					for (VuoCable *cable : internalPort->getConnectedCables())
					{
						if (cable->isPublished() && cable->getCompiler()->carriesData())
						{
							json_object *portObj = json_object_new_object();
							json_object *portNameOnMovedNode = json_object_new_string(internalPort->getClass()->getName().c_str());
							json_object_object_add(portObj, "copy", portNameOnMovedNode);
							json_object *portNameOnSubcompositionNode = json_object_new_string((r.compositionIdentifier + "/" + r.unqualifiedSubcompositionIdentifier + ":" + cable->getFromPort()->getClass()->getName()).c_str());
							json_object_object_add(portObj, "to", portNameOnSubcompositionNode);
							json_object_array_add(ports, portObj);
						}
					}
				}

				json_object_object_add(move, "ports", ports);
			}
		}
	}

	auto nodeReplacementExists = [this](const string &compositionIdentifier, const string &oldNodeIdentifier, const string &newNodeIdentifier)
	{
		NodeReplacement n;
		n.compositionIdentifier = compositionIdentifier;
		n.oldNodeIdentifier = oldNodeIdentifier;
		n.newNodeIdentifier = newNodeIdentifier;
		return nodeReplacements.find(n) != nodeReplacements.end();
	};

	// Add nodes that have the same identifier but different node classes to `nodeReplacements` (if not already there).

	for (map<string, pair<VuoNode *, VuoNode *> >::iterator i = oldAndNewNodeForIdentifier.begin(); i != oldAndNewNodeForIdentifier.end(); ++i)
		if (i->second.first && i->second.second && i->second.first->getNodeClass() != i->second.second->getNodeClass() &&
				! nodeReplacementExists(compositionIdentifier, i->first, i->first))
			addNodeReplacement(compositionIdentifier, i->first, i->first);

	// Add nodes whose node classes are being replaced to `nodeReplacements` (if not already there).

	for (map<string, pair<VuoNode *, VuoNode *> >::iterator i = oldAndNewNodeForIdentifier.begin(); i != oldAndNewNodeForIdentifier.end(); ++i)
	{
		if (i->second.first && i->second.second)
		{
			string nodeClassName = i->second.second->getNodeClass()->getClassName();
			for (const NodeClassReplacement &nodeClassReplacement : nodeClassReplacements)
			{
				if (nodeClassReplacement.nodeClassName == nodeClassName && ! nodeReplacementExists(compositionIdentifier, i->first, i->first))
				{
					addNodeReplacement(compositionIdentifier, i->first, i->first, nodeClassReplacement.oldAndNewPortNames);
					break;
				}
			}
		}
	}

	// Recursively diff nodes that are subcompositions.

	for (map<string, pair<VuoNode *, VuoNode *> >::iterator i = oldAndNewNodeForIdentifier.begin(); i != oldAndNewNodeForIdentifier.end(); ++i)
	{
		string nodeIdentifier;

		VuoCompilerComposition *newSubcomposition = NULL;
		if (i->second.second)
		{
			VuoCompilerNodeClass *newNodeClass = i->second.second->getNodeClass()->getCompiler();
			if (newNodeClass->isSubcomposition())
			{
				string newGraphvizDeclaration = newNodeClass->getSourceCode();
				newSubcomposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(newGraphvizDeclaration, compiler);

				nodeIdentifier = i->second.second->getCompiler()->getIdentifier();
			}
		}

		VuoCompilerComposition *oldSubcomposition = NULL;
		if (i->second.first)
		{
			string nodeClassName = i->second.first->getNodeClass()->getClassName();

			set<NodeClassReplacement>::iterator replacementIter;
			for (replacementIter = nodeClassReplacements.begin(); replacementIter != nodeClassReplacements.end(); ++replacementIter)
				if (replacementIter->nodeClassName == nodeClassName)
					break;

			if (replacementIter != nodeClassReplacements.end())
			{
				if (! replacementIter->oldSubcompositionSourceCode.empty())
				{
					oldSubcomposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(replacementIter->oldSubcompositionSourceCode, compiler);

					nodeIdentifier = i->second.first->getCompiler()->getIdentifier();
				}
			}
			else
				oldSubcomposition = newSubcomposition;
		}

		if (oldSubcomposition || newSubcomposition)
			diff(oldSubcomposition, newSubcomposition, compositionIdentifier, compositionPath, nodeIdentifier, compiler, diffJson);

		if (oldSubcomposition != newSubcomposition)
			delete oldSubcomposition;

		delete newSubcomposition;
	}

	// Diff nodes that are being replaced / replacing another.
	// For nodes that are subcompositions, this uses the port mappings added to `replacements` during the recursive calls.

	for (set<NodeReplacement>::iterator nodeReplacementIter = nodeReplacements.begin(); nodeReplacementIter != nodeReplacements.end(); ++nodeReplacementIter)
	{
		if (nodeReplacementIter->compositionIdentifier == compositionIdentifier)
		{
			// { "replace" : "<composition path>/<node identifier>", "with" : "<composition path>/<node identifier>",
			//   "ports" : [
			//          { "map" : "<port identifier>", "to" : "<port identifier>" },
			//          { "map" : "<port identifier>", "to" : "<port identifier>" },
			//          ... ] }

			map<string, VuoNode *>::iterator oldNodeIter = oldNodeForIdentifier.find(nodeReplacementIter->oldNodeIdentifier);
			if (oldNodeIter == oldNodeForIdentifier.end())
				continue;
			map<string, VuoNode *>::iterator newNodeIter = newNodeForIdentifier.find(nodeReplacementIter->newNodeIdentifier);
			if (newNodeIter == newNodeForIdentifier.end())
				continue;

			json_object *replaceObj = json_object_new_object();
			json_object *oldNodePath = json_object_new_string((compositionPath + "/" + nodeReplacementIter->oldNodeIdentifier).c_str());
			json_object_object_add(replaceObj, "replace", oldNodePath);
			json_object *newNodePath = json_object_new_string((compositionPath + "/" + nodeReplacementIter->newNodeIdentifier).c_str());
			json_object_object_add(replaceObj, "with", newNodePath);
			json_object *ports = json_object_new_array();

			map<string, string> oldAndNewPortNames = nodeReplacementIter->oldAndNewPortNames;

			if (nodeReplacementIter->shouldMapIdenticalPortNames)
			{
				vector<VuoPort *> oldInputPorts = oldNodeIter->second->getInputPorts();
				vector<VuoPort *> oldOutputPorts = oldNodeIter->second->getOutputPorts();
				vector<VuoPort *> newInputPorts = newNodeIter->second->getInputPorts();
				vector<VuoPort *> newOutputPorts = newNodeIter->second->getOutputPorts();
				for (vector<VuoPort *>::iterator oldPortIter = oldInputPorts.begin(); oldPortIter != oldInputPorts.end(); ++oldPortIter)
				{
					for (vector<VuoPort *>::iterator newPortIter = newInputPorts.begin(); newPortIter != newInputPorts.end(); ++newPortIter)
					{
						if (VuoCompilerComposition::portsMatch(*oldPortIter, *newPortIter))
						{
							string oldPortName = (*oldPortIter)->getClass()->getName();
							string newPortName = (*newPortIter)->getClass()->getName();
							oldAndNewPortNames[oldPortName] = newPortName;
							break;
						}
					}
				}
				for (vector<VuoPort *>::iterator oldPortIter = oldOutputPorts.begin(); oldPortIter != oldOutputPorts.end(); ++oldPortIter)
				{
					for (vector<VuoPort *>::iterator newPortIter = newOutputPorts.begin(); newPortIter != newOutputPorts.end(); ++newPortIter)
					{
						if (VuoCompilerComposition::portsMatch(*oldPortIter, *newPortIter))
						{
							string oldPortName = (*oldPortIter)->getClass()->getName();
							string newPortName = (*newPortIter)->getClass()->getName();
							oldAndNewPortNames[oldPortName] = newPortName;
							break;
						}
					}
				}
			}

			for (map<string, string>::const_iterator portMapIter = oldAndNewPortNames.begin(); portMapIter != oldAndNewPortNames.end(); ++portMapIter)
			{
				json_object *portObj = json_object_new_object();
				json_object *oldPortName = json_object_new_string(portMapIter->first.c_str());
				json_object_object_add(portObj, "map", oldPortName);
				json_object *newPortName = json_object_new_string(portMapIter->second.c_str());
				json_object_object_add(portObj, "to", newPortName);
				json_object_array_add(ports, portObj);
			}

			json_object_object_add(replaceObj, "ports", ports);
			json_object_array_add(diffJson, replaceObj);
		}
	}

	// Diff published ports.

	bool publishedPortsChanged[2] = { false, false };
	vector<VuoPublishedPort *> oldPublishedPorts[2];
	vector<VuoPublishedPort *> newPublishedPorts[2];
	string nodeIdentifier[2];
	map<string, string> oldAndNewPublishedPortNames[2];

	for (int i = 0; i < 2; ++i)
	{
		if (i == 0)
		{
			if (oldComposition)
				oldPublishedPorts[i] = oldComposition->getBase()->getPublishedInputPorts();
			if (newComposition)
				newPublishedPorts[i] = newComposition->getBase()->getPublishedInputPorts();
			nodeIdentifier[i] = "PublishedInputs";
		}
		else
		{
			if (oldComposition)
				oldPublishedPorts[i] = oldComposition->getBase()->getPublishedOutputPorts();
			if (newComposition)
				newPublishedPorts[i] = newComposition->getBase()->getPublishedOutputPorts();
			nodeIdentifier[i] = "PublishedOutputs";
		}

		for (size_t j = 0; j < oldPublishedPorts[i].size() && j < newPublishedPorts[i].size(); ++j)
		{
			VuoPublishedPort *oldPort = oldPublishedPorts[i][j];
			VuoPublishedPort *newPort = newPublishedPorts[i][j];

			if (VuoCompilerComposition::portsMatch(oldPort, newPort))
			{
				string oldPortName = oldPort->getClass()->getName();
				string newPortName = newPort->getClass()->getName();
				oldAndNewPublishedPortNames[i][oldPortName] = newPortName;
			}
			else
				publishedPortsChanged[i] = true;
		}

		if (oldPublishedPorts[i].size() != newPublishedPorts[i].size())
			publishedPortsChanged[i] = true;
	}

	if (publishedPortsChanged[0] || publishedPortsChanged[1])
	{
		bool addBoth = oldPublishedPorts[0].empty() && oldPublishedPorts[1].empty();
		bool removeBoth = newPublishedPorts[0].empty() && newPublishedPorts[1].empty();

		for (int i = 0; i < 2; ++i)
		{
			if (removeBoth)
			{
				// { "remove" : "<composition path>/PublishedInputs" }
				json_object *remove = json_object_new_object();
				json_object *nodePath = json_object_new_string((compositionPath + "/" + nodeIdentifier[i]).c_str());
				json_object_object_add(remove, "remove", nodePath);
				json_object_array_add(diffJson, remove);

				if (i == 0)
				{
					json_object *remove = json_object_new_object();
					json_object *nodePath = json_object_new_string((compositionPath + "/" + "PublishedInputsTrigger").c_str());
					json_object_object_add(remove, "remove", nodePath);
					json_object_array_add(diffJson, remove);
				}
			}
			else if (addBoth)
			{
				// { "add" : "<composition path>/PublishedInputs" }
				json_object *add = json_object_new_object();
				json_object *nodePath = json_object_new_string((compositionPath + "/" + nodeIdentifier[i]).c_str());
				json_object_object_add(add, "add", nodePath);
				json_object_array_add(diffJson, add);

				if (i == 0)
				{
					json_object *add = json_object_new_object();
					json_object *nodePath = json_object_new_string((compositionPath + "/" + "PublishedInputsTrigger").c_str());
					json_object_object_add(add, "add", nodePath);
					json_object_array_add(diffJson, add);
				}
			}
			else if (publishedPortsChanged[i])
			{
				// { "replace" : "<composition path>/PublishedInputs", "with" : "<composition path>/PublishedInputs",
				//   "ports" : [
				//			{ "map" : "<published port name>", "to" : "<published port name>" },
				//			{ "map" : "<published port name>", "to" : "<published port name>" },
				//			... ] }
				json_object *replaceObj = json_object_new_object();
				json_object *nodePath = json_object_new_string((compositionPath + "/" + nodeIdentifier[i]).c_str());
				json_object_get(nodePath);
				json_object_object_add(replaceObj, "replace", nodePath);
				json_object_object_add(replaceObj, "with", nodePath);
				json_object *ports = json_object_new_array();
				for (map<string, string>::iterator j = oldAndNewPublishedPortNames[i].begin(); j != oldAndNewPublishedPortNames[i].end(); ++j)
				{
					json_object *portObj = json_object_new_object();
					json_object *oldPublishedPortName = json_object_new_string(j->first.c_str());
					json_object_object_add(portObj, "map", oldPublishedPortName);
					json_object *newPublishedPortName = json_object_new_string(j->second.c_str());
					json_object_object_add(portObj, "to", newPublishedPortName);
					json_object_array_add(ports, portObj);
				}
				json_object_object_add(replaceObj, "ports", ports);
				json_object_array_add(diffJson, replaceObj);
			}
		}
	}

	if (! parentCompositionIdentifier.empty())
	{
		for (set<NodeReplacement>::iterator i = nodeReplacements.begin(); i != nodeReplacements.end(); ++i)
		{
			if (i->compositionIdentifier == parentCompositionIdentifier &&
					i->oldNodeIdentifier == unqualifiedCompositionIdentifier && i->newNodeIdentifier == unqualifiedCompositionIdentifier)
			{
				NodeReplacement n = *i;

				for (int j = 0; j < 2; ++j)
					n.oldAndNewPortNames.insert(oldAndNewPublishedPortNames[j].begin(), oldAndNewPublishedPortNames[j].end());

				n.shouldMapIdenticalPortNames = false;

				nodeReplacements.erase(i);
				nodeReplacements.insert(n);
				break;
			}
		}
	}
}

/**
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerCompositionDiff::NodeReplacement &lhs, const VuoCompilerCompositionDiff::NodeReplacement &rhs)
{
	return (lhs.compositionIdentifier != rhs.compositionIdentifier ?
											 lhs.compositionIdentifier < rhs.compositionIdentifier :
											 (lhs.oldNodeIdentifier != rhs.oldNodeIdentifier ?
																		   lhs.oldNodeIdentifier < rhs.oldNodeIdentifier :
																		   lhs.newNodeIdentifier < rhs.newNodeIdentifier));
}

/**
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerCompositionDiff::NodeClassReplacement &lhs, const VuoCompilerCompositionDiff::NodeClassReplacement &rhs)
{
	return (lhs.nodeClassName < rhs.nodeClassName);
}

/**
 * Needed so this type can be used in STL containers.
 */
bool operator<(const VuoCompilerCompositionDiff::Refactoring &lhs, const VuoCompilerCompositionDiff::Refactoring &rhs)
{
	return (lhs.compositionIdentifier != rhs.compositionIdentifier ?
											 lhs.compositionIdentifier < rhs.compositionIdentifier :
											 lhs.unqualifiedSubcompositionIdentifier < rhs.unqualifiedSubcompositionIdentifier);
}

/**
 * Adds a mapping for a node replaced in the top-level composition.
 * Ports in the old node are mapped to identically-named ports in the new node.
 */
void VuoCompilerCompositionDiff::addNodeReplacementInTopLevelComposition(const string &oldNodeIdentifier, const string &newNodeIdentifier)
{
	addNodeReplacement(VuoCompilerComposition::topLevelCompositionIdentifier, oldNodeIdentifier, newNodeIdentifier);
}

/**
 * Adds a mapping for a node replaced in the top-level composition.
 */
void VuoCompilerCompositionDiff::addNodeReplacementInTopLevelComposition(const string &oldNodeIdentifier, const string &newNodeIdentifier,
																		 const map<string, string> &oldAndNewPortNames)
{
	addNodeReplacement(VuoCompilerComposition::topLevelCompositionIdentifier, oldNodeIdentifier, newNodeIdentifier, oldAndNewPortNames);
}

/**
 * Adds a mapping for a node replaced in a composition.
 * Ports in the old node are mapped to identically-named ports in the new node.
 */
void VuoCompilerCompositionDiff::addNodeReplacement(const string &compositionIdentifier, const string &oldNodeIdentifier, const string &newNodeIdentifier)
{
	NodeReplacement n;
	n.compositionIdentifier = compositionIdentifier;
	n.oldNodeIdentifier = oldNodeIdentifier;
	n.newNodeIdentifier = newNodeIdentifier;
	n.shouldMapIdenticalPortNames = true;
	nodeReplacements.insert(n);
}

/**
 * Adds a mapping for a node replaced in a composition.
 */
void VuoCompilerCompositionDiff::addNodeReplacement(const string &compositionIdentifier, const string &oldNodeIdentifier, const string &newNodeIdentifier,
													const map<string, string> &oldAndNewPortNames)
{
	NodeReplacement n;
	n.compositionIdentifier = compositionIdentifier;
	n.oldNodeIdentifier = oldNodeIdentifier;
	n.newNodeIdentifier = newNodeIdentifier;
	n.oldAndNewPortNames = oldAndNewPortNames;
	n.shouldMapIdenticalPortNames = false;
	nodeReplacements.insert(n);
}

/**
 * Adds a mapping for a node class replaced with a new implementation.
 *
 * This class does not keep references to @a oldNodeClass and @a newNodeClass. It's safe to delete them after calling this function.
 */
void VuoCompilerCompositionDiff::addNodeClassReplacement(VuoCompilerNodeClass *oldNodeClass, VuoCompilerNodeClass *newNodeClass)
{
	NodeClassReplacement n;
	n.nodeClassName = oldNodeClass->getBase()->getClassName();

	auto makeOldAndNewPortNames = [&n] (const vector<VuoPortClass *> &oldPortClasses, const vector<VuoPortClass *> &newPortClasses)
	{
		for (VuoPortClass *oldPortClass : oldPortClasses)
		{
			for (VuoPortClass *newPortClass : newPortClasses)
			{
				if (VuoCompilerComposition::portClassesMatch(oldPortClass, newPortClass))
				{
					n.oldAndNewPortNames[oldPortClass->getName()] = newPortClass->getName();
					break;
				}
			}
		}
	};
	vector<VuoPortClass *> oldInputPorts = oldNodeClass->getBase()->getInputPortClasses();
	vector<VuoPortClass *> oldOutputPorts = oldNodeClass->getBase()->getOutputPortClasses();
	vector<VuoPortClass *> newInputPorts = newNodeClass->getBase()->getInputPortClasses();
	vector<VuoPortClass *> newOutputPorts = newNodeClass->getBase()->getOutputPortClasses();
	makeOldAndNewPortNames(oldInputPorts, newInputPorts);
	makeOldAndNewPortNames(oldOutputPorts, newOutputPorts);

	if (oldNodeClass->isSubcomposition())
		n.oldSubcompositionSourceCode = oldNodeClass->getSourceCode();

	nodeClassReplacements.insert(n);
}

/**
 * Adds a mapping for a module replaced with a new implementation. If not calling @ref addNodeClassReplacement, call this.
 */
void VuoCompilerCompositionDiff::addModuleReplacement(const string &moduleKey)
{
	moduleReplacements.insert(moduleKey);
}

/**
 * Adds a mapping for nodes refactored into a subcomposition.
 *
 * @param compositionIdentifier The composition containing the nodes in the old composition.
 * @param nodesMoved The nodes factored out, moved inside a subcomposition in the new composition.
 * @param subcompositionMovedTo The subcomposition in the new composition.
 */
void VuoCompilerCompositionDiff::addRefactoring(const string &compositionIdentifier, const set<VuoCompilerNode *> &nodesMoved, VuoCompilerNode *subcompositionMovedTo)
{
	Refactoring r;

	r.compositionIdentifier = compositionIdentifier;
	r.unqualifiedSubcompositionIdentifier = subcompositionMovedTo->getIdentifier();

	for (VuoCompilerNode *node : nodesMoved)
		r.nodeIdentifiers.insert(node->getIdentifier());

	refactorings.insert(r);
}

/**
 * Returns the names of all node classes and other modules being replaced with a new implementation.
 */
set<string> VuoCompilerCompositionDiff::getModuleKeysReplaced(void) const
{
	set<string> moduleKeys;

	for (const NodeClassReplacement &ncr : nodeClassReplacements)
		moduleKeys.insert(ncr.nodeClassName);

	for (const string &moduleKey : moduleReplacements)
		moduleKeys.insert(moduleKey);

	return moduleKeys;
}

/**
 * Returns true if @a oldNodeIdentifier is the `oldNodeIdentifier` of one of the node replacements.
 */
bool VuoCompilerCompositionDiff::isNodeBeingReplaced(const string &compositionIdentifier, const string &oldNodeIdentifier) const
{
	for (set<NodeReplacement>::const_iterator i = nodeReplacements.begin(); i != nodeReplacements.end(); ++i)
		if ((*i).compositionIdentifier == compositionIdentifier && (*i).oldNodeIdentifier == oldNodeIdentifier)
			return true;

	return false;
}

/**
 * Returns true if @a newNodeIdentifier is the `newNodeIdentifier` of one of the node replacements.
 */
bool VuoCompilerCompositionDiff::isNodeReplacingAnother(const string &compositionIdentifier, const string &newNodeIdentifier) const
{
	for (set<NodeReplacement>::const_iterator i = nodeReplacements.begin(); i != nodeReplacements.end(); ++i)
		if ((*i).compositionIdentifier == compositionIdentifier && (*i).newNodeIdentifier == newNodeIdentifier)
			return true;

	return false;
}

/**
 * Returns true if @a nodeIdentifier is one of the nodes being factored out of @a compositionIdentifier.
 */
bool VuoCompilerCompositionDiff::isNodeBeingRefactored(const string &parentCompositionIdentifier, const string &compositionIdentifier, const string &nodeIdentifier) const
{
	for (const Refactoring &r : refactorings)
		if ((r.compositionIdentifier == compositionIdentifier && r.nodeIdentifiers.find(nodeIdentifier) != r.nodeIdentifiers.end()) ||
				(r.compositionIdentifier == parentCompositionIdentifier && r.unqualifiedSubcompositionIdentifier == compositionIdentifier && r.nodeIdentifiers.find(nodeIdentifier) != r.nodeIdentifiers.end()))
			return true;

	return false;
}
