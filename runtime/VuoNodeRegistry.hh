/**
 * @file
 * VuoNodeRegistry interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

class VuoRuntimePersistentState;
#include "VuoCompositionState.h"

extern "C"
{
#include "VuoRuntimeContext.h"
}

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
	};

	map<string, vector<NodeMetadata> > nodeMetadatas;  ///< The metadata for each node, by composition identifier and node index. A node's index can change across a live-coding reload.
	map<string, vector<NodeMetadata> > carriedOverNodeMetadatas;  ///< Node metadata for nodes within subcompositions that are kept across a live-coding reload. The indexes for these nodes remain the same across the live-coding reload.

	map<unsigned long, map<unsigned long, NodeContext *> > nodeContextForIndex;  ///< A registry of all NodeContext values in the running composition, indexed by hashed composition identifier and node index.
	map<unsigned long, string> compositionIdentifierForHash;  ///< The composition identifier for each hash registered in `nodeContextForIndex`.
	map<string, map<string, void *> > dataForPort;  ///< The `data` field in the port's context, indexed by composition and port identifier.
	map<string, map<string, dispatch_semaphore_t> > nodeSemaphoreForPort;  ///< The `semaphore` field in the node's context, indexed by composition and port identifier.
	map<string, map<string, unsigned long> > nodeIndexForPort;  ///< The index for a node, indexed by composition and port identifier.
	map<string, map<string, unsigned long> > typeIndexForPort;  ///< The index for the port's type, indexed by composition and port identifier.

	map<string, map<string, NodeContext *> > carriedOverNodeContextForIdentifier;  ///< Info from `nodeContextForIndex` carried across a live-coding reload, indexed by composition identifier and node identifier.
	map<string, map<string, void *> > carriedOverDataForPort;  ///< Info from `dataForPort` carried across a live-coding reload.
	map<string, map<string, dispatch_semaphore_t> > carriedOverNodeSemaphoreForPort;  ///< Info from `nodeSemaphoreForPort` carried across a live-coding reload.
	map<string, map<string, unsigned long> > carriedOverNodeIndexForPort;  ///< Info from `nodeIndexForPort` carried across a live-coding reload.
	map<string, map<string, unsigned long> > carriedOverTypeIndexForPort;  ///< Info from `typeIndexForPort` carried across a live-coding reload.

	static const unsigned long topLevelCompositionIndex;  ///< The index for the top-level composition's node context in `nodeContextForIndex`.
	static const unsigned long invalidCompositionIndex;  ///< Used for error conditions.

	VuoRuntimePersistentState *persistentState;  ///< Reference to the parent VuoRuntimePersistentState.

	static void splitCompositionIdentifier(const string &compositionIdentifier, string &parentCompositionIdentifier, string &nodeIdentifier);
	static string joinCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier);

	string getNodeIdentifierForIndex(const char *compositionIdentifier, unsigned long nodeIndex);
	unsigned long getNodeIndexForIdentifier(const string &compositionIdentifier, const string &nodeIdentifier);
	string getCompositionIdentifierForHash(unsigned long compositionIdentifierHash);
	void addNodeContext(const char *compositionIdentifier, unsigned long nodeIndex, struct NodeContext *nodeContext);
	void removeNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	void relocateAllNodeContexts(void);
	NodeContext * carryOverNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	void removeCarriedOverNodeContext(const char *compositionIdentifier, const string &nodeIdentifier);
	void removeAllCarriedOverNodeContexts(void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *));
	void addPortIdentifier(const char *compositionIdentifier, const string &portIdentifier, void *data, dispatch_semaphore_t nodeSemaphore, unsigned long nodeIndex, unsigned long typeIndex);
	void removePortIdentifier(const char *compositionIdentifier, const string &portIdentifier);
	void relocateAllPortIdentifiers(void);
	void carryOverPortIdentifier(const char *compositionIdentifier, const string &portIdentifier, unsigned long nodeIndex, unsigned long typeIndex);
	void carryOverPortIdentifiersForNode(const char *compositionIdentifier, const string &nodeIdentifier, unsigned long nodeIndex, const vector<string> &portIdentifiers, const vector<unsigned long> typeIndexes);
	void removeCarriedOverPortIdentifier(const char *compositionIdentifier, const string &oldPortIdentifier);
	void removeAllCarriedOverPortIdentifiers(void);
	void carryOverPortData(const char *compositionIdentifier, const string &oldPortIdentifier, const string &newPortIdentifier, PortContext *newPortContext);

	//@{
	/**
	 * Defined in the composition's generated code.
	 */
	typedef const char *vuoTopLevelCompositionIdentifierType;
	vuoTopLevelCompositionIdentifierType vuoTopLevelCompositionIdentifier;
	//@}

public:
	VuoNodeRegistry(VuoRuntimePersistentState *persistentState);
	void updateCompositionSymbols(void *compositionBinaryHandle);

	void addNodeMetadata(const char *compositionIdentifier, const char *nodeIdentifier);
	void addPortMetadata(const char *compositionIdentifier, const char *portIdentifier, const char *portName, unsigned long typeIndex, const char *initialValue);
	NodeContext * getNodeContext(const char *compositionIdentifier, unsigned long nodeIndex);
	NodeContext * getCompositionContext(const char *compositionIdentifier);
	void * getDataForPort(const char *compositionIdentifier, const char *portIdentifier);
	dispatch_semaphore_t getNodeSemaphoreForPort(const char *compositionIdentifier, const char *portIdentifier);
	unsigned long getNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier);
	unsigned long getTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier);
	NodeContext * compositionContextInitHelper(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount,
											   NodeContext *(*compositionCreateNodeContext)(VuoCompositionState *, unsigned long),
											   void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
											   void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool));
	void compositionContextFiniHelper(VuoCompositionState *compositionState,
									  void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
									  void (*compositionReleasePortData)(void *, unsigned long));

};

extern "C"
{
void vuoAddNodeMetadata(VuoCompositionState *compositionState, const char *nodeIdentifier);
void vuoAddPortMetadata(VuoCompositionState *compositionState, const char *portIdentifier, const char *portName,
						unsigned long typeIndex, const char *initialValue);
NodeContext * vuoGetNodeContext(VuoCompositionState *compositionState, unsigned long nodeIndex);
NodeContext * vuoGetCompositionContext(VuoCompositionState *compositionState);
void * vuoGetDataForPort(VuoCompositionState *compositionState, const char *portIdentifier);
dispatch_semaphore_t vuoGetNodeSemaphoreForPort(VuoCompositionState *compositionState, const char *portIdentifier);
unsigned long vuoGetNodeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier);
unsigned long vuoGetTypeIndexForPort(VuoCompositionState *compositionState, const char *portIdentifier);
NodeContext * vuoCompositionContextInitHelper(VuoCompositionState *compositionState, bool hasInstanceData, unsigned long publishedOutputPortCount,
											  NodeContext *(*compositionCreateNodeContext)(VuoCompositionState *, unsigned long),
											  void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
											  void (*compositionSetPortValue)(VuoCompositionState *, const char *, const char *, bool, bool, bool, bool, bool));
void vuoCompositionContextFiniHelper(VuoCompositionState *compositionState,
									 void (*compositionDestroyNodeContext)(VuoCompositionState *, const char *, NodeContext *),
									 void (*compositionReleasePortData)(void *, unsigned long));
}
