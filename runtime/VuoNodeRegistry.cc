/**
 * @file
 * VuoNodeRegistry implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoNodeRegistry.hh"

#include <dlfcn.h>
#include <sstream>
#include <stdexcept>
#include "VuoCompositionDiff.hh"
#include "VuoRuntimePersistentState.hh"
#include "VuoRuntimeState.hh"
#include "VuoRuntimeUtilities.hh"

const unsigned long VuoNodeRegistry::topLevelCompositionIndex = ULONG_MAX;
const unsigned long VuoNodeRegistry::invalidCompositionIndex = ULONG_MAX - 1;

/**
 * Constructor. Does not take ownership of @a persistentState.
 */
VuoNodeRegistry::VuoNodeRegistry(VuoRuntimePersistentState *persistentState)
{
	this->persistentState = persistentState;
	vuoTopLevelCompositionIdentifier = NULL;
}

/**
 * Updates references to symbols defined in the composition's generated code.
 *
 * @throw std::runtime_error One of the symbols was not found in the composition binary.
 */
void VuoNodeRegistry::updateCompositionSymbols(void *compositionBinaryHandle)
{
	ostringstream errorMessage;

	vuoTopLevelCompositionIdentifierType *vuoTopLevelCompositionIdentifierPtr = (vuoTopLevelCompositionIdentifierType *) dlsym(compositionBinaryHandle, "vuoTopLevelCompositionIdentifier");
	if (! vuoTopLevelCompositionIdentifierPtr)
	{
		errorMessage << "The composition couldn't be started because its vuoTopLevelCompositionIdentifier variable couldn't be found : " << dlerror();
		throw std::runtime_error(errorMessage.str());
	}
	vuoTopLevelCompositionIdentifier = *vuoTopLevelCompositionIdentifierPtr;
}

/**
 * If @a compositionIdentifier refers to a subcomposition, outputs the identifier of the parent composition
 * and the subcomposition node within the parent composition. Otherwise, outputs @a compositionIdentifier as
 * the parent composition identifier.
 *
 * This needs to be kept in sync with `VuoCompilerNode::generateSubcompositionIdentifierValue()`.
 */
void VuoNodeRegistry::splitCompositionIdentifier(const string &compositionIdentifier, string &parentCompositionIdentifier, string &nodeIdentifier)
{
	size_t pos = compositionIdentifier.rfind("__");
	if (pos != string::npos)
	{
		parentCompositionIdentifier = compositionIdentifier.substr(0, pos);
		nodeIdentifier = compositionIdentifier.substr(pos + strlen("__"));
	}
	else
	{
		parentCompositionIdentifier = compositionIdentifier;
		nodeIdentifier = "";
	}
}

/**
 * Constructs the composition identifier for a subcomposition.
 */
string VuoNodeRegistry::joinCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier)
{
	return parentCompositionIdentifier + "__" + nodeIdentifier;
}

/**
 * Registers metadata for a node. When a (sub)composition is added or recompiled, this function should be
 * called for each node in the composition, in the same order as `VuoCompilerBitcodeGenerator::orderedNodes`.
 */
void VuoNodeRegistry::addNodeMetadata(const char *compositionIdentifier, const char *nodeIdentifier)
{
	NodeMetadata nodeMetadata = { nodeIdentifier };
	nodeMetadatas[compositionIdentifier].push_back(nodeMetadata);
}

/**
 * Registers metadata for a port. After vuoAddNodeMetadata() is called for a node, this function should be
 * called for each port on the node, in the same order that ports are added to `NodeContext.portContexts`.
 */
void VuoNodeRegistry::addPortMetadata(const char *compositionIdentifier, const char *portIdentifier, const char *portName,
					 unsigned long typeIndex, const char *initialValue)
{
	PortMetadata portMetadata = { portIdentifier, portName, typeIndex, initialValue };
	nodeMetadatas[compositionIdentifier].back().portMetadatas.push_back(portMetadata);
}

/**
 * Looks up the node identifier for the node with the given index.
 */
string VuoNodeRegistry::getNodeIdentifierForIndex(const char *compositionIdentifier, unsigned long nodeIndex)
{
	if (nodeIndex == topLevelCompositionIndex)
		return "";

	map<string, vector<NodeMetadata> >::iterator iter1 = nodeMetadatas.find(compositionIdentifier);
	if (iter1 != nodeMetadatas.end())
		if (nodeIndex < iter1->second.size())
			return iter1->second[nodeIndex].identifier;

	VUserLog("Couldn't find identifier for node %s:%lu", compositionIdentifier, nodeIndex);
	return "";
}

