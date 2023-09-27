/**
 * @file
 * VuoGlPool implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoGlPool.h"
#include "VuoGlContext.h"
#include "VuoEventLoop.h"

#include <vector>
#include <utility>	///< for pair
#include <queue>
#include <set>
#include <deque>
#include <map>
#include <locale>
#include <sstream>
using namespace std;

#include "VuoStringUtilities.hh"
#include "VuoShaderIssues.hh"

#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurface.h>

#include <OpenGL/CGLIOSurface.h>
#include <OpenGL/CGLMacro.h>
#include <ApplicationServices/ApplicationServices.h>


static map<VuoGlPoolType, map<unsigned long, vector<GLuint> > > VuoGlPool __attribute__((init_priority(101)));
static dispatch_semaphore_t VuoGlPool_semaphore;	///< Serializes access to VuoGlPool.

/**
 * Returns an OpenGL Buffer Object of type @c type.
 *
 * If an existing, unused buffer of the specified @c type and @c size is available, it is returned.
 * Otherwise, a new buffer is created.
 *
 * The returned buffer's storage is will be preallocated (so the caller can efficiently upload data using [glBufferSubData](https://www.khronos.org/opengl/wiki/GLAPI/glBufferSubData)).
 *
 * @threadAnyGL
 */
GLuint VuoGlPool_use(VuoGlContext glContext, VuoGlPoolType type, unsigned long size)
{
	GLuint name = 0;

	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (VuoGlPool[type][size].size())
		{
			name = VuoGlPool[type][size].back();
			VuoGlPool[type][size].pop_back();
//			VLog("using recycled type=%d name=%d",type,name);
		}
		else
		{
			CGLContextObj cgl_ctx = (CGLContextObj)glContext;

			if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
			{
				glGenBuffers(1, &name);
				GLenum bufferType = type == VuoGlPool_ArrayBuffer ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
				glBindBuffer(bufferType, name);
				glBufferData(bufferType, size, NULL, GL_STREAM_DRAW);
				glBindBuffer(bufferType, 0);
				VuoGlPool_logVRAMAllocated(size);
//				VLog("allocated type=%d name=%d",type,name);
			}
			else
				VUserLog("Unknown pool type %d.", type);
		}
	}
	dispatch_semaphore_signal(VuoGlPool_semaphore);

	return name;
}

/**
 * Indicates that the caller is done using the OpenGL object @c name of type @c type.
 *
 * The object is returned to the pool, so other callers can use it
 * (which is more efficient than deleting and re-generating objects).
 *
 * @threadAnyGL
 */
void VuoGlPool_disuse(VuoGlPoolType type, unsigned long size, GLuint name)
{
	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
			VuoGlPool[type][size].push_back(name);
		else
			VUserLog("Unknown pool type %d.", type);
	}
	dispatch_semaphore_signal(VuoGlPool_semaphore);
}

typedef map<GLuint, unsigned int> VuoGlPoolReferenceCounts;	///< The number of times each OpenGL Buffer Object is retained.
static VuoGlPoolReferenceCounts VuoGlPool_referenceCounts __attribute__((init_priority(101)));  ///< The reference count for each OpenGL Buffer Object.
static dispatch_semaphore_t VuoGlPool_referenceCountsSemaphore = NULL;  ///< Synchronizes access to @c VuoGlPool_referenceCounts.
static void __attribute__((constructor)) VuoGlPool_referenceCountsInit(void)
{
	VuoGlPool_referenceCountsSemaphore = dispatch_semaphore_create(1);
}

/**
 * Helper for @ref VuoGlPool_retain.
 */
