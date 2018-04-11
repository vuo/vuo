/**
 * @file
 * vuo.test.finiCallbackCreatesFile node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Fini Callback Creates File",
					 "description" : "Registers a fini callback that creates a file.",
					 "version" : "1.0.0",
				 });

static void finiCallback(void)
{
	int fd = open("/tmp/vuo.test.finiCallbackCreatesFile", O_CREAT);
	close(fd);
}

char * nodeInstanceInit()
{
	VuoAddCompositionFiniCallback(finiCallback);
	return NULL;
}

void nodeInstanceEvent(VuoInstanceData(char *) ctx)
{
}

void nodeInstanceFini(VuoInstanceData(char *) ctx)
{
}