/**
 * Looks up the node index for the node with the given identifier.
 */
unsigned long VuoNodeRegistry::getNodeIndexForIdentifier(const string &compositionIdentifier, const string &nodeIdentifier)
{
	if (nodeIdentifier.empty())
		return topLevelCompositionIndex;

	map<string, vector<NodeMetadata> >::iterator iter1 = nodeMetadatas.find(compositionIdentifier);
	if (iter1 != nodeMetadatas.end())
		for (unsigned long i = 0; i < iter1->second.size(); ++i)
			if (iter1->second[i].identifier == nodeIdentifier)
				return i;

	VUserLog("Couldn't find index for node %s:%s", compositionIdentifier.c_str(), nodeIdentifier.c_str());
	return invalidCompositionIndex;
}

/**
 * Looks up the composition identifier that corresponds to the given hash.
 */
string VuoNodeRegistry::getCompositionIdentifierForHash(unsigned long compositionIdentifierHash)
{
	map<unsigned long, string>::iterator iter1 = compositionIdentifierForHash.find(compositionIdentifierHash);
	if (iter1 != compositionIdentifierForHash.end())
		return iter1->second;

	VUserLog("Couldn't find composition identifier for hash %lu", compositionIdentifierHash);
	return "";
}

/**
 * Registers a node context.
 */
void VuoNodeRegistry::addNodeContext(const char *compositionIdentifier, unsigned long nodeIndex, NodeContext *nodeContext)
{
	unsigned long compositionIdentifierHash = VuoRuntimeUtilities::hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
			VUserLog("Context overwritten for node %s:%s", compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
	}

	nodeContextForIndex[compositionIdentifierHash][nodeIndex] = nodeContext;

	compositionIdentifierForHash[ VuoRuntimeUtilities::hash(compositionIdentifier) ] = compositionIdentifier;
}

/**
 * Un-registers a node context.
 */
void VuoNodeRegistry::removeNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	unsigned long compositionIdentifierHash = VuoRuntimeUtilities::hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
		{
			iter1->second.erase(iter2);
			if (iter1->second.empty())
				nodeContextForIndex.erase(iter1);

			string nodeIdentifier = getNodeIdentifierForIndex(compositionIdentifier, nodeIndex);
			string subcompositionIdentifier = joinCompositionIdentifier(compositionIdentifier, nodeIdentifier);
			unsigned long subcompositionIdentifierHash = VuoRuntimeUtilities::hash(subcompositionIdentifier.c_str());
			map<unsigned long, string>::iterator ciIter = compositionIdentifierForHash.find(subcompositionIdentifierHash);
			if (ciIter != compositionIdentifierForHash.end())
				compositionIdentifierForHash.erase(ciIter);

			return;
		}
	}

	VUserLog("Couldn't find context for node %s:%s", compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
}

/**
 * Preserves all remaining node contexts to be accessed after a live-coding reload, and clears the
 * list of registered node contexts.
 */
void VuoNodeRegistry::relocateAllNodeContexts(void)
{
	for (map<unsigned long, map<unsigned long, NodeContext *> >::iterator i = nodeContextForIndex.begin(); i != nodeContextForIndex.end(); ++i)
	{
		unsigned long compositionIdentifierHash = i->first;
		string compositionIdentifier = getCompositionIdentifierForHash(compositionIdentifierHash);

		map<string, NodeContext *> nodeContextForIdentifier;
		for (map<unsigned long, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			string nodeIdentifier = getNodeIdentifierForIndex(compositionIdentifier.c_str(), j->first);
			nodeContextForIdentifier[nodeIdentifier] = j->second;
		}

		carriedOverNodeContextForIdentifier[compositionIdentifier] = nodeContextForIdentifier;
	}

	nodeContextForIndex.clear();
}

/**
 * Carries over a node context from before a live-coding reload — registering it with the new node contexts
 * and removing it from the list of carried-over node contexts. If the node is a subcomposition, this function
 * also carries over the node contexts of all nodes within the subcomposition.
 */
