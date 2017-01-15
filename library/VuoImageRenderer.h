/**
 * @file
 * VuoImageRenderer interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoShader.h"
#include "VuoImage.h"
#include "VuoImageColorDepth.h"

/**
 * An object for rendering a VuoImage.
 */
typedef void * VuoImageRenderer;

VuoImageRenderer VuoImageRenderer_make(VuoGlContext glContext);
VuoImage VuoImageRenderer_draw(VuoImageRenderer imageRenderer, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth);

unsigned long int VuoImageRenderer_draw_internal(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth, bool outputToIOSurface, bool outputToGlTextureRectangle, unsigned int outputToSpecificTexture);

#ifdef __cplusplus
}
#endif
