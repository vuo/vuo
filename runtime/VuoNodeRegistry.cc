/**
 * @file
 * VuoNodeRegistry implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoNodeRegistry.hh"

#include <dlfcn.h>
#include <sstream>
#include "VuoCompositionDiff.hh"
#include "VuoException.hh"
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
 * @throw VuoException One of the symbols was not found in the composition binary.
 */
void VuoNodeRegistry::updateCompositionSymbols(void *compositionBinaryHandle)
{
	ostringstream errorMessage;

	vuoTopLevelCompositionIdentifierType *vuoTopLevelCompositionIdentifierPtr = (vuoTopLevelCompositionIdentifierType *) dlsym(compositionBinaryHandle, "vuoTopLevelCompositionIdentifier");
	if (! vuoTopLevelCompositionIdentifierPtr)
	{
		errorMessage << "The composition couldn't be started because its vuoTopLevelCompositionIdentifier variable couldn't be found : " << dlerror();
		throw VuoException(errorMessage.str());
	}
	vuoTopLevelCompositionIdentifier = *vuoTopLevelCompositionIdentifierPtr;
}

/**
 * Returns the top-level composition identifier if @a compositionIdentifier is empty, otherwise @a compositionIdentifier.
 * @version200New
 */
const char * VuoNodeRegistry::defaultToTopLevelCompositionIdentifier(const char *compositionIdentifier)
{
	return (strlen(compositionIdentifier) > 0 ? compositionIdentifier : vuoTopLevelCompositionIdentifier);
}

/**
 * If @a compositionIdentifier refers to a subcomposition, outputs the identifier of the parent composition
 * and the subcomposition node within the parent composition. Otherwise, outputs @a compositionIdentifier as
 * the parent composition identifier.
 *
 * This needs to be kept in sync with @ref VuoStringUtilities::buildCompositionIdentifier().
 */
void VuoNodeRegistry::splitCompositionIdentifier(const string &compositionIdentifier, string &parentCompositionIdentifier, string &nodeIdentifier)
{
	size_t pos = compositionIdentifier.rfind("/");
	if (pos != string::npos)
	{
		parentCompositionIdentifier = compositionIdentifier.substr(0, pos);
		nodeIdentifier = compositionIdentifier.substr(pos + strlen("/"));
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
string VuoNodeRegistry::buildCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier)
{
	return parentCompositionIdentifier + "/" + nodeIdentifier;
}

/**
 * Outputs the node identifier and port name parts of a port identifier.
 *
 * This needs to be kept in sync with @ref VuoStringUtilities::buildPortIdentifier().
 */
void VuoNodeRegistry::splitPortIdentifier(const string &portIdentifier, string &nodeIdentifier, string &portName)
{
	size_t pos = portIdentifier.rfind(":");
	if (pos != string::npos)
	{
		nodeIdentifier = portIdentifier.substr(0, pos);
		portName = portIdentifier.substr(pos + strlen(":"));
	}
}

/**
 * Registers metadata for a node. When a (sub)composition is added or recompiled, this function should be
 * called for each node in the composition, in the same order as `VuoCompilerBitcodeGenerator::orderedNodes`.
 *
 * @version200Changed{Added callback arguments.}
 */
void VuoNodeRegistry::addNodeMetadata(const char *compositionIdentifier, const char *nodeIdentifier,
									  NodeContext *(*compositionCreateContextForNode)(unsigned long),
									  void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool),
									  char * (*compositionGetPortValue)(VuoCompositionState *, const char *, int, bool),
									  void (*compositionFireTriggerPortEvent)(VuoCompositionState *, const char *),
									  void (*compositionReleasePortData)(void *, unsigned long))
{
	NodeMetadata nodeMetadata;
	nodeMetadata.identifier = nodeIdentifier;
	nodeMetadata.compositionCreateContextForNode = compositionCreateContextForNode;
	nodeMetadata.compositionSetPortValue = compositionSetPortValue;
	nodeMetadata.compositionGetPortValue = compositionGetPortValue;
	nodeMetadata.compositionFireTriggerPortEvent = compositionFireTriggerPortEvent;
	nodeMetadata.compositionReleasePortData = compositionReleasePortData;
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

	VUserLog("Couldn't find identifier for node %s/%lu", compositionIdentifier, nodeIndex);
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

	VUserLog("Couldn't find index for node %s", buildCompositionIdentifier(compositionIdentifier, nodeIdentifier).c_str());
	return invalidCompositionIndex;
}

