/**
 * @file
 * vuo.serial.make.url node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Make Serial Device from URL",
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
	VuoUrl u = VuoUrl_normalize(url, false);
	VuoRetain(u);
	device->path = VuoUrl_getPosixPath(u);
	VuoRelease(u);
}
