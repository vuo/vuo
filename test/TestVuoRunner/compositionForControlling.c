/**
 * @file
 * compositionForControlling implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "composition.h"
#include "VuoEventLoop.h"

void *VuoApp_mainThread = NULL;  ///< A reference to the main thread

FILE *file;

int main(int argc, char **argv)
{
	VuoApp_mainThread = (void *)pthread_self();

	vuoInit(argc, argv);

	char *outputPath = getenv("TESTVUORUNNER_OUTPUTPATH");
	file = fopen(outputPath, "a");
	fprintf(file, "started\n");

	while (! vuoIsCurrentCompositionStopped())
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
	return 0;
}

void vuoInstanceFini(void)
{
	fprintf(file, "stopped\n");
	fclose(file);
}
