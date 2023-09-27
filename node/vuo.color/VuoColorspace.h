/**
 * @file
 * VuoColorspace interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoColor.h"
#include "VuoData.h"

VuoColor VuoColorspace_makeCMYKAColor(VuoReal c, VuoReal m, VuoReal y, VuoReal k, VuoReal a, int colorspace);
void VuoColorspace_getCMYKA(VuoColor color,  int colorspace, VuoReal *c, VuoReal *m, VuoReal *y, VuoReal *k, VuoReal *a);

VuoColor VuoColorspace_makeICCColor_VuoInteger(VuoInteger colorspace, VuoList_VuoReal components);
VuoColor VuoColorspace_makeICCColor_VuoData(VuoData colorspace, VuoList_VuoReal components);

VuoList_VuoReal VuoColorspace_getICC_VuoInteger(VuoInteger colorspace, VuoColor color);
VuoList_VuoReal VuoColorspace_getICC_VuoData(VuoData colorspace, VuoColor color);
