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
#include <map>
#include <locale>
using namespace std;

#include <dispatch/dispatch.h>

#include <OpenGL/CGLMacro.h>



/// @todo remove after https://b33p.net/kosada/node/6909
static unsigned long long totalGLMemory = 256*1024*1024;	///< https://b33p.net/kosada/node/6908#comment-23731
static unsigned long long totalGLMemoryAllocated = 0;  ///< https://b33p.net/kosada/node/6908
typedef void (*sendErrorType)(const char *message);  ///< https://b33p.net/kosada/node/6908
static sendErrorType sendError;
static void __attribute__((constructor)) VuoGlPoolError_init(void)
{
	sendError = (sendErrorType) dlsym(RTLD_SELF, "sendError");  // for running composition in separate process as executable or in current process
	if (! sendError)
		sendError = (sendErrorType) dlsym(RTLD_DEFAULT, "sendError");  // for running composition in separate process as dynamic libraries
}



static map<VuoGlContext, map<VuoGlPoolType, map<unsigned long, vector<GLuint> > > > VuoGlPool;
static dispatch_semaphore_t VuoGlPool_semaphore;
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
GLuint VuoGlPool_use(VuoGlContext glContext, VuoGlPoolType type, unsigned long size)
{
	GLuint name = 0;

	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (VuoGlPool[glContext][type][size].size())
		{
			name = VuoGlPool[glContext][type][size].back();
			VuoGlPool[glContext][type][size].pop_back();
		}
		else
		{
//			VLog("allocating %d",type);
			CGLContextObj cgl_ctx = (CGLContextObj)glContext;

			if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
			{
				/// @todo remove VRAM check after https://b33p.net/kosada/node/6909
				totalGLMemoryAllocated += size;
				if (totalGLMemoryAllocated > totalGLMemory)
				{
					sendError("Out of video RAM.");
					goto error;
				}

				glGenBuffers(1, &name);
/// @todo https://b33p.net/kosada/node/6901
//				GLenum bufferType = type == VuoGlPool_ArrayBuffer ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
//				glBindBuffer(bufferType, name);
//				glBufferData(bufferType, size, NULL, GL_STREAM_DRAW);
//				glBindBuffer(bufferType, 0);
			}
			else
				VLog("Unknown pool type %d.", type);
		}
	}
error:
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
void VuoGlPool_disuse(VuoGlContext glContext, VuoGlPoolType type, unsigned long size, GLuint name)
{
	dispatch_semaphore_wait(VuoGlPool_semaphore, DISPATCH_TIME_FOREVER);
	{
		if (type == VuoGlPool_ArrayBuffer || type == VuoGlPool_ElementArrayBuffer)
			VuoGlPool[glContext][type][size].push_back(name);
		else
			VLog("Unknown pool type %d.", type);
	}
	dispatch_semaphore_signal(VuoGlPool_semaphore);
}




typedef pair<unsigned short,unsigned short> VuoGlTextureDimensionsType;	///< Texture width and height.
typedef pair<queue<GLuint>,double> VuoGlTextureLastUsed;	///< A queue of textures of a given format and size, including the last time any of the textures were used.
typedef map<GLenum, map<VuoGlTextureDimensionsType, VuoGlTextureLastUsed > > VuoGlTexturePoolType;	///< VuoGlTexturePool[internalformat][size] gives a list of unused textures.
static VuoGlTexturePoolType VuoGlTexturePool;
static dispatch_semaphore_t VuoGlTexturePool_semaphore;
static double textureTimeout = 0.1;	///< Seconds a texture can remain in the pool unused, before it gets purged.

static double VuoGlTexturePool_getTime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec / 1000000.;
}

#if 0
static void VuoGlTexturePool_dump(void *blah)
{
	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoGlTexturePool_getTime();
		VLog("pool (%llu MB allocated)", totalGLMemoryAllocated/1024/1024);
		for (VuoGlTexturePoolType::const_iterator internalformat = VuoGlTexturePool.begin(); internalformat != VuoGlTexturePool.end(); ++internalformat)
		{
			VLog("\t%s:",VuoGl_stringForConstant(internalformat->first));
			for (map<VuoGlTextureDimensionsType, VuoGlTextureLastUsed >::const_iterator dimensions = internalformat->second.begin(); dimensions != internalformat->second.end(); ++dimensions)
				VLog("\t\t%dx%d: %lu unused textures (last used %gs ago)",dimensions->first.first,dimensions->first.second, dimensions->second.first.size(), now - dimensions->second.second);
		}
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);
}
static void __attribute__((constructor)) VuoGlTexturePool_initDump(void)
{
	dispatch_async(dispatch_get_main_queue(), ^{
					   dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
					   dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*2, NSEC_PER_SEC*2);
					   dispatch_source_set_event_handler_f(timer, VuoGlTexturePool_dump);
					   dispatch_resume(timer);
				   });
}
#endif

