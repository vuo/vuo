/**
 * @file
 * VuoThreadManager implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoThreadManager.hh"

#include <stdexcept>
#include "VuoRuntimePersistentState.hh"
#include "VuoRuntimeState.hh"
#include "VuoRuntimeUtilities.hh"
#include "VuoEventLoop.h"

/**
 * Constructs a trigger worker.
 */
VuoThreadManager::Worker::Worker(dispatch_queue_t queue, void *context, void (*function)(void *),
								 int minThreadsNeeded, int maxThreadsNeeded,
								 unsigned long eventId, unsigned long compositionHash, int chainCount)
{
	this->queue = queue;
	this->context = context;
	this->function = function;
	this->isTrigger = true;
	this->minThreadsNeeded = minThreadsNeeded;
	this->maxThreadsNeeded = maxThreadsNeeded;
	this->eventId = eventId;
	this->compositionHash = compositionHash;
	this->chainIndex = ULONG_MAX;
	this->chainCount = chainCount;
	this->upstreamChainIndices = NULL;
	this->upstreamChainIndicesCount = 0;
}

/**
 * Constructs a chain worker.
 */
VuoThreadManager::Worker::Worker(dispatch_queue_t queue, void *context, void (*function)(void *),
								 int minThreadsNeeded, int maxThreadsNeeded,
								 unsigned long eventId, unsigned long compositionHash, unsigned long chainIndex,
								 unsigned long *upstreamChainIndices, int upstreamChainIndicesCount)
{
	this->queue = queue;
	this->context = context;
	this->function = function;
	this->isTrigger = false;
	this->minThreadsNeeded = minThreadsNeeded;
	this->maxThreadsNeeded = maxThreadsNeeded;
	this->eventId = eventId;
	this->compositionHash = compositionHash;
	this->chainIndex = chainIndex;
	this->chainCount = -1;
	this->upstreamChainIndices = upstreamChainIndices;
	this->upstreamChainIndicesCount = upstreamChainIndicesCount;
}

/**
 * Constructs and empty queue.
 */
VuoThreadManager::WorkerQueue::WorkerQueue(void)
{
	first = NULL;
	last = NULL;
}

/**
 * Adds an item to the queue. It's thread-safe for one thread to call this function
 * while another accesses existing items in the queue or iterates with getNextOldest().
 */
void VuoThreadManager::WorkerQueue::enqueue(Worker *worker)
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
void VuoThreadManager::WorkerQueue::dequeue(Node *node)
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
VuoThreadManager::WorkerQueue::Node * VuoThreadManager::WorkerQueue::getNextOldest(Node *newest, Node *node)
{
	if (node == newest)
		return NULL;
	else
		return node->prev;
}

/**
 * Returns the oldest item in the queue. Not thread-safe.
 */
VuoThreadManager::WorkerQueue::Node * VuoThreadManager::WorkerQueue::getOldest(void)
{
	return last;
}

/**
 * Returns the newest item in the queue. Not thread-safe.
 */
VuoThreadManager::WorkerQueue::Node * VuoThreadManager::WorkerQueue::getNewest(void)
{
	return first;
}

/**
 * Returns the number of items in the queue. Not thread-safe.
 */
size_t VuoThreadManager::WorkerQueue::size(void)
{
	size_t s = 0;
	for (Node *curr = first; curr != NULL; curr = curr->next)
		++s;
	return s;
}

/**
 * Constructs an empty thread pool with no threads available.
 */
VuoThreadManager::ThreadPool::ThreadPool(void)
{
	totalThreads = 0;
	threadsAvailable = 0;
	totalWorkers = 0;
}

/**
 * Sets the total and current number of threads available.
 */
void VuoThreadManager::ThreadPool::setTotalThreads(int totalThreads)
{
	this->totalThreads = totalThreads;
	this->threadsAvailable = totalThreads;
}

/**
 * If at least @a minThreadsNeeded threads are available in the thread pool, claims as many threads as possible
 * for this worker, up to @a maxThreadsNeeded, and returns true. Otherwise, does nothing and returns false.
 */
