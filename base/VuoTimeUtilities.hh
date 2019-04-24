/**
 * @file
 * VuoTimeUtilities interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
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
	static void printTime(const struct timeval &time);
};
