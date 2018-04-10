/**
 * @file
 * VuoRuntimeHelper implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

#include <stdexcept>
#include <sstream>

extern "C"
{

struct PortContext;
struct NodeContext;

//@{
/**
 * @internal
 * Defined in VuoRuntimeContext.
 */
extern NodeContext * vuoCreateNodeContext(bool hasInstanceData, bool isComposition, size_t outputEventCount);
extern void vuoFreeNodeContext(NodeContext *nodeContext);
extern PortContext * vuoGetNodeContextPortContext(NodeContext *nodeContext, size_t index);
extern dispatch_semaphore_t vuoGetNodeContextSemaphore(struct NodeContext *nodeContext);
extern void vuoRetainPortContextData(struct PortContext *portContext);
extern void * vuoGetPortContextData(struct PortContext *portContext);
//@}

bool isPaused = false;  ///< True if node execution is currently paused.

dispatch_group_t vuoTriggerWorkersScheduled = NULL;  ///< Keeps track of trigger workers that have been scheduled but have not yet launched an event into the composition.

unsigned long vuoLastEventId = 0;  ///< The ID most recently assigned to any event, composition-wide. Used to generate a unique ID for each event.

namespace
{
/**
 * A worker waiting for access to a thread pool.
 */
struct Worker
{
	dispatch_queue_t queue;
	void *context;
	void (*function)(void *);

	bool isTrigger;

	int minThreadsNeeded;
	int maxThreadsNeeded;

	unsigned long eventId;
	unsigned long compositionHash;
	unsigned long chainIndex;

	int chainCount;  ///< For trigger workers: the number of chains downstream.

	unsigned long *upstreamChainIndices;  ///< For chain workers: the chains immediately upstream.
	int upstreamChainIndicesCount;
};

/**
 * A queue-ish data structure that allows an item to be enqueued by one thread
 * while older items are accessed by another thread.
 *
 * Like a queue, this data structure stores items in the order they were added.
 * But it's not a true queue since items can be removed from anywhere.
 */
class WorkerQueue
{
public:
	class Node
	{
	public:
		Worker worker;
		Node *next;
		Node *prev;
	};

	Node *first;  ///< The newest item in the queue.
	Node *last;  ///< The oldest item in the queue.

	WorkerQueue(void)
	{
		first = NULL;
		last = NULL;
	}

	/**
	 * Adds an item to the queue. It's thread-safe for one thread to call this function
	 * while another accesses existing items in the queue or iterates with getNextOldest().
	 */
	void enqueue(const Worker &worker)
	{
		Node *node = new Node;
		node->worker = worker;
		node->next = NULL;
		node->prev = NULL;

		if (! first)
		{
			last = node;
		}
		else
		{
			first->prev = node;
			node->next = first;
		}

		first = node;
	}

	/**
	 * Dequeues the given item. Not thread-safe.
	 */
	void dequeue(Node *node)
	{
		if (node == last)
			last = node->prev;
		else
			node->next->prev = node->prev;

		if (node == first)
			first = node->next;
		else
			node->prev->next = node->next;

		delete node;
	}

	/**
	 * Returns the item that is next oldest to @a node, or NULL if no such node is found.
	 * The search is limited to nodes no newer than @a newest, making this function thread-safe
	 * to call concurrently with enqueue().
	 */
	Node * getNextOldest(Node *newest, Node *node)
	{
		if (node == newest)
			return NULL;
		else
			return node->prev;
	}

	/**
	 * Returns the oldest item in the queue. Not thread-safe.
	 */
	Node * getOldest(void)
	{
		return last;
	}

	/**
	 * Returns the newest item in the queue. Not thread-safe.
	 */
	Node * getNewest(void)
	{
		return first;
	}

	/**
	 * Returns the number of items in the queue. Not thread-safe.
	 */
	size_t size(void)
	{
		size_t s = 0;
		for (Node *curr = first; curr != NULL; curr = curr->next)
			++s;
		return s;
	}
};

/**
 * A thread pool that keeps track of the workers currently using threads.
 *
 * Trigger workers are identified by their event ID, chain workers by their chain index.
 */
class ThreadPool
{
public:
	int totalThreads;
	int threadsAvailable;
	map<unsigned long, int> workersClaimingThreads;

	int totalWorkers;
	set<unsigned long> workersCompleted;

	ThreadPool(void)
	{
		totalThreads = 0;
		threadsAvailable = 0;
		totalWorkers = 0;
	}

	void setTotalThreads(int totalThreads)
	{
		this->totalThreads = totalThreads;
		this->threadsAvailable = totalThreads;
	}

	bool tryClaimThreads(int minThreadsNeeded, int maxThreadsNeeded, unsigned long workerId, int &threadsClaimed)
	{
		if (minThreadsNeeded <= threadsAvailable)
		{
			threadsClaimed = min(threadsAvailable, maxThreadsNeeded);
			threadsAvailable -= threadsClaimed;
			workersClaimingThreads[workerId] = threadsClaimed;
			return true;
		}
		else
		{
			threadsClaimed = 0;
			return false;
		}
	}

	int getThreadsClaimed(unsigned long workerId)
	{
		map<unsigned long, int>::iterator iter = workersClaimingThreads.find(workerId);
		if (iter != workersClaimingThreads.end())
			return iter->second;
		else
			throw std::logic_error("Couldn't find worker in thread pool to get its claimed threads.");
	}

