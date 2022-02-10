/**
 * @file
 * VuoGlPool interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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

GLuint VuoGlPool_use(VuoGlContext glContext, VuoGlPoolType type, unsigned long size);

/**
 * Increments the reference count for @c glBufferName.
 *
 * @threadAny
 */
#define VuoGlPool_retain(glBufferName) VuoGlPool_retainF(glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_retainF(GLuint glBufferName, const char *file, unsigned int linenumber, const char *func);

/**
 * Decrements the reference count for @c glBufferName.
 *
 * @threadAny
 * @version200Changed{Removed `glContext` argument.}
 */
#define VuoGlPool_release(type, size, glBufferName) VuoGlPool_releaseF(type, size, glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_releaseF(VuoGlPoolType type, unsigned long size, GLuint glBufferName, const char *file, unsigned int linenumber, const char *func);

/**
 * Types of OpenGL texture allocations.
 */
typedef enum
{
	VuoGlTexturePool_NoAllocation,
	VuoGlTexturePool_Allocate,
	VuoGlTexturePool_AllocateIOSurface,
} VuoGlTexturePoolAllocation;

GLuint VuoGlTexturePool_use(VuoGlContext glContext, VuoGlTexturePoolAllocation allocation, GLenum target, GLenum internalformat, unsigned short width, unsigned short height, GLenum format, void *ioSurfaceRef);
void VuoGlTexturePool_disuse(VuoGlTexturePoolAllocation allocation, GLenum target, GLenum internalformat, unsigned short width, unsigned short height, GLuint name);

GLuint VuoGlTexture_getType(GLuint format);
unsigned char VuoGlTexture_getChannelCount(GLuint format);
unsigned char VuoGlTexture_getBytesPerPixel(GLuint internalformat, GLuint format);
unsigned char VuoGlTexture_getBytesPerPixelForInternalFormat(GLuint internalformat);
bool VuoGlTexture_formatHasAlphaChannel(GLuint format);

unsigned long VuoGlTexture_getMaximumTextureBytes(VuoGlContext glContext);

void VuoGlTexture_retain(GLuint glTextureName, VuoImage_freeCallback freeCallback, void *freeCallbackContext);
void VuoGlTexture_release(VuoGlTexturePoolAllocation allocation, GLuint glTextureTarget, GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName);
void VuoGlTexture_disown(GLuint glTextureName);

typedef void * VuoIoSurface;	///< A container for a Mac OS X IOSurface.
VuoIoSurface VuoIoSurfacePool_use(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, GLuint *outputTexture);
uint32_t VuoIoSurfacePool_getId(VuoIoSurface vis);
void *VuoIoSurfacePool_getIOSurfaceRef(VuoIoSurface vis);
unsigned short VuoIoSurfacePool_getWidth(VuoIoSurface vis);
unsigned short VuoIoSurfacePool_getHeight(VuoIoSurface vis);
GLuint VuoIoSurfacePool_getTexture(VuoIoSurface vis);
void VuoIoSurfacePool_disuse(VuoIoSurface vis, bool quarantine);

void VuoIoSurfacePool_signal(void *ioSurface);

GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source, void *outIssues);

/**
 * Information about a pooled GL Program Object.
 */
typedef struct
{
	GLuint programName;

	void *uniforms;
} VuoGlProgram;

VuoGlProgram VuoGlProgram_use(VuoGlContext glContext, const char *description, GLuint vertexShaderName, GLuint geometryShaderName, GLuint fragmentShaderName, VuoMesh_ElementAssemblyMethod assemblyMethod, unsigned int expectedOutputPrimitiveCount, void *outIssues);
int VuoGlProgram_getUniformLocation(VuoGlProgram program, const char *uniformIdentifier);

char *VuoGl_stringForConstant(GLenum constant);

void VuoGlPool_logVRAMAllocated(unsigned long bytesAllocated);
void VuoGlPool_logVRAMFreed(unsigned long bytesFreed);

#ifdef __cplusplus
}
#endif
