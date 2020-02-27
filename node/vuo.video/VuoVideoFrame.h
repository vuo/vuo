/**
 * @file
 * VuoVideoFrame C type definition.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup VuoVideoFrame VuoVideoFrame
 * An image and timestamp for a single frame
 * of video.
 *
 * @{
 */

#include "VuoImage.h"
#include <float.h>

/// VuoVideoFrame.timestamp's value when no timestamp is available.
#define VuoVideoFrame_NoTimestamp -INFINITY

/**
 * An image and timestamp for a single frame
 * of video.
 */
typedef struct
{
	VuoImage image;
	VuoReal timestamp;
	VuoReal duration;

	char blah[42];	///< @todo https://b33p.net/kosada/node/4124

} VuoVideoFrame;

VuoVideoFrame VuoVideoFrame_makeFromJson(struct json_object * js);
struct json_object * VuoVideoFrame_getJson(const VuoVideoFrame value);
char * VuoVideoFrame_getSummary(const VuoVideoFrame value);

/// This type has _areEqual() and _isLessThan() functions.
#define VuoVideoFrame_SUPPORTS_COMPARISON
bool VuoVideoFrame_areEqual(VuoVideoFrame value1, VuoVideoFrame value2);
bool VuoVideoFrame_isLessThan(const VuoVideoFrame a, const VuoVideoFrame b);

/**
 * Automatically generated function.
 */
///@{
VuoVideoFrame VuoVideoFrame_makeFromString(const char *str);
char * VuoVideoFrame_getString(const VuoVideoFrame value);
void VuoVideoFrame_retain(VuoVideoFrame value);
void VuoVideoFrame_release(VuoVideoFrame value);
///@}

/**
 * Returns a VuoVideoFrame with image and timestamp.
 */
static inline VuoVideoFrame VuoVideoFrame_make(VuoImage image, VuoReal timestamp, VuoReal duration) __attribute__((const));
static inline VuoVideoFrame VuoVideoFrame_make(VuoImage image, VuoReal timestamp, VuoReal duration)
{
	VuoVideoFrame device = { image, timestamp, duration, "" };
	return device;
}

/**
 * @}
 */