void VuoGlPool_retainF(GLuint glBufferName, const char *file, unsigned int linenumber, const char *func)
{
	if (glBufferName == 0)
		return;

	dispatch_semaphore_wait(VuoGlPool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlPoolReferenceCounts::iterator it = VuoGlPool_referenceCounts.find(glBufferName);
	if (it == VuoGlPool_referenceCounts.end())
		VuoGlPool_referenceCounts[glBufferName] = 1;
	else
		++VuoGlPool_referenceCounts[glBufferName];

	dispatch_semaphore_signal(VuoGlPool_referenceCountsSemaphore);
//	VuoLog(VuoLog_moduleName, file, linenumber, func, "VuoGlPool_retain(%d)", glBufferName);
}

/**
 * Helper for @ref VuoGlPool_release.
 */
void VuoGlPool_releaseF(VuoGlPoolType type, unsigned long size, GLuint glBufferName, const char *file, unsigned int linenumber, const char *func)
{
	if (glBufferName == 0)
		return;

	dispatch_semaphore_wait(VuoGlPool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlPoolReferenceCounts::iterator it = VuoGlPool_referenceCounts.find(glBufferName);
	if (it == VuoGlPool_referenceCounts.end())
		VuoLog(VuoLog_moduleName, file, linenumber, func, "Error: VuoGlPool_release() was called with OpenGL Buffer Object %d, which was never retained.", glBufferName);
	else
	{
		if (--VuoGlPool_referenceCounts[glBufferName] == 0)
			VuoGlPool_disuse(type, size, glBufferName);
	}

	dispatch_semaphore_signal(VuoGlPool_referenceCountsSemaphore);
//	VuoLog(VuoLog_moduleName, file, linenumber, func, "VuoGlPool_release(%d)", glBufferName);
}





/**
 * An entry in the GL Texture pool.
 */
class VuoGlTextureDescriptor
{
public:
	GLenum target;          ///< The texture target, e.g., GL_TEXTURE_2D.
	GLenum internalFormat;  ///< The texture internalformat, e.g., GL_RGBA
	unsigned short width;   ///< The texture width, in pixels.
	unsigned short height;  ///< The texture height, in pixels.

	/**
	 * Creates a texture descriptor.
	 */
	VuoGlTextureDescriptor(GLenum target, GLenum internalFormat, unsigned short width, unsigned short height)
		: target(target),
		  internalFormat(internalFormat),
		  width(width),
		  height(height)
	{
	}

	/**
	 * Returns true if `a` is less than `b`.
	 */
	bool operator<(const VuoGlTextureDescriptor &that) const
	{
		VuoType_returnInequality(VuoInteger, target,         that.target);
		VuoType_returnInequality(VuoInteger, internalFormat, that.internalFormat);
		VuoType_returnInequality(VuoInteger, width,          that.width);
		VuoType_returnInequality(VuoInteger, height,         that.height);
		return false;
	}

	/**
	 * Returns a summary of the texture descriptor.
	 */
	string toString() const
	{
		char *z;
		asprintf(&z, "%-24s %-21s %5dx%-5d", VuoGl_stringForConstant(target), VuoGl_stringForConstant(internalFormat), width, height);
		string s(z);
		free(z);
		return s;
	}
};
typedef pair<queue<GLuint>,double> VuoGlTextureLastUsed;	///< A queue of textures of a given format and size, including the last time any of the textures were used.
typedef map<VuoGlTextureDescriptor, VuoGlTextureLastUsed> VuoGlTexturePoolType;  ///< Type for VuoGlTexturePool.
static VuoGlTexturePoolType *VuoGlTexturePool;	///< A pool of GL Textures.
static dispatch_semaphore_t VuoGlTexturePool_semaphore;	///< Serializes access to VuoGlTexturePool.


/**
 * Returns the OpenGL texture data type corresponding with OpenGL texture `format`.
 */
GLuint VuoGlTexture_getType(GLuint format)
{
	if (format == GL_YCBCR_422_APPLE)
		return GL_UNSIGNED_SHORT_8_8_APPLE;
	else if (format == GL_RGB
		|| format == GL_LUMINANCE
		|| format == GL_LUMINANCE_ALPHA
		|| format == GL_BGR_EXT)
		return GL_UNSIGNED_BYTE;
	else if (format == GL_RGBA
		  || format == GL_RGBA8
		  || format == GL_BGRA)
		return GL_UNSIGNED_INT_8_8_8_8_REV;
	else if (format == GL_DEPTH_COMPONENT)
		// Don't use GL_UNSIGNED_SHORT or GL_UNSIGNED_INT; they cause glBlitFramebufferEXT to output garbage on some GPUs.
		// https://b33p.net/kosada/node/11305
		return GL_UNSIGNED_BYTE;

	char *formatString = VuoGl_stringForConstant(format);
	VUserLog("Unknown format %s", formatString);
	free(formatString);
	VuoLog_backtrace();
	return 0;
}

/**
 * Returns the number of color+alpha channels in the specified OpenGL texture format.
 */
unsigned char VuoGlTexture_getChannelCount(GLuint format)
{
	if (format == GL_LUMINANCE
	 || format == GL_LUMINANCE8
	 || format == GL_LUMINANCE16F_ARB
	 || format == GL_LUMINANCE32F_ARB
	 || format == GL_DEPTH_COMPONENT)
		return 1;
	else if (format == GL_YCBCR_422_APPLE
		  || format == GL_LUMINANCE_ALPHA
		  || format == GL_LUMINANCE_ALPHA16F_ARB
		  || format == GL_LUMINANCE_ALPHA32F_ARB)
		return 2;
	else if (format == GL_RGB
		  || format == GL_RGB16F_ARB
		  || format == GL_RGB32F_ARB
		  || format == GL_BGR_EXT)
		return 3;
	else if (format == GL_RGBA
		  || format == GL_RGBA8
		  || format == GL_RGBA16F_ARB
		  || format == GL_RGBA32F_ARB
		  || format == GL_BGRA)
		return 4;

	char *formatString = VuoGl_stringForConstant(format);
	VUserLog("Unknown format %s", formatString);
	free(formatString);
	VuoLog_backtrace();
	return 1;
}

/**
 * Returns the number of bytes required to store each pixel of the specified OpenGL texture format.
 */
unsigned char VuoGlTexture_getBytesPerPixel(GLuint internalformat, GLuint format)
{
	if (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		return 1;
	if (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	 || internalformat == GL_COMPRESSED_RED_RGTC1)
		return 1;  // Actually 0.5

	unsigned char bytes = VuoGlTexture_getChannelCount(format);
	if (internalformat == GL_RGB
	 || internalformat == GL_RGBA
	 || internalformat == GL_RGBA8
	 || internalformat == GL_LUMINANCE8
	 || internalformat == GL_LUMINANCE8_ALPHA8)
		return bytes;
	else if (internalformat == GL_RGB16F_ARB
		  || internalformat == GL_RGBA16F_ARB
		  || internalformat == GL_DEPTH_COMPONENT
		  || internalformat == GL_LUMINANCE16F_ARB
		  || internalformat == GL_LUMINANCE_ALPHA16F_ARB)
		return bytes * 2;
	else if (internalformat == GL_RGB32F_ARB
		  || internalformat == GL_RGBA32F_ARB
		  || internalformat == GL_LUMINANCE32F_ARB
		  || internalformat == GL_LUMINANCE_ALPHA32F_ARB)
		return bytes * 4;

	VUserLog("Unknown internalformat %s", VuoGl_stringForConstant(internalformat));
	VuoLog_backtrace();
	return 0;
}

/**
 * Returns the number of bytes required to store each pixel of the specified OpenGL texture format.
 * @version200New
 */
unsigned char VuoGlTexture_getBytesPerPixelForInternalFormat(GLuint internalformat)
{
	if (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	 || internalformat == GL_COMPRESSED_RED_RGTC1)
		return 1;  // Actually 0.5
	if (internalformat == GL_LUMINANCE8
	 || internalformat == GL_LUMINANCE
	 || internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		return 1;
	if (internalformat == GL_LUMINANCE8_ALPHA8
	 || internalformat == GL_LUMINANCE16F_ARB
	 || internalformat == GL_DEPTH_COMPONENT)
		return 2;
	if (internalformat == GL_RGB
	 || internalformat == GL_BGR)
		return 3;
	if (internalformat == GL_RGBA
	 || internalformat == GL_RGBA8
	 || internalformat == GL_BGRA
	 || internalformat == GL_LUMINANCE_ALPHA16F_ARB
	 || internalformat == GL_LUMINANCE32F_ARB)
		return 4;
	if (internalformat == GL_RGB16F_ARB)
		return 6;
	if (internalformat == GL_RGBA16F_ARB
	 || internalformat == GL_LUMINANCE_ALPHA32F_ARB)
		return 8;
	if (internalformat == GL_RGB32F_ARB)
		return 12;
	if (internalformat == GL_RGBA32F_ARB)
		return 16;

	char *formatString = VuoGl_stringForConstant(internalformat);
	VUserLog("Unknown internalformat %s", formatString);
	free(formatString);
	VuoLog_backtrace();
	return 0;
}

/**
 * Returns true if the specified OpenGL texture format has an alpha channel.
 * @version200New
 */
bool VuoGlTexture_formatHasAlphaChannel(GLuint format)
{
	if (format == GL_BGRA
	 || format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
	 || format == GL_LUMINANCE8_ALPHA8
	 || format == GL_LUMINANCE_ALPHA
	 || format == GL_LUMINANCE_ALPHA16F_ARB
	 || format == GL_LUMINANCE_ALPHA32F_ARB
	 || format == GL_RGBA
	 || format == GL_RGBA16F_ARB
	 || format == GL_RGBA32F_ARB
	 || format == GL_RGBA8)
		return true;

	return false;
}

/**
 * Returns the maximum number of bytes in Video RAM that a texture can occupy.
 */
unsigned long VuoGlTexture_getMaximumTextureBytes(VuoGlContext glContext)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	static unsigned long maximumTextureBytes = 0;
	static dispatch_once_t maximumTextureBytesQuery = 0;
	dispatch_once(&maximumTextureBytesQuery, ^{
					  GLint contextRendererID;
					  CGLGetParameter(cgl_ctx, kCGLCPCurrentRendererID, &contextRendererID);

					  // https://b33p.net/kosada/node/12177
					  // https://developer.apple.com/library/mac/qa/qa1168/_index.html says:
					  // "If you are looking for the VRAM sizes of all the renderers on your system […]
					  // you may specify a -1/0xFFFFFFFF display mask in the CGLQueryRendererInfo() function.
					  CGLRendererInfoObj ri;
					  GLint rendererCount = 0;
					  CGLQueryRendererInfo(-1, &ri, &rendererCount);
					  for (int i = 0; i < rendererCount; ++i)
					  {
						  GLint rendererID;
						  CGLDescribeRenderer(ri, i, kCGLRPRendererID, &rendererID);
						  if (rendererID == contextRendererID)
						  {
							  GLint textureMegabytes = 0;
							  if (CGLDescribeRenderer(ri, i, kCGLRPTextureMemoryMegabytes, &textureMegabytes) == kCGLNoError)
							  {
								  // In OS X, the GPU seems to often crash with individual textures that occupy more than a certain amount of memory.
								  // See https://b33p.net/kosada/node/10791
								  // See https://b33p.net/kosada/node/12030
								  double fudge = .9;
								  if ((rendererID & kCGLRendererIDMatchingMask) == kCGLRendererIntelHD4000ID)
									  // Reduce on Intel 4000, since on macOS 10.14, textures larger than this fail with GL_OUT_OF_MEMORY.
									  fudge = .74;

								  maximumTextureBytes = (textureMegabytes - 85) * 1048576UL * fudge;
								  VUserLog("%ld MB", maximumTextureBytes / 1024 / 1024);
								  break;
							  }
						  }
					  }
					  CGLDestroyRendererInfo(ri);
				  });
	return maximumTextureBytes;
#pragma clang diagnostic pop
}

/**
 * Returns the maximum dimension (in pixels) a texture can have.
 */
GLint VuoGlTexture_getMaximumTextureDimension(VuoGlContext glContext)
{
	static GLint maximumTextureDimension = 0;
	static dispatch_once_t maximumTextureDimensionQuery = 0;
	dispatch_once(&maximumTextureDimensionQuery, ^{
					  CGLContextObj cgl_ctx = (CGLContextObj)glContext;
					  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureDimension);
				  });
	return maximumTextureDimension;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
/**
 * Returns a summary of `allocation`.
 */
static const char *VuoGlTexturePool_stringForAllocation(VuoGlTexturePoolAllocation allocation)
{
	return allocation == VuoGlTexturePool_NoAllocation      ? "NoAlloc" :
		  (allocation == VuoGlTexturePool_Allocate          ? "Alloc" :
		   allocation == VuoGlTexturePool_AllocateIOSurface ? "AllocIOSurf" : "?");
}
#pragma clang diagnostic pop

/**
 * Returns an OpenGL texture.
 *
 * If an existing, unused texture matching the specified @c target, @c internalformat, @c width, and @c height is available, it is returned.
 * Otherwise, a new texture is created.
 *
 * The texturing properties are set to the defaults:
 *
 *    - wrapping: clamp to border
 *    - filtering: linear
 *
 * See [glTexImage2D](https://www.khronos.org/opengl/wiki/GLAPI/glTexImage2D) for information about @c internalformat and @c format.
 *
 * If `allocation` is `VuoGlTexturePool_Allocate`, the returned texture's storage is preallocated (so the caller can efficiently upload data using [glTexSubImage2D](https://www.khronos.org/opengl/wiki/GLAPI/glTexSubImage2D)).
 * If `VuoGlTexturePool_AllocateIOSurface`, the returned texture is backed by the specified `ioSurfaceRef`.
 * In any case, this function calls @ref VuoGlPool_logVRAMAllocated, so you doesn't need to.
 *
 * `ioSurfaceRef` should be NULL unless `allocation` is `VuoGlTexturePool_AllocateIOSurface`.
 *
 * Returns 0 if the texture would be too large to fit in Video RAM.
 *
 * @threadAnyGL
 * @version200Changed{Added `allocation`, `target`, `ioSurfaceRef` arguments.}
 */
GLuint VuoGlTexturePool_use(VuoGlContext glContext, VuoGlTexturePoolAllocation allocation, GLenum target, GLenum internalformat, unsigned short width, unsigned short height, GLenum format, void *ioSurfaceRef)
{
	if (allocation == VuoGlTexturePool_AllocateIOSurface
	 && target != GL_TEXTURE_RECTANGLE_ARB)
	{
		VUserLog("Error: To use VuoGlTexturePool_AllocateIOSurface, target must be GL_TEXTURE_RECTANGLE_ARB.");
		return 0;
	}

	unsigned char bytesPerPixel = VuoGlTexture_getBytesPerPixel(internalformat, format);
	unsigned long requiredBytes = width * height * bytesPerPixel;
	VuoGlTextureDescriptor descriptor(target, internalformat, width, height);
//	VLog("    %-11s %s: Requested",VuoGlTexturePool_stringForAllocation(allocation), descriptor.toString().c_str());

	GLuint name = 0;
	if (allocation != VuoGlTexturePool_AllocateIOSurface)
	{
		dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
		auto it = VuoGlTexturePool->find(descriptor);
		if (it != VuoGlTexturePool->end() && it->second.first.size())
		{
			name = it->second.first.front();
			it->second.first.pop();
//			if (name) VLog("                %s: Used recycled texture %d", descriptor.toString().c_str(), name);
			it->second.second = VuoLogGetTime();
		}
		dispatch_semaphore_signal(VuoGlTexturePool_semaphore);
	}


	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	if (name == 0)
	{
		// Check texture size against available Texture Video RAM (to avoid GPU crashes / kernel panics).
		unsigned long maximumTextureBytes = VuoGlTexture_getMaximumTextureBytes(glContext);
		if (maximumTextureBytes > 0)
		{
			if (requiredBytes > maximumTextureBytes)
			{
				/// @todo Better error handling per https://b33p.net/kosada/node/4724
				VUserLog("Not enough graphics memory for a %dx%d (%d bytes/pixel) texture.  Requires %lu MB, have %lu MB.", width, height, bytesPerPixel, requiredBytes/1024/1024, maximumTextureBytes/1024/1024);
				return 0;
			}
		}

		// Check texture size against the GPU's allowed texture size.
		GLint maxDimension = VuoGlTexture_getMaximumTextureDimension(glContext);
		if (width > maxDimension || height > maxDimension)
		{
			/// @todo Better error handling per https://b33p.net/kosada/node/4724
			VUserLog("This GPU doesn't support textures of size %dx%d (GPU's limit is %dx%d).", width, height, maxDimension, maxDimension);
			return 0;
		}

		glGenTextures(1, &name);
//		VLog("    %-11s %s: Generated new texture %d", VuoGlTexturePool_stringForAllocation(allocation), descriptor.toString().c_str(), name);
		glBindTexture(target, name);
		if (allocation == VuoGlTexturePool_Allocate)
		{
			glTexImage2D(target, 0, internalformat, width, height, 0, format, VuoGlTexture_getType(format), NULL);
//			VLog("glTexImage2D(%s, 0, %s, %d, %d, 0, %s, %s, NULL); -> %d", VuoGl_stringForConstant(target), VuoGl_stringForConstant(internalformat), width, height, VuoGl_stringForConstant(format), VuoGl_stringForConstant(VuoGlTexture_getType(format)), name);
		}
		else if (allocation == VuoGlTexturePool_AllocateIOSurface)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, internalformat, width, height, format, VuoGlTexture_getType(format), (IOSurfaceRef)ioSurfaceRef, 0);
			if (err != kCGLNoError)
			{
				char *internalFormatString = VuoGl_stringForConstant(internalformat);
				char *formatString = VuoGl_stringForConstant(format);
				char *typeString = VuoGl_stringForConstant(VuoGlTexture_getType(format));
				VUserLog("Error in CGLTexImageIOSurface2D(context, GL_TEXTURE_RECTANGLE_ARB, %s, %hu, %hu, %s, %s, iosurface, 0): %s",
						 internalFormatString, width, height, formatString, typeString, CGLErrorString(err));
				free(internalFormatString);
				free(formatString);
				free(typeString);
				VGL();
				return false;
			}
#pragma clang diagnostic pop
		}
		VuoGlPool_logVRAMAllocated(requiredBytes);
	}
	else
		glBindTexture(target, name);


	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(target, 0);

	return name;
}

/**
 * Indicates that the caller is done using the OpenGL texture @c name.
 *
 * The texture is returned to the pool, so other callers can use it
 * (which is more efficient than deleting and re-generating textures).
 *
 * Typically you should use @ref VuoGlTexture_retain and @ref VuoGlTexture_retain instead.
 *
 * @threadAny
 */
void VuoGlTexturePool_disuse(VuoGlTexturePoolAllocation allocation, GLenum target, GLenum internalformat, unsigned short width, unsigned short height, GLuint name)
{
	if (internalformat == 0)
	{
		VUserLog("Error:  Can't recycle texture %d since we don't know its internalformat.  Deleting.", name);
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glDeleteTextures(1, &name);
		});
		VuoGlPool_logVRAMFreed(width * height * 1 /* unknown BPP */);
		return;
	}

	if (allocation == VuoGlTexturePool_AllocateIOSurface)
	{
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glDeleteTextures(1, &name);
		});
