/**
 * @file
 * VuoLeapPointable C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLEAPPOINTABLE_H
#define VUOLEAPPOINTABLE_H

#include "VuoInteger.h"
#include "VuoPoint3d.h"
#include "VuoLeapPointableType.h"
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
} VuoLeapPointable;

VuoLeapPointable VuoLeapPointable_valueFromJson(struct json_object * js);
struct json_object * VuoLeapPointable_jsonFromValue(const VuoLeapPointable value);
char * VuoLeapPointable_summaryFromValue(const VuoLeapPointable value);

VuoLeapPointable VuoLeapPointable_make(VuoInteger id, VuoLeapPointableType type, VuoReal length, VuoReal width, VuoPoint3d direction, VuoPoint3d tipPosition, VuoPoint3d tipVelocity);

/// @{
/**
 * Automatically generated function.
 */
VuoLeapPointable VuoLeapPointable_valueFromString(const char * initializer);
char * VuoLeapPointable_stringFromValue(const VuoLeapPointable value);
/// @}

/**
 * @}
 */

#endif
