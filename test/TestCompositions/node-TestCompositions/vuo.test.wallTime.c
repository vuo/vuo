/**
 * @file
 * vuo.test.wallTime node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <sys/time.h>

VuoModuleMetadata({
					 "title" : "Get Wall Time",
					 "description" : "Gets the current wall-clock time, in seconds since the epoch.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
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
