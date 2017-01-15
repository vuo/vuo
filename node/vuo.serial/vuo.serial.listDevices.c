/**
 * @file
 * vuo.serial.listDevices node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerial.h"

VuoModuleMetadata({
					  "title" : "List Serial Devices",
					  "keywords" : [ "arduino", "rs232", "rs-232", "usb", "modem" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoSerialDevices"
					  ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ "ShowConnectedSerialDevices.vuo" ]
					  }
				  });

void *nodeInstanceInit()
{
	VuoSerial_use();
	return NULL;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoSerialDevice)
)
{
	VuoSerial_addDevicesChangedTriggers(devices);
}

void nodeInstanceEvent
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoSerialDevice)
)
{
	/// @todo https://b33p.net/kosada/node/10080
//	devices(VuoSerial_getDeviceList());
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(void *) nodeInstanceData,
		VuoOutputTrigger(devices, VuoList_VuoSerialDevice)
)
{
	VuoSerial_removeDevicesChangedTriggers(devices);
}

void nodeInstanceFini
(
		VuoInstanceData(void *) nodeInstanceData
)
{
	VuoSerial_disuse();
}
