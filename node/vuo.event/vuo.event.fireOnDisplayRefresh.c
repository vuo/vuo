/**
 * @file
 * vuo.event.fireOnDisplayRefresh node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoDisplayRefresh.h"

VuoModuleMetadata({
					 "title" : "Fire on Display Refresh",
					 "keywords" : [ "frame", "draw", "scenegraph", "graphics", "display", "view", "periodically" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoDisplayRefresh"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });


VuoDisplayRefresh nodeInstanceInit(void)
{
	return VuoDisplayRefresh_make(NULL);
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(VuoDisplayRefresh) context,
		VuoOutputTrigger(requestedFrame, VuoReal, {"name":"Refreshed at Time", "eventThrottling":"drop"})
)
{
	VuoDisplayRefresh_enableTriggers(*context, requestedFrame, NULL);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoDisplayRefresh) context
)
{
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(VuoDisplayRefresh) context
)
{
	VuoDisplayRefresh_disableTriggers(*context);
}

void nodeInstanceFini
(
		VuoInstanceData(VuoDisplayRefresh) context
)
{
}
