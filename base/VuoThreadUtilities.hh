/**
 * @file
 * VuoThreadUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <condition_variable>
#include <future>
#include <mutex>

/**
 * Functions and classes for multithreading.
 */
class VuoThreadUtilities
{
public:
	/**
	 * Calls the given lambda asynchronously. This function returns immediately after calling the lambda
	 * and does not wait for the lambda to finish executing.
	 *
	 * https://stackoverflow.com/questions/23454793/whats-the-c-11-way-to-fire-off-an-asynchronous-task-and-forget-about-it
	 */
	template<class F> static void async(F&& fun)
	{
		auto futptr = std::make_shared<std::future<void>>();
		*futptr = std::async(std::launch::async, [futptr, fun]() {
			fun();
		});
	}

	/**
	 * Provides a semaphore data structure, similar to the C++20 binary_semaphore.
	 *
	 * Although RAII methods of locking (e.g. std::lock_guard) are preferred, the semaphore makes it possible to
	 * acquire a lock on one thread and release it on another (which is not permitted with a std::mutex).
	 *
	 * https://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads
	 */
	class Semaphore
	{
	public:
		Semaphore(void);
		void acquire(void);
		void release(void);

	private:
		std::mutex mtx;
		std::condition_variable cv;
		int count;
	};
};
