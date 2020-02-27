/**
 * @file
 * vuo.osc.get.output node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoOscOutputDevice.h"
#include "VuoOsc.h"

VuoModuleMetadata({
					  "title" : "Get OSC Output Values",
					  "keywords" : [ ],
					  "version" : "1.1.0",
					  "dependencies" : [
						  "VuoOscDevices",
					  ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void *nodeInstanceInit()
{
	VuoOsc_use();
	return NULL;
}

void nodeInstanceEvent(
		VuoInstanceData(void *) nodeInstanceData,
		VuoInputData(VuoOscOutputDevice) device,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText, {"name":"IP Address"}) ipAddress,
		VuoOutputData(VuoInteger) port)
{
	VuoOscOutputDevice realizedDevice;
	if (VuoOscOutputDevice_realize(device, &realizedDevice))
	{
		*name = realizedDevice.name;
		*ipAddress = realizedDevice.ipAddress;
		*port = realizedDevice.port;
	}
	else
	{
		*name = device.name;
		*ipAddress = device.ipAddress;
		*port = device.port;
	}
}

void nodeInstanceFini(
	VuoInstanceData(void *) nodeInstanceData)
{
	VuoOsc_disuse();
}
