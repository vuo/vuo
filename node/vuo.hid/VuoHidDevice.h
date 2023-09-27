/**
 * @file
 * VuoHidDevice C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoHidDevice VuoHidDevice
 * Information about a Hid I/O device.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

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
	VuoHidDevice_MatchVendorAndProduct,
	VuoHidDevice_MatchUsage,
} VuoHidDevice_MatchType;

/**
 * Information about a USB or Bluetooth HID device.
 */
typedef struct
{
	VuoHidDevice_MatchType matchType;	///< Which field to use for matching a VuoHidDevice structure to an actual device.
	VuoText name;						///< The device's display name.
	VuoInteger location;                ///< Where the device is connected in the system topology.  `kIOHIDLocationIDKey << 8 + kUSBInterfaceNumber`

	VuoList_VuoHidControl controls;		///< All the device's controls.

	VuoInteger vendorID;
	VuoInteger productID;
	VuoInteger usagePage;
	VuoInteger usage;
} VuoHidDevice;

#define VuoHidDevice_SUPPORTS_COMPARISON

VuoHidDevice VuoHidDevice_makeFromJson(struct json_object *js);
struct json_object *VuoHidDevice_getJson(const VuoHidDevice value);
char *VuoHidDevice_getSummary(const VuoHidDevice value);

bool VuoHidDevice_areEqual(const VuoHidDevice valueA, const VuoHidDevice valueB);
bool VuoHidDevice_isLessThan(const VuoHidDevice valueA, const VuoHidDevice valueB);

bool VuoHidDevice_realize(VuoHidDevice device, VuoHidDevice *realizedDevice) VuoWarnUnusedResult;

/**
 * Automatically generated function.
 */
///@{
char *VuoHidDevice_getString(const VuoHidDevice value);
void VuoHidDevice_retain(VuoHidDevice value);
void VuoHidDevice_release(VuoHidDevice value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
