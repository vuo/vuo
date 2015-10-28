/**
 * @file
 * VuoGlPool implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoGlPool.h"
#include "module.h"

#include "VuoGlContext.h"

#include <vector>
#include <utility>	///< for pair
#include <queue>
#include <deque>
#include <map>
#include <locale>
using namespace std;

#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurface.h>

#include <OpenGL/CGLIOSurface.h>
#include <OpenGL/CGLMacro.h>


static map<VuoGlPoolType, map<unsigned long, vector<GLuint> > > VuoGlPool __attribute__((init_priority(101)));
static dispatch_semaphore_t VuoGlPool_semaphore;	///< Serializes access to VuoGlPool.
static void __attribute__((constructor)) VuoGlPool_init(void)
{
	VuoGlPool_semaphore = dispatch_semaphore_create(1);
}

/**
 * Returns an OpenGL Buffer Object of type @c type.
 *
 * If an existing, unused buffer of the specified @c type and @c size is available, it is returned.
 * Otherwise, a new buffer is created.
 *
 * @todo https://b33p.net/kosada/node/6901 The returned buffer's storage ~~is~~ will be preallocated (so the caller can efficiently upload data using [glBufferSubData](http://www.opengl.org/sdk/docs/man/xhtml/glBufferSubData.xml)),
 *
 * @threadAnyGL
 */
GLuint VuoGlPool_use(VuoGlPoolType type, unsigned long size)
{
	GLuint name = 0;

//	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
//		if (VuoGlPool[type][size].size())
//		{
//			name = VuoGlPool[type][size].back();
//			VuoGlPool[type][size].pop_back();
//		}
//		else
		{
//			VLog("allocating %d",type);
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

			if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
			{
				glGenBuffers(1, &name);
/// @todo https://b33p.net/kosada/node/6901
//				GLenum bufferType = type == VuoGlPool_ArrayBuffer ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
//				glBindBuffer(bufferType, name);
//				glBufferData(bufferType, size, NULL, GL_STREAM_DRAW);
//				glBindBuffer(bufferType, 0);
			}
			else
				VLog("Unknown pool type %d.", type);

			VuoGlContext_disuse(cgl_ctx);
		}
	}
//	dispatch_semaphore_signal(VuoGlPool_semaphore);

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
//	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
		{
			CGLContextObj cgl_ctx=(CGLContextObj)VuoGlContext_use();
			glDeleteBuffers(1, &name);
			VuoGlContext_disuse(cgl_ctx);
//			VuoGlPool[type][size].push_back(name);
		}
		else
			VLog("Unknown pool type %d.", type);
	}
//	dispatch_semaphore_signal(VuoGlPool_semaphore);
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
void VuoGlPool_retainF(GLuint glBufferName, const char *file, unsigned int line, const char *func)
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
//	VuoLog(file, line, func, "VuoGlPool_retain(%d)", glBufferName);
}

/**
 * Helper for @ref VuoGlPool_release.
 */
void VuoGlPool_releaseF(VuoGlPoolType type, unsigned long size, GLuint glBufferName, const char *file, unsigned int line, const char *func)
{
	if (glBufferName == 0)
		return;

	dispatch_semaphore_wait(VuoGlPool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlPoolReferenceCounts::iterator it = VuoGlPool_referenceCounts.find(glBufferName);
	if (it == VuoGlPool_referenceCounts.end())
		VuoLog(file, line, func, "Error: VuoGlPool_release() was called with OpenGL Buffer Object %d, which was never retained.", glBufferName);
	else
	{
		if (--VuoGlPool_referenceCounts[glBufferName] == 0)
			VuoGlPool_disuse(type, size, glBufferName);
	}

	dispatch_semaphore_signal(VuoGlPool_referenceCountsSemaphore);
//	VuoLog(file, line, func, "VuoGlPool_release(%d)", glBufferName);
}






typedef pair<unsigned short,unsigned short> VuoGlTextureDimensionsType;	///< Texture width and height.
typedef pair<queue<GLuint>,double> VuoGlTextureLastUsed;	///< A queue of textures of a given format and size, including the last time any of the textures were used.
typedef map<GLenum, map<VuoGlTextureDimensionsType, VuoGlTextureLastUsed > > VuoGlTexturePoolType;	///< VuoGlTexturePool[internalformat][size] gives a list of unused textures.
static VuoGlTexturePoolType *VuoGlTexturePool;	///< A pool of GL Textures.
static dispatch_semaphore_t VuoGlTexturePool_semaphore;	///< Serializes access to VuoGlTexturePool.
static dispatch_semaphore_t VuoGlTexturePool_canceledAndCompleted;	///< Signals when the last VuoGlTexturePool cleanup has completed.
static dispatch_source_t VuoGlTexturePool_timer;	///< Periodically cleans up VuoGlTexturePool.
static double textureTimeout = 0.1;	///< Seconds a texture can remain in the pool unused, before it gets purged.

/**
 * Returns the number of seconds (including fractional seconds) since midnight 1970.01.01 GMT.
 */
static double VuoGlTexturePool_getTime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec / 1000000.;
}