//		VuoGlTextureDescriptor descriptor(target, internalformat, width, height);
//		VLog("                %s: Deleted texture %d (can't recycle IOSurface backing textures)", descriptor.toString().c_str(), name);
	}
	else
	{
		VuoGlTextureDescriptor descriptor(target, internalformat, width, height);
		dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
		{
			auto it = &(*VuoGlTexturePool)[descriptor];
			it->first.push(name);
			it->second = VuoLogGetTime();
		}
		dispatch_semaphore_signal(VuoGlTexturePool_semaphore);
//		VLog("                %s: Recycled texture %d", descriptor.toString().c_str(), name);
	}
}





/**
 * Reference-counting information for an OpenGL texture.
 */
typedef struct
{
	unsigned int referenceCount;
	VuoImage_freeCallback freeCallback;
	void *freeCallbackContext;
} VuoGlTexture;
typedef map<GLuint, VuoGlTexture> VuoGlTextureReferenceCounts;	///< The number of times each glTextureName is retained..
static VuoGlTextureReferenceCounts *VuoGlTexture_referenceCounts;  ///< The reference count for each OpenGL Texture Object.
static dispatch_semaphore_t VuoGlTexture_referenceCountsSemaphore = NULL;  ///< Synchronizes access to @c VuoGlTexture_referenceCounts.
/**
 * Initializes the texture reference counting system.
 */
static void __attribute__((constructor(101))) VuoGlTexture_init(void)
{
	VuoGlTexture_referenceCounts = new VuoGlTextureReferenceCounts;
	VuoGlTexture_referenceCountsSemaphore = dispatch_semaphore_create(1);
}

/**
 * Increments the reference count for @c glTextureName.
 *
 * @threadAny
 */
void VuoGlTexture_retain(GLuint glTextureName, VuoImage_freeCallback freeCallback, void *freeCallbackContext)
{
//	VLog("%d", glTextureName);
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts->find(glTextureName);
	if (it == VuoGlTexture_referenceCounts->end())
		(*VuoGlTexture_referenceCounts)[glTextureName] = (VuoGlTexture){1, freeCallback, freeCallbackContext};
	else
		++(*it).second.referenceCount;

	dispatch_semaphore_signal(VuoGlTexture_referenceCountsSemaphore);
}

/**
 * Decrements the reference count for @c glTextureName.
 *
 * @threadAny
 * @version200Changed{Added `allocation` argument; reordered `glTextureTarget` argument.}
 */
