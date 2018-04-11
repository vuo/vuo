/**
 * @file
 * vuo.test.temporarilyDisableTermination node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Temporarily Disable Termination",
					 "description" : "Optionally takes a long time to execute and/or disables termination while it executes.",
					 "version" : "1.0.0",
				 });

char * nodeInstanceInit(void)
{
	return NULL;
}

void nodeInstanceEvent
(
		VuoInstanceData(char *) ctx,
		VuoInputData(VuoBoolean, {"default":false}) shouldDelay,
		VuoInputData(VuoBoolean, {"default":false}) shouldDisable
)
{
	if (shouldDisable)
		VuoDisableTermination();

	if (shouldDelay)
	{
		dispatch_semaphore_t temporaryDeadlock = dispatch_semaphore_create(0);
		dispatch_semaphore_wait(temporaryDeadlock, dispatch_time(DISPATCH_TIME_NOW, 7 * NSEC_PER_SEC));
		dispatch_release(temporaryDeadlock);
	}

	if (shouldDisable)
		VuoEnableTermination();
}

void nodeInstanceFini(VuoInstanceData(char *) ctx)
{
}
