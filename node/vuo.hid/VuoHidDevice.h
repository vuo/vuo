/**
 * @file
 * VuoHidDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoHidDevice VuoHidDevice
 * Information about a Hid I/O device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"
#include "VuoHidControl.h"
#include "VuoList_VuoHidControl.h"

/**
 * Which field to match on.
 */
typedef enum
{
	VuoHidDevice_MatchName,
	VuoHidDevice_MatchLocation,
} VuoHidDevice_MatchType;

/**
 * Information about a USB HID device.
 */
typedef struct
{
	VuoHidDevice_MatchType matchType;	///< Which field to use for matching a VuoHidDevice structure to an actual device.
	VuoText name;						///< The device's display name.
	VuoInteger location;				///< Where the device is connected in the USB topology.  `kIOHIDLocationIDKey << 8 + kUSBInterfaceNumber`

	VuoList_VuoHidControl controls;		///< All the device's controls.

	// IOHIDManager can't match on kIOHIDLocationIDKey, so we have to match on other criteria, then filter those results by location.
	VuoInteger vendorID;
	VuoInteger productID;
	VuoInteger usagePage;
	VuoInteger usage;
} VuoHidDevice;

VuoHidDevice VuoHidDevice_makeFromJson(struct json_object *js);
struct json_object *VuoHidDevice_getJson(const VuoHidDevice value);
char *VuoHidDevice_getSummary(const VuoHidDevice value);

#define VuoHidDevice_SUPPORTS_COMPARISON
bool VuoHidDevice_areEqual(const VuoHidDevice valueA, const VuoHidDevice valueB);
bool VuoHidDevice_isLessThan(const VuoHidDevice valueA, const VuoHidDevice valueB);

bool VuoHidDevice_realize(VuoHidDevice device, VuoHidDevice *realizedDevice);

/**
 * Automatically generated function.
 */
///@{
VuoHidDevice VuoHidDevice_makeFromString(const char *str);
char *VuoHidDevice_getString(const VuoHidDevice value);
void VuoHidDevice_retain(VuoHidDevice value);
void VuoHidDevice_release(VuoHidDevice value);
///@}

/**
 * @}
 */

