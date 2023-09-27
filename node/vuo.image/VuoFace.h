/**
 * @file
 * VuoFace C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoRectangle.h"
#include "VuoPoint2d.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoFace VuoFace
 * Coordinates of a face and its landmarks.
 *
 * @{
 */

/**
 * Coordinates of a face and its landmarks.
 */
typedef struct
{
	VuoRectangle face;
	VuoPoint2d leftEye;
	VuoPoint2d rightEye;
	VuoPoint2d nose;
	VuoPoint2d mouthLeftEdge;
	VuoPoint2d mouthRightEdge;
} VuoFace;

VuoFace VuoFace_makeFromJson(struct json_object *js);
struct json_object *VuoFace_getJson(const VuoFace f);
char *VuoFace_getSummary(const VuoFace f);

VuoFace VuoFace_make(VuoRectangle face,
					 VuoPoint2d leftEye,
					 VuoPoint2d rightEye,
					 VuoPoint2d nose,
					 VuoPoint2d mouthLeftEdge,
					 VuoPoint2d mouthRightEdge);

/// @{
/**
 * Automatically generated functions.
 */
char *VuoFace_getString(const VuoFace value);
void VuoFace_retain(VuoFace value);
void VuoFace_release(VuoFace value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