/**
 * Purges expired textures from the GL Texture Pool.
 */
static void VuoGlTexturePool_cleanup(void *blah)
{
	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoGlTexturePool_getTime();
//		VLog("pool:");
		for (VuoGlTexturePoolType::iterator internalformat = VuoGlTexturePool->begin(); internalformat != VuoGlTexturePool->end(); ++internalformat)
		{
//			VLog("\t%s:",VuoGl_stringForConstant(internalformat->first));
			for (map<VuoGlTextureDimensionsType, VuoGlTextureLastUsed >::iterator dimensions = internalformat->second.begin(); dimensions != internalformat->second.end(); )
			{
//				VLog("\t\t%dx%d: %lu unused textures (last used %gs ago)",dimensions->first.first,dimensions->first.second, dimensions->second.first.size(), now - dimensions->second.second);
				if (now - dimensions->second.second > textureTimeout)
				{
					unsigned long textureCount = dimensions->second.first.size();
					if (textureCount)
					{
//						VLog("\t\t\tpurging %lu expired textures", textureCount);
						CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
						while (!dimensions->second.first.empty())
						{
							GLuint textureName = dimensions->second.first.front();
							dimensions->second.first.pop();

							glDeleteTextures(1, &textureName);
						}
						VuoGlContext_disuse(cgl_ctx);
					}

					internalformat->second.erase(dimensions++);
				}
				else
					++dimensions;
			}
		}
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);
}
/**
 * Initializes the GL Texture Pool.
 */
static void __attribute__((constructor)) VuoGlTexturePool_init(void)
{
	VuoGlTexturePool_semaphore = dispatch_semaphore_create(1);
	VuoGlTexturePool_canceledAndCompleted = dispatch_semaphore_create(0);
	VuoGlTexturePool = new VuoGlTexturePoolType;

	VuoGlTexturePool_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(VuoGlTexturePool_timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*textureTimeout, NSEC_PER_SEC*textureTimeout);
	dispatch_source_set_event_handler_f(VuoGlTexturePool_timer, VuoGlTexturePool_cleanup);
	dispatch_source_set_cancel_handler(VuoGlTexturePool_timer, ^{
										   dispatch_semaphore_signal(VuoGlTexturePool_canceledAndCompleted);
									   });
	dispatch_resume(VuoGlTexturePool_timer);
}
/**
 * Destroys the GL Texture Pool.
 */
static void __attribute__((destructor)) VuoGlTexturePool_fini(void)
{
	dispatch_source_cancel(VuoGlTexturePool_timer);

	// Wait for the last cleanup to complete.
	dispatch_semaphore_wait(VuoGlTexturePool_canceledAndCompleted, DISPATCH_TIME_FOREVER);

	delete VuoGlTexturePool;
}

/**
 * Returns the OpenGL texture data type corresponding with OpenGL texture `format`.
 * Assumes `format` refers to an 8-bits-per-channel texture.
 */
GLuint VuoGlTexture_getType(GLuint format)
{
	if (format == GL_YCBCR_422_APPLE)
		return GL_UNSIGNED_SHORT_8_8_APPLE;

	return GL_UNSIGNED_BYTE;
}

