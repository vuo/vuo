/**
 * @file
 * VuoGlTexturePool implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoGlTexturePool.h"

#include "VuoGlContext.h"

#include <map>
using namespace std;

#include <dispatch/dispatch.h>

/// @todo After we drop 10.6 support, switch back to gl3.h.
//#include <OpenGL/gl3.h>
#include <OpenGL/gl.h>


typedef map<GLuint, unsigned int> VuoGlTexturePool;
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
 */
void VuoGlTexturePool_retain(GLuint glTextureName)
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
 */
void VuoGlTexturePool_release(GLuint glTextureName)
{
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexturePool_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTexturePool::iterator it = VuoGlTexturePool_referenceCounts.find(glTextureName);
	if (it == VuoGlTexturePool_referenceCounts.end())
		fprintf(stderr, "Error: VuoGlTexturePool_release() was called with OpenGL Texture Object %d, which was never retained.\n", glTextureName);
	else
	{
		if (--VuoGlTexturePool_referenceCounts[glTextureName] == 0)
		{
			VuoGlContext_use();
			glDeleteTextures(1, &glTextureName);
//			fprintf(stderr, "VuoGlTexturePool_release() deleted texture %d\n", glTextureName);
			VuoGlContext_disuse();
		}
	}

	dispatch_semaphore_signal(VuoGlTexturePool_referenceCountsSemaphore);
}