bool VuoThreadManager::ThreadPool::tryClaimThreads(int minThreadsNeeded, int maxThreadsNeeded, unsigned long workerId, int &threadsClaimed)
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

/**
 * Returns the number of threads currently claimed by the given worker.
 *
 * @throw std::logic_error The worker is not currently using the thread pool.
 */
int VuoThreadManager::ThreadPool::getThreadsClaimed(unsigned long workerId)
{
	map<unsigned long, int>::iterator iter = workersClaimingThreads.find(workerId);
	if (iter != workersClaimingThreads.end())
		return iter->second;
	else
		throw std::logic_error("Couldn't find worker in thread pool to get its claimed threads.");
}

/**
 * Puts the worker's threads back into the thread pool, making them available again.
 *
 * @throw std::logic_error The worker is not currently using the thread pool.
 */
void VuoThreadManager::ThreadPool::returnThreads(unsigned long workerId)
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


/**
 * Constructor.
 */
VuoThreadManager::VuoThreadManager(void)
{
	mainThreadPool.setTotalThreads(60);  // maximum number of worker threads in use simultaneously
	workersWaitingSync = dispatch_queue_create("org.vuo.runtime.workersWaiting", VuoEventLoop_getDispatchInteractiveAttribute());
	threadPoolSync = dispatch_queue_create("org.vuo.runtime.threadPool", VuoEventLoop_getDispatchInteractiveAttribute());
	workersUpdated = dispatch_semaphore_create(0);
	completed = dispatch_semaphore_create(0);
}

/**
 * Destructor.
 */
VuoThreadManager::~VuoThreadManager(void)
{
	dispatch_release(workersWaitingSync);
	dispatch_release(threadPoolSync);
	dispatch_release(workersUpdated);
}

/**
 * Starts dequeuing workers as they are scheduled.
 */
void VuoThreadManager::enableSchedulingWorkers(void)
{
	mayMoreWorkersBeEnqueued = true;
	mayMoreWorkersBeDequeued = true;

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
	dispatch_async(queue, ^{
					   while (mayMoreWorkersBeDequeued)
					   {
						   vector<Worker *> workers = dequeueWorkers();

						   for (vector<Worker *>::iterator i = workers.begin(); i != workers.end(); ++i)
						   {
							   Worker *w = *i;
							   dispatch_async_f(w->queue, w->context, w->function);
							   delete w;
						   }
					   }

					   dispatch_semaphore_signal(completed);
				   });
}

/**
 * Finishes dequeuing workers that have been scheduled.
 */
