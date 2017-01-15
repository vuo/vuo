/**
 * @file
 * vuo.serial.configure node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerial.h"

VuoModuleMetadata({
					  "title" : "Configure Serial Device",
					  "keywords" : [
						  "set", "change",
						  "speed",
						  "arduino", "rs232", "rs-232", "usb", "modem"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoSerialIO"
					  ],
					  "node" : {
						  "isInterface" : true,
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoSerialDevice) device,
		VuoInputData(VuoBaudRate, {"default":9600}) baudRate,
		VuoInputData(VuoInteger, {"default":8, "suggestedMin":5, "suggestedMax":8}) dataBits,
		VuoInputData(VuoParity, {"default":"none"}) parity,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":2}) stopBits
)
{
	VuoSerialDevice realizedDevice;
	if (!VuoSerialDevice_realize(device, &realizedDevice))
	{
		VUserLog("Couldn't realize device %s.", VuoSerialDevice_getSummary(device));
		return;
	}

	VuoSerialDevice_retain(realizedDevice);
	VuoSerial s = VuoSerial_getShared(realizedDevice.path);
	VuoRetain(s);

	VuoSerial_configureDevice(s, baudRate, dataBits, parity, stopBits);

	VuoRelease(s);
	VuoSerialDevice_release(realizedDevice);
}
