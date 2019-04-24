/**
 * @file
 * VuoBlackmagicOutputDevice C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlackmagicOutputDevice VuoBlackmagicOutputDevice
 * Information about a Blackmagic video capture device.
 *
 * @{
 */

#include "VuoInteger.h"
#include "VuoText.h"
#include "VuoBlackmagicConnection.h"
#include "VuoBlackmagicVideoMode.h"
#include "VuoDeinterlacing.h"

/**
 * Information about a Blackmagic video capture device.
 */
typedef struct
{
	VuoText name;
	VuoInteger subDevice;
	VuoBlackmagicConnection connection;
	VuoBlackmagicVideoMode videoMode;

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoBlackmagicOutputDevice;

VuoBlackmagicOutputDevice VuoBlackmagicOutputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoBlackmagicOutputDevice_getJson(const VuoBlackmagicOutputDevice value);
char *VuoBlackmagicOutputDevice_getSummary(const VuoBlackmagicOutputDevice value);
char *VuoBlackmagicOutputDevice_getShortSummary(const VuoBlackmagicOutputDevice value);

VuoBlackmagicOutputDevice VuoBlackmagicOutputDevice_make(const VuoText name, const VuoInteger subDevice, const VuoBlackmagicConnection connection, const VuoBlackmagicVideoMode videoMode);

#define VuoBlackmagicOutputDevice_SUPPORTS_COMPARISON
bool VuoBlackmagicOutputDevice_areEqual(const VuoBlackmagicOutputDevice value1, const VuoBlackmagicOutputDevice value2);
bool VuoBlackmagicOutputDevice_isLessThan(const VuoBlackmagicOutputDevice value1, const VuoBlackmagicOutputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoBlackmagicOutputDevice VuoBlackmagicOutputDevice_makeFromString(const char *str);
char *VuoBlackmagicOutputDevice_getString(const VuoBlackmagicOutputDevice value);
void VuoBlackmagicOutputDevice_retain(VuoBlackmagicOutputDevice value);
void VuoBlackmagicOutputDevice_release(VuoBlackmagicOutputDevice value);
///@}

/**
 * @}
 */
