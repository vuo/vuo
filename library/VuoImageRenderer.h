/**
 * @file
 * VuoImageRenderer interface.
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

#include "VuoShader.h"
#include "VuoImage.h"
#include "VuoImageColorDepth.h"

VuoImage VuoImageRenderer_render(VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth);

unsigned long int VuoImageRenderer_draw_internal(VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth, bool outputToIOSurface, bool outputToGlTextureRectangle, unsigned int outputToSpecificTexture, GLuint *outputInternalFormat);

#ifdef __cplusplus
}
#endif