/**
 * Looks up the node metadata for the node containing a port with the given identifier.
 */
const VuoNodeRegistry::NodeMetadata * VuoNodeRegistry::getNodeMetadataForPort(const string &compositionIdentifier, const string &portIdentifier)
{
	string nodeIdentifier, portName;
	splitPortIdentifier(portIdentifier, nodeIdentifier, portName);

	map<string, vector<NodeMetadata> >::iterator iter1 = nodeMetadatas.find(compositionIdentifier);
	if (iter1 != nodeMetadatas.end())
		for (unsigned long i = 0; i < iter1->second.size(); ++i)
			if (iter1->second[i].identifier == nodeIdentifier)
				return &(iter1->second[i]);

	VUserLog("Couldn't find node metadata for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	return NULL;
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
			VUserLog("Context overwritten for node %s", buildCompositionIdentifier(compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex)).c_str());
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
			string subcompositionIdentifier = buildCompositionIdentifier(compositionIdentifier, nodeIdentifier);
			unsigned long subcompositionIdentifierHash = VuoRuntimeUtilities::hash(subcompositionIdentifier.c_str());
			map<unsigned long, string>::iterator ciIter = compositionIdentifierForHash.find(subcompositionIdentifierHash);
			if (ciIter != compositionIdentifierForHash.end())
				compositionIdentifierForHash.erase(ciIter);

			return;
		}
	}

	VUserLog("Couldn't find context for node %s", buildCompositionIdentifier(compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex)).c_str());
}

/**
 * Preserves a node context to be accessed after a live-coding reload.
 */
void VuoNodeRegistry::relocateNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	unsigned long compositionIdentifierHash = VuoRuntimeUtilities::hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
		{
			string nodeIdentifier = getNodeIdentifierForIndex(compositionIdentifier, nodeIndex);
			carriedOverNodeContextForIdentifier[compositionIdentifier][nodeIdentifier] = iter2->second;
			iter1->second.erase(iter2);

			if (iter1->second.empty())
				nodeContextForIndex.erase(iter1);

			return;
		}
	}

	VUserLog("Couldn't find context for node %s", buildCompositionIdentifier(compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex)).c_str());
}

/**
 * Carries over a node context from before a live-coding reload — registering it with the new node contexts
 * and removing it from the list of carried-over node contexts.
 */
