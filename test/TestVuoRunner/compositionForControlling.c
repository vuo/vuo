/**
 * @file
 * compositionForControlling implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "composition.h"


FILE *file;

int main(int argc, char **argv)
{
	vuoInit(argc, argv);

	char *outputPath = getenv("TESTVUORUNNER_OUTPUTPATH");
	file = fopen(outputPath, "a");
	fprintf(file, "started\n");

	while (! isStopped)
	{
		_dispatch_main_queue_callback_4CF(0);
		usleep(10000);
	}
	return 0;
}

void nodeInstanceFini(void)
{
	fprintf(file, "stopped\n");
	fclose(file);
}