/**
 * Returns an OpenGL texture.
 *
 * If an existing, unused texture matching the specified @c internalformat, @c width, and @c height is available, it is returned.
 * Otherwise, a new texture is created.
 *
 * The returned texture's storage is preallocated (so the caller can efficiently upload data using [glTexSubImage2D](http://www.opengl.org/sdk/docs/man/xhtml/glTexSubImage2D.xml)),
 * and its texturing properties are set to the defaults:
 *
 *    - wrapping: clamp to border
 *    - filtering: linear
 *
 * See [glTexImage2D](http://www.opengl.org/sdk/docs/man/xhtml/glTexImage2D.xml) for information about @c internalformat and @c format.
 *
 * @threadAnyGL
 */
GLuint VuoGlTexturePool_use(VuoGlContext glContext, GLenum internalformat, unsigned short width, unsigned short height, GLenum format)
{
//	VLog("want (%s %dx%d)", VuoGl_stringForConstant(internalformat), width, height);

	GLuint name = 0;
	VuoGlTextureDimensionsType dimensions(width,height);
	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	if ((*VuoGlTexturePool)[internalformat][dimensions].first.size())
	{
		name = (*VuoGlTexturePool)[internalformat][dimensions].first.front();
		(*VuoGlTexturePool)[internalformat][dimensions].first.pop();
//		if (name) VLog("using recycled %d (%s %dx%d)", name, VuoGl_stringForConstant(internalformat), width, height);
	}
	(*VuoGlTexturePool)[internalformat][dimensions].second = VuoGlTexturePool_getTime();
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);


	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	if (name == 0)
	{
		glGenTextures(1, &name);
		glBindTexture(GL_TEXTURE_2D, name);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, VuoGlTexture_getType(format), NULL);
//		VLog("allocated %d (%s %dx%d)", name, VuoGl_stringForConstant(internalformat), width, height);
	}
	else
		glBindTexture(GL_TEXTURE_2D, name);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return name;
}

/**
 * Indicates that the caller is done using the OpenGL texture @c name.
 *
 * The texture is returned to the pool, so other callers can use it
 * (which is more efficient than deleting and re-generating textures).
 *
 * @threadAnyGL
 */
static void VuoGlTexurePool_disuse(VuoGlContext glContext, GLenum internalformat, unsigned short width, unsigned short height, GLuint name)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	if (internalformat == 0)
	{
		VLog("Error:  Can't recycle texture %d since we don't know its internalformat.  Deleting.", name);
		glDeleteTextures(1, &name);
		return;
	}

	VuoGlTextureDimensionsType dimensions(width,height);
	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		(*VuoGlTexturePool)[internalformat][dimensions].first.push(name);
		(*VuoGlTexturePool)[internalformat][dimensions].second = VuoGlTexturePool_getTime();
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);

//	VLog("recycled %d (%s %dx%d)", name, VuoGl_stringForConstant(internalformat), width, height);
}





typedef map<GLuint, unsigned int> VuoGlTextureReferenceCounts;	///< The number of times each glTextureName is retained..
static VuoGlTextureReferenceCounts VuoGlTexture_referenceCounts __attribute__((init_priority(101)));  ///< The reference count for each OpenGL Texture Object.
static dispatch_semaphore_t VuoGlTexture_referenceCountsSemaphore = NULL;  ///< Synchronizes access to @c VuoGlTexture_referenceCounts.
static void __attribute__((constructor)) VuoGlTexture_init(void)
{
	VuoGlTexture_referenceCountsSemaphore = dispatch_semaphore_create(1);
}

/**
 * Increments the reference count for @c glTextureName.
 *
 * @threadAny
 */
void VuoGlTexture_retain(GLuint glTextureName)
{
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts.find(glTextureName);
	if (it == VuoGlTexture_referenceCounts.end())
		VuoGlTexture_referenceCounts[glTextureName] = 1;
	else
		++VuoGlTexture_referenceCounts[glTextureName];

	dispatch_semaphore_signal(VuoGlTexture_referenceCountsSemaphore);
}

/**
 * Decrements the reference count for @c glTextureName.
 *
 * @threadAny
 */
