/**
 * @file
 * VuoGlContext interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGLCONTEXT_H
#define VUOGLCONTEXT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * A framebuffer and state for GL rendering.
 */
typedef void * VuoGlContext;

void VuoGlContext_setGlobalRootContext(void *rootContext);

VuoGlContext VuoGlContext_use(void);
void VuoGlContext_useSpecific(VuoGlContext glContext);
void VuoGlContext_disuse(void);
void VuoGlContext_disuseSpecific(VuoGlContext glContext);

void _VGL(const char *file, const unsigned int line, const char *func);

/**
 * If there's an OpenGL error, prints info about it.  Useful for debugging.
 *
 * \eg{
 * void nodeEvent()
 * {
 *     VuoGlContext_use();
 *
 *     glEnable(GL_ALPHA);
 *     VGL();
 *
 *     VuoGlContext_disuse();
 * }
 * }
 */
#define VGL() _VGL(__FILE__, __LINE__, __func__);

#ifdef __cplusplus
}
#endif

#endif
