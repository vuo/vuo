/**
 * @file
 * vuo.hid.find.name node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHidDevice.h"

#include "VuoList_VuoHidDevice.h"

VuoModuleMetadata({
	"title" : "Find HIDs by Name",
	"keywords" : [ "filter", "search" ],
	"version" : "2.0.0",
	"node" : {
		"exampleCompositions" : ["MoveDotsWithTwoMice.vuo"]
	}
});

void nodeEvent(
	VuoInputData(VuoList_VuoHidDevice, { "name" : "HIDs" }) hids,
	VuoInputData(VuoText, { "default" : "*" }) name,
	VuoOutputData(VuoList_VuoHidDevice, { "name" : "Found HIDs" }) foundHids)
{
	*foundHids = VuoListCreate_VuoHidDevice();
	unsigned long deviceCount = VuoListGetCount_VuoHidDevice(hids);
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoHidDevice d = VuoListGetValue_VuoHidDevice(hids, i);
		if (VuoText_compare(d.name, (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, name))
			VuoListAppendValue_VuoHidDevice(*foundHids, d);
	}
}
