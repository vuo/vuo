/**
 * @file
 * vuo.hid.make.name node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoHidDevice.h"

VuoModuleMetadata({
					  "title" : "Specify HID by Name",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "MoveIcosahedronWithSpacenavigator.vuo", "ListKeypresses.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText) name,
		VuoOutputData(VuoHidDevice) device
)
{
	device->matchType = VuoHidDevice_MatchName;
	device->name = name;
	device->controls = NULL;
}
