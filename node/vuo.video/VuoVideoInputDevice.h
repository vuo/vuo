/**
 * @file
 * VUOVIDEOINPUTDEVICE C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoVideoInputDevice VuoVideoInputDevice
 * Information about a video input device.
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
} VuoVideoInputDevice;

#define VuoVideoInputDevice_SUPPORTS_COMPARISON

VuoVideoInputDevice VuoVideoInputDevice_makeFromJson(struct json_object * js);
struct json_object * VuoVideoInputDevice_getJson(const VuoVideoInputDevice value);
char * VuoVideoInputDevice_getSummary(const VuoVideoInputDevice value);

bool VuoVideoInputDevice_areEqual(VuoVideoInputDevice value1, VuoVideoInputDevice value2);
bool VuoVideoInputDevice_isLessThan(const VuoVideoInputDevice a, const VuoVideoInputDevice b);

/**
 * Automatically generated function.
 */
///@{
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
    return (VuoVideoInputDevice){VuoVideoInputDevice_MatchIdThenName, id, name};
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
