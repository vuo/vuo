/**
 * @file
 * VuoModuleCompilationQueue implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCompilationQueue.hh"

const int VuoModuleCompilationQueue::maxConcurrentCompilations = 8;

/**
 * Creates an empty item. Set its data members before enqueueing it.
 */
VuoModuleCompilationQueue::Item::Item()
{
	sourceFile = nullptr;
	loadingGroup = nullptr;
}

/**
 * Returns true if this item should be compiled before @a other.
 */
bool VuoModuleCompilationQueue::Item::hasHigherPriority(Item *other)
{
	return (priority.first != other->priority.first ?
								  (priority.first < other->priority.first) :
								  (priority.second < other->priority.second));
}

/**
 * Creates an empty queue.
 */
VuoModuleCompilationQueue::VuoModuleCompilationQueue()
{
}

/**
 * Adds the item to the queue.
 */
void VuoModuleCompilationQueue::enqueue(VuoModuleCompilationQueue::Item *item)
{
	std::unique_lock<std::mutex> lock(queueMutex);

	queue[item->sourcePath].push_back({item, false});
}

/**
 * Returns an item that is ready to be compiled, blocking if none are available.
 *
 * This call is assumed to follow a corresponding call to @ref VuoModuleCompilationQueue::enqueue.
 */
VuoModuleCompilationQueue::Item * VuoModuleCompilationQueue::next()
{
	Item *item = nullptr;

	std::unique_lock<std::mutex> lock(queueMutex);
	queueChanged.wait(lock, [this, &item]()
	{
		int concurrentCompilations = 0;

		// Select the highest-priority item that is not already being compiled (if any).
		auto selectedIter = queue.end();
		for (auto i = queue.begin(); i != queue.end(); ++i)
		{
			if (! ripeItemIsCompiling(i) &&
					(selectedIter == queue.end() || ripeItem(i)->hasHigherPriority(ripeItem(selectedIter))))
				selectedIter = i;

			if (ripeItemIsCompiling(i))
				++concurrentCompilations;
		}

		// If an item was selected and we haven't already reached the limit of concurrent compilations, return the item.
		if (selectedIter != queue.end() && concurrentCompilations < maxConcurrentCompilations)
		{
			item = ripeItem(selectedIter);
			ripeItemIsCompiling(selectedIter) = true;
			return true;
		}

		return false;
	});

	return item;
}

/**
 * Indicates that an item previously returned by @ref VuoModuleCompilationQueue::next is finished compiling.
 */
void VuoModuleCompilationQueue::completed(VuoModuleCompilationQueue::Item *item)
{
	std::unique_lock<std::mutex> lock(queueMutex);

	auto i = queue.find(item->sourcePath);
	pluckRipeItem(i);

	queueChanged.notify_one();
}

/**
 * For the selected source path in the queue (@a iter), retrieves the source version that is at the front
 * of the line (either ready to be compiled next or already being compiled).
 */
VuoModuleCompilationQueue::Item * VuoModuleCompilationQueue::ripeItem(map<string, list< pair<Item *, bool> > >::iterator iter)
{
	return iter->second.front().first;
}

/**
 * For the selected source path in the queue (@a iter), checks if the source version at the front of the line
 * is being compiled.
 */
bool& VuoModuleCompilationQueue::ripeItemIsCompiling(map<string, list< pair<Item *, bool> > >::iterator iter)
{
	return iter->second.front().second;
}

/**
 * For the selected source path in the queue (@a iter), dequeues the source version at the front of the line.
 */
void VuoModuleCompilationQueue::pluckRipeItem(map<string, list< pair<Item *, bool> > >::iterator iter)
{
	delete iter->second.front().first;
	iter->second.pop_front();

	if (iter->second.empty())
		queue.erase(iter);
}