NodeContext * VuoNodeRegistry::carryOverNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	NodeContext *nodeContext = NULL;
	string nodeIdentifier = getNodeIdentifierForIndex(compositionIdentifier, nodeIndex);

	{
		bool found = false;
		map<string, map<string, NodeContext *> >::iterator coIter1 = carriedOverNodeContextForIdentifier.find(compositionIdentifier);
		if (coIter1 != carriedOverNodeContextForIdentifier.end())
		{
			map<string, NodeContext *>::iterator coIter2 = coIter1->second.find(nodeIdentifier);
			if (coIter2 != coIter1->second.end())
			{
				nodeContext = coIter2->second;
				coIter1->second.erase(coIter2);
				addNodeContext(compositionIdentifier, nodeIndex, nodeContext);
				found = true;
			}
		}

		if (! found)
			VUserLog("Couldn't find context for node %s:%s", compositionIdentifier, nodeIdentifier.c_str());
	}

	if (! nodeIdentifier.empty())
	{
		string subcompositionIdentifier = joinCompositionIdentifier(compositionIdentifier, nodeIdentifier);
		vector<string> compositionIdentifiersToErase;
		for (map<string, map<string, NodeContext *> >::iterator i = carriedOverNodeContextForIdentifier.begin(); i != carriedOverNodeContextForIdentifier.end(); ++i)
		{
			string currCompositionIdentifier = i->first;
			unsigned long currCompositionIdentifierHash = VuoRuntimeUtilities::hash(currCompositionIdentifier.c_str());

			if (currCompositionIdentifier.substr(0, subcompositionIdentifier.length()) == subcompositionIdentifier)
			{
				map<unsigned long, NodeContext *> currNodeContextForIndex;
				for (map<string, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					unsigned long currNodeIndex = getNodeIndexForIdentifier(currCompositionIdentifier, j->first);
					currNodeContextForIndex[currNodeIndex] = j->second;
				}
				nodeContextForIndex[currCompositionIdentifierHash] = currNodeContextForIndex;

				compositionIdentifiersToErase.push_back(currCompositionIdentifier);
			}
		}

		for (vector<string>::iterator i = compositionIdentifiersToErase.begin(); i != compositionIdentifiersToErase.end(); ++i)
			carriedOverNodeContextForIdentifier.erase(*i);
	}

	return nodeContext;
}

/**
 * Removes the node from the list of carried-over node contexts.
 */
void VuoNodeRegistry::removeCarriedOverNodeContext(const char *compositionIdentifier, const string &nodeIdentifier)
{
	carriedOverNodeContextForIdentifier[compositionIdentifier].erase(nodeIdentifier);
}

/**
 * Removes all remaining items from the list of carried-over node contexts.
 */
void VuoNodeRegistry::removeAllCarriedOverNodeContexts(void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *))
{
	for (map<string, map<string, NodeContext *> >::iterator i = carriedOverNodeContextForIdentifier.begin(); i != carriedOverNodeContextForIdentifier.end(); ++i)
	{
		for (map<string, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			VuoCompositionState compositionState = { persistentState->runtimeState, i->first.c_str() };
			compositionDestroyNodeContext(&compositionState, j->first.c_str(), j->second);
		}
	}

	carriedOverNodeContextForIdentifier.clear();
}

/**
 * Returns the node context registered for the node index, or null if none is found.
 */
NodeContext * VuoNodeRegistry::getNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	unsigned long compositionIdentifierHash = VuoRuntimeUtilities::hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
			return iter2->second;
	}

	VUserLog("Couldn't find context for node %s:%s", compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
	return NULL;
}

/**
 * Returns the node context registered for the composition (top-level or subcomposition), or null if none is found.
 */
NodeContext * VuoNodeRegistry::getCompositionContext(const char *compositionIdentifier)
{
	string parentCompositionIdentifier, subcompositionNodeIdentifier;
	splitCompositionIdentifier(compositionIdentifier, parentCompositionIdentifier, subcompositionNodeIdentifier);
	unsigned long subcompositionNodeIndex = getNodeIndexForIdentifier(parentCompositionIdentifier, subcompositionNodeIdentifier);
	return getNodeContext(parentCompositionIdentifier.c_str(), subcompositionNodeIndex);
}


/**
 * Returns the `data` field in a port's context, given the port's identifier.
 */
void * VuoNodeRegistry::getDataForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, void *> >::iterator compIter = dataForPort.find(compositionIdentifier);
	if (compIter != dataForPort.end())
	{
		map<string, void *>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find data for port %s:%s", compositionIdentifier, portIdentifier);
	return NULL;
}

/**
 * Returns the `semaphore` field in a node's context, given the identifier of a port on the node.
 */
