/**
 * @file
 * VuoThreadManager interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include <dispatch/dispatch.h>
#include <map>
#include <set>
#include <vector>
#include "VuoCompositionState.h"
using namespace std;

/**
 * Manages the number of threads used by a composition to avoid hitting the Dispatch Thread Soft Limit.
 *
 * When a composition needs to schedule a block of code that may need to wait on stuff, it requests a
 * thread from the thread manager. The thread manager, which manages several thread pools, schedules
 * the block of code when a thread becomes available. When the block of code completes, the composition
 * informs the thread manager, and the thread manager returns the threads to their thread pool.
 */
class VuoThreadManager
{
private:
	/**
	 * A worker waiting for access to a thread pool.
	 */
	class Worker
	{
	public:
		dispatch_queue_t queue;  ///< The dispatch queue on which to schedule the worker function.
		void *context;  ///< The argument to pass to the worker function.
		void (*function)(void *);  ///< The trigger or chain worker function.

		bool isTrigger;  ///< True if this is a trigger worker, false if this is a chain worker.

		int minThreadsNeeded;  ///< The minimum number of threads needed for the trigger/chain to complete without deadlock.
		int maxThreadsNeeded;  ///< The number of threads needed for the trigger/chain to execute with as much parallelism as possible.

		unsigned long eventId;  ///< The ID of the event that has prompted this trigger/chain to fire/execute.
		unsigned long compositionHash;  ///< The hashed ID of the (sub)composition containing this trigger/chain.

		int chainCount;  ///< For trigger workers: the number of chains downstream.

		unsigned long chainIndex;  ///< For chain workers: the index of the chain in the compiler's ordered list of chains.
		unsigned long *upstreamChainIndices;  ///< For chain workers: the indices of the chains immediately upstream.
		int upstreamChainIndicesCount;  ///< For chain workers: the number of items in upstreamChainIndices.

		Worker(dispatch_queue_t queue, void *context, void (*function)(void *), int minThreadsNeeded, int maxThreadsNeeded,
			   unsigned long eventId, unsigned long compositionHash, int chainCount);
		Worker(dispatch_queue_t queue, void *context, void (*function)(void *), int minThreadsNeeded, int maxThreadsNeeded,
			   unsigned long eventId, unsigned long compositionHash, unsigned long chainIndex,
			   unsigned long *upstreamChainIndices, int upstreamChainIndicesCount);
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
		/**
		 * An item in the queue.
		 */
		struct Node
		{
			Worker *worker;  ///< The item itself.
			Node *next;  ///< Link to the next (older) node.
			Node *prev;  ///< Link to the previous (newer) node.
		};

		Node *first;  ///< The newest item in the queue.
		Node *last;  ///< The oldest item in the queue.

		WorkerQueue(void);
		void enqueue(Worker *worker);
		void dequeue(Node *node);
		Node * getNextOldest(Node *newest, Node *node);
		Node * getOldest(void);
		Node * getNewest(void);
		size_t size(void);
	};

	/**
	 * A thread pool that keeps track of the workers currently using threads.
	 *
	 * Workers are identified by a worker ID. For trigger workers, this is the event ID. For chain workers, this is the chain index.
	 */
	class ThreadPool
	{
	public:
		int totalThreads;  ///< The maximum number of threads that can be used simultaneously by this thread pool.
		int threadsAvailable;  ///< The number of threads that are currently available to be used.
		map<unsigned long, int> workersClaimingThreads;  ///< For each worker ID, the number of threads being used.

		int totalWorkers;  ///< The number of chain workers to be completed by this thread pool (for thread pools that represent a single event).
		set<unsigned long> workersCompleted;  ///< The chain workers completed so far by this thread pool.

		ThreadPool(void);
		void setTotalThreads(int totalThreads);
		bool tryClaimThreads(int minThreadsNeeded, int maxThreadsNeeded, unsigned long workerId, int &threadsClaimed);
		int getThreadsClaimed(unsigned long workerId);
		void returnThreads(unsigned long workerId);
	};

	WorkerQueue workersWaitingForThreads;  ///< Queue of workers waiting to use threads from the pool.
	ThreadPool mainThreadPool;  ///< Trigger workers claim threads from this thread pool.
	map<unsigned long, map<unsigned long, ThreadPool> > triggerThreadPools;  ///< Chain workers claim threads from the thread pool for their event ID and hashed composition identifier.
	dispatch_queue_t workersWaitingSync;  ///< Synchronizes access to workersWaitingForThreads.
	dispatch_queue_t threadPoolSync;  ///< Synchronizes access to mainThreadPool and triggerThreadPools.
	dispatch_semaphore_t workersUpdated;  ///< Notifies dequeueWorker() when there may be a new worker available to dequeue.
	dispatch_semaphore_t completed;  ///< Notifies the destructor when the dequeueWorker() loop has completed.
	bool mayMoreWorkersBeEnqueued;  ///< Becomes true when the composition is stopping, indicating that dequeueWorker() is now flushing out the remaining workers and shouldn't expect new events.
	bool mayMoreWorkersBeDequeued;  ///< Becomes true when dequeueWorker() has finished flushing out the remaining workers.

	vector<Worker *> workersDequeued;  ///< Temporary storage in dequeueWorkers(), made persistent to avoid the cost of reallocating with every call.
	vector<WorkerQueue::Node *> workersDequeuedNodes;  ///< Temporary storage in dequeueWorkers(), made persistent to avoid the cost of reallocating with every call.

	vector<Worker *> dequeueWorkers(void);

public:
	VuoThreadManager(void);
	~VuoThreadManager(void);
	void enableSchedulingWorkers(void);
	void disableSchedulingWorkers(void);
	void scheduleTriggerWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
							   int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, const char *compositionIdentifier,
							   int chainCount);
	void scheduleChainWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
							 int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, const char *compositionIdentifier,
							 unsigned long chainIndex, unsigned long *upstreamChainIndices, int upstreamChainIndicesCount);
	void grantThreadsToChain(int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, const char *compositionIdentifier,
							 unsigned long chainIndex);
	void grantThreadsToSubcomposition(unsigned long eventId, const char *compositionIdentifier, unsigned long chainIndex, const char *subcompositionIdentifier);
	void returnThreadsForTriggerWorker(unsigned long eventId);
	void returnThreadsForChainWorker(unsigned long eventId, const char *compositionIdentifier, unsigned long chainIndex);
};

extern "C"
{
void vuoScheduleTriggerWorker(VuoCompositionState *compositionState, dispatch_queue_t queue, void *context, void (*function)(void *),
							  int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, int chainCount);
void vuoScheduleChainWorker(VuoCompositionState *compositionState, dispatch_queue_t queue, void *context, void (*function)(void *),
							int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, unsigned long chainIndex,
							unsigned long *upstreamChainIndices, int upstreamChainIndicesCount);
void vuoGrantThreadsToChain(VuoCompositionState *compositionState,
							int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, unsigned long chainIndex);
void vuoGrantThreadsToSubcomposition(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex,
									 char *subcompositionIdentifier);
void vuoReturnThreadsForTriggerWorker(VuoCompositionState *compositionState, unsigned long eventId);
void vuoReturnThreadsForChainWorker(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex);
}
