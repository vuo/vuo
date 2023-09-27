/**
 * @file
 * vuo.test.wallTime node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <sys/time.h>

VuoModuleMetadata({
					 "title" : "Get Wall Time",
					 "description" : "Gets the current wall-clock time, in seconds since the epoch.",
					 "version" : "1.0.0",
				 });

void nodeEvent
(
	VuoOutputData(VuoReal) seconds
)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	*seconds = t.tv_sec + t.tv_usec / 1000000.;
}
