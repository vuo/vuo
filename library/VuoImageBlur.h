/**
 * @file
 * VuoImageBlur interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoImage.h"

typedef void *VuoImageBlur;	///< State data for the image blurrer.

VuoImageBlur VuoImageBlur_make(void);
VuoImage VuoImageBlur_blur(VuoImageBlur blur, VuoImage image, VuoReal radius, VuoBoolean expandBounds);

#ifdef __cplusplus
}
#endif