NodeContext * VuoNodeRegistry::carryOverNodeContext(const char *oldCompositionIdentifier, const char *newCompositionIdentifier,
													unsigned long nodeIndex)
{
	NodeContext *nodeContext = NULL;
	string nodeIdentifier = getNodeIdentifierForIndex(newCompositionIdentifier, nodeIndex);

	{
		bool found = false;
		map<string, map<string, NodeContext *> >::iterator coIter1 = carriedOverNodeContextForIdentifier.find(oldCompositionIdentifier);
		if (coIter1 != carriedOverNodeContextForIdentifier.end())
		{
			map<string, NodeContext *>::iterator coIter2 = coIter1->second.find(nodeIdentifier);
			if (coIter2 != coIter1->second.end())
			{
				nodeContext = coIter2->second;
				coIter1->second.erase(coIter2);
				addNodeContext(newCompositionIdentifier, nodeIndex, nodeContext);
				found = true;

				if (coIter1->second.empty())
					carriedOverNodeContextForIdentifier.erase(coIter1);
			}
		}

		if (! found)
			VUserLog("Couldn't find context for node %s", buildCompositionIdentifier(oldCompositionIdentifier, nodeIdentifier).c_str());
	}

	return nodeContext;
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

	VUserLog("Couldn't find context for node %s", buildCompositionIdentifier(compositionIdentifier, getNodeIdentifierForIndex(compositionIdentifier, nodeIndex)).c_str());
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
 * Helper function. Checks if cached info exists in one of the cached port info maps, and if so, provides an iterator pointing to it.
 */
template<typename T>
bool VuoNodeRegistry::findCachedInfoForPort(const map<string, map<string, T> > &cachedInfoForPort, const string &compositionIdentifier,
											const string &portIdentifier, typename map<string, T>::const_iterator &foundIter)
{
	typename map<string, map<string, T> >::const_iterator compIter = cachedInfoForPort.find(compositionIdentifier);
	if (compIter != cachedInfoForPort.end())
	{
		foundIter = compIter->second.find(portIdentifier);
		if (foundIter != compIter->second.end())
			return true;
	}

	return false;
}

/**
 * Returns the `data` field in a port's context, given the port's identifier.
 */
void * VuoNodeRegistry::getDataForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, void *>::const_iterator foundIter;
	bool found = findCachedInfoForPort<void *>(dataForPort, compositionIdentifier, portIdentifier, foundIter);
	if (found)
		return foundIter->second;

	VUserLog("Couldn't find data for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	return NULL;
}

/**
 * Returns the numerical index for a node, given the identifier of a port on the node.
 */
unsigned long VuoNodeRegistry::getNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, unsigned long>::const_iterator foundIter;
	bool found = findCachedInfoForPort<unsigned long>(nodeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
	if (found)
		return foundIter->second;

	VUserLog("Couldn't find node index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	return 0;
}

/**
 * Returns the numerical index for a port's type, given the port's identifier.
 */
unsigned long VuoNodeRegistry::getTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
{
	map<string, unsigned long>::const_iterator foundIter;
	bool found = findCachedInfoForPort<unsigned long>(typeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
	if (found)
		return foundIter->second;

	VUserLog("Couldn't find type index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	return 0;
}

/**
 * Caches information about the port so it can be efficiently retrieved later.
 */
void VuoNodeRegistry::addPortIdentifier(const char *compositionIdentifier, const string &portIdentifier,
										void *data, unsigned long nodeIndex, unsigned long typeIndex)
{
	map<string, void *>::const_iterator foundIter;
	bool found = findCachedInfoForPort<void *>(dataForPort, compositionIdentifier, portIdentifier, foundIter);
	if (found)
		VUserLog("Cache overwritten for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());

	dataForPort[compositionIdentifier][portIdentifier] = data;
	nodeIndexForPort[compositionIdentifier][portIdentifier] = nodeIndex;
	typeIndexForPort[compositionIdentifier][portIdentifier] = typeIndex;
}

/**
 * Removes the port from the port cache.
 */
void VuoNodeRegistry::removePortIdentifier(const char *compositionIdentifier, const string &portIdentifier)
{
	{
		map<string, void *>::const_iterator foundIter;
		bool found = findCachedInfoForPort<void *>(dataForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
		{
			dataForPort[compositionIdentifier].erase(portIdentifier);

			if (dataForPort[compositionIdentifier].empty())
				dataForPort.erase(compositionIdentifier);
		}
		else
			VUserLog("Couldn't find data for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}
	{
		map<string, unsigned long>::const_iterator foundIter;
		bool found = findCachedInfoForPort<unsigned long>(nodeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
		{
			nodeIndexForPort[compositionIdentifier].erase(portIdentifier);

			if (nodeIndexForPort[compositionIdentifier].empty())
				nodeIndexForPort.erase(compositionIdentifier);
		}
		else
			VUserLog("Couldn't find node index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}
	{
		map<string, unsigned long>::const_iterator foundIter;
		bool found = findCachedInfoForPort<unsigned long>(typeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
		{
			typeIndexForPort[compositionIdentifier].erase(portIdentifier);

			if (typeIndexForPort[compositionIdentifier].empty())
				typeIndexForPort.erase(compositionIdentifier);
		}
		else
			VUserLog("Couldn't find type index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}
}

/**
 * Preserves a port's cache entries to be accessed after a live-coding reload.
 */
void VuoNodeRegistry::relocatePortIdentifier(const char *compositionIdentifier, const string &portIdentifier)
{
	{
		map<string, void *>::const_iterator foundIter;
		bool found = findCachedInfoForPort<void *>(dataForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
			carriedOverDataForPort[compositionIdentifier][portIdentifier] = foundIter->second;
		else
			VUserLog("Couldn't find data for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}
	{
		map<string, unsigned long>::const_iterator foundIter;
		bool found = findCachedInfoForPort<unsigned long>(nodeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
			carriedOverNodeIndexForPort[compositionIdentifier][portIdentifier] = foundIter->second;
		else
			VUserLog("Couldn't find node index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}
	{
		map<string, unsigned long>::const_iterator foundIter;
		bool found = findCachedInfoForPort<unsigned long>(typeIndexForPort, compositionIdentifier, portIdentifier, foundIter);
		if (found)
			carriedOverTypeIndexForPort[compositionIdentifier][portIdentifier] = foundIter->second;
		else
			VUserLog("Couldn't find type index for port %s", buildCompositionIdentifier(compositionIdentifier, portIdentifier).c_str());
	}

	removePortIdentifier(compositionIdentifier, portIdentifier);
}

/**
 * Carries over a port's cache entry from before a live-coding reload — transferring the entry to the new
 * port cache and removing it from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::carryOverPortIdentifier(const char *oldCompositionIdentifier, const char *newCompositionIdentifier,
											  const string &portIdentifier, unsigned long nodeIndex, unsigned long typeIndex)
{
	map<string, void *>::const_iterator foundIter;
	bool found = findCachedInfoForPort<void *>(carriedOverDataForPort, oldCompositionIdentifier, portIdentifier, foundIter);
	if (! found)
	{
		VUserLog("Couldn't find cache for port %s", buildCompositionIdentifier(oldCompositionIdentifier, portIdentifier).c_str());
		return;
	}

	dataForPort[newCompositionIdentifier][portIdentifier] = carriedOverDataForPort[oldCompositionIdentifier][portIdentifier];
	nodeIndexForPort[newCompositionIdentifier][portIdentifier] = nodeIndex;
	typeIndexForPort[newCompositionIdentifier][portIdentifier] = typeIndex;

	removeCarriedOverPortIdentifier(oldCompositionIdentifier, portIdentifier);
}

/**
 * Carries over all of the node's port cache entries from before a live-coding reload — transferring the entries
 * to the new port cache and removing them from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::carryOverPortIdentifiersForNode(const char *oldCompositionIdentifier, const char *newCompositionIdentifier,
													  const string &nodeIdentifier, unsigned long nodeIndex,
													  const vector<string> &portIdentifiers, const vector<unsigned long> typeIndexes)
{
	for (size_t j = 0; j < portIdentifiers.size(); ++j)
		carryOverPortIdentifier(oldCompositionIdentifier, newCompositionIdentifier, portIdentifiers[j], nodeIndex, typeIndexes[j]);
}

/**
 * Removes the port from the list of carried-over port cache entries.
 */
void VuoNodeRegistry::removeCarriedOverPortIdentifier(const char *compositionIdentifier, const string &oldPortIdentifier)
{
	map<string, void *>::const_iterator foundIter;
	bool found = findCachedInfoForPort<void *>(carriedOverDataForPort, compositionIdentifier, oldPortIdentifier, foundIter);
	if (! found)
	{
		VUserLog("Couldn't find cache for port %s", buildCompositionIdentifier(compositionIdentifier, oldPortIdentifier).c_str());
		return;
	}

	carriedOverDataForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverNodeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverTypeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);

	if (carriedOverDataForPort[compositionIdentifier].empty())
		carriedOverDataForPort.erase(compositionIdentifier);
	if (carriedOverNodeIndexForPort[compositionIdentifier].empty())
		carriedOverNodeIndexForPort.erase(compositionIdentifier);
	if (carriedOverTypeIndexForPort[compositionIdentifier].empty())
		carriedOverTypeIndexForPort.erase(compositionIdentifier);
}

/**
 * For a node being replaced across a live-coding reload, transfers port data from a port on the old node
 * to the corresponding port on the new node (by copying the port data's heap address from the old PortContext
 * to the new PortContext). Updates the port data cache.
 */
void VuoNodeRegistry::carryOverPortData(const char *oldCompositionIdentifier, const char *newCompositionIdentifier,
										const string &oldPortIdentifier, const string &newPortIdentifier, PortContext *newPortContext)
{
	void *carriedOverData;
	{
		map<string, void *>::const_iterator foundIter;
		bool found = findCachedInfoForPort<void *>(carriedOverDataForPort, oldCompositionIdentifier, oldPortIdentifier, foundIter);
		if (! found)
		{
			VUserLog("Couldn't find data for carried-over port %s", buildCompositionIdentifier(oldCompositionIdentifier, oldPortIdentifier).c_str());
			return;
		}

		carriedOverData = foundIter->second;
	}

	vuoSetPortContextData(newPortContext, carriedOverData);

	{
		map<string, void *>::const_iterator foundIter;
		bool found = findCachedInfoForPort<void *>(dataForPort, newCompositionIdentifier, newPortIdentifier, foundIter);
		if (! found)
		{
			VUserLog("Couldn't find data for port %s", buildCompositionIdentifier(newCompositionIdentifier, newPortIdentifier).c_str());
			return;
		}

		dataForPort[newCompositionIdentifier][newPortIdentifier] = carriedOverData;
	}
}

/**
 * Initializes a node context for the top-level composition, for each node and port in the top-level composition,
 * and recursively for each subcomposition.
 *
 * @version200New
 */
void VuoNodeRegistry::initContextForTopLevelComposition(VuoCompositionState *compositionState, bool hasInstanceData,
														unsigned long publishedOutputPortCount)
{
	const char *compositionIdentifier = compositionState->compositionIdentifier;

	if (persistentState->compositionDiff->isCompositionStartingOrStopping())
	{
		// Create and register a node context for the top-level composition.
		NodeContext *compositionContext = vuoCreateNodeContext(hasInstanceData, true, publishedOutputPortCount);
		addNodeContext(compositionIdentifier, topLevelCompositionIndex, compositionContext);
	}
	else
	{
		carryOverNodeContext(compositionIdentifier, compositionIdentifier, topLevelCompositionIndex);
	}

	initContextsForCompositionContents(compositionState);
}

/**
 * Initializes the node and port contexts for each node in the composition, and recursively for each subcomposition.
 * The contexts are either created anew or carried over from before a live-coding reload.
 */
void VuoNodeRegistry::initContextsForCompositionContents(VuoCompositionState *compositionState)
{
	const char *compositionIdentifier = compositionState->compositionIdentifier;

	for (size_t nodeIndex = 0; nodeIndex < nodeMetadatas[compositionIdentifier].size(); ++nodeIndex)
	{
		NodeMetadata nodeMetadata = nodeMetadatas[compositionIdentifier][nodeIndex];
		NodeContext *nodeContext = NULL;

		json_object *replacementObj = NULL;
		VuoCompositionDiff::ChangeType changeType = persistentState->compositionDiff->findNode(compositionIdentifier,
																							   nodeMetadata.identifier.c_str(),
																							   &replacementObj);
		if (changeType == VuoCompositionDiff::ChangeStartStop ||
				changeType == VuoCompositionDiff::ChangeAdd || changeType == VuoCompositionDiff::ChangeReplace)
		{
			// Create and register a node context for the added node and a port context for each of its ports.
			NodeContext *nodeContext = nodeMetadata.compositionCreateContextForNode(nodeIndex);
			addNodeContext(compositionIdentifier, nodeIndex, nodeContext);

			// Add the added node's ports to the port cache.
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				addPortIdentifier(compositionIdentifier, portMetadata.identifier, portData, nodeIndex, portMetadata.typeIndex);
			}

			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				if (portData)
				{
					string oldNodeIdentifier;
					string oldPortIdentifier;
					if (changeType == VuoCompositionDiff::ChangeReplace &&
							persistentState->compositionDiff->isPortReplacingAnother(portMetadata.name.c_str(), replacementObj, oldNodeIdentifier, oldPortIdentifier))
					{
						// Set the replacement port's data from the port it replaces.
						carryOverPortData(compositionIdentifier, compositionIdentifier, oldPortIdentifier, portMetadata.identifier, portContext);

						// Remove the port from the carried-over port info.
						removeCarriedOverPortIdentifier(compositionIdentifier, oldPortIdentifier);
					}
					else
					{
						// Set the added port's data to its initial value.
						nodeMetadata.compositionSetPortValue(compositionState, portMetadata.identifier.c_str(), portMetadata.initialValue.c_str(),
															 false, false, false, false, true);
					}
				}
			}
		}
		else
		{
			string oldCompositionIdentifier = compositionIdentifier;
			if (changeType == VuoCompositionDiff::ChangeMove &&
					(! persistentState->compositionDiff->isNodeBeingMovedToHere(compositionIdentifier, nodeMetadata.identifier.c_str(), replacementObj, oldCompositionIdentifier)))
				continue;

			// Restore the kept node's context.
			nodeContext = carryOverNodeContext(oldCompositionIdentifier.c_str(), compositionIdentifier, nodeIndex);

			// Restore the kept node's ports to the port cache.
			vector<string> portIdentifiers;
			vector<unsigned long> typeIndexes;
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				portIdentifiers.push_back(portMetadata.identifier);
				typeIndexes.push_back(portMetadata.typeIndex);
			}
			carryOverPortIdentifiersForNode(oldCompositionIdentifier.c_str(), compositionIdentifier, nodeMetadata.identifier, nodeIndex, portIdentifiers, typeIndexes);

			// If the node has been packaged into a subcomposition, copy the port values with connected published input cables
			// from here (the node inside the subcomposition) to the new subcomposition node in the parent composition.
			if (changeType == VuoCompositionDiff::ChangeMove)
			{
				for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
				{
					PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];

					string destinationCompositionIdentifier;
					string destinationPortIdentifier;
					if (persistentState->compositionDiff->isPortBeingCopied(portMetadata.name.c_str(), replacementObj,
																			destinationCompositionIdentifier, destinationPortIdentifier))
					{
						char *portValue = getPortValue(compositionState, portMetadata.identifier.c_str(), false);

						const NodeMetadata *destinationPortMetadata = getNodeMetadataForPort(destinationCompositionIdentifier.c_str(), destinationPortIdentifier.c_str());
						VuoCompositionState *destinationCompositionState = vuoCreateCompositionState(compositionState->runtimeState, destinationCompositionIdentifier.c_str());
						destinationPortMetadata->compositionSetPortValue(destinationCompositionState, destinationPortIdentifier.c_str(), portValue, false, false, false, true, true);
					}
				}
			}
		}

		json_object_put(replacementObj);

		// If the node is a subcomposition, initialize the node and port contexts within it.
		string subcompositionIdentifier = buildCompositionIdentifier(compositionIdentifier, nodeMetadata.identifier);
		if (nodeMetadatas.find(subcompositionIdentifier) != nodeMetadatas.end())
		{
			VuoCompositionState *subcompositionState = vuoCreateCompositionState(compositionState->runtimeState, subcompositionIdentifier.c_str());
			initContextsForCompositionContents(subcompositionState);
			vuoFreeCompositionState(subcompositionState);
		}
	}
}

/**
 * Finalizes the node context for the top-level composition, for each node and port in the top-level composition,
 * and recursively for each subcomposition. Also erases all stored node metadatas.
 *
 * @version200New
 */
void VuoNodeRegistry::finiContextForTopLevelComposition(VuoCompositionState *compositionState)
{
	const char *compositionIdentifier = compositionState->compositionIdentifier;

	if (persistentState->compositionDiff->isCompositionStartingOrStopping())
	{
		// Unregister and destroy the node context for the top-level composition.
		NodeContext *compositionContext = getNodeContext(compositionIdentifier, topLevelCompositionIndex);
		removeNodeContext(compositionIdentifier, topLevelCompositionIndex);
		vuoFreeNodeContext(compositionContext);
	}
	else
	{
		relocateNodeContext(compositionIdentifier, topLevelCompositionIndex);
	}

	finiContextsForCompositionContents(compositionState);

	nodeMetadatas.clear();
}

/**
 * Finalizes the node and port contexts for each node in the composition, and recursively for each subcomposition.
 * The contexts are either destroyed or relocated to be carried across a live-coding reload.
 */
void VuoNodeRegistry::finiContextsForCompositionContents(VuoCompositionState *compositionState)
{
	const char *compositionIdentifier = compositionState->compositionIdentifier;

	for (size_t nodeIndex = 0; nodeIndex < nodeMetadatas[compositionIdentifier].size(); ++nodeIndex)
	{
		NodeMetadata nodeMetadata = nodeMetadatas[compositionIdentifier][nodeIndex];

		json_object *replacementObj = NULL;
		VuoCompositionDiff::ChangeType changeType = persistentState->compositionDiff->findNode(compositionIdentifier,
																							   nodeMetadata.identifier.c_str(),
																							   &replacementObj);
		if (changeType == VuoCompositionDiff::ChangeStartStop ||
				changeType == VuoCompositionDiff::ChangeRemove || changeType == VuoCompositionDiff::ChangeReplace)
		{
			NodeContext *nodeContext = getNodeContext(compositionIdentifier, nodeIndex);

			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				bool relocated = false;
				if (portData)
				{
					if (changeType == VuoCompositionDiff::ChangeReplace &&
							persistentState->compositionDiff->isPortBeingReplaced(portMetadata.name.c_str(), replacementObj))
					{
						// Retain the being-replaced port's data for after the live-coding reload.
						vuoRetainPortContextData(portContext);

						// Carry over the port's data
						relocatePortIdentifier(compositionIdentifier, portMetadata.identifier);
						relocated = true;
					}
					else
					{
						// Release the port's data.
						nodeMetadata.compositionReleasePortData(portData, portMetadata.typeIndex);
					}
				}

				if (! relocated)
				{
					// Remove the port from the (non-carried-over) port cache.
					removePortIdentifier(compositionIdentifier, portMetadata.identifier);
				}
			}

			// Unregister and destroy the node's context.
			removeNodeContext(compositionIdentifier, nodeIndex);
			vuoFreeNodeContext(nodeContext);
		}
		else
		{
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				// Carry over the port's data
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				relocatePortIdentifier(compositionIdentifier, portMetadata.identifier);
			}

			// Carry over the node's context
			relocateNodeContext(compositionIdentifier, nodeIndex);
		}

		json_object_put(replacementObj);

		string subcompositionIdentifier = buildCompositionIdentifier(compositionIdentifier, nodeMetadata.identifier);
		if (nodeMetadatas.find(subcompositionIdentifier) != nodeMetadatas.end())
		{
			// Recursive call for subcomposition.
			VuoCompositionState *subcompositionState = vuoCreateCompositionState(compositionState->runtimeState, subcompositionIdentifier.c_str());
			finiContextsForCompositionContents(subcompositionState);
			vuoFreeCompositionState(subcompositionState);
		}
	}
}

/**
 * Sets the data value of the port to @a valueAsString. The port is found by looking in the (sub)composition specified by
 * @a compositionState for a port with the given identifier.
 *
 * @version200New
 */
void VuoNodeRegistry::setPortValue(VuoCompositionState *compositionState, const char *portIdentifier, const char *valueAsString)
{
	const NodeMetadata *nm = getNodeMetadataForPort(compositionState->compositionIdentifier, portIdentifier);
	if (!nm)
		return;

	nm->compositionSetPortValue(compositionState, portIdentifier, valueAsString, true, true, true, true, true);
}

/**
 * Returns the data value of the port. The port is found by looking in the (sub)composition specified by
 * @a compositionState for a port with the given identifier.
 *
 * @version200New
 */
char * VuoNodeRegistry::getPortValue(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization)
{
	const NodeMetadata *nm = getNodeMetadataForPort(compositionState->compositionIdentifier, portIdentifier);
	if (!nm)
		return nullptr;

	return nm->compositionGetPortValue(compositionState, portIdentifier, shouldUseInterprocessSerialization ? 2 : 1, true);
}

/**
 * Returns a text summary of the data value of the port. The port is found by looking in the (sub)composition specified by
 * @a compositionState for a port with the given identifier.
 *
 * @version200New
 */
char * VuoNodeRegistry::getPortSummary(VuoCompositionState *compositionState, const char *portIdentifier)
{
	const NodeMetadata *nm = getNodeMetadataForPort(compositionState->compositionIdentifier, portIdentifier);
	if (!nm)
		return nullptr;

	return nm->compositionGetPortValue(compositionState, portIdentifier, 0, true);
}

/**
 * Fires an event from the trigger port. The port is found by looking in the (sub)composition specified by
 * @a compositionState for a port with the given identifier.
 *
 * @version200New
 */
void VuoNodeRegistry::fireTriggerPortEvent(VuoCompositionState *compositionState, const char *portIdentifier)
{
	const NodeMetadata *nm = getNodeMetadataForPort(compositionState->compositionIdentifier, portIdentifier);
	if (!nm)
		return;

	nm->compositionFireTriggerPortEvent(compositionState, portIdentifier);
}

/**
 * Prints the state of the node registry to stdout for debugging.
 */
void VuoNodeRegistry::print(void)
{
	const char *indent = "  ";

	ostringstream oss;
	oss << "=== node metadatas ===" << endl << endl;
	for (map<string, vector<NodeMetadata> >::iterator i = nodeMetadatas.begin(); i != nodeMetadatas.end(); ++i)
	{
		oss << indent << i->first << endl;
		for (size_t j = 0; j < i->second.size(); ++j)
			oss << indent << indent << j << " " << i->second[j].identifier << ", " << i->second[j].portMetadatas.size() << " ports" << endl;
	}
	oss << endl;

	oss << "=== node contexts ===" << endl << endl;
	for (map<unsigned long, map<unsigned long, NodeContext *> >::iterator i = nodeContextForIndex.begin(); i != nodeContextForIndex.end(); ++i)
	{
		string compositionIdentifier = getCompositionIdentifierForHash(i->first);
		oss << indent << compositionIdentifier << endl;
		for (map<unsigned long, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			oss << indent << indent << getNodeIdentifierForIndex(compositionIdentifier.c_str(), j->first) << " " << j->second << endl;
	}
	oss << endl;

	oss << "=== carried-over node contexts ===" << endl << endl;
	for (map<string, map<string, NodeContext *> >::iterator i = carriedOverNodeContextForIdentifier.begin(); i != carriedOverNodeContextForIdentifier.end(); ++i)
	{
		oss << indent << i->first << endl;
		for (map<string, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			oss << indent << indent << j->first << " " << j->second << endl;
	}
	oss << endl;

	oss << "=== cached ports ===" << endl << endl;
	for (map<string, map<string, void *> >::iterator i = dataForPort.begin(); i != dataForPort.end(); ++i)
	{
		oss << indent << i->first << endl;
		for (map<string, void *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			oss << indent << indent << j->first << " " << j->second << endl;
	}
	oss << endl;

	oss << "=== carried-over cached ports ===" << endl << endl;
	for (map<string, map<string, void *> >::iterator i = carriedOverDataForPort.begin(); i != carriedOverDataForPort.end(); ++i)
	{
		oss << indent << i->first << endl;
		for (map<string, void *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			oss << indent << indent << j->first << " " << j->second << endl;
	}
	oss << endl;

	VUserLog("\n%s", oss.str().c_str());
}

extern "C"
{

/**
 * C wrapper for VuoNodeRegistry::addNodeMetadata().
 *
 * @version200Changed{Added callback arguments.}
 */
void vuoAddNodeMetadata(VuoCompositionState *compositionState, const char *nodeIdentifier,
						NodeContext *(*compositionCreateContextForNode)(unsigned long),
						void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool),
						char * (*compositionGetPortValue)(VuoCompositionState *, const char *, int, bool),
						void (*compositionFireTriggerPortEvent)(VuoCompositionState *, const char *),
						void (*compositionReleasePortData)(void *, unsigned long))
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->nodeRegistry->addNodeMetadata(compositionIdentifier, nodeIdentifier,
																 compositionCreateContextForNode, compositionSetPortValue,
																 compositionGetPortValue, compositionFireTriggerPortEvent,
																 compositionReleasePortData);
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
 * C wrapper for VuoNodeRegistry::initContextForTopLevelComposition().
 *
 * @version200New
 */
void vuoInitContextForTopLevelComposition(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->nodeRegistry->initContextForTopLevelComposition(compositionState, hasInstanceData, publishedOutputPortCount);
}

/**
 * C wrapper for VuoNodeRegistry::finiContextForTopLevelComposition().
 *
 * @version200New
 */
void vuoFiniContextForTopLevelComposition(VuoCompositionState *compositionState)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	return runtimeState->persistentState->nodeRegistry->finiContextForTopLevelComposition(compositionState);
}

}