dispatch_semaphore_t VuoNodeRegistry::getNodeSemaphoreForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, dispatch_semaphore_t> >::iterator compIter = nodeSemaphoreForPort.find(compositionIdentifier);
	if (compIter != nodeSemaphoreForPort.end())
	{
		map<string, dispatch_semaphore_t>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find node semaphore for port %s:%s", compositionIdentifier, portIdentifier);
	return NULL;
}

/**
 * Returns the numerical index for a node, given the identifier of a port on the node.
 */
unsigned long VuoNodeRegistry::getNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, unsigned long> >::iterator compIter = nodeIndexForPort.find(compositionIdentifier);
	if (compIter != nodeIndexForPort.end())
	{
		map<string, unsigned long>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find node index for port %s:%s", compositionIdentifier, portIdentifier);
	return 0;
}

/**
 * Returns the numerical index for a port's type, given the port's identifier.
 */
unsigned long VuoNodeRegistry::getTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, map<string, unsigned long> >::iterator compIter = typeIndexForPort.find(compositionIdentifier);
	if (compIter != typeIndexForPort.end())
	{
		map<string, unsigned long>::iterator portIter = compIter->second.find(portIdentifier);
		if (portIter != compIter->second.end())
			return portIter->second;
	}

	VUserLog("Couldn't find type index for port %s:%s", compositionIdentifier, portIdentifier);
	return 0;
}

/**
 * Caches information about the port so it can be efficiently retrieved later.
 */
void VuoNodeRegistry::addPortIdentifier(const char *compositionIdentifier, const string &portIdentifier,
										void *data, dispatch_semaphore_t nodeSemaphore, unsigned long nodeIndex, unsigned long typeIndex)
{
	map<string, map<string, void *> >::iterator iter1 = dataForPort.find(compositionIdentifier);
	if (iter1 != dataForPort.end())
	{
		map<string, void *>::iterator iter2 = iter1->second.find(portIdentifier);
		if (iter2 != iter1->second.end())
			VUserLog("Cache overwritten for port %s:%s", compositionIdentifier, portIdentifier.c_str());
	}

	dataForPort[compositionIdentifier][portIdentifier] = data;
	nodeSemaphoreForPort[compositionIdentifier][portIdentifier] = nodeSemaphore;
	nodeIndexForPort[compositionIdentifier][portIdentifier] = nodeIndex;
	typeIndexForPort[compositionIdentifier][portIdentifier] = typeIndex;
}

/**
 * Removes the port from the port cache.
 */
void VuoNodeRegistry::removePortIdentifier(const char *compositionIdentifier, const string &portIdentifier)
{
	{
		bool found = false;
		map<string, map<string, void *> >::iterator iter1 = dataForPort.find(compositionIdentifier);
		if (iter1 != dataForPort.end())
		{
			map<string, void *>::iterator iter2 = iter1->second.find(portIdentifier);
			if (iter2 != iter1->second.end())
			{
				iter1->second.erase(iter2);
				found = true;
			}
		}

		if (! found)
			VUserLog("Couldn't find data for port %s:%s", compositionIdentifier, portIdentifier.c_str());
	}
	{
		bool found = false;
		map<string, map<string, dispatch_semaphore_t> >::iterator iter1 = nodeSemaphoreForPort.find(compositionIdentifier);
		if (iter1 != nodeSemaphoreForPort.end())
		{
			map<string, dispatch_semaphore_t>::iterator iter2 = iter1->second.find(portIdentifier);
			if (iter2 != iter1->second.end())
			{
				iter1->second.erase(iter2);
				found = true;
			}
		}

		if (! found)
			VUserLog("Couldn't find node semaphore for port %s:%s", compositionIdentifier, portIdentifier.c_str());
	}
	{
		bool found = false;
		map<string, map<string, unsigned long> >::iterator iter1 = nodeIndexForPort.find(compositionIdentifier);
		if (iter1 != nodeIndexForPort.end())
		{
			map<string, unsigned long>::iterator iter2 = iter1->second.find(portIdentifier);
			if (iter2 != iter1->second.end())
			{
				iter1->second.erase(iter2);
				found = true;
			}
		}

		if (! found)
			VUserLog("Couldn't find node index for port %s:%s", compositionIdentifier, portIdentifier.c_str());
	}
	{
		bool found = false;
		map<string, map<string, unsigned long> >::iterator iter1 = typeIndexForPort.find(compositionIdentifier);
		if (iter1 != typeIndexForPort.end())
		{
			map<string, unsigned long>::iterator iter2 = iter1->second.find(portIdentifier);
			if (iter2 != iter1->second.end())
			{
				iter1->second.erase(iter2);
				found = true;
			}
		}

		if (! found)
			VUserLog("Couldn't find type index for port %s:%s", compositionIdentifier, portIdentifier.c_str());
	}
}