void VuoGlTexture_release(GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName)
{
//	VLog("%d (%s %dx%d)", glTextureName, VuoGl_stringForConstant(internalformat), width, height);
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts.find(glTextureName);
	if (it == VuoGlTexture_referenceCounts.end())
		VLog("Error: VuoGlTexture_release() was called with OpenGL Texture Object %d, which was never retained.", glTextureName);
	else
	{
		if (--VuoGlTexture_referenceCounts[glTextureName] == 0)
		{
			VuoGlContext glContext = VuoGlContext_use();
			VuoGlTexurePool_disuse(glContext, internalformat, width, height, glTextureName);
			VuoGlContext_disuse(glContext);
		}
	}

	dispatch_semaphore_signal(VuoGlTexture_referenceCountsSemaphore);
}



/**
 * An entry in the IOSurface pool.
 */
typedef struct
{
	VuoIoSurface ioSurface;
	GLuint texture;
	double lastUsedTime;
} VuoIoSurfacePoolEntryType;
typedef map<VuoGlTextureDimensionsType, deque<VuoIoSurfacePoolEntryType> > VuoIoSurfacePoolType;	///< VuoIoSurfacePoolType[size] gives a list of IOSurfaces.
static VuoIoSurfacePoolType *VuoIoSurfacePool;	///< Unused IOSurfaces.
static VuoIoSurfacePoolType *VuoIoSurfaceQuarantine;	///< IOSurfaces which might still be in use by the receiver.
static dispatch_semaphore_t VuoIoSurfacePool_semaphore;	///< Serializes access to the IOSurface pool.
static dispatch_semaphore_t VuoIoSurfacePool_canceledAndCompleted;	///< Signals when the last IOSurface pool cleanup has completed.
static dispatch_source_t VuoIoSurfacePool_timer;	///< Periodically cleans up the IOSurface pool.
static CFStringRef receiverFinishedWithIoSurfaceKey = CFSTR("VuoReceiverFinished");	///< Signals from the receiver to the sender, when the receiver is finished using the IOSurface.

static double ioSurfaceCleanupInterval = 0.1;	///< Interval (in seconds) to promote IOSurfaces from the quarantine to the pool, and to clean old IOSurfaces from the pool.

/**
 * Periodically cleans up the IOSurface pool.
 */
