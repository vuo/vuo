/**
 * @file
 * VuoHidDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoHidDevice.h"
#include "VuoHid.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "HID Device",
					  "description" : "Information about a USB HID device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoHidDevices",
						  "VuoHidIo",
						  "VuoInteger",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Returns a device matching type constant for the specified identifier string.
 */
static VuoHidDevice_MatchType VuoHidDevice_getMatchTypeForString(const char *string)
{
	if (strcmp(string, "location") == 0)
		return VuoHidDevice_MatchLocation;

	return VuoHidDevice_MatchName;
}

/**
 * Returns an identifier string for the specified device matching type.
 */
static const char *VuoHidDevice_getStringForMatchType(VuoHidDevice_MatchType type)
{
	if (type == VuoHidDevice_MatchLocation)
		return "location";

	return "name";
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "matchType": "location",
 *     "name": "Multi-axis Controller: 3Dconnexion SpaceNavigator",
 *     "location": 337641472
 *   }
 * }
 */
VuoHidDevice VuoHidDevice_makeFromJson(json_object *js)
{
	VuoHidDevice value = {VuoHidDevice_MatchName, NULL, 0, NULL, 0, 0, 0, 0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "matchType", &o))
		value.matchType = VuoHidDevice_getMatchTypeForString(json_object_get_string(o));
	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	if (json_object_object_get_ex(js, "location", &o))
		value.location = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "controls", &o))
		value.controls = VuoList_VuoHidControl_makeFromJson(o);
	if (json_object_object_get_ex(js, "vendorID", &o))
		value.vendorID = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "productID", &o))
		value.productID = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "usagePage", &o))
		value.usagePage = VuoInteger_makeFromJson(o);
	if (json_object_object_get_ex(js, "usage", &o))
		value.usage = VuoInteger_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoHidDevice_getJson(const VuoHidDevice value)
{
	json_object *js = json_object_new_object();

	json_object *matchTypeObject = json_object_new_string(VuoHidDevice_getStringForMatchType(value.matchType));
	json_object_object_add(js, "matchType", matchTypeObject);

	if (value.name)
	{
		json_object *o = VuoText_getJson(value.name);
		json_object_object_add(js, "name", o);
	}

	if (value.location)
	{
		json_object *o = VuoInteger_getJson(value.location);
		json_object_object_add(js, "location", o);
	}

	if (value.controls)
	{
		json_object *o = VuoList_VuoHidControl_getJson(value.controls);
		json_object_object_add(js, "controls", o);
	}

	if (value.vendorID)
	{
		json_object *o = VuoInteger_getJson(value.vendorID);
		json_object_object_add(js, "vendorID", o);
	}

	if (value.productID)
	{
		json_object *o = VuoInteger_getJson(value.productID);
		json_object_object_add(js, "productID", o);
	}

	if (value.usagePage)
	{
		json_object *o = VuoInteger_getJson(value.usagePage);
		json_object_object_add(js, "usagePage", o);
	}

	if (value.usage)
	{
		json_object *o = VuoInteger_getJson(value.usage);
		json_object_object_add(js, "usage", o);
	}

	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoHidDevice_areEqual(const VuoHidDevice valueA, const VuoHidDevice valueB)
{
	if (valueA.matchType != valueB.matchType)
		return false;

	if (!VuoText_areEqual(valueA.name, valueB.name))
		return false;

	if (!VuoInteger_areEqual(valueA.location, valueB.location))
		return false;

//	if (!VuoList_VuoHidControl_areEqual(valueA.controls, valueB.controls))
//		return false;

	if (!VuoInteger_areEqual(valueA.vendorID, valueB.vendorID))
		return false;

	if (!VuoInteger_areEqual(valueA.productID, valueB.productID))
		return false;

	if (!VuoInteger_areEqual(valueA.usagePage, valueB.usagePage))
		return false;

	if (!VuoInteger_areEqual(valueA.usage, valueB.usage))
		return false;

	return true;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoHidDevice_isLessThan(const VuoHidDevice a, const VuoHidDevice b)
{
	return a.location < b.location;
}

/**
 * If `device` specifies name-matching:
 *
 *    - If a matching HID device is connected, sets `realizedDevice` to a specific, existing device matched by its location, and returns true.
 *    - If no matching HID device is connected, returns false, leaving `realizedDevice` unset.
 *
 * If `device` specifies location-matching, sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the location actually exists.)
 */
bool VuoHidDevice_realize(VuoHidDevice device, VuoHidDevice *realizedDevice)
{
//	VLog("%s (%08llx) realized=%d",device.name,device.location,device.matchType==VuoHidDevice_MatchLocation);

	// Already have a location; nothing to do.
	if (device.matchType == VuoHidDevice_MatchLocation)
	{
		realizedDevice->matchType = device.matchType;
		realizedDevice->name = VuoText_make(device.name);
		realizedDevice->location = device.location;
		realizedDevice->controls = VuoListCopy_VuoHidControl(device.controls);
		realizedDevice->vendorID = device.vendorID;
		realizedDevice->productID = device.productID;
		realizedDevice->usagePage = device.usagePage;
		realizedDevice->usage = device.usage;
		return true;
	}

	// Otherwise, try to find a matching name.

	if (!device.name)
		return false;

	VuoList_VuoHidDevice devices = VuoHid_getDeviceList();
	VuoRetain(devices);

	unsigned long deviceCount = VuoListGetCount_VuoHidDevice(devices);
	if (deviceCount == 0)
	{
		VUserLog("Warning: No HID devices found.");
		VuoRelease(devices);
		return false;
	}

	VuoBoolean found = false;
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoHidDevice d = VuoListGetValue_VuoHidDevice(devices, i);
		if (strstr(d.name, device.name))
		{
			realizedDevice->matchType = VuoHidDevice_MatchLocation;
			realizedDevice->name = VuoText_make(d.name);
			realizedDevice->location = d.location;
			realizedDevice->controls = VuoListCopy_VuoHidControl(d.controls);
			realizedDevice->vendorID = d.vendorID;
			realizedDevice->productID = d.productID;
			realizedDevice->usagePage = d.usagePage;
			realizedDevice->usage = d.usage;
			found = true;
			break;
		}
	}

	if (found)
	{
		VuoRelease(devices);
		return true;
	}

//	VLog("Warning: Didn't find a HID device matching '%s'.", device.name);
	VuoRelease(devices);
	return false;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoHidDevice_getSummary(const VuoHidDevice value)
{
	VuoHidDevice realizedDevice;
	if (VuoHidDevice_realize(value, &realizedDevice))
	{
		VuoHidDevice_retain(realizedDevice);
		char *outputText = strdup(realizedDevice.name);
		VuoHidDevice_release(realizedDevice);
		return outputText;
	}

	if (value.name)
		return strdup(value.name);

	return strdup("(unknown device)");
}
