/**
 * @file
 * vuo.test.writeTimeToFile node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <stdio.h>
#include <sys/time.h>

VuoModuleMetadata({
					 "title" : "Write Time to File",
					 "description" : "Appends the current wall-clock time, in seconds since the epoch, to the file.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : true
					 }
				 });

int timesExecuted = 0;

void nodeEvent
(
)
{
	if (++timesExecuted > 1000)
		exit(0);  // Avoid writing to the file indefinitely. Workaround until https://b33p.net/kosada/node/3678

	struct timeval t;
	gettimeofday(&t, NULL);
	double seconds = t.tv_sec + t.tv_usec / 1000000.;

	char *path = getenv("TESTCONTROLANDTELEMETRY_WRITETIMESTOFILE");
	FILE *file = fopen(path, "a");
	fprintf(file, "%f\n", seconds);
	fclose(file);
}
