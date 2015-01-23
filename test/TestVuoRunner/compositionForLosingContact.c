/**
 * @file
 * compositionForLosingContact implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

	while (! isStopped)
		CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
	return 0;
}

void nodeInstanceFini(void)
{
}
