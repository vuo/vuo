/**
 * @file
 * VuoGlPool interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoGlContext.h"
#include "VuoImage.h"
#include "VuoMesh.h"

#include <stdint.h>

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

GLuint VuoGlPool_use(VuoGlPoolType type, unsigned long size);

/**
 * Increments the reference count for @c glBufferName.
 *
 * @threadAny
 */
#define VuoGlPool_retain(glBufferName) VuoGlPool_retainF(glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_retainF(GLuint glBufferName, const char *file, unsigned int line, const char *func);

/**
 * Decrements the reference count for @c glBufferName.
 *
 * @threadAny
 */
#define VuoGlPool_release(type, size, glBufferName) VuoGlPool_releaseF(type, size, glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_releaseF(VuoGlPoolType type, unsigned long size, GLuint glBufferName, const char *file, unsigned int line, const char *func);

GLuint VuoGlTexturePool_use(VuoGlContext glContext, GLenum internalformat, unsigned short width, unsigned short height, GLenum format);
GLuint VuoGlTexture_getType(GLuint format);
unsigned char VuoGlTexture_getChannelCount(GLuint format);
unsigned char VuoGlTexture_getBytesPerPixel(GLuint internalformat, GLuint format);
unsigned long VuoGlTexture_getMaximumTextureBytes(VuoGlContext glContext);

void VuoGlTexture_retain(GLuint glTextureName, VuoImage_freeCallback freeCallback, void *freeCallbackContext);
void VuoGlTexture_release(GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName, GLuint glTextureTarget);
void VuoGlTexture_disown(GLuint glTextureName);

typedef void * VuoIoSurface;	///< A container for a Mac OS X IOSurface.
VuoIoSurface VuoIoSurfacePool_use(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, GLuint *outputTexture);
uint32_t VuoIoSurfacePool_getId(VuoIoSurface vis);
void *VuoIoSurfacePool_getIOSurfaceRef(VuoIoSurface vis);
unsigned short VuoIoSurfacePool_getWidth(VuoIoSurface vis);
unsigned short VuoIoSurfacePool_getHeight(VuoIoSurface vis);
GLuint VuoIoSurfacePool_getTexture(VuoIoSurface vis);
void VuoIoSurfacePool_disuse(VuoIoSurface vis);

void VuoIoSurfacePool_signal(void *ioSurface);

GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source);

/**
 * Information about a pooled GL Program Object.
 */
typedef struct
{
	GLuint programName;

	void *uniforms;
} VuoGlProgram;

VuoGlProgram VuoGlProgram_use(VuoGlContext glContext, const char *description, GLuint vertexShaderName, GLuint geometryShaderName, GLuint fragmentShaderName, VuoMesh_ElementAssemblyMethod assemblyMethod, unsigned int expectedOutputPrimitiveCount);
int VuoGlProgram_getUniformLocation(VuoGlProgram program, const char *uniformIdentifier);
void VuoGlProgram_lock(GLuint programName);
void VuoGlProgram_unlock(GLuint programName);

char *VuoGl_stringForConstant(GLenum constant);

extern dispatch_semaphore_t VuoGlSemaphore;

#ifdef __cplusplus
}
#endif
