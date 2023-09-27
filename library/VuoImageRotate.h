/**
 * @file
 * VuoImageRotate interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"

typedef void *VuoImageRotate;  ///< Rotation state data.

VuoImageRotate VuoImageRotate_make(void);
VuoImage VuoImageRotate_rotate(VuoImage image, VuoImageRotate rotator, VuoReal angleInDegrees, VuoBoolean expandBounds);

#ifdef __cplusplus
}
#endif