/**
 * Preserves all remaining port cache entries to be accessed after a live-coding reload, and clears the port cache.
 */
void VuoNodeRegistry::relocateAllPortIdentifiers(void)
{
	carriedOverDataForPort = dataForPort;
	carriedOverNodeSemaphoreForPort = nodeSemaphoreForPort;
	carriedOverNodeIndexForPort = nodeIndexForPort;
	carriedOverTypeIndexForPort = typeIndexForPort;

	dataForPort.clear();
	nodeSemaphoreForPort.clear();
	nodeIndexForPort.clear();
	typeIndexForPort.clear();
}

/**
 * Carries over a port's cache entry from before a live-coding reload — transferring the entry to the new
 * port cache and removing it from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::carryOverPortIdentifier(const char *compositionIdentifier, const string &portIdentifier,
											  unsigned long nodeIndex, unsigned long typeIndex)
{
	bool found = false;
	map<string, map<string, void *> >::iterator iter1 = carriedOverDataForPort.find(compositionIdentifier);
	if (iter1 != carriedOverDataForPort.end())
	{
		map<string, void *>::iterator iter2 = iter1->second.find(portIdentifier);
		if (iter2 != iter1->second.end())
			found = true;
	}

	if (! found)
	{
		VUserLog("Couldn't find cache for port %s:%s", compositionIdentifier, portIdentifier.c_str());
		return;
	}

	dataForPort[compositionIdentifier][portIdentifier] = carriedOverDataForPort[compositionIdentifier][portIdentifier];
	nodeSemaphoreForPort[compositionIdentifier][portIdentifier] = carriedOverNodeSemaphoreForPort[compositionIdentifier][portIdentifier];
	nodeIndexForPort[compositionIdentifier][portIdentifier] = nodeIndex;
	typeIndexForPort[compositionIdentifier][portIdentifier] = typeIndex;

	carriedOverDataForPort[compositionIdentifier].erase(portIdentifier);
	carriedOverNodeSemaphoreForPort[compositionIdentifier].erase(portIdentifier);
	carriedOverNodeIndexForPort[compositionIdentifier].erase(portIdentifier);
	carriedOverTypeIndexForPort[compositionIdentifier].erase(portIdentifier);
}

/**
 * Carries over all of the node's port cache entries from before a live-coding reload — transferring the entries
 * to the new port cache and removing them from the list of carried-over port cache entries. If the node is a
 * subcomposition, this function also carries over the port cache entries for all nodes within the subcomposition.
 */
void VuoNodeRegistry::carryOverPortIdentifiersForNode(const char *compositionIdentifier, const string &nodeIdentifier, unsigned long nodeIndex,
													  const vector<string> &portIdentifiers, const vector<unsigned long> typeIndexes)
{
	for (size_t j = 0; j < portIdentifiers.size(); ++j)
		carryOverPortIdentifier(compositionIdentifier, portIdentifiers[j], nodeIndex, typeIndexes[j]);

	string subcompositionIdentifier = joinCompositionIdentifier(compositionIdentifier, nodeIdentifier);
	vector<string> compositionIdentifiersToErase;
	for (map<string, map<string, void *> >::iterator i = carriedOverDataForPort.begin(); i != carriedOverDataForPort.end(); ++i)
	{
		string currCompositionIdentifier = i->first;
		if (currCompositionIdentifier.substr(0, subcompositionIdentifier.length()) == subcompositionIdentifier)
		{
			dataForPort[currCompositionIdentifier] = carriedOverDataForPort[currCompositionIdentifier];
			nodeSemaphoreForPort[currCompositionIdentifier] = carriedOverNodeSemaphoreForPort[currCompositionIdentifier];
			nodeIndexForPort[currCompositionIdentifier] = carriedOverNodeIndexForPort[currCompositionIdentifier];
			typeIndexForPort[currCompositionIdentifier] = carriedOverTypeIndexForPort[currCompositionIdentifier];

			compositionIdentifiersToErase.push_back(currCompositionIdentifier);
		}
	}

	for (vector<string>::iterator i = compositionIdentifiersToErase.begin(); i != compositionIdentifiersToErase.end(); ++i)
	{
		string currCompositionIdentifier = *i;
		carriedOverDataForPort.erase(currCompositionIdentifier);
		carriedOverNodeSemaphoreForPort.erase(currCompositionIdentifier);
		carriedOverNodeIndexForPort.erase(currCompositionIdentifier);
		carriedOverTypeIndexForPort.erase(currCompositionIdentifier);
	}
}

