/**
 * @file
 * VuoLeapTouchZone C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapTouchZone VuoLeapTouchZone
 * How close a pointable is to the touch zone.
 *
 * @{
 */

/**
 * How close a pointable is to the touch zone.
 */
typedef enum {
	VuoLeapTouchZone_None,
	VuoLeapTouchZone_Hovering,
	VuoLeapTouchZone_Touching
} VuoLeapTouchZone;

#include "VuoList_VuoLeapTouchZone.h"

VuoLeapTouchZone VuoLeapTouchZone_makeFromJson(struct json_object * js);
struct json_object * VuoLeapTouchZone_getJson(const VuoLeapTouchZone value);
VuoList_VuoLeapTouchZone VuoLeapTouchZone_getAllowedValues(void);
char * VuoLeapTouchZone_getSummary(const VuoLeapTouchZone value);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoLeapTouchZone_getString(const VuoLeapTouchZone value);
void VuoLeapTouchZone_retain(VuoLeapTouchZone value);
void VuoLeapTouchZone_release(VuoLeapTouchZone value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif
