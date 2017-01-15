/**
 * @file
 * VuoTimeUtilities implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <sys/time.h>
#include "VuoTimeUtilities.hh"

/**
 * Returns a floating-point representation of the number of seconds since 1970 (the Unix Epoch).
 */
double VuoTimeUtilities::getCurrentTimeInSeconds(void)
{
	struct timeval t = getCurrentTime();
	return t.tv_sec + t.tv_usec / 1000000.;
}

/**
 * Returns a structure containing the number of seconds and microseconds since 1970 (the Unix Epoch).
 */
struct timeval VuoTimeUtilities::getCurrentTime(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now;
}

/**
 * Returns a structure containing the number of seconds and microseconds elapsed between @c start and @c end.
 */
struct timeval VuoTimeUtilities::getElapsedTime(const struct timeval &start, const struct timeval &end)
{
	long int diff = (end.tv_usec + 1000000 * end.tv_sec) - (start.tv_usec + 1000000 * start.tv_sec);
	struct timeval result;
	result.tv_sec = diff / 1000000;
	result.tv_usec = diff % 1000000;
	return result;
}

/**
 * Prints @c time to @c stdout.
 */
void VuoTimeUtilities::printTime(const struct timeval &time)
{
	printf("%ld.%06d\n", time.tv_sec, time.tv_usec);
}
