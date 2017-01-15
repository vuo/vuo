/**
 * @file
 * vuo.hid.make.name node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoHidDevice.h"

VuoModuleMetadata({
					  "title" : "Make HID from Name",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "MoveIcosahedronWithSpacenavigator.vuo" ]
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