void VuoThreadManager::disableSchedulingWorkers(void)
{
	dispatch_sync(workersWaitingSync, ^{
					  mayMoreWorkersBeEnqueued = false;
				  });

	dispatch_semaphore_signal(workersUpdated);
	dispatch_semaphore_wait(completed, DISPATCH_TIME_FOREVER);

	mainThreadPool.workersCompleted.clear();
}

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
vector<VuoThreadManager::Worker *> VuoThreadManager::dequeueWorkers(void)
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
							  Worker *w = n->worker;

							  if (w->isTrigger)
							  {
								  if (w->minThreadsNeeded >= 0 && w->maxThreadsNeeded >= 0)
								  {
									  // Trigger worker for new event

									  if (! hasTriggerBeenSkipped)
									  {
										  int threadsClaimed;
										  bool gotThreads = mainThreadPool.tryClaimThreads(w->minThreadsNeeded, w->maxThreadsNeeded, w->eventId, threadsClaimed);
										  if (gotThreads)
										  {
											  ThreadPool &triggerThreadPool = triggerThreadPools[w->eventId][w->compositionHash];
											  triggerThreadPool.setTotalThreads(threadsClaimed);
											  triggerThreadPool.totalWorkers = w->chainCount;
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

									  triggerThreadPools[w->eventId][w->compositionHash].totalWorkers = w->chainCount;
									  workersDequeuedNodes.push_back(n);
								  }
							  }
							  else
							  {
								  // Chain worker

								  ThreadPool &triggerThreadPool = triggerThreadPools[w->eventId][w->compositionHash];

								  bool haveAllUpstreamChainsCompleted = true;
								  for (int i = 0; i < w->upstreamChainIndicesCount; ++i)
								  {
									  unsigned long index = w->upstreamChainIndices[i];

									  set<unsigned long>::iterator iter = triggerThreadPool.workersCompleted.find( index );
									  if (iter == triggerThreadPool.workersCompleted.end())
									  {
										  haveAllUpstreamChainsCompleted = false;
										  break;
									  }
								  }

								  if (haveAllUpstreamChainsCompleted)
								  {
									  int threadsClaimed;
									  bool gotThreads = triggerThreadPool.tryClaimThreads(w->minThreadsNeeded, w->maxThreadsNeeded, w->chainIndex, threadsClaimed);
									  if (gotThreads)
									  {
										  workersDequeuedNodes.push_back(n);

										  free(w->upstreamChainIndices);
										  w->upstreamChainIndices = NULL;
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
 * Schedules a trigger worker function to be called when enough threads are available from the thread pool.
 *
 * For the published input trigger of a subcomposition node, pass -1 for @a minThreadsNeeded and @a maxThreadsNeeded.
 *
 * After this function is called, either `vuoReturnThreadsForTriggerWorker()` should be called for the trigger or
 * `vuoReturnThreadsForChainWorker()` should be called for each chain downstream of the trigger.
 */
void VuoThreadManager::scheduleTriggerWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
											 int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId,
											 const char *compositionIdentifier, int chainCount)
{
	// Assumes mainThreadPool.totalThreads is constant, thus safe to access outside of threadPoolSync.
	int adjustedMinThreadsNeeded = minThreadsNeeded;
	if (adjustedMinThreadsNeeded > mainThreadPool.totalThreads) {
		adjustedMinThreadsNeeded = mainThreadPool.totalThreads;
		VUserLog("Warning: Couldn't allocate as many threads to a trigger worker as it requires.");
	}

	unsigned long compositionHash = VuoRuntimeUtilities::hash(compositionIdentifier);
	Worker *worker = new Worker(queue, context, function, adjustedMinThreadsNeeded, maxThreadsNeeded, eventId, compositionHash, chainCount);

	dispatch_sync(workersWaitingSync, ^{
					   workersWaitingForThreads.enqueue(worker);
				   });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Schedules a chain worker function to be called when enough threads are available from the thread pool.
 *
 * After this function is called, `vuoReturnThreadsForChainWorker()` should be called to return the threads to the thread pool.
 */
void VuoThreadManager::scheduleChainWorker(dispatch_queue_t queue, void *context, void (*function)(void *),
										   int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, const char *compositionIdentifier,
										   unsigned long chainIndex, unsigned long *upstreamChainIndices, int upstreamChainIndicesCount)
{
	unsigned long compositionHash = VuoRuntimeUtilities::hash(compositionIdentifier);
	Worker *worker = new Worker(queue, context, function, minThreadsNeeded, maxThreadsNeeded, eventId, compositionHash, chainIndex, upstreamChainIndices, upstreamChainIndicesCount);

	dispatch_sync(workersWaitingSync, ^{
					  workersWaitingForThreads.enqueue(worker);
				  });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Allocates threads for use by a chain executed synchronously by the caller instead of scheduled with `vuoScheduleChainWorker()`.
 *
 * After this function is called, `vuoReturnThreadsForChainWorker()` should be called to return the threads to the thread pool.
 */
void VuoThreadManager::grantThreadsToChain(int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, const char *compositionIdentifier,
										   unsigned long chainIndex)
{
	unsigned long compositionHash = VuoRuntimeUtilities::hash(compositionIdentifier);

	dispatch_sync(threadPoolSync, ^{
					  ThreadPool &triggerThreadPool = triggerThreadPools[eventId][compositionHash];
					  int threadsClaimed;
					  bool gotThreads = triggerThreadPool.tryClaimThreads(minThreadsNeeded, maxThreadsNeeded, chainIndex, threadsClaimed);
					  if (! gotThreads) {
						  throw std::logic_error("Not enough threads available in the thread pool to execute the chain.");
					  }
				  });

	dispatch_semaphore_signal(workersUpdated);
}

/**
 * Allows the published input trigger of a subcomposition node to use the threads allocated for the chain containing the node.
 *
 * This function should be called before `vuoScheduleTriggerWorker()` is called for the published input trigger.
 */
void VuoThreadManager::grantThreadsToSubcomposition(unsigned long eventId, const char *compositionIdentifier, unsigned long chainIndex,
													const char *subcompositionIdentifier)
{
	unsigned long compositionHash = VuoRuntimeUtilities::hash(compositionIdentifier);
	unsigned long subcompositionHash = VuoRuntimeUtilities::hash(subcompositionIdentifier);

	dispatch_sync(threadPoolSync, ^{
					  map<unsigned long, ThreadPool> &eventThreadPools = triggerThreadPools[eventId];
					  int threadsClaimedForChain = eventThreadPools[compositionHash].getThreadsClaimed(chainIndex);
					  eventThreadPools[subcompositionHash].setTotalThreads(threadsClaimedForChain);
				  });
}

/**
 * Returns the threads allocated for the trigger worker to the thread pool.
 */
void VuoThreadManager::returnThreadsForTriggerWorker(unsigned long eventId)
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
void VuoThreadManager::returnThreadsForChainWorker(unsigned long eventId, const char *compositionIdentifier, unsigned long chainIndex)
{
	unsigned long compositionHash = VuoRuntimeUtilities::hash(compositionIdentifier);

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

extern "C"
{

/**
 * C wrapper for VuoThreadManager::scheduleTriggerWorker().
 */
void vuoScheduleTriggerWorker(VuoCompositionState *compositionState, dispatch_queue_t queue, void *context, void (*function)(void *),
							  int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, int chainCount)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->threadManager->scheduleTriggerWorker(queue, context, function,
																		minThreadsNeeded, maxThreadsNeeded, eventId,
																		compositionIdentifier, chainCount);
}

/**
 * C wrapper for VuoThreadManager::scheduleChainWorker().
 */
void vuoScheduleChainWorker(VuoCompositionState *compositionState, dispatch_queue_t queue, void *context, void (*function)(void *),
							int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, unsigned long chainIndex,
							unsigned long *upstreamChainIndices, int upstreamChainIndicesCount)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->threadManager->scheduleChainWorker(queue, context, function,
																	  minThreadsNeeded, maxThreadsNeeded, eventId, compositionIdentifier,
																	  chainIndex, upstreamChainIndices, upstreamChainIndicesCount);
}

/**
 * C wrapper for VuoThreadManager::grantThreadsToChain().
 */
void vuoGrantThreadsToChain(VuoCompositionState *compositionState,
							int minThreadsNeeded, int maxThreadsNeeded, unsigned long eventId, unsigned long chainIndex)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->threadManager->grantThreadsToChain(minThreadsNeeded, maxThreadsNeeded, eventId, compositionIdentifier,
																	  chainIndex);
}

/**
 * C wrapper for VuoThreadManager::grantThreadsToSubcomposition().
 */
void vuoGrantThreadsToSubcomposition(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex,
									 char *subcompositionIdentifier)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->threadManager->grantThreadsToSubcomposition(eventId, compositionIdentifier, chainIndex,
																			   subcompositionIdentifier);
}

/**
 * C wrapper for VuoThreadManager::returnThreadsForTriggerWorker().
 */
void vuoReturnThreadsForTriggerWorker(VuoCompositionState *compositionState, unsigned long eventId)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	runtimeState->persistentState->threadManager->returnThreadsForTriggerWorker(eventId);
}

/**
 * C wrapper for VuoThreadManager::returnThreadsForChainWorker().
 */
void vuoReturnThreadsForChainWorker(VuoCompositionState *compositionState, unsigned long eventId, unsigned long chainIndex)
{
	VuoRuntimeState *runtimeState = (VuoRuntimeState *)compositionState->runtimeState;
	const char *compositionIdentifier = compositionState->compositionIdentifier;
	runtimeState->persistentState->threadManager->returnThreadsForChainWorker(eventId, compositionIdentifier, chainIndex);
}

}
