/**
 * @file
 * VuoArtNetInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOARTNETINPUTDEVICE_H
#define VUOARTNETINPUTDEVICE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoArtNetInputDevice VuoArtNetInputDevice
 * Information about an Art-Net input device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"

/**
 * A set of values that specifiy a DMX512 universe.
 */
typedef struct
{
	VuoInteger net;
	VuoInteger subNet;
	VuoInteger universe;
} VuoArtNetAddress;

/**
 * Information about an Art-Net input device.
 */
typedef struct
{
	VuoText name;
	VuoText ipAddress;
	VuoText ethernetAddress;
	VuoArtNetAddress address;
} VuoArtNetInputDevice;

VuoArtNetInputDevice VuoArtNetInputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoArtNetInputDevice_getJson(const VuoArtNetInputDevice value);
char *VuoArtNetInputDevice_getSummary(const VuoArtNetInputDevice value);

VuoArtNetInputDevice VuoArtNetInputDevice_make(const VuoInteger net, const VuoInteger subNet, const VuoInteger universe);

#define VuoArtNetInputDevice_SUPPORTS_COMPARISON
bool VuoArtNetInputDevice_areEqual(const VuoArtNetInputDevice value1, const VuoArtNetInputDevice value2);
bool VuoArtNetInputDevice_isLessThan(const VuoArtNetInputDevice value1, const VuoArtNetInputDevice value2);

bool VuoArtNetAddress_areEqual(const VuoArtNetAddress value1, const VuoArtNetAddress value2);
bool VuoArtNetAddress_isLessThan(const VuoArtNetAddress valueA, const VuoArtNetAddress valueB);


/**
 * Automatically generated function.
 */
///@{
VuoArtNetInputDevice VuoArtNetInputDevice_makeFromString(const char *str);
char *VuoArtNetInputDevice_getString(const VuoArtNetInputDevice value);
void VuoArtNetInputDevice_retain(VuoArtNetInputDevice value);
void VuoArtNetInputDevice_release(VuoArtNetInputDevice value);
///@}

/**
 * @}
 */

#endif // VUOARTNETINPUTDEVICE_H