void VuoGlTexture_release(VuoGlTexturePoolAllocation allocation, GLuint glTextureTarget, GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName)
{
//	VLog("    %-11s %d (%s %s %dx%d)", VuoGlTexturePool_stringForAllocation(allocation), glTextureName, VuoGl_stringForConstant(glTextureTarget), VuoGl_stringForConstant(internalformat), width, height);
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts->find(glTextureName);
	if (it == VuoGlTexture_referenceCounts->end())
		VUserLog("Error: VuoGlTexture_release() was called with OpenGL Texture Object %d, which was never retained.", glTextureName);
	else
	{
		VuoGlTexture t = (*VuoGlTexture_referenceCounts)[glTextureName];
		if (--t.referenceCount == 0)
		{
			if (t.freeCallback)
			{
				// Client-owned texture
				struct _VuoImage i = (struct _VuoImage){glTextureName, internalformat, glTextureTarget, width, height, 1, t.freeCallbackContext, NULL, NULL, NULL};
				t.freeCallback(&i);
			}
			else
			{
				// Vuo-owned texture
				VuoGlTexturePool_disuse(allocation, glTextureTarget, internalformat, width, height, glTextureName);
			}
			VuoGlTexture_referenceCounts->erase(it);
		}
		else
			(*VuoGlTexture_referenceCounts)[glTextureName] = t;
	}

	dispatch_semaphore_signal(VuoGlTexture_referenceCountsSemaphore);
}

/**
 * Removes `glTextureName` from Vuo's reference count table (without deleting it like @ref VuoGlTexture_release does).
 *
 * `glTextureName` must have a reference count of exactly 1 (i.e., a texture being used in multiple places throughout Vuo cannot be disowned).
 *
 * After Vuo disowns the texture, the caller is responsible for eventually deleting it.
 *
 * @threadAny
 */
void VuoGlTexture_disown(GLuint glTextureName)
{
//	VLog("%d", glTextureName);
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts->find(glTextureName);
	if (it == VuoGlTexture_referenceCounts->end())
		VUserLog("Error: VuoGlTexture_disown() was called with OpenGL Texture Object %d, which was never retained.", glTextureName);
	else
	{
		VuoGlTexture t = (*VuoGlTexture_referenceCounts)[glTextureName];
		if (t.referenceCount != 1)
			VUserLog("Error: VuoGlTexture_disown() was called with OpenGL Texture Object %d, which has a reference count of %d (but it should be 1).", glTextureName, t.referenceCount);
		else
			VuoGlTexture_referenceCounts->erase(it);
	}

	dispatch_semaphore_signal(VuoGlTexture_referenceCountsSemaphore);
}



/**
 * An entry in the IOSurface pool.
 */
typedef struct
{
	IOSurfaceRef ioSurface;
	GLuint texture;
	unsigned short pixelsWide;
	unsigned short pixelsHigh;
	double lastUsedTime;
} VuoIoSurfacePoolEntryType;
typedef pair<unsigned short,unsigned short> VuoGlTextureDimensionsType;  ///< Texture width and height.
typedef map<VuoGlTextureDimensionsType, deque<VuoIoSurfacePoolEntryType> > VuoIoSurfacePoolType;	///< VuoIoSurfacePoolType[size] gives a list of IOSurfaces.
static VuoIoSurfacePoolType *VuoIoSurfacePool;	///< Unused IOSurfaces.
static VuoIoSurfacePoolType *VuoIoSurfaceQuarantine;	///< IOSurfaces which might still be in use by the receiver.
static dispatch_semaphore_t VuoIoSurfacePool_semaphore;	///< Serializes access to the IOSurface pool.
static CFStringRef receiverFinishedWithIoSurfaceKey = CFSTR("VuoReceiverFinished");	///< Signals from the receiver to the sender, when the receiver is finished using the IOSurface.

const double VuoGlPool_cleanupInterval = 0.1;  ///< Interval (in seconds) to flush the texture and IOSurface pools.
static dispatch_source_t VuoGlPool_timer;	///< Periodically cleans up the IOSurface pool.
static dispatch_semaphore_t VuoGlPool_canceledAndCompleted;	///< Signals when the final cleanup has completed.

static unsigned long VuoGlPool_allocatedBytes    = 0;  ///< The approximate current amount of VRAM allocated by this process.  @see VuoGlPool_logVRAMAllocation
static unsigned long VuoGlPool_allocatedBytesMax = 0;  ///< The approximate maximum amount of VRAM allocated by this process.  @see VuoGlPool_logVRAMAllocation

/**
 * Updates the process-wide VRAM count to include `bytesAllocated`.
 *
 * This function is automatically called by other VuoGlPool functions,
 * so you should only call this if you're directly making OpenGL calls that allocate VRAM.
 *
 * @threadAny
 * @version200New
 */
void VuoGlPool_logVRAMAllocated(unsigned long bytesAllocated)
{
//	VLog("Alloc %lu", bytesAllocated);
	__sync_add_and_fetch(&VuoGlPool_allocatedBytes, bytesAllocated);

	// Not thread-safe, but the worst that could happen is that,
	// if multiple threads exceed the limit simultaneously,
	// we might not get the largest of the new maximums.
	if (VuoGlPool_allocatedBytes > VuoGlPool_allocatedBytesMax)
		VuoGlPool_allocatedBytesMax = VuoGlPool_allocatedBytes;
}

/**
 * Updates the process-wide VRAM count to no longer include `bytesAllocated`.
 *
 * This function is automatically called by other VuoGlPool functions,
 * so you should only call this if you're directly making OpenGL calls that deallocate VRAM.
 *
 * @threadAny
 * @version200New
 */
void VuoGlPool_logVRAMFreed(unsigned long bytesFreed)
{
//	VLog("Freed %lu", bytesFreed);
	__sync_sub_and_fetch(&VuoGlPool_allocatedBytes, bytesFreed);
}

/**
 * Periodically cleans up the texture and IOSurface pools.
 */
