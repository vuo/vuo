/**
 * @file
 * VuoGlPool interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGLPOOL_H
#define VUOGLPOOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoGlContext.h"

/**
 * Types of OpenGL objects supported by Vuo's GL object pool.
 */
typedef enum
{
	VuoGlPool_ArrayBuffer,
	VuoGlPool_ElementArrayBuffer
//	VuoGlPool_Texture		///< @see VuoGlTexturePool_use
//	VuoGlPool_Framebuffer	///< Cannot be shared between contexts
//	VuoGlPool_VertexArray	///< Cannot be shared between contexts
} VuoGlPoolType;

GLuint VuoGlPool_use(VuoGlContext glContext, VuoGlPoolType type, unsigned long size);
void VuoGlPool_disuse(VuoGlContext glContext, VuoGlPoolType type, unsigned long size, GLuint name);

GLuint VuoGlTexturePool_use(VuoGlContext glContext, GLenum internalformat, unsigned short width, unsigned short height, GLenum format);

void VuoGlTexture_retain(GLuint glTextureName);
void VuoGlTexture_release(GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName);

GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source);

const char *VuoGl_stringForConstant(GLenum constant);

#ifdef __cplusplus
}
#endif

#endif
