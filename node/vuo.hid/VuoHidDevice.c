/**
 * @file
 * VuoHidDevice implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHidDevice.h"
#include "VuoHid.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "HID Device",
					  "description" : "Information about a USB or Bluetooth HID device.",
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
static VuoHidDevice_MatchType VuoHidDevice_MatchType_makeFromJson(json_object *js)
{
	if (strcmp(json_object_get_string(js), "location") == 0)
		return VuoHidDevice_MatchLocation;
	else if (strcmp(json_object_get_string(js), "vendor-product") == 0)
		return VuoHidDevice_MatchVendorAndProduct;
	else if (strcmp(json_object_get_string(js), "usage") == 0)
		return VuoHidDevice_MatchUsage;

	return VuoHidDevice_MatchName;
}

/**
 * Returns an identifier string for the specified device matching type.
 */
static const char *VuoHidDevice_getStringForMatchType(VuoHidDevice_MatchType type)
{
	if (type == VuoHidDevice_MatchLocation)
		return "location";
	else if (type == VuoHidDevice_MatchVendorAndProduct)
		return "vendor-product";
	else if (type == VuoHidDevice_MatchUsage)
		return "usage";

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
	return (VuoHidDevice){
		VuoJson_getObjectValue(VuoHidDevice_MatchType, js, "matchType", VuoHidDevice_MatchName),
		VuoJson_getObjectValue(VuoText, js, "name", NULL),
		VuoJson_getObjectValue(VuoInteger, js, "location", 0),
		VuoJson_getObjectValue(VuoList_VuoHidControl, js, "controls", NULL),
		VuoJson_getObjectValue(VuoInteger, js, "vendorID", 0),
		VuoJson_getObjectValue(VuoInteger, js, "productID", 0),
		VuoJson_getObjectValue(VuoInteger, js, "usagePage", 0),
		VuoJson_getObjectValue(VuoInteger, js, "usage", 0)
	};
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
 * If `device` specifies a `matchType` of name, vendorAndProduct, or usage:
 *
 *    - If a matching HID device is connected, sets `realizedDevice` to a specific, existing device matched by its location, and returns true.
 *    - If no matching HID device is connected, returns false, leaving `realizedDevice` unset.
 *
 * If `device` specifies location-matching, sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the location actually exists.)
 */
bool VuoHidDevice_realize(VuoHidDevice device, VuoHidDevice *realizedDevice)
{
	VUserLog("Matching by %s:  name=%s  location=%08llx  product=%04llx:%04llx  usage=%04llx:%04llx", VuoHidDevice_getStringForMatchType(device.matchType), device.name, device.location, device.vendorID, device.productID, device.usagePage, device.usage);

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

	// Otherwise, try to find a matching name, productAndVendor, or usage.

	if ((device.matchType == VuoHidDevice_MatchName && !device.name)
	 || (device.matchType == VuoHidDevice_MatchVendorAndProduct && (!device.vendorID || !device.productID))
	 || (device.matchType == VuoHidDevice_MatchUsage && (!device.usagePage || !device.usage)))
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
		if ((device.matchType == VuoHidDevice_MatchName && strstr(d.name, device.name))
		 || (device.matchType == VuoHidDevice_MatchVendorAndProduct && d.vendorID == device.vendorID && d.productID == device.productID)
		 || (device.matchType == VuoHidDevice_MatchUsage && d.usagePage == device.usagePage && d.usage == device.usage))
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
		VUserLog("Matched \"%s\".", realizedDevice->name);
		VuoRelease(devices);
		return true;
	}

	VUserLog("Warning: Didn't find a device matching those criteria.");
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

	return strdup("Unknown device");
}
