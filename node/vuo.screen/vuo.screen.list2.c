/**
 * @file
 * vuo.screen.list2 node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoScreenCommon.h"

VuoModuleMetadata({
	"title" : "List Screens",
	"keywords" : [ "display", "monitor", "device" ],
	"version" : "2.0.0",
	"dependencies" : [
		"VuoScreenCommon"
	],
	"node" : {
		"exampleCompositions" : [ ]
	}
});

void *nodeInstanceInit()
{
	VuoScreen_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(screens, VuoList_VuoScreen)
)
{
	VuoScreen_addDevicesChangedTriggers(screens);
}

void nodeInstanceEvent
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(screens, VuoList_VuoScreen)
)
{
}

void nodeInstanceTriggerStop
(
	VuoInstanceData(void *) nodeInstanceData,
	VuoOutputTrigger(screens, VuoList_VuoScreen)
)
{
	VuoScreen_removeDevicesChangedTriggers(screens);
}

void nodeInstanceFini
(
	VuoInstanceData(void *) nodeInstanceData
)
{
	VuoScreen_disuse();
}
