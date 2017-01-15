/**
 * @file
 * VuoOscOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoOscOutputDevice VuoOscOutputDevice
 * Information about an OSC Output device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * Information about an OSC Output device.
 */
typedef struct
{
	VuoText name;
	VuoText ipAddress;
	VuoInteger port;
} VuoOscOutputDevice;

VuoOscOutputDevice VuoOscOutputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoOscOutputDevice_getJson(const VuoOscOutputDevice value);
char *VuoOscOutputDevice_getSummary(const VuoOscOutputDevice value);

VuoOscOutputDevice VuoOscOutputDevice_makeBroadcast(const VuoText name, const VuoInteger port);
VuoOscOutputDevice VuoOscOutputDevice_makeUnicast(const VuoText name, const VuoText ipAddress, const VuoInteger port);

#define VuoOscOutputDevice_SUPPORTS_COMPARISON
bool VuoOscOutputDevice_areEqual(const VuoOscOutputDevice value1, const VuoOscOutputDevice value2);
bool VuoOscOutputDevice_isLessThan(const VuoOscOutputDevice value1, const VuoOscOutputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoOscOutputDevice VuoOscOutputDevice_makeFromString(const char *str);
char *VuoOscOutputDevice_getString(const VuoOscOutputDevice value);
void VuoOscOutputDevice_retain(VuoOscOutputDevice value);
void VuoOscOutputDevice_release(VuoOscOutputDevice value);
///@}

/**
 * @}
 */