static void VuoGlPool_cleanup(void *blah)
{
//	VLog("{");
	unsigned long totalTextureCount = 0;
	static unsigned long totalTextureCountMax = 0;

	vector<GLuint> texturesToDelete;
	vector<VuoIoSurfacePoolEntryType> iosurfaceTexturesToDisuse;

	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoLogGetTime();
		for (VuoGlTexturePoolType::iterator poolItem = VuoGlTexturePool->begin(); poolItem != VuoGlTexturePool->end(); )
		{
			unsigned long textureCount = poolItem->second.first.size();
			double timeSinceLastUsed = now - poolItem->second.second;
//			VLog("    %s: %lu unused texture%s (last used %.02gs ago)", poolItem->first.toString().c_str(), textureCount, textureCount == 1 ? "" : "s", timeSinceLastUsed);
			if (timeSinceLastUsed > VuoGlPool_cleanupInterval)
			{
				if (textureCount)
				{
//					VLog("                                                                Purging %lu expired texture%s", textureCount, textureCount == 1 ? "" : "s");
					while (!poolItem->second.first.empty())
					{
						GLuint textureName = poolItem->second.first.front();
						poolItem->second.first.pop();
						texturesToDelete.push_back(textureName);
						VuoGlPool_logVRAMFreed(poolItem->first.width * poolItem->first.height * VuoGlTexture_getBytesPerPixelForInternalFormat(poolItem->first.internalFormat));
					}
				}

				VuoGlTexturePool->erase(poolItem++);
			}
			else
			{
				totalTextureCount += textureCount;
				++poolItem;
			}
		}
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);

	if (totalTextureCount > totalTextureCountMax)
		totalTextureCountMax = totalTextureCount;


	unsigned long totalIOSurfaceCount = 0;
	static unsigned long totalIOSurfaceCountMax = 0;

	dispatch_semaphore_wait(VuoIoSurfacePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		// Promote IOSurfaces from the quarantine to the pool, if the receiver has finished using them.
		for (VuoIoSurfacePoolType::iterator quarantinedQueue = VuoIoSurfaceQuarantine->begin(); quarantinedQueue != VuoIoSurfaceQuarantine->end(); ++quarantinedQueue)
		{
			for (deque<VuoIoSurfacePoolEntryType>::iterator quarantinedIoSurfaceEntry = quarantinedQueue->second.begin(); quarantinedIoSurfaceEntry != quarantinedQueue->second.end();)
			{
				VuoIoSurfacePoolEntryType e = *quarantinedIoSurfaceEntry;
				CFBooleanRef finished = (CFBooleanRef)IOSurfaceCopyValue(e.ioSurface, receiverFinishedWithIoSurfaceKey);
				if (finished)
				{
					IOSurfaceRemoveValue(e.ioSurface, receiverFinishedWithIoSurfaceKey);

					(*VuoIoSurfacePool)[quarantinedQueue->first].push_back(e);
					quarantinedIoSurfaceEntry = quarantinedQueue->second.erase(quarantinedIoSurfaceEntry);

//					VLog("    Promoted IOSurface %d + GL Texture %d (%dx%d) from quarantine to pool", IOSurfaceGetID(e.ioSurface), e.texture, quarantinedQueue->first.first, quarantinedQueue->first.second);
				}
				else
				{
					++totalIOSurfaceCount;
					++quarantinedIoSurfaceEntry;
				}
			}
		}


		double now = VuoLogGetTime();
		for (VuoIoSurfacePoolType::iterator poolQueue = VuoIoSurfacePool->begin(); poolQueue != VuoIoSurfacePool->end(); ++poolQueue)
		{
			for (deque<VuoIoSurfacePoolEntryType>::iterator poolIoSurfaceEntry = poolQueue->second.begin(); poolIoSurfaceEntry != poolQueue->second.end();)
			{
				VuoIoSurfacePoolEntryType e = *poolIoSurfaceEntry;
				if (now - e.lastUsedTime > VuoGlPool_cleanupInterval * 2.)
				{
//					VLog("    Purging expired IOSurface %d + GL Texture %d (%dx%d) — it's %gs old", IOSurfaceGetID(e.ioSurface), e.texture, poolQueue->first.first, poolQueue->first.second, now - e.lastUsedTime);

					CFRelease(e.ioSurface);
					iosurfaceTexturesToDisuse.push_back(e);
					poolIoSurfaceEntry = poolQueue->second.erase(poolIoSurfaceEntry);
				}
				else
				{
					++totalIOSurfaceCount;
					++poolIoSurfaceEntry;
				}
			}
		}
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);

	if (totalIOSurfaceCount > totalIOSurfaceCountMax)
		totalIOSurfaceCountMax = totalIOSurfaceCount;


	// Delete textures after we've released the pool semaphores, to avoid deadlock, and to avoid hogging the shared GL context.
	if (texturesToDelete.size())
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			for (vector<GLuint>::const_iterator i = texturesToDelete.begin(); i != texturesToDelete.end(); ++i)
			{
				GLuint t = *i;
				glDeleteTextures(1, &t);
				// VuoGlPool_logVRAMFreed was called above when texturesToDelete was being built.
			}
		});
	for (auto e : iosurfaceTexturesToDisuse)
		VuoGlTexturePool_disuse(VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, e.pixelsWide, e.pixelsHigh, e.texture);
//	VLog("}");


	// Log VRAM use every 10 seconds.
	static unsigned long cleanupCount = 0;
	if ((++cleanupCount % (long)(10. / VuoGlPool_cleanupInterval) == 0)
		&& VuoIsDebugEnabled()
		&& (VuoGlPool_allocatedBytes > 0 || VuoGlPool_allocatedBytesMax > 0
			|| totalTextureCount > 0 || totalTextureCountMax > 0
			|| totalIOSurfaceCount > 0 || totalIOSurfaceCountMax > 0))
	{
		__block unsigned long maximumTextureBytes;
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			maximumTextureBytes = VuoGlTexture_getMaximumTextureBytes(cgl_ctx);
		});

		if (maximumTextureBytes > 0)
			VUserLog("VRAM used: %5lu MB (%5lu MB max, %3lu%%).  Textures in pool: %3lu (%3lu max).  IOSurfaces in pool: %3lu (%3lu max).",
					 VuoGlPool_allocatedBytes/1024/1024,
					 VuoGlPool_allocatedBytesMax/1024/1024,
					 VuoGlPool_allocatedBytesMax*100/maximumTextureBytes,
					 totalTextureCount, totalTextureCountMax,
					 totalIOSurfaceCount, totalIOSurfaceCountMax);
		else
			VUserLog("VRAM used: %5lu MB (%5lu MB max).  Textures in pool: %3lu (%3lu max).  IOSurfaces in pool: %3lu (%3lu max).",
					 VuoGlPool_allocatedBytes/1024/1024,
					 VuoGlPool_allocatedBytesMax/1024/1024,
					 totalTextureCount, totalTextureCountMax,
					 totalIOSurfaceCount, totalIOSurfaceCountMax);
	}
}
static void __attribute__((constructor)) VuoGlPool_init(void)
{
	VuoGlPool_semaphore = dispatch_semaphore_create(1);

	VuoGlTexturePool_semaphore = dispatch_semaphore_create(1);
	VuoGlTexturePool = new VuoGlTexturePoolType;

	VuoIoSurfacePool_semaphore = dispatch_semaphore_create(1);
	VuoIoSurfacePool = new VuoIoSurfacePoolType;
	VuoIoSurfaceQuarantine = new VuoIoSurfacePoolType;

	VuoGlPool_canceledAndCompleted = dispatch_semaphore_create(0);
	VuoGlPool_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(VuoGlPool_timer, dispatch_walltime(NULL,0), NSEC_PER_SEC * VuoGlPool_cleanupInterval, NSEC_PER_SEC * VuoGlPool_cleanupInterval);
	dispatch_source_set_event_handler_f(VuoGlPool_timer, VuoGlPool_cleanup);
	dispatch_source_set_cancel_handler(VuoGlPool_timer, ^{
										   dispatch_semaphore_signal(VuoGlPool_canceledAndCompleted);
									   });
	dispatch_resume(VuoGlPool_timer);
}
/**
 * Stops the cleanup timer, and deletes the pools.
 */
static void __attribute__((destructor)) VuoGlPool_fini(void)
{
	dispatch_source_cancel(VuoGlPool_timer);

	// Wait for the last cleanup to complete.
	dispatch_semaphore_wait(VuoGlPool_canceledAndCompleted, DISPATCH_TIME_FOREVER);

	delete VuoGlTexturePool;

	delete VuoIoSurfacePool;
	delete VuoIoSurfaceQuarantine;
}

/**
 * Returns an IOSurface (backed by @c outputTexture) with the specified dimensions.
 * Uses an IOSurface from the pool, if possible.  If not, creates a new IOSurface.
 */
VuoIoSurface VuoIoSurfacePool_use(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, GLuint *outputTexture)
{
	VuoIoSurface ioSurface = NULL;
	VuoGlTextureDimensionsType dimensions(pixelsWide,pixelsHigh);

	dispatch_semaphore_wait(VuoIoSurfacePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if ((*VuoIoSurfacePool)[dimensions].size())
		{
			VuoIoSurfacePoolEntryType e = (*VuoIoSurfacePool)[dimensions].front();
			ioSurface = malloc(sizeof(VuoIoSurfacePoolEntryType));
			memcpy(ioSurface, &e, sizeof(VuoIoSurfacePoolEntryType));
			*outputTexture = e.texture;
			(*VuoIoSurfacePool)[dimensions].pop_front();
//			VLog("    Using recycled IOSurface %d + GL Texture %d (%dx%d) from pool", IOSurfaceGetID(e.ioSurface), *outputTexture, pixelsWide, pixelsHigh);
		}
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);

	if (!ioSurface)
	{
		CFMutableDictionaryRef properties = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		/// @todo kIOSurfaceIsGlobal is deprecated on 10.11; replace int32 lookup with IOSurfaceCreateXPCObject or something.
		/// https://web.archive.org/web/20151220161520/https://lists.apple.com/archives/mac-opengl/2009/Sep/msg00110.html
		CFDictionaryAddValue(properties, kIOSurfaceIsGlobal, kCFBooleanTrue);
#pragma clang diagnostic pop

		long long pixelsWideLL = pixelsWide;
		CFNumberRef pixelsWideCF = CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsWideLL);
		CFDictionaryAddValue(properties, kIOSurfaceWidth, pixelsWideCF);

		long long pixelsHighLL = pixelsHigh;
		CFNumberRef pixelsHighCF = CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsHighLL);
		CFDictionaryAddValue(properties, kIOSurfaceHeight, pixelsHighCF);

		long long bytesPerElement = 4;
		CFNumberRef bytesPerElementCF = CFNumberCreate(NULL, kCFNumberLongLongType, &bytesPerElement);
		CFDictionaryAddValue(properties, kIOSurfaceBytesPerElement, bytesPerElementCF);

		uint32_t pixelFormat = 'BGRA';  // kCVPixelFormatType_32BGRA;
		CFNumberRef pixelFormatCF = CFNumberCreate(NULL, kCFNumberSInt32Type, &pixelFormat);
		CFDictionaryAddValue(properties, kIOSurfacePixelFormat, pixelFormatCF);

		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)malloc(sizeof(VuoIoSurfacePoolEntryType));
		ioSurface = e;
		e->ioSurface = IOSurfaceCreate(properties);
		e->pixelsWide = pixelsWide;
		e->pixelsHigh = pixelsHigh;
		e->lastUsedTime = -1;
		CFRelease(pixelsWideCF);
		CFRelease(pixelsHighCF);
		CFRelease(bytesPerElementCF);
		CFRelease(properties);

		*outputTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_AllocateIOSurface, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, pixelsWide, pixelsHigh, GL_BGRA, e->ioSurface);
		e->texture = *outputTexture;

