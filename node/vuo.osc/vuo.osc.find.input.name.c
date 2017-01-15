/**
 * @file
 * vuo.osc.find.input.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscInputDevice.h"
#include "VuoList_VuoOscInputDevice.h"

VuoModuleMetadata({
					  "title" : "Find OSC Inputs by Name",
					  "keywords" : [ "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoList_VuoOscInputDevice) devices,
		VuoInputData(VuoText) name,
		VuoOutputData(VuoList_VuoOscInputDevice) foundDevices
)
{
	*foundDevices = VuoListCreate_VuoOscInputDevice();
	unsigned long deviceCount = VuoListGetCount_VuoOscInputDevice(devices);
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoOscInputDevice d = VuoListGetValue_VuoOscInputDevice(devices, i);
		if (!name || (d.name && strstr(d.name, name)))
			VuoListAppendValue_VuoOscInputDevice(*foundDevices, d);
	}
}
