/**
 * @file
 * VUOVIDEOINPUTDEVICE C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoVideoInputDevice VuoVideoInputDevice
 * Information about an Quicktime input device.
 *
 * @{
 */

#include "VuoText.h"

/**
 * Which field to match on.
 */
typedef enum
{
	VuoVideoInputDevice_MatchIdThenName,
	VuoVideoInputDevice_MatchId,
} VuoVideoInputDevice_MatchType;

/**
 * Information about a video input device.
 */
typedef struct
{
	VuoVideoInputDevice_MatchType matchType;	///< Which field to use for matching a VuoVideoInputDevice structure to an actual device.

	VuoText id;		///< [QtCaptureDevice modelUniqueID]
	VuoText name;	///< [QtCaptureDevice localizedDisplayName]

	char blah[42];	///< @todo https://b33p.net/kosada/node/4124

} VuoVideoInputDevice;

VuoVideoInputDevice VuoVideoInputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoVideoInputDevice_getJson(const VuoVideoInputDevice value);
char * VuoVideoInputDevice_getSummary(const VuoVideoInputDevice value);
bool VuoVideoInputDevice_areEqual(VuoVideoInputDevice value1, VuoVideoInputDevice value2);

/**
 * Automatically generated function.
 */
///@{
VuoVideoInputDevice VuoVideoInputDevice_makeFromString(const char *str);
char * VuoVideoInputDevice_getString(const VuoVideoInputDevice value);
void VuoVideoInputDevice_retain(VuoVideoInputDevice value);
void VuoVideoInputDevice_release(VuoVideoInputDevice value);
///@}

/**
 * Returns a video input device with the specified values.
 */
static inline VuoVideoInputDevice VuoVideoInputDevice_make(VuoText id, VuoText name) __attribute__((const));
static inline VuoVideoInputDevice VuoVideoInputDevice_make(VuoText id, VuoText name)
{
	VuoVideoInputDevice device = { VuoVideoInputDevice_MatchIdThenName, id, name };
	return device;
}

/**
 * @}
 */