static void VuoIoSurfacePool_cleanup(void *blah)
{
	dispatch_semaphore_wait(VuoIoSurfacePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		// Promote IOSurfaces from the quarantine to the pool, if the receiver has finished using them.
		for (VuoIoSurfacePoolType::iterator quarantinedQueue = VuoIoSurfaceQuarantine->begin(); quarantinedQueue != VuoIoSurfaceQuarantine->end(); ++quarantinedQueue)
		{
			for (deque<VuoIoSurfacePoolEntryType>::iterator quarantinedIoSurfaceEntry = quarantinedQueue->second.begin(); quarantinedIoSurfaceEntry != quarantinedQueue->second.end();)
			{
				VuoIoSurfacePoolEntryType e = *quarantinedIoSurfaceEntry;
				IOSurfaceRef s = (IOSurfaceRef)e.ioSurface;
				CFBooleanRef finished = (CFBooleanRef)IOSurfaceCopyValue(s, receiverFinishedWithIoSurfaceKey);
				if (finished)
				{
					IOSurfaceRemoveValue(s, receiverFinishedWithIoSurfaceKey);

					(*VuoIoSurfacePool)[quarantinedQueue->first].push_back(e);
					quarantinedIoSurfaceEntry = quarantinedQueue->second.erase(quarantinedIoSurfaceEntry);

//					VLog("Promoted IOSurface %d + GL Texture %d (%dx%d) from quarantine to pool", IOSurfaceGetID(s), e.texture, quarantinedQueue->first.first, quarantinedQueue->first.second);
				}
				else
					++quarantinedIoSurfaceEntry;
			}
		}


		double now = VuoGlTexturePool_getTime();
		for (VuoIoSurfacePoolType::iterator poolQueue = VuoIoSurfacePool->begin(); poolQueue != VuoIoSurfacePool->end(); ++poolQueue)
		{
			for (deque<VuoIoSurfacePoolEntryType>::iterator poolIoSurfaceEntry = poolQueue->second.begin(); poolIoSurfaceEntry != poolQueue->second.end();)
			{
				VuoIoSurfacePoolEntryType e = *poolIoSurfaceEntry;
				if (now - e.lastUsedTime > ioSurfaceCleanupInterval*2.)
				{
					IOSurfaceRef s = (IOSurfaceRef)e.ioSurface;
//					VLog("Purging expired IOSurface %d + GL Texture %d (%dx%d) — it's %gs old", IOSurfaceGetID(s), e.texture, poolQueue->first.first, poolQueue->first.second, now - e.lastUsedTime);

					CFRelease(s);

					CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
					glDeleteTextures(1, &e.texture);
					VuoGlContext_disuse(cgl_ctx);

					poolIoSurfaceEntry = poolQueue->second.erase(poolIoSurfaceEntry);
				}
				else
					++poolIoSurfaceEntry;
			}
		}
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);
}
static void __attribute__((constructor)) VuoIoSurfacePool_init(void)
{
	VuoIoSurfacePool_semaphore = dispatch_semaphore_create(1);
	VuoIoSurfacePool_canceledAndCompleted = dispatch_semaphore_create(0);

	VuoIoSurfacePool = new VuoIoSurfacePoolType;
	VuoIoSurfaceQuarantine = new VuoIoSurfacePoolType;

	VuoIoSurfacePool_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
	dispatch_source_set_timer(VuoIoSurfacePool_timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*ioSurfaceCleanupInterval, NSEC_PER_SEC*ioSurfaceCleanupInterval);
	dispatch_source_set_event_handler_f(VuoIoSurfacePool_timer, VuoIoSurfacePool_cleanup);
	dispatch_source_set_cancel_handler(VuoIoSurfacePool_timer, ^{
										   dispatch_semaphore_signal(VuoIoSurfacePool_canceledAndCompleted);
									   });
	dispatch_resume(VuoIoSurfacePool_timer);
}
static void __attribute__((destructor)) VuoIoSurfacePool_fini(void)
{
	dispatch_source_cancel(VuoIoSurfacePool_timer);

	// Wait for the last cleanup to complete.
	dispatch_semaphore_wait(VuoIoSurfacePool_canceledAndCompleted, DISPATCH_TIME_FOREVER);

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
			ioSurface = e.ioSurface;
			*outputTexture = e.texture;
			(*VuoIoSurfacePool)[dimensions].pop_front();
//			VLog("Using recycled IOSurface %d + GL Texture %d (%dx%d) from pool", IOSurfaceGetID((IOSurfaceRef)ioSurface), *outputTexture, pixelsWide, pixelsHigh);
		}
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);

	if (!ioSurface)
	{
		CFMutableDictionaryRef properties = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
		CFDictionaryAddValue(properties, kIOSurfaceIsGlobal, kCFBooleanTrue);

		long long pixelsWideLL = pixelsWide;
		CFNumberRef pixelsWideCF = CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsWideLL);
		CFDictionaryAddValue(properties, kIOSurfaceWidth, pixelsWideCF);

		long long pixelsHighLL = pixelsHigh;
		CFNumberRef pixelsHighCF = CFNumberCreate(NULL, kCFNumberLongLongType, &pixelsHighLL);
		CFDictionaryAddValue(properties, kIOSurfaceHeight, pixelsHighCF);

		long long bytesPerElement = 4;
		CFNumberRef bytesPerElementCF = CFNumberCreate(NULL, kCFNumberLongLongType, &bytesPerElement);
		CFDictionaryAddValue(properties, kIOSurfaceBytesPerElement, bytesPerElementCF);

		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		glGenTextures(1, outputTexture);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *outputTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		ioSurface = (VuoIoSurface)IOSurfaceCreate(properties);
		CFRelease(pixelsWideCF);
		CFRelease(pixelsHighCF);
		CFRelease(bytesPerElementCF);
		CFRelease(properties);
		CGLError err = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_ARB, GL_RGBA, (GLsizei)pixelsWide, (GLsizei)pixelsHigh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (IOSurfaceRef)ioSurface, 0);
		if (err != kCGLNoError)
		{
			VLog("Error in CGLTexImageIOSurface2D(): %s", CGLErrorString(err));
			return NULL;
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

//		VLog("Couldn't find a viable IOSurface to recycle; created IOSurface %d (%dx%d)", IOSurfaceGetID((IOSurfaceRef)ioSurface), pixelsWide, pixelsHigh);
	}

	return ioSurface;
}

