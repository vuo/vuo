/**
 * @file
 * vuo.serial.make.name node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSerialDevice.h"

VuoModuleMetadata({
					  "title" : "Specify Serial Device by Name",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoOutputData(VuoSerialDevice) device
)
{
	device->matchType = VuoSerialDevice_MatchName;
	device->name = name;
}
