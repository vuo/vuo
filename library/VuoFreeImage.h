/**
 * @file
 * VuoFreeImage interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImage.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FREEIMAGE_H
/// FreeImage bitmap.
typedef struct FIBITMAP FIBITMAP;
#endif

FIBITMAP *VuoFreeImage_convertVuoImageToFreeImage(VuoImage image, bool allowFloat, bool unpremultiply);
VuoImage VuoFreeImage_convertFreeImageToVuoImage(FIBITMAP *fi, void *dataToFree, const char *imageURL);

#ifdef __cplusplus
}
#endif
