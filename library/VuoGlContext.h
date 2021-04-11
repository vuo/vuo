/**
 * @file
 * VuoGlContext interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoMacOSSDKWorkaround.h"
#include <OpenGL/OpenGL.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * A framebuffer and state for GL rendering.
 */
typedef void * VuoGlContext;

int VuoGlContext_getMaximumSupportedMultisampling(VuoGlContext context);

void VuoGlContext_setGlobalRootContext(void *rootContext);

VuoGlContext VuoGlContext_use(void);

void VuoGlContext_perform(void (^function)(CGLContextObj cgl_ctx));

void VuoGlContext_disuseF(VuoGlContext glContext, const char *file, const unsigned int linenumber, const char *func);
/**
 * Throws the specified @c VuoGlContext back in the pool.
 *
 * @threadAny
 */
#define VuoGlContext_disuse(glContext) VuoGlContext_disuseF(glContext, __FILE__, __LINE__, __func__)

void *VuoGlContext_makePlatformPixelFormat(bool hasDepthBuffer, bool openGL32Core, GLint displayMask);

bool VuoGlContext_isOpenGL32Core(VuoGlContext context);

void _VGL(CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func);

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