/**
 * Removes the port from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::removeCarriedOverPortIdentifier(const char *compositionIdentifier, const string &oldPortIdentifier)
{
	carriedOverDataForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverNodeSemaphoreForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverNodeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverTypeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);
}

/**
 * Removes all remaining items from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::removeAllCarriedOverPortIdentifiers(void)
{
	carriedOverDataForPort.clear();
	carriedOverNodeSemaphoreForPort.clear();
	carriedOverNodeIndexForPort.clear();
	carriedOverTypeIndexForPort.clear();
}

/**
 * For a node being replaced across a live-coding reload, transfers port data from a port on the old node
 * to the corresponding port on the new node (by copying the port data's heap address from the old PortContext
 * to the new PortContext).
 */
void VuoNodeRegistry::carryOverPortData(const char *compositionIdentifier, const string &oldPortIdentifier,
										const string &newPortIdentifier, PortContext *newPortContext)
{
	void *carriedOverData = NULL;
	bool foundPort = false;
	map<string, map<string, void *> >::iterator compIter = carriedOverDataForPort.find(compositionIdentifier);
	if (compIter != carriedOverDataForPort.end())
	{
		map<string, void *>::iterator portIter = compIter->second.find(oldPortIdentifier);
		if (portIter != compIter->second.end())
		{
			carriedOverData = portIter->second;
			foundPort = true;
		}
	}

	if (! foundPort)
	{
		VUserLog("Couldn't find data for carried-over port %s:%s", compositionIdentifier, newPortIdentifier.c_str());
		return;
	}

	vuoSetPortContextData(newPortContext, carriedOverData);
	dataForPort[compositionIdentifier][newPortIdentifier] = carriedOverData;
}

/**
 * Helper function for `compositionContextInit()`. If the composition contains subcomposition nodes,
 * this function calls each subcomposition's `compositionContextInit()`.
 */
