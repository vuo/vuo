/**
 * @file
 * VuoNodeRegistry interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoRuntimePersistentState;
#include "VuoCompositionState.h"
#include "VuoCompositionDiff.hh"
#include "VuoRuntimeContext.hh"

/**
 * Manages runtime information associated with each node and port in the composition.
 */
class VuoNodeRegistry
{
private:
	/**
	 * Port info used to manage port data values and the port cache.
	 */
	struct PortMetadata
	{
		string identifier;
		string name;
		unsigned long typeIndex;
		string initialValue;
	};

	/**
	 * Node info used to manage node contexts and port info.
	 */
	struct NodeMetadata
	{
		string identifier;
		vector<PortMetadata> portMetadatas;

		NodeContext *(*compositionCreateContextForNode)(unsigned long);
		void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool);
		char * (*compositionGetPortValue)(VuoCompositionState *, const char *, int, bool);
		void (*compositionFireTriggerPortEvent)(VuoCompositionState *, const char *);
		void (*compositionReleasePortData)(void *, unsigned long);
	};

	map<string, vector<NodeMetadata> > nodeMetadatas;  ///< The metadata for each node, by composition identifier and node index. A node's index can change across a live-coding reload. This does not contain metadata for the top-level composition.

	map<unsigned long, map<unsigned long, NodeContext *> > nodeContextForIndex;  ///< A registry of all NodeContext values in the running composition, indexed by hashed composition identifier and node index.
	map<unsigned long, string> compositionIdentifierForHash;  ///< The composition identifier for each hash registered in `nodeContextForIndex`.
	map<string, map<string, void *> > dataForPort;  ///< The `data` field in the port's context, indexed by composition and port identifier.
	map<string, map<string, unsigned long> > nodeIndexForPort;  ///< The index for a node, indexed by composition and port identifier.
	map<string, map<string, unsigned long> > typeIndexForPort;  ///< The index for the port's type, indexed by composition and port identifier.

	map<string, map<string, NodeContext *> > carriedOverNodeContextForIdentifier;  ///< Info from @ref nodeContextForIndex carried across a live-coding reload, indexed by composition identifier and node identifier. For the top-level composition, the node identifier is the empty string.
	map<string, map<string, void *> > carriedOverDataForPort;  ///< Info from @ref dataForPort carried across a live-coding reload.
	map<string, map<string, dispatch_semaphore_t> > carriedOverNodeSemaphoreForPort;  ///< Info from @ref nodeSemaphoreForPort carried across a live-coding reload.
	map<string, map<string, unsigned long> > carriedOverNodeIndexForPort;  ///< Info from @ref nodeIndexForPort carried across a live-coding reload.
	map<string, map<string, unsigned long> > carriedOverTypeIndexForPort;  ///< Info from @ref typeIndexForPort carried across a live-coding reload.

	static const unsigned long topLevelCompositionIndex;  ///< The index for the top-level composition's node context in @ref nodeContextForIndex.
	static const unsigned long invalidCompositionIndex;  ///< Used for error conditions.

	VuoRuntimePersistentState *persistentState;  ///< Reference to the parent VuoRuntimePersistentState.

	static void splitCompositionIdentifier(const string &compositionIdentifier, string &parentCompositionIdentifier, string &nodeIdentifier);
	static string buildCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier);
	static void splitPortIdentifier(const string &portIdentifier, string &nodeIdentifier, string &portName);

	string getNodeIdentifierForIndex(const char *compositionIdentifier, unsigned long nodeIndex);
	unsigned long getNodeIndexForIdentifier(const string &compositionIdentifier, const string &nodeIdentifier);
	const NodeMetadata * getNodeMetadataForPort(const string &compositionIdentifier, const string &portIdentifier);
	string getCompositionIdentifierForHash(unsigned long compositionIdentifierHash);
	void addNodeContext(const char *compositionIdentifier, unsigned long nodeIndex, struct NodeContext *nodeContext);
	void removeNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	void relocateNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	NodeContext * carryOverNodeContext(const char *oldCompositionIdentifier, const char *newCompositionIdentifier, unsigned long nodeIndex);

	template<typename T>
	static bool findCachedInfoForPort(const map<string, map<string, T> > &cachedInfoForPort, const string &compositionIdentifier, const string &portIdentifier, typename map<string, T>::const_iterator &foundIter);
	void addPortIdentifier(const char *compositionIdentifier, const string &portIdentifier, void *data, unsigned long nodeIndex, unsigned long typeIndex);
	void removePortIdentifier(const char *compositionIdentifier, const string &portIdentifier);
	void relocatePortIdentifier(const char *compositionIdentifier, const string &portIdentifier);
	void carryOverPortIdentifier(const char *oldCompositionIdentifier, const char *newCompositionIdentifier, const string &portIdentifier, unsigned long nodeIndex, unsigned long typeIndex);
	void carryOverPortIdentifiersForNode(const char *oldCompositionIdentifier, const char *newCompositionIdentifier, const string &nodeIdentifier, unsigned long nodeIndex, const vector<string> &portIdentifiers, const vector<unsigned long> typeIndexes);
	void removeCarriedOverPortIdentifier(const char *compositionIdentifier, const string &oldPortIdentifier);
	void carryOverPortData(const char *oldCompositionIdentifier, const char *newCompositionIdentifier, const string &oldPortIdentifier, const string &newPortIdentifier, PortContext *newPortContext);

	void initContextsForCompositionContents(VuoCompositionState *compositionState);
	void finiContextsForCompositionContents(VuoCompositionState *compositionState);

	void print(void);

	/// @{
	/**
	 * Defined in the composition's generated code.
	 */
	typedef const char *vuoTopLevelCompositionIdentifierType;
	vuoTopLevelCompositionIdentifierType vuoTopLevelCompositionIdentifier;
	/// @}

