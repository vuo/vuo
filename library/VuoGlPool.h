/**
 * @file
 * VuoGlPool interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGLPOOL_H
#define VUOGLPOOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OpenGL/OpenGL.h>

/**
 * Types of OpenGL objects supported by Vuo's GL object pool.
 */
typedef enum
{
	VuoGlPool_ArrayBuffer,
	VuoGlPool_ElementArrayBuffer,
	VuoGlPool_Texture
//	VuoGlPool_Framebuffer	// Cannot be shared between contexts
//	VuoGlPool_VertexArray	// Cannot be shared between contexts
} VuoGlPoolType;

GLuint VuoGlPool_use(VuoGlPoolType type);
void VuoGlPool_disuse(VuoGlPoolType type, GLuint name);

void VuoGlTexture_retain(GLuint glTextureName);
void VuoGlTexture_release(GLuint glTextureName);

#ifdef __cplusplus
}
#endif

#endif
