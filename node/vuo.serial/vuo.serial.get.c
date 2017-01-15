/**
 * @file
 * vuo.serial.get node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Get Serial Device Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
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
		*name = realizedDevice.name;
		*url = VuoUrl_normalize(realizedDevice.path, false);
		VuoRetain(*name);
		VuoRetain(*url);
		VuoSerialDevice_release(realizedDevice);
	}
	else
	{
		*name = VuoText_make("");
		*url = VuoText_make("");
	}
}
