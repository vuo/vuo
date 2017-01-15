/**
 * @file
 * VuoGlContext interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGLCONTEXT_H
#define VUOGLCONTEXT_H

#include <OpenGL/OpenGL.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * A framebuffer and state for GL rendering.
 */
typedef void * VuoGlContext;

bool VuoGlContext_isMultisamplingFunctional(VuoGlContext context);

void VuoGlContext_setGlobalRootContext(void *rootContext);

VuoGlContext VuoGlContext_use(void);

void VuoGlContext_disuseF(VuoGlContext glContext, const char *file, const unsigned int line, const char *func);
/**
 * Throws the specified @c VuoGlContext back in the pool.
 *
 * @threadAny
 */
#define VuoGlContext_disuse(glContext) VuoGlContext_disuseF(glContext, __FILE__, __LINE__, __func__)

void *VuoGlContext_makePlatformPixelFormat(bool hasDepthBuffer);

void _VGL(CGLContextObj cgl_ctx, const char *file, const unsigned int line, const char *func);

/**
 * If there's an OpenGL error, prints info about it.  Useful for debugging.
 *
 * \eg{
 * void nodeEvent()
 * {
 *     CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
 *
 *     glEnable(GL_ALPHA);
 *     VGL();
 *
 *     VuoGlContext_disuse(cgl_ctx);
 * }
 * }
 */
#define VGL() _VGL(cgl_ctx, __FILE__, __LINE__, __func__);

#ifdef __cplusplus
}
#endif

#endif
