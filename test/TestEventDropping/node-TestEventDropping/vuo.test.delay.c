/**
 * @file
 * vuo.test.delay node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <dispatch/dispatch.h>

VuoModuleMetadata({
					 "title" : "Delay",
					 "description" : "",
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":1,"suggestedMin":0,"suggestedStep":0.1}) seconds,
		VuoOutputEvent() event
)
{
	dispatch_semaphore_t temporaryDeadlock = dispatch_semaphore_create(0);
	dispatch_semaphore_wait(temporaryDeadlock, dispatch_time(DISPATCH_TIME_NOW, seconds * NSEC_PER_SEC));
	dispatch_release(temporaryDeadlock);
}
