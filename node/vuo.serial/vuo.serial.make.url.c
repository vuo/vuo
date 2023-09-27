/**
 * @file
 * vuo.serial.make.url node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Specify Serial Device by URL",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoUrl"
					  ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL","default":"/dev/cu.usbserial-A60049zx"}) url,
		VuoOutputData(VuoSerialDevice) device
)
{
	device->matchType = VuoSerialDevice_MatchPath;
	VuoUrl u = VuoUrl_normalize(url, VuoUrlNormalize_default);
	VuoRetain(u);
	device->path = VuoUrl_getPosixPath(u);
	VuoRelease(u);
}
