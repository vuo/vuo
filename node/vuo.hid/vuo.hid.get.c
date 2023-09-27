/**
 * @file
 * vuo.hid.get node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHidDevice.h"

VuoModuleMetadata({
					  "title" : "Get HID Values",
					  "keywords" : [ ],
					  "version" : "1.1.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoHidDevice) device,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoList_VuoHidControl) controls
)
{
	VuoHidDevice realizedDevice;
	if (VuoHidDevice_realize(device, &realizedDevice))
	{
		*name = realizedDevice.name;
		*controls = realizedDevice.controls;
	}
	else
	{
		*name = device.name;
		*controls = device.controls;
	}
}
