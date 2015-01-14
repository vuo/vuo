/**
 * @file
 * VuoGlPool implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoGlPool.h"
#include "module.h"

#include "VuoGlContext.h"

#include <vector>
#include <map>
using namespace std;

#include <dispatch/dispatch.h>

#include <OpenGL/CGLMacro.h>


static map<VuoGlPoolType,vector<GLuint> > VuoGlPool;
static dispatch_semaphore_t VuoGlPool_semaphore;
static void __attribute__((constructor)) VuoGlPool_init(void)
{
	VuoGlPool_semaphore = dispatch_semaphore_create(1);
}

/**
 * Returns an OpenGL object of type @c type.
 *
 * If an existing, unused object is available, it is returned.
 * Otherwise, a new object is created.
 *
 * @threadAny
 */
GLuint VuoGlPool_use(VuoGlPoolType type)
{
	GLuint name = 0;

	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (VuoGlPool[type].size())
		{
			name = VuoGlPool[type].back();
			VuoGlPool[type].pop_back();
		}
		else
		{
//			VLog("allocating %d",type);
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

			if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
				glGenBuffers(1, &name);
			else if (type == VuoGlPool_Texture)
				glGenTextures(1, &name);
			else
				VLog("Unknown pool type %d.", type);

			VuoGlContext_disuse(cgl_ctx);
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
 * @threadAny
 */
void VuoGlPool_disuse(VuoGlPoolType type, GLuint name)
{
	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
		{
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			glDeleteBuffers(1, &name);
			VuoGlContext_disuse(cgl_ctx);

			/// @todo https://b33p.net/kosada/node/6752
//			VuoGlPool[type].push_back(name);
		}
		else if (type == VuoGlPool_Texture)
		{
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			glDeleteTextures(1, &name);
			VuoGlContext_disuse(cgl_ctx);

			/// @todo https://b33p.net/kosada/node/5937
//			VuoGlPool[type].push_back(name);
		}
		else
			VLog("Unknown pool type %d.", type);
	}
	dispatch_semaphore_signal(VuoGlPool_semaphore);
}



typedef map<GLuint, unsigned int> VuoGlTexturePool;	///< The number of times each glTextureName is retained.
VuoGlTexturePool VuoGlTexturePool_referenceCounts;  ///< The reference count for each OpenGL Texture Object.
dispatch_semaphore_t VuoGlTexturePool_referenceCountsSemaphore = NULL;  ///< Synchronizes access to @c VuoGlTexturePool_referenceCounts.

/**
 * Initializes @c VuoGlTexturePool_referenceCountsSemaphore.
 */
__attribute__((constructor)) static void VuoGlTexturePool_init(void)
{
	VuoGlTexturePool_referenceCountsSemaphore = dispatch_semaphore_create(1);
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

	dispatch_semaphore_wait(VuoGlTexturePool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTexturePool::iterator it = VuoGlTexturePool_referenceCounts.find(glTextureName);
	if (it == VuoGlTexturePool_referenceCounts.end())
		VuoGlTexturePool_referenceCounts[glTextureName] = 1;
	else
		++VuoGlTexturePool_referenceCounts[glTextureName];

	dispatch_semaphore_signal(VuoGlTexturePool_referenceCountsSemaphore);
}

/**
 * Decrements the reference count for @c glTextureName.
 *
 * @threadAny
 */
void VuoGlTexture_release(GLuint glTextureName)
{
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexturePool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTexturePool::iterator it = VuoGlTexturePool_referenceCounts.find(glTextureName);
	if (it == VuoGlTexturePool_referenceCounts.end())
		fprintf(stderr, "Error: VuoGlTexture_release() was called with OpenGL Texture Object %d, which was never retained.\n", glTextureName);
	else
	{
		if (--VuoGlTexturePool_referenceCounts[glTextureName] == 0)
			VuoGlPool_disuse(VuoGlPool_Texture, glTextureName);
	}

	dispatch_semaphore_signal(VuoGlTexturePool_referenceCountsSemaphore);
}