//		VLog("    Created IOSurface %d (%dx%d)", IOSurfaceGetID(e->ioSurface), pixelsWide, pixelsHigh);
	}

	return ioSurface;
}

/**
 * Returns the IOSurface's interprocess ID.
 */
uint32_t VuoIoSurfacePool_getId(VuoIoSurface vis)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
	return IOSurfaceGetID(e->ioSurface);
}

/**
 * Returns the IOSurface.
 */
void *VuoIoSurfacePool_getIOSurfaceRef(VuoIoSurface vis)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
	return e->ioSurface;
}

/**
 * Returns the IOSurface's width in pixels.
 */
unsigned short VuoIoSurfacePool_getWidth(VuoIoSurface vis)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
	return e->pixelsWide;
}

/**
 * Returns the IOSurface's height in pixels.
 */
unsigned short VuoIoSurfacePool_getHeight(VuoIoSurface vis)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
	return e->pixelsHigh;
}

/**
 * Returns the IOSurface's OpenGL texture name.
 */
GLuint VuoIoSurfacePool_getTexture(VuoIoSurface vis)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
	return e->texture;
}

/**
 * Called by the sending end of an IOSurface texture transfer to indicate that it is finished using the IOSurface's texture.
 * This enables the IOSurface to be reused for another texture transfer (once the receiving end indicates it is done with it, via VuoIoSurfacePool_signal()).
 *
 * `quarantine` should typically be true — this indicates that the IOSurface should be placed in the quarantine,
 * meaning that it will not be reused until the receiver signals it.
 * If you're sure both sides are immediately finished using the IOSurface (e.g., during texture transfer within a process),
 * you can set `quarantine` to false, which will allow reusing it sooner (saving some VRAM).
 * You still need to call @ref VuoIoSurfacePool_signal though (since the IOSurface might be recycled and later require quarantine).
 *
 * @version200Changed{Added `quarantine` argument.}
 */
void VuoIoSurfacePool_disuse(VuoIoSurface vis, bool quarantine)
{
	VuoIoSurfacePoolEntryType *e = (VuoIoSurfacePoolEntryType *)vis;
//	VLog("    Sender disusing IOSurface %d + GL Texture %d (%dx%d)", IOSurfaceGetID(e->ioSurface), e->texture, e->pixelsWide, e->pixelsHigh);
	VuoGlTextureDimensionsType dimensions(e->pixelsWide, e->pixelsHigh);
	e->lastUsedTime = VuoLogGetTime();

	dispatch_semaphore_wait(VuoIoSurfacePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (quarantine)
			(*VuoIoSurfaceQuarantine)[dimensions].push_back(*e);
		else
			(*VuoIoSurfacePool)[dimensions].push_back(*e);
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);

	free(e);
}

/**
 * Called by the receiving end of an IOSurface texture transfer to indicate that it is finished using the IOSurface's texture.
 * This enables the sender to reuse the IOSurface for another texture transfer.
 */
void VuoIoSurfacePool_signal(void *ios)
{
	IOSurfaceRef ioSurface = (IOSurfaceRef)ios;
	IOSurfaceSetValue(ioSurface, receiverFinishedWithIoSurfaceKey, kCFBooleanTrue);
}



/**
 * Parses GLSL debug information into `outIssues`.
 *
 * @threadAnyGL
 */
void VuoGlShader_parseShaderInfoLog(CGLContextObj cgl_ctx, GLenum type, GLuint obj, const GLchar *source, VuoShaderFile::Stage stage, VuoShaderIssues *outIssues)
{
	int infologLength = 0;
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
	if (infologLength > 0)
	{
		char *infoLog = (char *)malloc(infologLength);
		int charsWritten = 0;
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);

		if (outIssues)
		{
			istringstream iss(infoLog);
			string ln;
			while (getline(iss, ln))
			{
				int lineNumber = 0;
				char *message = (char *)malloc(ln.length() + 1);
				char *quotation = (char *)malloc(ln.length() + 1);
				if (sscanf(ln.c_str(), "ERROR: 0:%d: '' :   %[^\n]", &lineNumber, message) == 2)
					outIssues->addIssue(stage, lineNumber, message);
				else if (sscanf(ln.c_str(), "ERROR: 0:%d: '%[^']' : syntax error: %[^\n]", &lineNumber, quotation, message) == 3)
					outIssues->addIssue(stage, lineNumber, message + string(": '") + quotation + "'");
				else if (sscanf(ln.c_str(), "ERROR: 0:%d: %[^\n]", &lineNumber, message) == 2)
					outIssues->addIssue(stage, lineNumber, message);
				else if (sscanf(ln.c_str(), "WARNING: 0:%d: %[^\n]", &lineNumber, message) == 2)
					outIssues->addIssue(stage, lineNumber, message);
				else
				{
					VUserLog("Warning: Couldn't parse GLSL log message: \"%s\"", ln.c_str());
					outIssues->addIssue(stage, VuoShaderIssues::NoLine, ln);
				}
				free(message);
				free(quotation);
			}
		}
		else
		{
			VUserLog("%s", infoLog);
			VDebugLog("Source code:\n%s", source);
		}

		free(infoLog);
	}
}

map<GLenum, map<long, GLuint> > VuoGlShaderPool __attribute__((init_priority(101)));	///< Shaders, keyed by type (vertex, fragment, ...) and source code hash.
dispatch_semaphore_t VuoGlShaderPool_semaphore = NULL;  ///< Synchronizes access to @c VuoGlShaderPool.

/**
 * Initializes @c VuoGlShaderPool_semaphore.
 */
__attribute__((constructor)) static void VuoGlShaderPool_init(void)
{
	VuoGlShaderPool_semaphore = dispatch_semaphore_create(1);
}

/**
 * Replaces @c \#include statements with their contents.
 *
 * Returns true if successful.
 */
static bool VuoGlShader_resolveIncludes(string &source, VuoShaderFile::Stage stage, VuoShaderIssues *outIssues)
{
	const char *cwd = VuoGetWorkingDirectory();
	const string vuoFrameworkPath = VuoFileUtilities::getVuoFrameworkPath();
	const string vuoRunnerFrameworkPath = VuoFileUtilities::getVuoRunnerFrameworkPath();
	string userModulesPath = VuoFileUtilities::getUserModulesPath();
	if (!VuoFileUtilities::dirExists(userModulesPath))
		userModulesPath = "";
	string systemModulesPath = VuoFileUtilities::getSystemModulesPath();
	if (!VuoFileUtilities::dirExists(systemModulesPath))
		systemModulesPath = "";

	set<string> includedFiles;

	while (1)
	{
		const string includeKeyword = "#include";

		size_t includeStart = source.find(includeKeyword);
		if (includeStart == string::npos)
			return true;

		size_t offset = includeStart + includeKeyword.length();

		while (source[offset] == ' ')
			++offset;

		if (source[offset] != '"'
		 && source[offset] != '<')
		{
			/// @todo calculate line number
			if (outIssues)
				outIssues->addIssue(stage, VuoShaderIssues::NoLine, "Syntax error in #include statement.  Expected syntax: #include \"filename.glsl\"");
			else
				VUserLog("Error: Syntax error in #include statement.  Expected syntax: #include \"filename.glsl\"");
			return false;
		}
		++offset;

		size_t filenameStart = offset;

		while (source[offset] != '"'
			&& source[offset] != '>')
			++offset;

		size_t filenameEnd = offset;

		string filename = source.substr(filenameStart, filenameEnd - filenameStart);

		vector<string> pathsToTest;
		if (strcmp(cwd, "/") != 0)
			pathsToTest.push_back(cwd + string("/") + filename);
		if (!vuoFrameworkPath.empty())
			pathsToTest.push_back(vuoFrameworkPath + "/Resources/shaders/" + filename);
		if (!vuoRunnerFrameworkPath.empty())
			pathsToTest.push_back(vuoRunnerFrameworkPath + "/Resources/shaders/" + filename);
		if (!userModulesPath.empty())
			pathsToTest.push_back(userModulesPath + "/" + filename);
		if (!systemModulesPath.empty())
			pathsToTest.push_back(systemModulesPath + "/" + filename);
		bool found = false;
		for (vector<string>::iterator i = pathsToTest.begin(); i != pathsToTest.end(); ++i)
		{
			if (VuoFileUtilities::fileIsReadable(*i))
			{
				found = true;

				source.erase(includeStart, offset - includeStart + 1);

				// Prevent recursive includes.
				if (includedFiles.count(*i))
				{
					source.insert(includeStart,
						"\n"
						"// ============================================================\n"
						"// Skipped including file \"" + *i + "\" since it was already included.\n"
						"// ============================================================\n"
						"\n"
					);
					break;
				}
				includedFiles.insert(*i);

				source.insert(includeStart,
					"\n"
					"// ============================================================\n"
					"// Begin included file \"" + *i + "\"\n"
				  + VuoFileUtilities::readFileToString(*i)
				  + "\n"
					"// End included file \"" + *i + "\"\n"
					"// ============================================================\n"
					"\n"
				);
				break;
			}
		}

		if (!found)
		{
			/// @todo calculate line number
			if (outIssues)
				outIssues->addIssue(stage, VuoShaderIssues::NoLine, "Couldn't find include file \"" + filename + "\"");
			else
				VUserLog("Error: Couldn't find include file \"%s\".", filename.c_str());
			return false;
		}
	}
}

