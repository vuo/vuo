/**
 * @file
 * vuo.hid.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoHid.h"

VuoModuleMetadata({
					  "title" : "List HIDs",
					  "keywords" : [ "USB" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoHidDevices"
					  ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ "MoveDotsWithTwoMice.vuo" ]
					  }
				  });

void *nodeInstanceInit()
{
	VuoHid_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoHidDevice)
)
{
	VuoHid_addDevicesChangedTriggers(devices);
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoHidDevice)
)
{
	/// @todo https://b33p.net/kosada/node/10080
//	devices(VuoHid_getDeviceList());
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoHidDevice)
)
{
	VuoHid_removeDevicesChangedTriggers(devices);
}

void nodeInstanceFini
(
		VuoInstanceData(void *) nodeInstanceData
)
{
	VuoHid_disuse();
}
