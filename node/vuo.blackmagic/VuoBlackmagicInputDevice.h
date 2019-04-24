/**
 * @file
 * VuoBlackmagicInputDevice C type definition.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoBlackmagicInputDevice VuoBlackmagicInputDevice
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
	VuoDeinterlacing deinterlacing;

	char blah[42]; ///< @todo https://b33p.net/kosada/node/4124
} VuoBlackmagicInputDevice;

VuoBlackmagicInputDevice VuoBlackmagicInputDevice_makeFromJson(struct json_object *js);
struct json_object *VuoBlackmagicInputDevice_getJson(const VuoBlackmagicInputDevice value);
char *VuoBlackmagicInputDevice_getSummary(const VuoBlackmagicInputDevice value);
char *VuoBlackmagicInputDevice_getShortSummary(const VuoBlackmagicInputDevice value);

VuoBlackmagicInputDevice VuoBlackmagicInputDevice_make(const VuoText name, const VuoInteger subDevice, const VuoBlackmagicConnection connection, const VuoBlackmagicVideoMode videoMode, const VuoDeinterlacing deinterlacing);

#define VuoBlackmagicInputDevice_SUPPORTS_COMPARISON
bool VuoBlackmagicInputDevice_areEqual(const VuoBlackmagicInputDevice value1, const VuoBlackmagicInputDevice value2);
bool VuoBlackmagicInputDevice_isLessThan(const VuoBlackmagicInputDevice value1, const VuoBlackmagicInputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoBlackmagicInputDevice VuoBlackmagicInputDevice_makeFromString(const char *str);
char *VuoBlackmagicInputDevice_getString(const VuoBlackmagicInputDevice value);
void VuoBlackmagicInputDevice_retain(VuoBlackmagicInputDevice value);
void VuoBlackmagicInputDevice_release(VuoBlackmagicInputDevice value);
///@}

/**
 * @}
 */