public:
	VuoNodeRegistry(VuoRuntimePersistentState *persistentState);
	void updateCompositionSymbols(void *compositionBinaryHandle);

	const char * defaultToTopLevelCompositionIdentifier(const char *compositionIdentifier);

	void addNodeMetadata(const char *compositionIdentifier, const char *nodeIdentifier,
						 NodeContext *(*compositionCreateContextForNode)(unsigned long),
						 void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool),
						 char * (*compositionGetPortValue)(VuoCompositionState *, const char *, int, bool),
						 void (*compositionFireTriggerPortEvent)(VuoCompositionState *, const char *),
						 void (*compositionReleasePortData)(void *, unsigned long));
	void addPortMetadata(const char *compositionIdentifier, const char *portIdentifier, const char *portName,
						 unsigned long typeIndex, const char *initialValue);
	NodeContext * getNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	NodeContext * getCompositionContext(const char *compositionIdentifier);
	void * getDataForPort(const char *compositionIdentifier, const char *portIdentifier);
	unsigned long getNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier);
	unsigned long getTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier);

	void initContextForTopLevelComposition(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount);
	void finiContextForTopLevelComposition(VuoCompositionState *compositionState);

	void setPortValue(VuoCompositionState *compositionState, const char *portIdentifier, const char *valueAsString);
	char * getPortValue(VuoCompositionState *compositionState, const char *portIdentifier, bool shouldUseInterprocessSerialization);
	char * getPortSummary(VuoCompositionState *compositionState, const char *portIdentifier);
	void fireTriggerPortEvent(VuoCompositionState *compositionState, const char *portIdentifier);
};

extern "C"
{
void vuoAddNodeMetadata(VuoCompositionState *compositionState, const char *nodeIdentifier,
						NodeContext *(*compositionCreateContextForNode)(unsigned long),
						void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool),
						char * (*compositionGetPortValue)(VuoCompositionState *, const char *, int, bool),
						void (*compositionFireTriggerPortEvent)(VuoCompositionState *, const char *),
						void (*compositionReleasePortData)(void *, unsigned long));
void vuoAddPortMetadata(VuoCompositionState *compositionState, const char *portIdentifier, const char *portName,
						unsigned long typeIndex, const char *initialValue);
NodeContext * vuoGetNodeContext(VuoCompositionState *compositionState, unsigned long nodeIndex);
NodeContext * vuoGetCompositionContext(VuoCompositionState *compositionState);
void * vuoGetDataForPort(VuoCompositionState *compositionState, const char *portIdentifier);
unsigned long vuoGetNodeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier);
unsigned long vuoGetTypeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier);
void vuoInitContextForTopLevelComposition(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount);
void vuoFiniContextForTopLevelComposition(VuoCompositionState *compositionState);
}
