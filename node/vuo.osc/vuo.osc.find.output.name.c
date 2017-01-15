/**
 * @file
 * vuo.osc.find.output.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscOutputDevice.h"
#include "VuoList_VuoOscOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Find OSC Outputs by Name",
					  "keywords" : [ "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoOscOutputDevice) devices,
		VuoInputData(VuoText) name,
		VuoOutputData(VuoList_VuoOscOutputDevice) foundDevices
)
{
	*foundDevices = VuoListCreate_VuoOscOutputDevice();
	unsigned long deviceCount = VuoListGetCount_VuoOscOutputDevice(devices);
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoOscOutputDevice d = VuoListGetValue_VuoOscOutputDevice(devices, i);
		if (!name || (d.name && strstr(d.name, name)))
			VuoListAppendValue_VuoOscOutputDevice(*foundDevices, d);
	}
}
