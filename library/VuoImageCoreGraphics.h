/**
 * @file
 * VuoImageCoreGraphics interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"

#include <ImageIO/ImageIO.h>

VuoImage VuoImage_makeFromCGImage(CGImageRef cgi);

#ifdef __cplusplus
}
#endif
