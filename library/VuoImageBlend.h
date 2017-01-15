/**
 * @file
 * VuoImageBlend interface.
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
#include "VuoList_VuoImage.h"

typedef void *VuoImageBlend;	///< State data for the image blender.

VuoImageBlend VuoImageBlend_make(void);
VuoImage VuoImageBlend_blend(VuoImageBlend blend, VuoList_VuoImage images);

#ifdef __cplusplus
}
#endif
