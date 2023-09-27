/**
 * @file
 * VuoThreadUtilities implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoThreadUtilities.hh"

/**
 * Creates a semaphore that is ready to be acquired.
 */
VuoThreadUtilities::Semaphore::Semaphore(void) :
	count(1)
{
}

/**
 * Waits until the semaphore is available, then claims it.
 */
void VuoThreadUtilities::Semaphore::acquire(void)
{
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [this](){ return count > 0; });
	--count;
}

/**
 * Un-claims the semaphore, making it available to be acquired again.
 */
void VuoThreadUtilities::Semaphore::release(void)
{
	std::unique_lock<std::mutex> lock(mtx);
	count++;
	lock.unlock();
	cv.notify_one();
}
