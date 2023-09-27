/**
 * @file
 * VuoTimeUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Functions for measuring times at microsecond precision.
 */
class VuoTimeUtilities
{
public:
	static double getCurrentTimeInSeconds(void);
	static struct timeval getCurrentTime(void);
	static struct timeval getElapsedTime(const struct timeval &start, const struct timeval &end);
	static string getCurrentDateTimeForFileName(void);
};
