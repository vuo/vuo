/**
 * @file
 * VuoArtNetOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOARTNETOUTPUTDEVICE_H
#define VUOARTNETOUTPUTDEVICE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoArtNetOutputDevice VuoArtNetOutputDevice
 * Information about an Art-Net output device.
 *
 * @{
 */

#include "VuoArtNetInputDevice.h"

/**
 * Information about an Art-Net output device.
 */
typedef struct
{
	VuoText name;
	VuoText ipAddress;
	VuoText ethernetAddress;
	VuoArtNetAddress address;
} VuoArtNetOutputDevice;

VuoArtNetOutputDevice VuoArtNetOutputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoArtNetOutputDevice_getJson(const VuoArtNetOutputDevice value);
char *VuoArtNetOutputDevice_getSummary(const VuoArtNetOutputDevice value);

VuoArtNetOutputDevice VuoArtNetOutputDevice_makeBroadcast(const VuoInteger net, const VuoInteger subNet, const VuoInteger universe);
VuoArtNetOutputDevice VuoArtNetOutputDevice_makeUnicast(const VuoText ipAddress, const VuoInteger net, const VuoInteger subNet, const VuoInteger universe);

#define VuoArtNetOutputDevice_SUPPORTS_COMPARISON
bool VuoArtNetOutputDevice_areEqual(const VuoArtNetOutputDevice value1, const VuoArtNetOutputDevice value2);
bool VuoArtNetOutputDevice_isLessThan(const VuoArtNetOutputDevice value1, const VuoArtNetOutputDevice value2);


/**
 * Automatically generated function.
 */
///@{
VuoArtNetOutputDevice VuoArtNetOutputDevice_makeFromString(const char *str);
char *VuoArtNetOutputDevice_getString(const VuoArtNetOutputDevice value);
void VuoArtNetOutputDevice_retain(VuoArtNetOutputDevice value);
void VuoArtNetOutputDevice_release(VuoArtNetOutputDevice value);
///@}

/**
 * @}
 */

#endif // VUOARTNETOUTPUTDEVICE_H