/**
 * Returns the IOSurface's interprocess ID.
 */
uint32_t VuoIoSurfacePool_getId(VuoIoSurface ioSurface)
{
	IOSurfaceRef surf = (IOSurfaceRef)ioSurface;
	return IOSurfaceGetID(surf);
}

/**
 * Called by the sending end of an IOSurface texture transfer to indicate that it is finished using the IOSurface's texture.
 * This enables the IOSurface to be reused for another texture transfer (once the receiving end indicates it is done with it, via VuoIoSurfacePool_signal()).
 */
void VuoIoSurfacePool_disuse(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, VuoIoSurface ioSurface, GLuint texture)
{
	VuoGlTextureDimensionsType dimensions(pixelsWide,pixelsHigh);

	VuoIoSurfacePoolEntryType e;
	e.ioSurface = ioSurface;
	e.texture = texture;
	e.lastUsedTime = VuoGlTexturePool_getTime();

	dispatch_semaphore_wait(VuoIoSurfacePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		(*VuoIoSurfaceQuarantine)[dimensions].push_back(e);
	}
	dispatch_semaphore_signal(VuoIoSurfacePool_semaphore);
}

/**
 * Called by the receiving end of an IOSurface texture transfer to indicate that it is finished using the IOSurface's texture.
 * This enables the sender to reuse the IOSurface for another texture transfer.
 */
void VuoIoSurfacePool_signal(VuoIoSurface ioSurface)
{
	IOSurfaceRef surf = (IOSurfaceRef)ioSurface;
	IOSurfaceSetValue(surf, receiverFinishedWithIoSurfaceKey, kCFBooleanTrue);
}



/**
 * Prints GLSL debug information to the console.
 *
 * @threadAnyGL
 */
void VuoGlShader_printShaderInfoLog(CGLContextObj cgl_ctx, GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		VLog("%s",infoLog);
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

#include "deform.h"
#include "hsl.h"
#include "lighting.h"
#include "noise2D.h"
#include "noise3D.h"
#include "noise4D.h"
#include "triangle.h"
#include "triangleLine.h"
#include "trianglePoint.h"

/**
 * If @c source contains the string `include(includeFileName)`, replaces that string with @c includeContents.
 */
string VuoGlShader_replaceInclude(string source, string includeFileName, const unsigned char *includeContents, unsigned int includeContentsLength)
{
	string includeToken = string("include(") + includeFileName + ")";
	string::size_type includeTokenLocation;
	if ((includeTokenLocation = source.find(includeToken)) == std::string::npos)
		return source;

	return source.substr(0, includeTokenLocation) + "\n"
			+ string((const char *)includeContents, includeContentsLength) + "\n"
			+ source.substr(includeTokenLocation + includeToken.length());
}

/**
 * Returns an OpenGL Shader Object representing the specified @c source.
 *
 * To improve performance, this function keeps a cache of precompiled shaders.
 * If a precompiled shader exists for the specified @c source, that shader is returned.
 * Otherwise, @c source is passed off to OpenGL to be compiled.
 *
 * Do not call `glDeleteShaders()` on the returned shader;
 * it's expected to persist throughout the lifetime of the process.
 */
GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source)
{
	std::locale loc;
	const std::collate<char> &coll = std::use_facet<std::collate<char> >(loc);
	long hash = coll.hash(source, source+strlen(source));

	dispatch_semaphore_wait(VuoGlShaderPool_semaphore, DISPATCH_TIME_FOREVER);

	GLuint shader;
	if (VuoGlShaderPool[type].find(hash) != VuoGlShaderPool[type].end())
		shader = VuoGlShaderPool[type][hash];
	else
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		string combinedSource = source;

		if (strcmp((const char *)glGetString(GL_VENDOR), "ATI Technologies Inc.") == 0)
		{
			// On ATI Radeon HD 5770 (and possibly others), using snoise4D3D() causes
			// GLSL compliation to crash inside ATIRadeonX3000GLDriver,
			// so use the built-in noise function.
			// Continue using snoise4D3D() on other systems since it (at least on some configurations)
			// prevents falling back to the software renderer.
			// https://b33p.net/kosada/node/8285

			// Ignore "include(noise4D)".
			string includeToken = string("include(noise4D)");
			string::size_type includeTokenLocation;
			if ((includeTokenLocation = combinedSource.find(includeToken)) != std::string::npos)
				combinedSource.replace(includeTokenLocation, includeToken.length(), "");

			// Replace snoise4D3D() calls with noise3().
			string::size_type snoise4D3DLocation = 0;
			string snoise4D3DToken = string("snoise4D3D");
			string noise3Token = string("noise3");
			while ((snoise4D3DLocation = combinedSource.find(snoise4D3DToken, snoise4D3DLocation)) != std::string::npos)
			{
				 combinedSource.replace(snoise4D3DLocation, snoise4D3DToken.length(), noise3Token);
				 snoise4D3DLocation += noise3Token.length();
			}
		}

		combinedSource = VuoGlShader_replaceInclude(combinedSource, "deform",        deform_glsl,        deform_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "hsl",           hsl_glsl,           hsl_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "lighting",      lighting_glsl,      lighting_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "noise2D",       noise2D_glsl,       noise2D_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "noise3D",       noise3D_glsl,       noise3D_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "noise4D",       noise4D_glsl,       noise4D_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "triangle",      triangle_glsl,      triangle_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "triangleLine",  triangleLine_glsl,  triangleLine_glsl_len);
		combinedSource = VuoGlShader_replaceInclude(combinedSource, "trianglePoint", trianglePoint_glsl, trianglePoint_glsl_len);

		shader = glCreateShader(type);
		GLint length = combinedSource.length();
		const GLchar *combinedSourceCString = combinedSource.c_str();
		glShaderSource(shader, 1, (const GLchar**)&combinedSourceCString, &length);
		glCompileShader(shader);
		VuoGlShader_printShaderInfoLog(cgl_ctx, shader);

		VuoGlShaderPool[type][hash] = shader;
	}

	dispatch_semaphore_signal(VuoGlShaderPool_semaphore);

	return shader;
}

