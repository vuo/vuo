/**
 * @file
 * VuoTimeUtilities implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTimeUtilities.hh"

#include <chrono>
#include <ctime>
#include <sys/time.h>

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
 * Returns the current timestamp, at millisecond resolution, formatted for use as part of a file name.
 */
string VuoTimeUtilities::getCurrentDateTimeForFileName(void)
{
	// https://stackoverflow.com/questions/15845505/how-to-get-higher-precision-fractions-of-a-second-in-a-printout-of-current-tim

	auto now = std::chrono::system_clock::now();
	auto secondsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));
	std::time_t now_t(std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(secondsSinceEpoch)));

	const int length = 16;
	char nowToSeconds[length] = {0};

	strftime(nowToSeconds, length, "%Y%m%d_%H%M%S", std::localtime(&now_t));
	auto nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - secondsSinceEpoch).count();

	char nowFull[length+3] = {0};
	snprintf(nowFull, length+4, "%s%03lld", nowToSeconds, nowMilliseconds);

	return nowFull;
}
