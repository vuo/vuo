/**
 * @file
 * VuoSerialDevice implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSerialDevice.h"
#include "VuoSerial.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Serial Device",
					  "description" : "Information about a serial I/O device.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoSerialDevices",
						  "VuoText"
					  ]
				  });
#endif
/// @}

/**
 * Returns a device matching type constant for the specified identifier string.
 */
static VuoSerialDevice_MatchType VuoSerialDevice_getMatchTypeForString(const char *string)
{
	if (strcmp(string, "path") == 0)
		return VuoSerialDevice_MatchPath;

	return VuoSerialDevice_MatchName;
}

/**
 * Returns an identifier string for the specified device matching type.
 */
static const char *VuoSerialDevice_getStringForMatchType(VuoSerialDevice_MatchType type)
{
	if (type == VuoSerialDevice_MatchPath)
		return "path";

	return "name";
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "matchType": "path",
 *     "name": "FTDI FT232R USB UART (A60049zx)",
 *     "path": "/dev/cu.usbserial-A60049zx"
 *   }
 * }
 */
VuoSerialDevice VuoSerialDevice_makeFromJson(json_object *js)
{
	VuoSerialDevice value = {VuoSerialDevice_MatchName, NULL, NULL};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "matchType", &o))
		value.matchType = VuoSerialDevice_getMatchTypeForString(json_object_get_string(o));
	if (json_object_object_get_ex(js, "name", &o))
		value.name = VuoText_makeFromJson(o);
	if (json_object_object_get_ex(js, "path", &o))
		value.path = VuoText_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object *VuoSerialDevice_getJson(const VuoSerialDevice value)
{
	json_object *js = json_object_new_object();

	json_object *matchTypeObject = json_object_new_string(VuoSerialDevice_getStringForMatchType(value.matchType));
	json_object_object_add(js, "matchType", matchTypeObject);

	if (value.name)
	{
		json_object *o = VuoText_getJson(value.name);
		json_object_object_add(js, "name", o);
	}

	if (value.path)
	{
		json_object *o = VuoText_getJson(value.path);
		json_object_object_add(js, "path", o);
	}

	return js;
}

/**
 * Returns true if the two values are equal.
 */
bool VuoSerialDevice_areEqual(const VuoSerialDevice valueA, const VuoSerialDevice valueB)
{
	if (valueA.matchType != valueB.matchType)
		return false;

	if (!VuoText_areEqual(valueA.name, valueB.name))
		return false;

	if (!VuoText_areEqual(valueA.path, valueB.path))
		return false;

	return true;
}

/**
 * Returns true if `valueA` is less than `valueB`.
 */
bool VuoSerialDevice_isLessThan(const VuoSerialDevice valueA, const VuoSerialDevice valueB)
{
	return VuoText_isLessThan(valueA.name, valueB.name);
}

/**
 * If `device` specifies name-matching:
 *
 *    - If at least one serial device is connected, sets `realizedDevice` to a specific, existing device matched by its POSIX path, and returns true.
 *    - If no serial devices are connected, returns false, leaving `realizedDevice` unset.
 *
 * If `device` specifies path-matching, sets `realizedDevice` to a copy of `device`, and returns true.
 * (Doesn't bother checking whether the path actually exists.)
 */
bool VuoSerialDevice_realize(VuoSerialDevice device, VuoSerialDevice *realizedDevice)
{
	// Already have a path; nothing to do.
	if (device.matchType == VuoSerialDevice_MatchPath)
	{
		realizedDevice->matchType = device.matchType;
		realizedDevice->name = VuoText_make(device.name);
		realizedDevice->path = VuoText_make(device.path);
		return true;
	}

	// Otherwise, try to find a matching name.

	VuoList_VuoSerialDevice devices = VuoSerial_getDeviceList();
	VuoRetain(devices);

	unsigned long deviceCount = VuoListGetCount_VuoSerialDevice(devices);
	if (deviceCount == 0)
	{
		VUserLog("Warning: No serial devices found.");
		VuoRelease(devices);
		return false;
	}

	VuoText nameToMatch = "";
	if (device.name)
		nameToMatch = device.name;

	VuoText foundName = NULL;
	VuoText foundPath = NULL;
	for (unsigned long i = 1; i <= deviceCount; ++i)
	{
		VuoSerialDevice d = VuoListGetValue_VuoSerialDevice(devices, i);
		if (strstr(d.name, nameToMatch))
		{
			foundName = d.name;
			foundPath = d.path;
			break;
		}
	}

	if (foundPath)
	{
		realizedDevice->matchType = VuoSerialDevice_MatchPath;
		realizedDevice->name = VuoText_make(foundName);
		realizedDevice->path = VuoText_make(foundPath);
		VuoRelease(devices);
		return true;
	}

	VUserLog("Warning: Didn't find a serial device matching '%s'.", device.name);
	VuoRelease(devices);
	return false;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoSerialDevice_getSummary(const VuoSerialDevice value)
{
	VuoSerialDevice realizedDevice;
	if (VuoSerialDevice_realize(value, &realizedDevice))
	{
		VuoSerialDevice_retain(realizedDevice);
		char *outputText = VuoText_format("%s<br>%s", realizedDevice.name, realizedDevice.path);
		VuoSerialDevice_release(realizedDevice);
		return outputText;
	}
	return strdup("(unknown device)");
}
