/**
 * @file
 * VuoLeapPointable C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoInteger.h"
#include "VuoPoint3d.h"
#include "VuoLeapPointableType.h"
#include "VuoLeapTouchZone.h"
#include "VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoLeapPointable VuoLeapPointable
 * Coordinates of a single Leap "pointable" (a finger or tool).
 *
 * @{
 */

/**
 * Coordinates of a single Leap "pointable" (a finger or tool).
 */
typedef struct
{
	VuoInteger id;

	VuoLeapPointableType type;

	VuoReal length;
	VuoReal width;
	VuoPoint3d direction;

	VuoPoint3d tipPosition;
	VuoPoint3d tipVelocity;

	VuoReal timeVisible;
	VuoReal touchDistance;

	VuoLeapTouchZone touchZone;

	VuoBoolean isExtended;
} VuoLeapPointable;

VuoLeapPointable VuoLeapPointable_makeFromJson(struct json_object * js);
struct json_object * VuoLeapPointable_getJson(const VuoLeapPointable value);
char * VuoLeapPointable_getSummary(const VuoLeapPointable value);

VuoLeapPointable VuoLeapPointable_make(VuoInteger id,
	VuoLeapPointableType type,
	VuoReal length,
	VuoReal width,
	VuoPoint3d direction,
	VuoPoint3d tipPosition,
	VuoPoint3d tipVelocity,
	VuoReal timeVisible,
	VuoReal touchDistance,
	VuoLeapTouchZone touchZone,
	VuoBoolean isExtended);

/// @{
/**
 * Automatically generated functions.
 */
char * VuoLeapPointable_getString(const VuoLeapPointable value);
void VuoLeapPointable_retain(VuoLeapPointable value);
void VuoLeapPointable_release(VuoLeapPointable value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