	void returnThreads(unsigned long workerId)
	{
		map<unsigned long, int>::iterator iter = workersClaimingThreads.find(workerId);
		if (iter != workersClaimingThreads.end())
		{
			threadsAvailable += iter->second;
			if (totalWorkers > 0)
				workersCompleted.insert(iter->first);
			workersClaimingThreads.erase(iter);
		}
		else
			throw std::logic_error("Couldn't find worker in thread pool to return its threads.");
	}
};
}

static WorkerQueue workersWaitingForThreads;  ///< Queue of workers waiting to use threads from the pool.
ThreadPool mainThreadPool;  ///< Trigger workers claim threads from this thread pool.
map<unsigned long, map<unsigned long, ThreadPool> > triggerThreadPools;  ///< Chain workers claim threads from the thread pool for their event ID and hashed composition identifier.
static dispatch_queue_t workersWaitingSync;  ///< Synchronizes access to `workersWaitingForThreads`.
static dispatch_queue_t threadPoolSync;  ///< Synchronizes access to `mainThreadPool` and `triggerThreadPools`.
static dispatch_semaphore_t workersUpdated;  ///< Notifies `dequeueWorker()` when there may be a new worker available to dequeue.
static bool mayMoreWorkersBeEnqueued;  ///< Becomes true when the composition is stopping, indicating that `dequeueWorker()` is now flushing out the remaining workers and shouldn't expect new events.
static bool mayMoreWorkersBeDequeued;  ///< Becomes true when `dequeueWorker()` has finished flushing out the remaining workers.

static map<unsigned long, map<unsigned long, struct NodeContext *> > nodeContextForIndex;  ///< A registry of all NodeContext values in the running composition, indexed by hashed composition identifier and node index.
static map<unsigned long, string> compositionIdentifierForHash;  ///< The composition identifier for each hash registered in `nodeContextForIndex`.
static map<string, map<string, void *> > dataForPort;  ///< The `data` field in the port's context, indexed by composition and port identifier.
static map<string, map<string, dispatch_semaphore_t> > nodeSemaphoreForPort;  ///< The `semaphore` field in the node's context, indexed by composition and port identifier.
static map<string, map<string, unsigned long> > nodeIndexForPort;  ///< The index for a node, indexed by composition and port identifier.
static map<string, map<string, unsigned long> > typeIndexForPort;  ///< The index for the port's type, indexed by composition and port identifier.

static map<string, map<string, struct NodeContext *> > carriedOverNodeContextForIdentifier;  ///< Info from `nodeContextForIndex` carried across a live-coding reload, indexed by composition identifier and node identifier.
static map<string, map<string, void *> > carriedOverDataForPort;  ///< Info from `dataForPort` carried across a live-coding reload.
static map<string, map<string, dispatch_semaphore_t> > carriedOverNodeSemaphoreForPort;  ///< Info from `nodeSemaphoreForPort` carried across a live-coding reload.
static map<string, map<string, unsigned long> > carriedOverNodeIndexForPort;  ///< Info from `nodeIndexForPort` carried across a live-coding reload.
static map<string, map<string, unsigned long> > carriedOverTypeIndexForPort;  ///< Info from `typeIndexForPort` carried across a live-coding reload.

char *vuoTopLevelCompositionIdentifier;  ///< The composition identifier of the top-level composition.
static const unsigned long topLevelCompositionIndex = ULONG_MAX;  ///< The index for the top-level composition's node context in `nodeContextForIndex`.
static const unsigned long invalidCompositionIndex = ULONG_MAX - 1;  ///< Used for error conditions.

char *compositionDiff = NULL;  ///< Differences between the old and new composition, when replacing compositions for live coding.

namespace
{
/**
 * Possible changes to a node across a live-coding reload.
 */
enum ChangeType
{
	ChangeStartStop,  ///< The composition is being started or stopped (not a live-coding reload).
	ChangeNone,  ///< The node is carried across the live-coding reload.
	ChangeAdd,  ///< The node has been added.
	ChangeRemove,  ///< The node has been removed.
	ChangeReplace  ///< The node is being removed with a replacement provided or added as a replacement for another.
};
}

extern void vuoSetPortContextData(struct PortContext *portContext, void *data);


/**
 * Returns an integer hash for a C string.
 */
