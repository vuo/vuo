/**
 * @file
 * vuo.serial.get node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Get Serial Device Values",
					  "keywords" : [ ],
					  "version" : "1.1.0",
					  "dependencies" : [
						  "VuoUrl"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ShowConnectedSerialDevices.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoSerialDevice) device,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText, {"name":"URL"}) url
)
{
	VuoSerialDevice realizedDevice;
	if (VuoSerialDevice_realize(device, &realizedDevice))
	{
		VuoSerialDevice_retain(realizedDevice);
		*name = VuoText_make(realizedDevice.name);
		*url = VuoUrl_normalize(realizedDevice.path, VuoUrlNormalize_default);
		VuoSerialDevice_release(realizedDevice);
	}
	else
	{
		*name = device.name;
		*url = VuoUrl_normalize(device.path, VuoUrlNormalize_default);
	}
}