static const std::locale VuoGlPool_locale;	///< For hashing strings.
static const std::collate<char> &VuoGlPool_collate = std::use_facet<std::collate<char> >(VuoGlPool_locale);	///< For hashing strings.

/**
 * Returns an OpenGL Shader Object representing the specified @c source,
 * or 0 if the shader couldn't be compiled.
 *
 * To improve performance, this function keeps a cache of precompiled shaders.
 * If a precompiled shader exists for the specified @c source, that shader is returned.
 * Otherwise, @c source is passed off to OpenGL to be compiled.
 *
 * Do not call `glDeleteShaders()` on the returned shader;
 * it's expected to persist throughout the lifetime of the process.
 *
 * If `outIssues` is non-NULL, it is expected to be a @ref VuoShaderIssues instance,
 * and compilation warnings/errors will be added to it.
 * In this case, the cache is not used.
 *
 * @version200Changed{Added `outIssues` argument.}
 */
GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source, void *outIssues)
{
	long hash = VuoGlPool_collate.hash(source, source+strlen(source));

	dispatch_semaphore_wait(VuoGlShaderPool_semaphore, DISPATCH_TIME_FOREVER);

	GLuint shader;
	if (!outIssues && VuoGlShaderPool[type].find(hash) != VuoGlShaderPool[type].end())
		shader = VuoGlShaderPool[type][hash];
	else
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		string combinedSource = source;

		VuoShaderIssues *oi = static_cast<VuoShaderIssues *>(outIssues);
		VuoShaderFile::Stage stage;
		if (type == GL_VERTEX_SHADER)
			stage = VuoShaderFile::Vertex;
		else if (type == GL_GEOMETRY_SHADER_EXT)
			stage = VuoShaderFile::Geometry;
		else // if (type == GL_FRAGMENT_SHADER)
			stage = VuoShaderFile::Fragment;

		if (!VuoGlShader_resolveIncludes(combinedSource, stage, oi))
		{
			shader = 0;
			goto done;
		}

//		VLog("\n%s\n\n", combinedSource.c_str());

		shader = glCreateShader(type);
		GLint length = combinedSource.length();
		const GLchar *combinedSourceCString = combinedSource.c_str();
		glShaderSource(shader, 1, (const GLchar**)&combinedSourceCString, &length);
		glCompileShader(shader);
		VuoGlShader_parseShaderInfoLog(cgl_ctx, type, shader, combinedSourceCString, stage, oi);

		GLint status = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			glDeleteShader(shader);
			shader = 0;
			goto done;
		}

		VuoGlShaderPool[type][hash] = shader;
	}

done:
	dispatch_semaphore_signal(VuoGlShaderPool_semaphore);

	return shader;
}

// Too bad we can't use std::tuple yet…
typedef pair<GLuint, pair<GLuint, pair<GLuint, pair<VuoMesh_ElementAssemblyMethod, unsigned int> > > > VuoGlProgramDescriptorType;	///< An entry in the GL Program pool: vertexShaderName, geometryShaderName, fragmentShaderName, assemblyMethod, expectedOutputPrimitiveCount.
typedef map<VuoGlProgramDescriptorType, VuoGlProgram> VuoGlProgramPoolType;	///< Type for VuoGlProgramPool.
static VuoGlProgramPoolType VuoGlProgramPool;	///< All the GL Programs.
static dispatch_semaphore_t VuoGlProgramPool_semaphore;	///< Serializes access to VuoGlProgramPool.
static void __attribute__((constructor)) VuoGlProgramPool_init(void)
{
	VuoGlProgramPool_semaphore = dispatch_semaphore_create(1);
}

typedef std::map<long, GLuint> VuoGlUniformMap;	///< A quick way to look up a uniform location given a hash of its name.

/**
 * Links shaders together into a program (or finds an existing program if one already exists for the given shaders, element assembly method, and output primitive count),
 * and returns the GL Program Object.
 *
 * `vertexShaderName` must be nonzero, but it's OK for `geometryShaderName` and/or `fragmentShaderName` to be 0 (same rules as @ref VuoShader_addSource).
 *
 * `description` is used just to print errors/warnings; it is not used for cache matching.
 *
 * Do not call `glDeleteShaders()` on the returned program;
 * it's expected to persist throughout the lifetime of the process.
 *
 * If `outIssues` is non-NULL, it is expected to be a @ref VuoShaderIssues instance,
 * and compilation warnings/errors will be added to it.
 * In this case, the cache is not used.
 *
 * @version200Changed{Added `outIssues` argument.}
 */
