/**
 * @file
 * VuoImageAverage interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoImage.h"
#include "VuoList_VuoImage.h"

typedef void *VuoImageAverage;  ///< State data for the image averager.

VuoImageAverage VuoImageAverage_make(void);
VuoImage VuoImageAverage_average(VuoImageAverage average, VuoList_VuoImage images);

#ifdef __cplusplus
}
#endif
