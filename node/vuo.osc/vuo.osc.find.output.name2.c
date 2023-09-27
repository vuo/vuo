/**
 * @file
 * vuo.osc.find.output.name node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscOutputDevice.h"

#include "VuoList_VuoOscOutputDevice.h"

VuoModuleMetadata({
	"title" : "Find OSC Outputs by Name",
	"keywords" : [ "filter", "search" ],
	"version" : "2.0.0",
	"node" : {
		"exampleCompositions" : []
	}
});

void nodeEvent(
	VuoInputData(VuoList_VuoOscOutputDevice) devices,
	VuoInputData(VuoText, { "default" : "*" }) name,
	VuoOutputData(VuoList_VuoOscOutputDevice) foundDevices)
{
	*foundDevices = VuoListCreate_VuoOscOutputDevice();
	unsigned long deviceCount = VuoListGetCount_VuoOscOutputDevice(devices);
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoOscOutputDevice d = VuoListGetValue_VuoOscOutputDevice(devices, i);
		if (VuoText_compare(d.name, (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, name))
			VuoListAppendValue_VuoOscOutputDevice(*foundDevices, d);
	}
}