VuoGlProgram VuoGlProgram_use(VuoGlContext glContext, const char *description, GLuint vertexShaderName, GLuint geometryShaderName, GLuint fragmentShaderName, VuoMesh_ElementAssemblyMethod assemblyMethod, unsigned int expectedOutputPrimitiveCount, void *outIssues)
{
//	VLog("looking for %d %d %d %d %d", vertexShaderName, geometryShaderName, fragmentShaderName, assemblyMethod, expectedOutputPrimitiveCount);
	VuoGlProgram program;
	VuoGlProgramDescriptorType e(vertexShaderName, std::make_pair(geometryShaderName, std::make_pair(fragmentShaderName, std::make_pair(assemblyMethod, expectedOutputPrimitiveCount))));
	dispatch_semaphore_wait(VuoGlProgramPool_semaphore, DISPATCH_TIME_FOREVER);
	VuoGlProgramPoolType::iterator it = VuoGlProgramPool.find(e);
	if (!outIssues && it != VuoGlProgramPool.end())
		program = it->second;
	else
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		GLuint programName = glCreateProgram();
		glAttachShader(programName, vertexShaderName);
		if (geometryShaderName)
			glAttachShader(programName, geometryShaderName);
		if (fragmentShaderName)
			glAttachShader(programName, fragmentShaderName);

		bool transformFeedback = false;
		// If there's no fragment shader, this program is being used for transform feedback.
		if (!fragmentShaderName)
			transformFeedback = true;

		// Make sure `position` is at location 0, since location 0 is required in order for glDraw*() to work.
		glBindAttribLocation(programName, 0, "position");

		if (geometryShaderName)
		{
			GLuint inputPrimitiveGlMode = GL_TRIANGLES;
			if (assemblyMethod == VuoMesh_IndividualLines
			 || assemblyMethod == VuoMesh_LineStrip)
				inputPrimitiveGlMode = GL_LINES;
			else if (assemblyMethod == VuoMesh_Points)
				inputPrimitiveGlMode = GL_POINTS;
			glProgramParameteriEXT(programName, GL_GEOMETRY_INPUT_TYPE_EXT, inputPrimitiveGlMode);

			GLuint outputPrimitiveGlMode = GL_TRIANGLE_STRIP;
			if (transformFeedback)
			{
				// In transform feedback, the output primitive mode needs to match the input primitive mode.
				if (assemblyMethod == VuoMesh_IndividualLines
				 || assemblyMethod == VuoMesh_LineStrip)
					outputPrimitiveGlMode = GL_LINE_STRIP;
				else if (assemblyMethod == VuoMesh_Points)
					outputPrimitiveGlMode = GL_POINTS;
			}
			glProgramParameteriEXT(programName, GL_GEOMETRY_OUTPUT_TYPE_EXT, outputPrimitiveGlMode);

			unsigned int expectedVertexCount = expectedOutputPrimitiveCount;
			if (outputPrimitiveGlMode == GL_TRIANGLE_STRIP)
				expectedVertexCount *= 3;
			else if (outputPrimitiveGlMode == GL_LINE_STRIP)
				expectedVertexCount *= 2;
			glProgramParameteriEXT(programName, GL_GEOMETRY_VERTICES_OUT_EXT, expectedVertexCount);
		}

		if (transformFeedback)
		{
			const GLchar *varyings[] = { "outPosition", "outNormal", "outTextureCoordinate", "outVertexColor" };
			glTransformFeedbackVaryingsEXT(programName, 4, varyings, GL_SEPARATE_ATTRIBS_EXT);
		}

		glLinkProgram(programName);

		{
			int infologLength = 0;
			glGetProgramiv(programName, GL_INFO_LOG_LENGTH, &infologLength);
			if (infologLength > 0)
			{
				char *infoLog = (char *)malloc(infologLength);
				int charsWritten = 0;
				glGetProgramInfoLog(programName, infologLength, &charsWritten, infoLog);

				if (outIssues)
				{
					VuoShaderIssues *oi = static_cast<VuoShaderIssues *>(outIssues);

					istringstream iss(infoLog);
					string ln;
					while (getline(iss, ln))
					{
						string s;
						if ( !(s = VuoStringUtilities::substrAfter(ln, "ERROR: ")).empty() )
							oi->addIssue(VuoShaderFile::Program, VuoShaderIssues::NoLine, s);
						else if ( !(s = VuoStringUtilities::substrAfter(ln, "WARNING: ")).empty() )
							oi->addIssue(VuoShaderFile::Program, VuoShaderIssues::NoLine, s);
						else
						{
							VUserLog("Warning: Couldn't parse GLSL log message: \"%s\"", ln.c_str());
							oi->addIssue(VuoShaderFile::Program, VuoShaderIssues::NoLine, ln);
						}
					}
				}
				else
					VUserLog("%s: %s", description, infoLog);

				free(infoLog);
			}
		}

		int linkStatus = 0;
		glGetProgramiv(programName, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			glDeleteProgram(programName);
			dispatch_semaphore_signal(VuoGlProgramPool_semaphore);
			return (VuoGlProgram){0,NULL};
		}

		program.programName = programName;

		GLint uniformCount;
		glGetProgramiv(programName, GL_ACTIVE_UNIFORMS, &uniformCount);

		GLint maxNameLength;
		glGetProgramiv(programName, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
		char *name = (char *)malloc(maxNameLength + 1);

		VuoGlUniformMap *uniforms = new VuoGlUniformMap;
		program.uniforms = (void *)uniforms;

		for (GLuint i = 0; i < uniformCount; ++i)
		{
			GLint size;
			glGetActiveUniform(programName, i, maxNameLength+1, NULL, &size, NULL, name);

			// The uniform location is _not_ the same as the active uniform index!
			size_t nameLen = strlen(name);
			long hash = VuoGlPool_collate.hash(name, name+nameLen);
			(*uniforms)[hash] = glGetUniformLocation(programName, name);

			if (size > 1)
			{
				// For arrays, glGetActiveUniform() returns only the first array index.  Take care of the rest of the indices.
				// E.g., `uniform float offset[3];` would return `offset[0]` with size 3, so we need to synthesize `offset[1]` and `offset[2]`.
				if (name[nameLen-2] == '0' && name[nameLen-1] == ']')
				{
					name[nameLen-2] = 0;
					for (int i = 1; i < size; ++i)
					{
						std::stringstream ss;
						ss << name << i << "]";
						string sss = ss.str();
						long sHash = VuoGlPool_collate.hash(sss.data(), sss.data()+sss.length());
						(*uniforms)[sHash] = glGetUniformLocation(programName, sss.c_str());
					}
				}
			}
		}

		VuoGlProgramPool[e] = program;
	}

	dispatch_semaphore_signal(VuoGlProgramPool_semaphore);
	return program;
}

/**
 * Returns the location (suitable for use with `glUniform*()`) for a given uniform identifier, or -1 if the uniform isn't found.
 */
int VuoGlProgram_getUniformLocation(VuoGlProgram program, const char *uniformIdentifier)
{
	VuoGlUniformMap *uniforms = (VuoGlUniformMap *)program.uniforms;

	long hash = VuoGlPool_collate.hash(uniformIdentifier, uniformIdentifier+strlen(uniformIdentifier));

	VuoGlUniformMap::iterator i = uniforms->find(hash);
	if (i != uniforms->end())
		return i->second;

	return -1;
}

/// Helper for @ref VuoGl_stringForConstant.
#define RETURN_STRING_IF_EQUAL(value) if (constant == value) return strdup(#value)

/**
 * Returns a string for the specified OpenGL constant.
 *
 * The caller is responsible for freeing the string returned by this function.
 */
char *VuoGl_stringForConstant(GLenum constant)
{
	if (constant == 0)
		return strdup("(GL_ZERO or GL_POINTS)");
	if (constant == 1)
		return strdup("(GL_ONE or GL_LINES)");
	RETURN_STRING_IF_EQUAL(GL_LINE_LOOP);
	RETURN_STRING_IF_EQUAL(GL_LINE_STRIP);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLES);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLE_STRIP);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLE_FAN);
	RETURN_STRING_IF_EQUAL(GL_QUADS);
	RETURN_STRING_IF_EQUAL(GL_QUAD_STRIP);
	RETURN_STRING_IF_EQUAL(GL_POLYGON);
	RETURN_STRING_IF_EQUAL(GL_BACK);
	RETURN_STRING_IF_EQUAL(GL_FRONT);
	RETURN_STRING_IF_EQUAL(GL_R8);
	RETURN_STRING_IF_EQUAL(GL_RED);
	RETURN_STRING_IF_EQUAL(GL_RGB);
	RETURN_STRING_IF_EQUAL(GL_RGB16);
	RETURN_STRING_IF_EQUAL(GL_RGB16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGB32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGBA);
	RETURN_STRING_IF_EQUAL(GL_RGBA8);
	RETURN_STRING_IF_EQUAL(GL_RGBA16);
	RETURN_STRING_IF_EQUAL(GL_RGBA16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGBA32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_BGRA);
	RETURN_STRING_IF_EQUAL(GL_BGR_EXT);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE8);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16I_EXT);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE8_ALPHA8);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16_ALPHA16);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE_ALPHA16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE_ALPHA32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE_ALPHA);
	RETURN_STRING_IF_EQUAL(GL_DEPTH_COMPONENT);
	RETURN_STRING_IF_EQUAL(GL_DEPTH_COMPONENT16);
	RETURN_STRING_IF_EQUAL(GL_TEXTURE_2D);
	RETURN_STRING_IF_EQUAL(GL_TEXTURE_RECTANGLE_ARB);
	RETURN_STRING_IF_EQUAL(GL_FLOAT);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_BYTE);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_BYTE_3_3_2);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_4_4_4_4);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_5_5_5_1);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_8_8_APPLE);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_INT_8_8_8_8);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_INT_10_10_10_2);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_BYTE_2_3_3_REV);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_5_6_5);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_5_6_5_REV);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_4_4_4_4_REV);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_SHORT_1_5_5_5_REV);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_INT_8_8_8_8_REV);
	RETURN_STRING_IF_EQUAL(GL_UNSIGNED_INT_2_10_10_10_REV);
	RETURN_STRING_IF_EQUAL(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
	RETURN_STRING_IF_EQUAL(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
	RETURN_STRING_IF_EQUAL(GL_COMPRESSED_RED_RGTC1);
	RETURN_STRING_IF_EQUAL(GL_YCBCR_422_APPLE);
	RETURN_STRING_IF_EQUAL(GL_ALREADY_SIGNALED);
	RETURN_STRING_IF_EQUAL(GL_TIMEOUT_EXPIRED);
	RETURN_STRING_IF_EQUAL(GL_CONDITION_SATISFIED);
	RETURN_STRING_IF_EQUAL(GL_WAIT_FAILED);

	return VuoText_format("(unknown: %x)", constant);
}
