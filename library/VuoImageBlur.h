/**
 * @file
 * VuoImageBlur interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoImage.h"
#include "VuoDispersion.h"
#include "VuoBlurShape.h"

typedef void *VuoImageBlur;	///< State data for the image blurrer.

VuoImageBlur VuoImageBlur_make(void);
VuoImage VuoImageBlur_blur(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoReal radius, VuoReal quality, VuoBoolean expandBounds);
VuoImage VuoImageBlur_blurDirectionally(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoReal radius, VuoReal quality, VuoReal angle, VuoBoolean symmetric, VuoBoolean expandBounds);
VuoImage VuoImageBlur_blurRadially(VuoImageBlur blur, VuoImage image, VuoImage mask, VuoBlurShape shape, VuoPoint2d center, VuoReal radius, VuoReal quality, VuoDispersion dispersion, VuoCurveEasing symmetry, VuoBoolean expandBounds);

#ifdef __cplusplus
}
#endif
