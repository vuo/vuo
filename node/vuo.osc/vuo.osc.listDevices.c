/**
 * @file
 * vuo.osc.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOsc.h"

VuoModuleMetadata({
					  "title" : "List OSC Devices",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoOscDevices"
					  ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  }
				  });

void *nodeInstanceInit()
{
	VuoOsc_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(inputDevices, VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
)
{
	VuoOsc_addDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(inputDevices, VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
)
{
	/// @todo https://b33p.net/kosada/node/10080
//	inputDevices(VuoOsc_getInputDeviceList());
//	outputDevices(VuoOsc_getOutputDeviceList());
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(inputDevices, VuoList_VuoOscInputDevice),
		VuoOutputTrigger(outputDevices, VuoList_VuoOscOutputDevice)
)
{
	VuoOsc_removeDevicesChangedTriggers(inputDevices, outputDevices);
}

void nodeInstanceFini
(
		VuoInstanceData(void *) nodeInstanceData
)
{
	VuoOsc_disuse();
}
