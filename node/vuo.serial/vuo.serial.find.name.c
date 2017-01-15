/**
 * @file
 * vuo.serial.find.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerialDevice.h"
#include "VuoList_VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Find Serial Devices by Name",
					  "keywords" : [ "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoSerialDevice) serialDevices,
		VuoInputData(VuoText) name,
		VuoOutputData(VuoList_VuoSerialDevice) foundSerialDevices
)
{
	*foundSerialDevices = VuoListCreate_VuoSerialDevice();
	unsigned long deviceCount = VuoListGetCount_VuoSerialDevice(serialDevices);
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoSerialDevice d = VuoListGetValue_VuoSerialDevice(serialDevices, i);
		if (!name || (d.name && strstr(d.name, name)))
			VuoListAppendValue_VuoSerialDevice(*foundSerialDevices, d);
	}
}