NodeContext * VuoNodeRegistry::compositionContextInitHelper(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount,
															NodeContext *(*compositionCreateNodeContext)(VuoCompositionState *, unsigned long),
															void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
															void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool))
{
	NodeContext *compositionContext = NULL;

	const char *compositionIdentifier = compositionState->compositionIdentifier;

	bool isTopLevelComposition = ! strcmp(compositionIdentifier, vuoTopLevelCompositionIdentifier);
	if (isTopLevelComposition)
	{
		if (persistentState->compositionDiff->isCompositionStartingOrStopping())
		{
			// The top-level composition is starting for the first time. Create and register its context.
			compositionContext = vuoCreateNodeContext(hasInstanceData, true, publishedOutputPortCount);
			addNodeContext(compositionIdentifier, topLevelCompositionIndex, compositionContext);
		}
		else
		{
			// Restore the top-level composition's context from before the live-coding reload.
			compositionContext = carryOverNodeContext(compositionIdentifier, topLevelCompositionIndex);

			nodeMetadatas.insert(carriedOverNodeMetadatas.begin(), carriedOverNodeMetadatas.end());
			carriedOverNodeMetadatas.clear();
		}
	}
	else
	{
		compositionContext = vuoCreateNodeContext(hasInstanceData, true, publishedOutputPortCount);
	}

	for (size_t nodeIndex = 0; nodeIndex < nodeMetadatas[compositionIdentifier].size(); ++nodeIndex)
	{
		NodeMetadata nodeMetadata = nodeMetadatas[compositionIdentifier][nodeIndex];
		NodeContext *nodeContext = NULL;

		json_object *replacementObj = NULL;
		VuoCompositionDiff::ChangeType changeType = (isTopLevelComposition ?
														 persistentState->compositionDiff->findNode(nodeMetadata.identifier.c_str(), &replacementObj) :
														 VuoCompositionDiff::ChangeAdd);

		if (changeType == VuoCompositionDiff::ChangeStartStop ||
				changeType == VuoCompositionDiff::ChangeAdd || changeType == VuoCompositionDiff::ChangeReplace)
		{
			// Create the added node's context.
			// If the node is a subcomposition, this calls compositionContextInit().
			nodeContext = compositionCreateNodeContext(compositionState, nodeIndex);

			// Register the added node's context.
			addNodeContext(compositionIdentifier, nodeIndex, nodeContext);

			dispatch_semaphore_t nodeSemaphore = vuoGetNodeContextSemaphore(nodeContext);

			// Add the added node's ports to the port cache.
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				addPortIdentifier(compositionIdentifier, portMetadata.identifier, portData, nodeSemaphore, nodeIndex, portMetadata.typeIndex);
			}

			string oldNodeIdentifier;
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				if (! portData)
					continue;

				string oldPortIdentifier;
				if (changeType == VuoCompositionDiff::ChangeReplace &&
						persistentState->compositionDiff->isPortReplacingAnother(portMetadata.name.c_str(), replacementObj, oldNodeIdentifier, oldPortIdentifier))
				{
					// Set the replacement port's data from the port it replaces.
					carryOverPortData(compositionIdentifier, oldPortIdentifier, portMetadata.identifier, portContext);

					// Remove the port being replaced from the port cache.
					removeCarriedOverPortIdentifier(compositionIdentifier, oldPortIdentifier);
				}
				else
				{
					// Set the added port's data to its initial value.
					compositionSetPortValue(compositionState, portMetadata.identifier.c_str(), portMetadata.initialValue.c_str(),
											false, false, false, false, true);
				}
			}

			if (changeType == VuoCompositionDiff::ChangeReplace && ! oldNodeIdentifier.empty())
			{
				// Remove the node context for the node being replaced.
				removeCarriedOverNodeContext(compositionIdentifier, oldNodeIdentifier);
			}
		}
		else
		{
			// Restore the kept node's context.
			// If the node is a subcomposition, this also restores the contexts of all nodes within it.
			nodeContext = carryOverNodeContext(compositionIdentifier, nodeIndex);

			// Restore the kept node's ports to the port cache.
			// If the node is a subcomposition, this also restores the ports of all nodes within it.
			vector<string> portIdentifiers;
			vector<unsigned long> typeIndexes;
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				portIdentifiers.push_back(portMetadata.identifier);
				typeIndexes.push_back(portMetadata.typeIndex);
			}
			carryOverPortIdentifiersForNode(compositionIdentifier, nodeMetadata.identifier, nodeIndex, portIdentifiers, typeIndexes);
		}

		json_object_put(replacementObj);
	}

	if (isTopLevelComposition)
	{
		// Clean up the stuff carried over for nodes being replaced across the live-coding reload.
		removeAllCarriedOverPortIdentifiers();
		removeAllCarriedOverNodeContexts(compositionDestroyNodeContext);
	}

	return compositionContext;
}

/**
 * Helper function for `compositionContextFini()`. If the composition contains subcomposition nodes,
 * this function calls each subcomposition's `compositionContextFini()`.
 */