static void VuoGlTexturePool_cleanup(void *blah)
{
	dispatch_semaphore_wait(VuoGlTexturePool_semaphore, DISPATCH_TIME_FOREVER);
	{
		double now = VuoGlTexturePool_getTime();
		for (VuoGlTexturePoolType::iterator internalformat = VuoGlTexturePool.begin(); internalformat != VuoGlTexturePool.end(); ++internalformat)
		{
			for (map<VuoGlTextureDimensionsType, VuoGlTextureLastUsed >::iterator dimensions = internalformat->second.begin(); dimensions != internalformat->second.end(); )
				if (now - dimensions->second.second > textureTimeout)
				{
					unsigned short width = dimensions->first.first;
					unsigned short height = dimensions->first.second;
					unsigned long textureCount = dimensions->second.first.size();
					if (textureCount)
					{
//						VLog("purging %lu expired textures (%s: %dx%d)", textureCount, VuoGl_stringForConstant(internalformat->first), width, height);
						CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
						while (!dimensions->second.first.empty())
						{
							GLuint textureName = dimensions->second.first.front();
							dimensions->second.first.pop();

							glDeleteTextures(1, &textureName);

							/// @todo remove VRAM check after https://b33p.net/kosada/node/6909
							totalGLMemoryAllocated -= width*height*4;
						}
						VuoGlContext_disuse(cgl_ctx);
					}

					internalformat->second.erase(dimensions++);
				}
				else
					++dimensions;
		}
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);
}
static void __attribute__((constructor)) VuoGlTexturePool_init(void)
{
	VuoGlTexturePool_semaphore = dispatch_semaphore_create(1);

	dispatch_async(dispatch_get_main_queue(), ^{
					   dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
					   dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC*textureTimeout, NSEC_PER_SEC*textureTimeout);
					   dispatch_source_set_event_handler_f(timer, VuoGlTexturePool_cleanup);
					   dispatch_resume(timer);
				   });
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
	if (VuoGlTexturePool[internalformat][dimensions].first.size())
	{
		name = VuoGlTexturePool[internalformat][dimensions].first.front();
		VuoGlTexturePool[internalformat][dimensions].first.pop();
//		if (name) VLog("using recycled %d (%s %dx%d)", name, VuoGl_stringForConstant(internalformat), width, height);
	}
	VuoGlTexturePool[internalformat][dimensions].second = VuoGlTexturePool_getTime();
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);


	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	if (name == 0)
	{
		/// @todo remove VRAM check after https://b33p.net/kosada/node/6909
		totalGLMemoryAllocated += width*height*4;
		if (totalGLMemoryAllocated > totalGLMemory)
		{
			sendError("Out of video RAM.");
			return 0;
		}

		glGenTextures(1, &name);
		glBindTexture(GL_TEXTURE_2D, name);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
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
		VuoGlTexturePool[internalformat][dimensions].first.push(name);
		VuoGlTexturePool[internalformat][dimensions].second = VuoGlTexturePool_getTime();
	}
	dispatch_semaphore_signal(VuoGlTexturePool_semaphore);

//	VLog("recycled %d (%s %dx%d)", name, VuoGl_stringForConstant(internalformat), width, height);
}





typedef map<GLuint, unsigned int> VuoGlTextureReferenceCounts;	///< The number of times each glTextureName is retained..
static VuoGlTextureReferenceCounts VuoGlTexture_referenceCounts;  ///< The reference count for each OpenGL Texture Object.
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
	if (glTextureName == 0)
		return;

	dispatch_semaphore_wait(VuoGlTexture_referenceCountsSemaphore, DISPATCH_TIME_FOREVER);

	VuoGlTextureReferenceCounts::iterator it = VuoGlTexture_referenceCounts.find(glTextureName);
	if (it == VuoGlTexture_referenceCounts.end())
		fprintf(stderr, "Error: VuoGlTexture_release() was called with OpenGL Texture Object %d, which was never retained.\n", glTextureName);
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
		fprintf(stderr,"%s\n",infoLog);
		free(infoLog);
	}
}

map<GLenum, map<long, GLuint> > VuoGlShaderPool;	///< Shaders, keyed by type (vertex, fragment, ...) and source code hash.
dispatch_semaphore_t VuoGlShaderPool_semaphore = NULL;  ///< Synchronizes access to @c VuoGlShaderPool.

/**
 * Initializes @c VuoGlShaderPool_semaphore.
 */
__attribute__((constructor)) static void VuoGlShaderPool_init(void)
{
	VuoGlShaderPool_semaphore = dispatch_semaphore_create(1);
}

/**
 * Returns an OpenGL Shader Object representing the specified @c source.
 *
 * To improve performance, this function keeps a cache of precompiled shaders.
 * If a precompiled shader exists for the specified @c source, that shader is returned.
 * Otherwise, @c source is passed off to OpenGL to be compiled.
 */
GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source)
{
	GLint length = strlen(source);

	std::locale loc;
	const std::collate<char> &coll = std::use_facet<std::collate<char> >(loc);
	long hash = coll.hash(source, source+length);

	dispatch_semaphore_wait(VuoGlShaderPool_semaphore, DISPATCH_TIME_FOREVER);

	GLuint shader;
	if (VuoGlShaderPool[type].find(hash) != VuoGlShaderPool[type].end())
		shader = VuoGlShaderPool[type][hash];
	else
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		shader = glCreateShader(type);
		glShaderSource(shader, 1, (const GLchar**)&source, &length);
		glCompileShader(shader);
		VuoGlShader_printShaderInfoLog(cgl_ctx, shader);

		VuoGlShaderPool[type][hash] = shader;
	}

	dispatch_semaphore_signal(VuoGlShaderPool_semaphore);

	return shader;
}

/**
 * Returns a string for the specified OpenGL constant.
 *
 * Don't free the string returned by this function.
 */
const char * VuoGl_stringForConstant(GLenum constant)
{
	if (constant == GL_RGB)
		return "GL_RGB";
	if (constant == GL_RGBA)
		return "GL_RGBA";
	if (constant == GL_LUMINANCE8)
		return "GL_LUMINANCE8";
	if (constant == GL_LUMINANCE8_ALPHA8)
		return "GL_LUMINANCE8_ALPHA8";
	if (constant == GL_DEPTH_COMPONENT)
		return "GL_DEPTH_COMPONENT";

	return "(unknown)";
}
