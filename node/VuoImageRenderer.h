/**
 * @file
 * VuoImageRenderer interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoShader.h"
#include "VuoImage.h"

/**
 * An object for rendering a VuoImage.
 */
typedef void * VuoImageRenderer;

VuoImageRenderer VuoImageRenderer_make(void);
VuoImage VuoImageRenderer_draw(VuoImageRenderer imageRenderer, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh);

unsigned long int VuoImageRenderer_draw_internal(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, bool outputToIOSurface);

#ifdef __cplusplus
}
#endif
