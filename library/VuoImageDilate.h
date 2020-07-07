/**
 * @file
 * VuoImageDilate interface.
 *
 * @copyright Copyright © 2012–2019 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoImage.h"
#include "VuoBlurShape.h"

typedef void *VuoImageDilate;  ///< State data for the image dilator.

VuoImageDilate VuoImageDilate_make(void);
VuoImage VuoImageDilate_dilate(VuoImageDilate dilate, VuoImage image, VuoBlurShape shape, VuoReal radius, VuoBoolean rescind);

#ifdef __cplusplus
}
#endif