void VuoNodeRegistry::compositionContextFiniHelper(VuoCompositionState *compositionState,
												   void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
												   void (*compositionReleasePortData)(void *, unsigned long))
{
	const char *compositionIdentifier = compositionState->compositionIdentifier;

	bool isTopLevelComposition = ! strcmp(compositionIdentifier, vuoTopLevelCompositionIdentifier);

	for (size_t nodeIndex = 0; nodeIndex < nodeMetadatas[compositionIdentifier].size(); ++nodeIndex)
	{
		NodeMetadata nodeMetadata = nodeMetadatas[compositionIdentifier][nodeIndex];

		json_object *replacementObj = NULL;
		VuoCompositionDiff::ChangeType changeType = (isTopLevelComposition ?
														 persistentState->compositionDiff->findNode(nodeMetadata.identifier.c_str(), &replacementObj) :
														 VuoCompositionDiff::ChangeRemove);

		if (changeType == VuoCompositionDiff::ChangeStartStop ||
				changeType == VuoCompositionDiff::ChangeRemove || changeType == VuoCompositionDiff::ChangeReplace)
		{
			NodeContext *nodeContext = getNodeContext(compositionIdentifier, nodeIndex);

			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				if (changeType == VuoCompositionDiff::ChangeReplace &&
						persistentState->compositionDiff->isPortBeingReplaced(portMetadata.name.c_str(), replacementObj))
				{
					if (portData)
					{
						// Retain the being-replaced port's data for after the live-coding reload.
						vuoRetainPortContextData(portContext);
					}
				}
				else
				{
					if (portData)
					{
						// Release the removed port's data.
						compositionReleasePortData(portData, portMetadata.typeIndex);
					}

					// Remove the removed port from the port cache.
					removePortIdentifier(compositionIdentifier, portMetadata.identifier);
				}
			}

			if (changeType == VuoCompositionDiff::ChangeStartStop || changeType == VuoCompositionDiff::ChangeRemove)
			{
				// Destroy the removed node's context.
				// If the node is a subcomposition, this calls compositionContextFini().
				compositionDestroyNodeContext(compositionState, nodeMetadata.identifier.c_str(), nodeContext);

				// Un-register the removed node's context.
				removeNodeContext(compositionIdentifier, nodeIndex);
			}
		}

		json_object_put(replacementObj);
	}

	if (isTopLevelComposition)
	{
		if (persistentState->compositionDiff->isCompositionStartingOrStopping())
		{
			// The top-level composition is stopping for the last time. Un-register and destroy its context.
			NodeContext *compositionContext = getCompositionContext(compositionIdentifier);
			removeNodeContext(compositionIdentifier, topLevelCompositionIndex);
			vuoFreeNodeContext(compositionContext);
		}
		else
		{
			// Carry over stuff for kept and being-replaced nodes for after the live-coding reload.
			// If any nodes are subcompositions, this also carries over stuff for all nodes within them.
			relocateAllPortIdentifiers();
			relocateAllNodeContexts();

			// Clear the node metadata for all nodes in this composition (but not for nodes within subcomposition nodes).
			nodeMetadatas.erase(compositionIdentifier);

			// Carry over the node metadata for nodes within kept subcomposition nodes.
			carriedOverNodeMetadatas = nodeMetadatas;
		}

		nodeMetadatas.clear();
	}
	else
	{
		// Clear the node metadata for all nodes in this composition (but not for nodes within subcomposition nodes).
		nodeMetadatas.erase(compositionIdentifier);
	}
}

extern "C"
{

/**
 * C wrapper for VuoNodeRegistry::addNodeMetadata().
 */
void vuoAddNodeMetadata(VuoCompositionState *compositionState, const char *nodeIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->nodeRegistry->addNodeMetadata(compositionIdentifier, nodeIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::addPortMetadata().
 */
void vuoAddPortMetadata(VuoCompositionState *compositionState, const char *portIdentifier, const char *portName,
						unsigned long typeIndex, const char *initialValue)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->nodeRegistry->addPortMetadata(compositionIdentifier, portIdentifier, portName, typeIndex, initialValue);
}

/**
 * C wrapper for VuoNodeRegistry::getNodeContext().
 */
NodeContext * vuoGetNodeContext(VuoCompositionState *compositionState, unsigned long nodeIndex)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getNodeContext(compositionIdentifier, nodeIndex);
}

/**
 * C wrapper for VuoNodeRegistry::getCompositionContext().
 */
NodeContext * vuoGetCompositionContext(VuoCompositionState *compositionState)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getCompositionContext(compositionIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::getDataForPort().
 */
void * vuoGetDataForPort(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getDataForPort(compositionIdentifier, portIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::getNodeSemaphoreForPort().
 */
dispatch_semaphore_t vuoGetNodeSemaphoreForPort(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getNodeSemaphoreForPort(compositionIdentifier, portIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::getNodeIndexForPort().
 */
unsigned long vuoGetNodeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getNodeIndexForPort(compositionIdentifier, portIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::getTypeIndexForPort().
 */
unsigned long vuoGetTypeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	return runtimeState->persistentState->nodeRegistry->getTypeIndexForPort(compositionIdentifier, portIdentifier);
}

/**
 * C wrapper for VuoNodeRegistry::compositionContextInitHelper().
 */
NodeContext * vuoCompositionContextInitHelper(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount,
											  NodeContext *(*compositionCreateNodeContext)(VuoCompositionState *, unsigned long),
											  void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
											  void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool))
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->nodeRegistry->compositionContextInitHelper(compositionState, hasInstanceData, publishedOutputPortCount,
																					 compositionCreateNodeContext, compositionDestroyNodeContext, compositionSetPortValue);
}

/**
 * C wrapper for VuoNodeRegistry::compositionContextFiniHelper().
 */
void vuoCompositionContextFiniHelper(VuoCompositionState *compositionState,
									 void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
									 void (*compositionReleasePortData)(void *, unsigned long))
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->nodeRegistry->compositionContextFiniHelper(compositionState,
																			  compositionDestroyNodeContext, compositionReleasePortData);
}

}