static unsigned long hash(const char *str)
{
	// sdbm algorithm (http://www.cse.yorku.ca/~oz/hash.html) —
	// very low probability of collisions (http://programmers.stackexchange.com/a/145633/38390)

	unsigned long hash = 0;
	int c;

	while ((c = *str++))
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static vector<Worker> workersDequeued;  ///< Temporary storage in dequeueWorkers(), made global to avoid the cost of reallocating with every call.
static vector<WorkerQueue::Node *> workersDequeuedNodes;  ///< Temporary storage in dequeueWorkers(), made global to avoid the cost of reallocating with every call.

/**
 * Chooses one or more workers, from those waiting for threads, to be the next workers to get threads.
 *
 * This function goes through the enqueued workers in order from oldest to newest. It chooses a worker if one of the
 * following is true:
 *    - It's a trigger worker for a new event, there are enough threads available in `mainThreadPool`, and no trigger
 *      worker earlier in the queue has been skipped over because it needs more threads than are available.
 *    - It's a trigger worker for the published input trigger of a subcomposition node.
 *    - It's a chain worker, there are enough threads available in `triggerThreadPools`, and no other chain worker in
 *      the queue has a smaller chain index (i.e., comes before this chain in topological order).
 *
 * If no eligible workers are available, this function blocks until some become available.
 */
static vector<Worker> dequeueWorkers(void)
{
	workersDequeued.clear();
	workersDequeuedNodes.clear();

	while (workersDequeued.empty() && mayMoreWorkersBeDequeued)
	{
		dispatch_semaphore_wait(workersUpdated, DISPATCH_TIME_FOREVER);

		__block WorkerQueue::Node *newest;
		__block WorkerQueue::Node *oldest;
		dispatch_sync(workersWaitingSync, ^{
						  newest = workersWaitingForThreads.getNewest();
						  oldest = workersWaitingForThreads.getOldest();
					  });

		// During this block, additional workers may be enqueued in `workersWaitingForThreads`,
		// but the section of the queue from `oldest` to `newest` remains unchanged.
		dispatch_sync(threadPoolSync, ^{
						  bool hasTriggerBeenSkipped = false;

						  for (WorkerQueue::Node *n = oldest; n != NULL; n = workersWaitingForThreads.getNextOldest(newest, n))
						  {
							  Worker w = n->worker;

							  if (w.isTrigger)
							  {
								  if (w.minThreadsNeeded >= 0 && w.maxThreadsNeeded >= 0)
								  {
									  // Trigger worker for new event

									  if (! hasTriggerBeenSkipped)
									  {
										  int threadsClaimed;
										  bool gotThreads = mainThreadPool.tryClaimThreads(w.minThreadsNeeded, w.maxThreadsNeeded, w.eventId, threadsClaimed);
										  if (gotThreads)
										  {
											  ThreadPool &triggerThreadPool = triggerThreadPools[w.eventId][w.compositionHash];
											  triggerThreadPool.setTotalThreads(threadsClaimed);
											  triggerThreadPool.totalWorkers = w.chainCount;
											  workersDequeuedNodes.push_back(n);
										  }
										  else
										  {
											  hasTriggerBeenSkipped = true;
										  }
									  }
								  }
								  else
								  {
									  // Trigger worker for published input trigger of a subcomposition node

									  triggerThreadPools[w.eventId][w.compositionHash].totalWorkers = w.chainCount;
									  workersDequeuedNodes.push_back(n);
								  }
							  }
							  else
							  {
								  // Chain worker

								  ThreadPool &triggerThreadPool = triggerThreadPools[w.eventId][w.compositionHash];

								  bool haveAllUpstreamChainsClaimedThreads = true;
								  for (int i = 0; i < w.upstreamChainIndicesCount; ++i)
								  {
									  unsigned long index = w.upstreamChainIndices[i];

									  map<unsigned long, int>::iterator iter1 = triggerThreadPool.workersClaimingThreads.find( index );
									  if (iter1 == triggerThreadPool.workersClaimingThreads.end())
									  {
										  set<unsigned long>::iterator iter = triggerThreadPool.workersCompleted.find( index );
										  if (iter == triggerThreadPool.workersCompleted.end())
										  {
											  haveAllUpstreamChainsClaimedThreads = false;
											  break;
										  }
									  }
								  }

								  if (haveAllUpstreamChainsClaimedThreads)
								  {
									  int threadsClaimed;
									  bool gotThreads = triggerThreadPool.tryClaimThreads(w.minThreadsNeeded, w.maxThreadsNeeded, w.chainIndex, threadsClaimed);
									  if (gotThreads)
									  {
										  workersDequeuedNodes.push_back(n);

										  free(w.upstreamChainIndices);
										  w.upstreamChainIndices = NULL;
									  }
								  }
							  }

							  if (n == newest) {
								  break;
							  }
						  }
					  });

		dispatch_sync(workersWaitingSync, ^{
						  for (vector<WorkerQueue::Node *>::iterator i = workersDequeuedNodes.begin(); i != workersDequeuedNodes.end(); ++i)
						  {
							  WorkerQueue::Node *n = *i;
							  workersDequeued.push_back(n->worker);
							  workersWaitingForThreads.dequeue(n);
						  }

						  if (! mayMoreWorkersBeEnqueued && workersWaitingForThreads.first == NULL) {
							  mayMoreWorkersBeDequeued = false;
						  }
					  });
	}

	return workersDequeued;
}

/**
 * Sets up the pool of worker threads.
 */
void vuoInitWorkerThreadPool(void)
{
	mainThreadPool.setTotalThreads(60);  // maximum number of worker threads in use simultaneously
	workersWaitingSync = dispatch_queue_create("org.vuo.runtime.workersWaiting", NULL);
	threadPoolSync = dispatch_queue_create("org.vuo.runtime.threadPool", NULL);
	workersUpdated = dispatch_semaphore_create(0);
	mayMoreWorkersBeEnqueued = true;
	mayMoreWorkersBeDequeued = true;
	dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

	// Dequeue workers.
	dispatch_async(globalQueue, ^{
					   while (mayMoreWorkersBeDequeued)
					   {
						   vector<Worker> workers = dequeueWorkers();

						   for (vector<Worker>::iterator i = workers.begin(); i != workers.end(); ++i) {
							   dispatch_async_f((*i).queue, (*i).context, (*i).function);
						   }
					   }
				   });
}

/**
 * Cleans up the pool of worker threads.
 */
void vuoFiniWorkerThreadPool(void)
{
	dispatch_sync(workersWaitingSync, ^{
					  mayMoreWorkersBeEnqueued = false;
				  });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Schedules a trigger worker function to be called when enough threads are available from the thread pool.
 *
 * For the published input trigger of a subcomposition node, pass -1 for @a minThreadsNeeded and @a maxThreadsNeeded.
 *
 * After this function is called, either `vuoReturnThreadsForTriggerWorker()` should be called for the trigger or
 * `vuoReturnThreadsForChainWorker()` should be called for each chain downstream of the trigger.
 */
void vuoScheduleTriggerWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
							  int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, char *compositionIdentifier,
							  int chainCount)
{
	// Assumes mainThreadPool.totalThreads is constant, thus safe to access outside of threadPoolSync.
	int adjustedMinThreadsNeeded = minThreadsNeeded;
	if (adjustedMinThreadsNeeded > mainThreadPool.totalThreads) {
		adjustedMinThreadsNeeded = mainThreadPool.totalThreads;
		VUserLog("Warning: Couldn't allocate as many threads to a trigger worker as it requires.");
	}

	unsigned long compositionHash = hash(compositionIdentifier);
	Worker worker = { queue, context, function, true, adjustedMinThreadsNeeded, maxThreadsNeeded, eventId, compositionHash, ULONG_MAX, chainCount, NULL, 0 };

	dispatch_sync(workersWaitingSync, ^{
					   workersWaitingForThreads.enqueue(worker);
				   });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Schedules a chain worker function to be called when enough threads are available from the thread pool.
 */
void vuoScheduleChainWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
							int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, char *compositionIdentifier,
							unsigned long chainIndex, unsigned long *upstreamChainIndices, int upstreamChainIndicesCount)
{
	unsigned long compositionHash = hash(compositionIdentifier);
	Worker worker = { queue, context, function, false, minThreadsNeeded, maxThreadsNeeded, eventId, compositionHash, chainIndex, -1, upstreamChainIndices, upstreamChainIndicesCount };

	dispatch_sync(workersWaitingSync, ^{
					   workersWaitingForThreads.enqueue(worker);
				   });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Allows the published input trigger of a subcomposition node to use the threads allocated for the chain containing the node.
 *
 * This function should be called before `vuoScheduleTriggerWorker()` is called for the published input trigger.
 */
void vuoGrantThreadsToSubcomposition(unsigned long eventId, char *compositionIdentifier, unsigned long chainIndex, char *subcompositionIdentifier)
{
	unsigned long compositionHash = hash(compositionIdentifier);
	unsigned long subcompositionHash = hash(subcompositionIdentifier);

	dispatch_sync(threadPoolSync, ^{
					  map<unsigned long, ThreadPool> &eventThreadPools = triggerThreadPools[eventId];
					  int threadsClaimedForChain = eventThreadPools[compositionHash].getThreadsClaimed(chainIndex);
					  eventThreadPools[subcompositionHash].setTotalThreads(threadsClaimedForChain);
				  });
}

/**
 * Returns the threads allocated for the trigger worker to the thread pool.
 */
void vuoReturnThreadsForTriggerWorker(unsigned long eventId)
{
	dispatch_sync(threadPoolSync, ^{
					   triggerThreadPools.erase(eventId);
					   mainThreadPool.returnThreads(eventId);
				   });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Returns the threads allocated for the chain worker to the thread pool.
 */
void vuoReturnThreadsForChainWorker(unsigned long eventId, char *compositionIdentifier, unsigned long chainIndex)
{
	unsigned long compositionHash = hash(compositionIdentifier);

	dispatch_sync(threadPoolSync, ^{
					  ThreadPool &triggerThreadPool = triggerThreadPools[eventId][compositionHash];
					  triggerThreadPool.returnThreads(chainIndex);

					  // If this was the last chain for the event (for the current subcomposition or overall),
					  // erase the thread pool entries for the event.
					  set<unsigned long> &workersCompleted = triggerThreadPool.workersCompleted;
					  if (workersCompleted.size() == triggerThreadPool.totalWorkers)
					  {
						  map<unsigned long, ThreadPool> &eventThreadPools = triggerThreadPools[eventId];
						  eventThreadPools.erase(compositionHash);
						  if (eventThreadPools.empty())
						  {
							  triggerThreadPools.erase(eventId);
							  mainThreadPool.returnThreads(eventId);
						  }
					  }
				  });

	dispatch_semaphore_signal(workersUpdated);
}


/**
 * Returns a unique event ID.
 */
unsigned long vuoGetNextEventId(void)
{
	return __sync_add_and_fetch(&vuoLastEventId, 1);
}


/**
 * If @a compositionIdentifier refers to a subcomposition, outputs the identifier of the parent composition
 * and the subcomposition node within the parent composition. Otherwise, outputs @a compositionIdentifier as
 * the parent composition identifier.
 *
 * This needs to be kept in sync with `VuoCompilerNode::generateSubcompositionIdentifierValue()`.
 */
static void splitCompositionIdentifier(const string &compositionIdentifier, string &parentCompositionIdentifier, string &nodeIdentifier)
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
static string joinCompositionIdentifier(const string &parentCompositionIdentifier, const string &nodeIdentifier)
{
	return parentCompositionIdentifier + "__" + nodeIdentifier;
}

/**
 * Constructs the identifier of a port on a node.
 *
 * This needs to be kept in sync with `VuoCompilerPort::getIdentifier()`.
 */
static string joinPortIdentifier(const string &nodeIdentifier, const string &portName)
{
	return nodeIdentifier + "__" + portName;
}


namespace
{
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
}

/**
 * The metadata for each node, by composition identifier and node index.
 * A node's index can change across a live-coding reload.
 */
static map<string, vector<NodeMetadata> > nodeMetadatas;

/**
 * Node metadata for nodes within subcompositions that are kept across a live-coding reload.
 * The indexes for these nodes remain the same across the live-coding reload.
 */
static map<string, vector<NodeMetadata> > carriedOverNodeMetadatas;

/**
 * Registers metadata for a node. When a (sub)composition is added or recompiled, this function should be
 * called for each node in the composition, in the same order as `VuoCompilerBitcodeGenerator::orderedNodes`.
 */
void vuoAddNodeMetadata(const char *compositionIdentifier, const char *nodeIdentifier)
{
	NodeMetadata nodeMetadata = { nodeIdentifier };
	nodeMetadatas[compositionIdentifier].push_back(nodeMetadata);
}

/**
 * Registers metadata for a port. After vuoAddNodeMetadata() is called for a node, this function should be
 * called for each port on the node, in the same order that ports are added to `NodeContext.portContexts`.
 */
void vuoAddPortMetadata(const char *compositionIdentifier, const char *portIdentifier, const char *portName,
						unsigned long typeIndex, const char *initialValue)
{
	PortMetadata portMetadata = { portIdentifier, portName, typeIndex, initialValue };
	nodeMetadatas[compositionIdentifier].back().portMetadatas.push_back(portMetadata);
}

/**
 * Looks up the node identifier for the node with the given index.
 */
static string vuoGetNodeIdentifierForIndex(const char *compositionIdentifier, unsigned long nodeIndex)
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
static unsigned long vuoGetNodeIndexForIdentifier(const string &compositionIdentifier, const string &nodeIdentifier)
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
static string vuoGetCompositionIdentifierForHash(unsigned long compositionIdentifierHash)
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
static void vuoAddNodeContext(const char *compositionIdentifier, unsigned long nodeIndex, struct NodeContext *nodeContext)
{
	unsigned long compositionIdentifierHash = hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, struct NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, struct NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
			VUserLog("Context overwritten for node %s:%s", compositionIdentifier, vuoGetNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
	}

	nodeContextForIndex[compositionIdentifierHash][nodeIndex] = nodeContext;

	compositionIdentifierForHash[ hash(compositionIdentifier) ] = compositionIdentifier;
}

/**
 * Un-registers a node context.
 */
static void vuoRemoveNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	unsigned long compositionIdentifierHash = hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, struct NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, struct NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
		{
			iter1->second.erase(iter2);
			if (iter1->second.empty())
				nodeContextForIndex.erase(iter1);

			string nodeIdentifier = vuoGetNodeIdentifierForIndex(compositionIdentifier, nodeIndex);
			string subcompositionIdentifier = joinCompositionIdentifier(compositionIdentifier, nodeIdentifier);
			unsigned long subcompositionIdentifierHash = hash(subcompositionIdentifier.c_str());
			map<unsigned long, string>::iterator ciIter = compositionIdentifierForHash.find(subcompositionIdentifierHash);
			if (ciIter != compositionIdentifierForHash.end())
				compositionIdentifierForHash.erase(ciIter);

			return;
		}
	}

	VUserLog("Couldn't find context for node %s:%s", compositionIdentifier, vuoGetNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
}

/**
 * Preserves all remaining node contexts to be accessed after a live-coding reload, and clears the
 * list of registered node contexts.
 */
static void vuoRelocateAllNodeContexts(void)
{
	for (map<unsigned long, map<unsigned long, NodeContext *> >::iterator i = nodeContextForIndex.begin(); i != nodeContextForIndex.end(); ++i)
	{
		unsigned long compositionIdentifierHash = i->first;
		string compositionIdentifier = vuoGetCompositionIdentifierForHash(compositionIdentifierHash);

		map<string, NodeContext *> nodeContextForIdentifier;
		for (map<unsigned long, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			string nodeIdentifier = vuoGetNodeIdentifierForIndex(compositionIdentifier.c_str(), j->first);
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
static NodeContext * vuoCarryOverNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	NodeContext *nodeContext = NULL;
	string nodeIdentifier = vuoGetNodeIdentifierForIndex(compositionIdentifier, nodeIndex);

	{
		bool found = false;
		map<string, map<string, struct NodeContext *> >::iterator coIter1 = carriedOverNodeContextForIdentifier.find(compositionIdentifier);
		if (coIter1 != carriedOverNodeContextForIdentifier.end())
		{
			map<string, struct NodeContext *>::iterator coIter2 = coIter1->second.find(nodeIdentifier);
			if (coIter2 != coIter1->second.end())
			{
				nodeContext = coIter2->second;
				coIter1->second.erase(coIter2);
				vuoAddNodeContext(compositionIdentifier, nodeIndex, nodeContext);
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
		for (map<string, map<string, struct NodeContext *> >::iterator i = carriedOverNodeContextForIdentifier.begin(); i != carriedOverNodeContextForIdentifier.end(); ++i)
		{
			string currCompositionIdentifier = i->first;
			unsigned long currCompositionIdentifierHash = hash(currCompositionIdentifier.c_str());

			if (currCompositionIdentifier.substr(0, subcompositionIdentifier.length()) == subcompositionIdentifier)
			{
				map<unsigned long, NodeContext *> currNodeContextForIndex;
				for (map<string, NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					unsigned long currNodeIndex = vuoGetNodeIndexForIdentifier(currCompositionIdentifier, j->first);
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
static void vuoRemoveCarriedOverNodeContext(const char *compositionIdentifier, const string &nodeIdentifier)
{
	carriedOverNodeContextForIdentifier[compositionIdentifier].erase(nodeIdentifier);
}

/**
 * Removes all remaining items from the list of carried-over node contexts.
 */
static void vuoRemoveAllCarriedOverNodeContexts(void (*compositionDestroyNodeContext)(const char *, const char *, NodeContext *))
{
	for (map<string, map<string, struct NodeContext *> >::iterator i = carriedOverNodeContextForIdentifier.begin(); i != carriedOverNodeContextForIdentifier.end(); ++i)
		for (map<string, struct NodeContext *>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			compositionDestroyNodeContext(i->first.c_str(), j->first.c_str(), j->second);

	carriedOverNodeContextForIdentifier.clear();
}

/**
 * Returns the node context registered for the node index, or null if none is found.
 */
struct NodeContext * vuoGetNodeContext(const char *compositionIdentifier, unsigned long nodeIndex)
{
	unsigned long compositionIdentifierHash = hash(compositionIdentifier);

	map<unsigned long, map<unsigned long, struct NodeContext *> >::iterator iter1 = nodeContextForIndex.find(compositionIdentifierHash);
	if (iter1 != nodeContextForIndex.end())
	{
		map<unsigned long, struct NodeContext *>::iterator iter2 = iter1->second.find(nodeIndex);
		if (iter2 != iter1->second.end())
			return iter2->second;
	}

	VUserLog("Couldn't find context for node %s:%s", compositionIdentifier, vuoGetNodeIdentifierForIndex(compositionIdentifier, nodeIndex).c_str());
	return NULL;
}

/**
 * Returns the node context registered for the composition (top-level or subcomposition), or null if none is found.
 */
struct NodeContext * vuoGetCompositionContext(const char *compositionIdentifier)
{
	string parentCompositionIdentifier, subcompositionNodeIdentifier;
	splitCompositionIdentifier(compositionIdentifier, parentCompositionIdentifier, subcompositionNodeIdentifier);
	unsigned long subcompositionNodeIndex = vuoGetNodeIndexForIdentifier(parentCompositionIdentifier, subcompositionNodeIdentifier);
	return vuoGetNodeContext(parentCompositionIdentifier.c_str(), subcompositionNodeIndex);
}


/**
 * Returns the `data` field in a port's context, given the port's identifier.
 */
void * vuoGetDataForPort(const char *compositionIdentifier, const char *portIdentifier)
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
dispatch_semaphore_t vuoGetNodeSemaphoreForPort(const char *compositionIdentifier, const char *portIdentifier)
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
unsigned long vuoGetNodeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
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
unsigned long vuoGetTypeIndexForPort(const char *compositionIdentifier, const char *portIdentifier)
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
static void vuoAddPortIdentifier(const char *compositionIdentifier, const string &portIdentifier,
								 void *data, dispatch_semaphore_t nodeSemaphore,
								 unsigned long nodeIndex, unsigned long typeIndex)
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
static void vuoRemovePortIdentifier(const char *compositionIdentifier, const string &portIdentifier)
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
static void vuoRelocateAllPortIdentifiers(void)
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
static void vuoCarryOverPortIdentifier(const char *compositionIdentifier, const string &portIdentifier,
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
static void vuoCarryOverPortIdentifiersForNode(const char *compositionIdentifier, const string &nodeIdentifier, unsigned long nodeIndex,
											   const vector<string> &portIdentifiers, const vector<unsigned long> typeIndexes)
{
	for (size_t j = 0; j < portIdentifiers.size(); ++j)
		vuoCarryOverPortIdentifier(compositionIdentifier, portIdentifiers[j], nodeIndex, typeIndexes[j]);

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
static void vuoRemoveCarriedOverPortIdentifier(const char *compositionIdentifier, const string &oldPortIdentifier)
{
	carriedOverDataForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverNodeSemaphoreForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverNodeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);
	carriedOverTypeIndexForPort[compositionIdentifier].erase(oldPortIdentifier);
}

/**
 * Removes all remaining items from the list of carried-over port cache entries.
 */
static void vuoRemoveAllCarriedOverPortIdentifiers(void)
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
static void vuoCarryOverPortData(const char *compositionIdentifier, const string &oldPortIdentifier,
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
 * Searches `compositionDiff` for changes made to the node across a live-coding reload.
 *
 * This needs to be kept in sync with `VuoCompilerComposition::diffAgainstOlderComposition()`.
 */
static enum ChangeType findNodeInCompositionDiff(const char *nodeIdentifier, json_object **replacementObj)
{
	if (! compositionDiff)
		return ChangeStartStop;

	json_object *diff = json_tokener_parse(compositionDiff);
	if (! diff)
	{
		VUserLog("Couldn't parse the composition diff: %s", compositionDiff);
		return ChangeNone;
	}

	set<string> keysFound;
	int numChanges = json_object_array_length(diff);
	for (int i = 0; i < numChanges; ++i)
	{
		json_object *change = json_object_array_get_idx(diff, i);
		json_object_object_foreach(change, key, val)
		{
			if (! strcmp(nodeIdentifier, json_object_get_string(val)))
			{
				keysFound.insert(key);

				if (! strcmp(key, "map") || ! strcmp(key, "to"))
				{
					json_object_get(change);
					*replacementObj = change;
				}
			}
		}
	}

	json_object_put(diff);

	if (keysFound.find("add") != keysFound.end())
	{
		if (keysFound.find("to") != keysFound.end())
			return ChangeReplace;
		else
			return ChangeAdd;
	}
	else if (keysFound.find("remove") != keysFound.end())
	{
		if (keysFound.find("map") != keysFound.end())
			return ChangeReplace;
		else
			return ChangeRemove;
	}

	return ChangeNone;
}

/**
 * Returns true if the node is among the removals or replacees across a live-coding reload,
 * or if the composition is being stopped (not a live-coding reload).
 */
bool vuoIsNodeBeingRemovedOrReplaced(const char *nodeIdentifier, json_object **replacementObj)
{
	enum ChangeType changeType = findNodeInCompositionDiff(nodeIdentifier, replacementObj);
	return changeType == ChangeStartStop || changeType == ChangeRemove || changeType == ChangeReplace;
}

/**
 * Returns true if the node is among the additions or replacers across a live-coding reload,
 * or if the composition is being started (not a live-coding reload).
 */
bool vuoIsNodeBeingAddedOrReplaced(const char *nodeIdentifier, json_object **replacementObj)
{
	enum ChangeType changeType = findNodeInCompositionDiff(nodeIdentifier, replacementObj);
	return changeType == ChangeStartStop || changeType == ChangeAdd || changeType == ChangeReplace;
}

/**
 * Returns true if the port is among the replacees across a live-coding reload.
 */
static bool vuoIsPortBeingReplaced(const char *portName, json_object *replacementObj)
{
	json_object *portMappingArray = NULL;
	{
		json_object *o;
		if (json_object_object_get_ex(replacementObj, "ports", &o))
			portMappingArray = o;
	}
	if (! portMappingArray)
		return false;

	int numPortMappings = json_object_array_length(portMappingArray);
	for (int i = 0; i < numPortMappings; ++i)
	{
		json_object *portMapping = json_object_array_get_idx(portMappingArray, i);
		json_object *o;

		if (json_object_object_get_ex(portMapping, "map", &o) && ! strcmp(json_object_get_string(o), portName))
			return true;
	}

	return false;
}

/**
 * Returns true if the port is among the replacers across a live-coding reload.
 */
static bool vuoIsPortReplacingAnother(const char *portName, json_object *replacementObj,
									  string &oldNodeIdentifier, string &oldPortIdentifier)
{
	json_object *portMappingArray = NULL;
	{
		json_object *o;

		if (json_object_object_get_ex(replacementObj, "map", &o))
			oldNodeIdentifier = json_object_get_string(o);

		if (json_object_object_get_ex(replacementObj, "ports", &o))
			portMappingArray = o;
	}
	if (! portMappingArray)
		return false;

	string oldPortName;
	bool foundPort = false;
	int numPortMappings = json_object_array_length(portMappingArray);
	for (int i = 0; i < numPortMappings; ++i)
	{
		json_object *portMapping = json_object_array_get_idx(portMappingArray, i);
		json_object *o;

		if (json_object_object_get_ex(portMapping, "to", &o) && ! strcmp(json_object_get_string(o), portName))
		{
			if (json_object_object_get_ex(portMapping, "map", &o))
				oldPortName = json_object_get_string(o);

			foundPort = true;
			break;
		}
	}
	if (! foundPort)
		return false;

	oldPortIdentifier = joinPortIdentifier(oldNodeIdentifier, oldPortName);
	return true;
}


/**
 * Helper function for `compositionContextInit()`. If the composition contains subcomposition nodes,
 * this function calls each subcomposition's `compositionContextInit()`.
 */
NodeContext * vuoCompositionContextInitHelper(const char *compositionIdentifier, bool hasInstanceData, unsigned long publishedOutputPortCount,
											  NodeContext *(*compositionCreateNodeContext)(const char *, unsigned long),
											  void (*compositionDestroyNodeContext)(const char *, const char *, NodeContext *),
											  void (*compositionSetPortValue)(const char *, const char *, const char *, bool, bool, bool, bool, bool))
{
	NodeContext *compositionContext = NULL;

	bool isTopLevelComposition = ! strcmp(compositionIdentifier, vuoTopLevelCompositionIdentifier);
	if (isTopLevelComposition)
	{
		if (! compositionDiff)
		{
			// The top-level composition is starting for the first time. Create and register its context.
			compositionContext = vuoCreateNodeContext(hasInstanceData, true, publishedOutputPortCount);
			vuoAddNodeContext(compositionIdentifier, topLevelCompositionIndex, compositionContext);
		}
		else
		{
			// Restore the top-level composition's context from before the live-coding reload.
			compositionContext = vuoCarryOverNodeContext(compositionIdentifier, topLevelCompositionIndex);

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
		ChangeType changeType = (isTopLevelComposition ?
									 findNodeInCompositionDiff(nodeMetadata.identifier.c_str(), &replacementObj) :
									 ChangeAdd);

		if (changeType == ChangeStartStop || changeType == ChangeAdd || changeType == ChangeReplace)
		{
			// Create the added node's context.
			// If the node is a subcomposition, this calls compositionContextInit().
			nodeContext = compositionCreateNodeContext(compositionIdentifier, nodeIndex);

			// Register the added node's context.
			vuoAddNodeContext(compositionIdentifier, nodeIndex, nodeContext);

			dispatch_semaphore_t nodeSemaphore = vuoGetNodeContextSemaphore(nodeContext);

			// Add the added node's ports to the port cache.
			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				vuoAddPortIdentifier(compositionIdentifier, portMetadata.identifier, portData, nodeSemaphore, nodeIndex, portMetadata.typeIndex);
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
				if (changeType == ChangeReplace &&
						vuoIsPortReplacingAnother(portMetadata.name.c_str(), replacementObj, oldNodeIdentifier, oldPortIdentifier))
				{
					// Set the replacement port's data from the port it replaces.
					vuoCarryOverPortData(compositionIdentifier, oldPortIdentifier, portMetadata.identifier, portContext);

					// Remove the port being replaced from the port cache.
					vuoRemoveCarriedOverPortIdentifier(compositionIdentifier, oldPortIdentifier);
				}
				else
				{
					// Set the added port's data to its initial value.
					compositionSetPortValue(compositionIdentifier, portMetadata.identifier.c_str(), portMetadata.initialValue.c_str(),
											false, false, false, false, true);
				}
			}

			if (changeType == ChangeReplace && ! oldNodeIdentifier.empty())
			{
				// Remove the node context for the node being replaced.
				vuoRemoveCarriedOverNodeContext(compositionIdentifier, oldNodeIdentifier);
			}
		}
		else
		{
			// Restore the kept node's context.
			// If the node is a subcomposition, this also restores the contexts of all nodes within it.
			nodeContext = vuoCarryOverNodeContext(compositionIdentifier, nodeIndex);

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
			vuoCarryOverPortIdentifiersForNode(compositionIdentifier, nodeMetadata.identifier, nodeIndex, portIdentifiers, typeIndexes);
		}

		json_object_put(replacementObj);
	}

	if (isTopLevelComposition)
	{
		// Clean up the stuff carried over for nodes being replaced across the live-coding reload.
		vuoRemoveAllCarriedOverPortIdentifiers();
		vuoRemoveAllCarriedOverNodeContexts(compositionDestroyNodeContext);
	}

	return compositionContext;
}

/**
 * Helper function for `compositionContextFini()`. If the composition contains subcomposition nodes,
 * this function calls each subcomposition's `compositionContextFini()`.
 */
void vuoCompositionContextFiniHelper(const char *compositionIdentifier,
									 void (*compositionDestroyNodeContext)(const char *, const char *, NodeContext *),
									 void (*compositionReleasePortData)(void *, unsigned long))
{
	bool isTopLevelComposition = ! strcmp(compositionIdentifier, vuoTopLevelCompositionIdentifier);

	for (size_t nodeIndex = 0; nodeIndex < nodeMetadatas[compositionIdentifier].size(); ++nodeIndex)
	{
		NodeMetadata nodeMetadata = nodeMetadatas[compositionIdentifier][nodeIndex];

		json_object *replacementObj = NULL;
		ChangeType changeType = (isTopLevelComposition ?
									 findNodeInCompositionDiff(nodeMetadata.identifier.c_str(), &replacementObj):
									 ChangeRemove);

		if (changeType == ChangeStartStop || changeType == ChangeRemove || changeType == ChangeReplace)
		{
			NodeContext *nodeContext = vuoGetNodeContext(compositionIdentifier, nodeIndex);

			for (size_t portIndex = 0; portIndex < nodeMetadata.portMetadatas.size(); ++portIndex)
			{
				PortMetadata portMetadata = nodeMetadata.portMetadatas[portIndex];
				PortContext *portContext = vuoGetNodeContextPortContext(nodeContext, portIndex);
				void *portData = vuoGetPortContextData(portContext);

				if (changeType == ChangeReplace && vuoIsPortBeingReplaced(portMetadata.name.c_str(), replacementObj))
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
					vuoRemovePortIdentifier(compositionIdentifier, portMetadata.identifier);
				}
			}

			if (changeType == ChangeStartStop || changeType == ChangeRemove)
			{
				// Destroy the removed node's context.
				// If the node is a subcomposition, this calls compositionContextFini().
				compositionDestroyNodeContext(compositionIdentifier, nodeMetadata.identifier.c_str(), nodeContext);

				// Un-register the removed node's context.
				vuoRemoveNodeContext(compositionIdentifier, nodeIndex);
			}
		}

		json_object_put(replacementObj);
	}

	if (isTopLevelComposition)
	{
		if (! compositionDiff)
		{
			// The top-level composition is stopping for the last time. Un-register and destroy its context.
			NodeContext *compositionContext = vuoGetCompositionContext(compositionIdentifier);
			vuoRemoveNodeContext(compositionIdentifier, topLevelCompositionIndex);
			vuoFreeNodeContext(compositionContext);
		}
		else
		{
			// Carry over stuff for kept and being-replaced nodes for after the live-coding reload.
			// If any nodes are subcompositions, this also carries over stuff for all nodes within them.
			vuoRelocateAllPortIdentifiers();
			vuoRelocateAllNodeContexts();

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


/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings2(const char *s0, const char *s1)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strcat(buffer, s0);
	strcat(buffer, s1);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings3(const char *s0, const char *s1, const char *s2)
{
	size_t bufferLength = strlen(s0) + strlen(s1) + strlen(s2) + 1;
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	strcat(buffer, s0);
	strcat(buffer, s1);
	strcat(buffer, s2);
	return buffer;
}

/**
 * Returns the strings appended in order.
 */
char * vuoConcatenateStrings(const char **strings, size_t stringCount)
{
	size_t bufferLength = 1;
	for (size_t i = 0; i < stringCount; ++i)
		bufferLength += strlen(strings[i]);
	char *buffer = (char *)malloc(bufferLength);
	buffer[0] = 0;
	for (size_t i = 0; i < stringCount; ++i)
		strcat(buffer, strings[i]);
	return buffer;
}

}  // extern "C"
