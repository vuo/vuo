/**
 * @file
 * VuoSerialDevice C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoSerialDevice VuoSerialDevice
 * Information about a serial I/O device.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoText.h"

/**
 * Which field to match on.
 */
typedef enum
{
	VuoSerialDevice_MatchName,
	VuoSerialDevice_MatchPath,
} VuoSerialDevice_MatchType;

/**
 * Information about a serial I/O device.
 */
typedef struct
{
	VuoSerialDevice_MatchType matchType;	///< Which field to use for matching a VuoSerialDevice structure to an actual device.

	VuoText name;	///< The device's display name.
	VuoText path;	///< The POSIX filesystem path to the device file.
} VuoSerialDevice;

#define VuoSerialDevice_SUPPORTS_COMPARISON

VuoSerialDevice VuoSerialDevice_makeFromJson(struct json_object *js);
struct json_object *VuoSerialDevice_getJson(const VuoSerialDevice value);
char *VuoSerialDevice_getSummary(const VuoSerialDevice value);

bool VuoSerialDevice_areEqual(const VuoSerialDevice valueA, const VuoSerialDevice valueB);
bool VuoSerialDevice_isLessThan(const VuoSerialDevice valueA, const VuoSerialDevice valueB);

bool VuoSerialDevice_realize(VuoSerialDevice device, VuoSerialDevice *realizedDevice) VuoWarnUnusedResult;

/**
 * Automatically generated function.
 */
///@{
char *VuoSerialDevice_getString(const VuoSerialDevice value);
void VuoSerialDevice_retain(VuoSerialDevice value);
void VuoSerialDevice_release(VuoSerialDevice value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
