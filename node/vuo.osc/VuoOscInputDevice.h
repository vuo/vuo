/**
 * @file
 * VuoOscInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscInputDevice VuoOscInputDevice
 * Information about an OSC input device.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * Information about an OSC input device.
 */
typedef struct
{
	VuoText name;
	VuoText ipAddress;
	VuoInteger port;
} VuoOscInputDevice;

#define VuoOscInputDevice_SUPPORTS_COMPARISON

VuoOscInputDevice VuoOscInputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoOscInputDevice_getJson(const VuoOscInputDevice value);
char *VuoOscInputDevice_getSummary(const VuoOscInputDevice value);

VuoOscInputDevice VuoOscInputDevice_make(const VuoText name, const VuoText ipAddress, const VuoInteger port);

bool VuoOscInputDevice_areEqual(const VuoOscInputDevice value1, const VuoOscInputDevice value2);
bool VuoOscInputDevice_isLessThan(const VuoOscInputDevice value1, const VuoOscInputDevice value2);

/**
 * Automatically generated function.
 */
///@{
char *VuoOscInputDevice_getString(const VuoOscInputDevice value);
void VuoOscInputDevice_retain(VuoOscInputDevice value);
void VuoOscInputDevice_release(VuoOscInputDevice value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