/// Helper for @ref VuoGl_stringForConstant.
#define RETURN_STRING_IF_EQUAL(value) if (constant == value) return #value

/**
 * Returns a string for the specified OpenGL constant.
 *
 * Don't free the string returned by this function.
 */
const char * VuoGl_stringForConstant(GLenum constant)
{
	if (constant == 0)
		return "(GL_ZERO or GL_POINTS)";
	if (constant == 1)
		return "(GL_ONE or GL_LINES)";
	RETURN_STRING_IF_EQUAL(GL_LINE_LOOP);
	RETURN_STRING_IF_EQUAL(GL_LINE_STRIP);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLES);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLE_STRIP);
	RETURN_STRING_IF_EQUAL(GL_TRIANGLE_FAN);
	RETURN_STRING_IF_EQUAL(GL_QUADS);
	RETURN_STRING_IF_EQUAL(GL_QUAD_STRIP);
	RETURN_STRING_IF_EQUAL(GL_POLYGON);
	RETURN_STRING_IF_EQUAL(GL_RGB);
	RETURN_STRING_IF_EQUAL(GL_RGB16);
	RETURN_STRING_IF_EQUAL(GL_RGB16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGB32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGBA);
	RETURN_STRING_IF_EQUAL(GL_RGBA16);
	RETURN_STRING_IF_EQUAL(GL_RGBA16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_RGBA32F_ARB);
	RETURN_STRING_IF_EQUAL(GL_BGRA);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE8);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE8_ALPHA8);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE16_ALPHA16);
	RETURN_STRING_IF_EQUAL(GL_LUMINANCE_ALPHA16F_ARB);
	RETURN_STRING_IF_EQUAL(GL_DEPTH_COMPONENT);
	RETURN_STRING_IF_EQUAL(GL_TEXTURE_2D);
	RETURN_STRING_IF_EQUAL(GL_TEXTURE_RECTANGLE_ARB);
	RETURN_STRING_IF_EQUAL(GL_YCBCR_422_APPLE);

	return "(unknown)";
}
