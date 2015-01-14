/**
 * @file
 * VuoGlContext implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <vector>
#include <algorithm>
using namespace std;

#include "VuoGlContext.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#include <dispatch/dispatch.h>

/**
 * A process-wide set of mutually-shared OpenGL contexts.
 */
class VuoGlContextPool
{
public:
	/**
	 * Returns the process-wide pool singleton instance.
	 */
	static VuoGlContextPool *getPool()
	{
		// This will first be called by a nodeInstanceInit() method.
		// Since those methods are invoked serially, we don't need to worry about
		// the possibility of multiple threads initializing the singleton simultaneously.
		if (!singletonInstance)
			singletonInstance = new VuoGlContextPool();
		return singletonInstance;
	}

	/**
	 * Finds an unused GL context in the process-wide shared context pool (or creates one if none is available),
	 * marks it used, activates it on the current thread, and returns it.
	 *
	 * Can be called from any thread.
	 */
	CGLContextObj use(void)
	{
		CGLContextObj context;

		dispatch_semaphore_wait(poolSemaphore, DISPATCH_TIME_FOREVER);
		{
			if (avaialbleSharedContexts.size())
			{
				context = avaialbleSharedContexts.back();
				avaialbleSharedContexts.pop_back();
//				fprintf(stderr, "VuoGlContextPool::use() found existing context %p\n", context);
			}
			else
			{
				context = createContext(rootContext);
				if (!context)
				{
					fprintf(stderr, "Error: VuoGlContextPool::use() couldn't create a context.\n");
					return NULL;
				}
				allSharedContexts.push_back(context);
//				fprintf(stderr, "VuoGlContextPool::use() created %p\n", context);
			}
		}
		dispatch_semaphore_signal(poolSemaphore);

		CGLSetCurrentContext(context);

		return context;
	}

	/**
	 * Throws the specified context back in the pool.
	 *
	 * Can be called from any thread.
	 */
	void disuse(CGLContextObj context)
	{
//		fprintf(stderr, "VuoGlContextPool::disuse(%p)\n", context);

		dispatch_semaphore_wait(poolSemaphore, DISPATCH_TIME_FOREVER);
		{
			if (std::find(allSharedContexts.begin(), allSharedContexts.end(), context) != allSharedContexts.end())
				avaialbleSharedContexts.push_back(context);
			else
				fprintf(stderr, "Error: Called VuoGlContextPool::disuse() with context %p that isn't in the global share pool.  I'm not going to muddy the waters.\n", context);
		}
		dispatch_semaphore_signal(poolSemaphore);
	}

private:
	static VuoGlContextPool *singletonInstance;

	friend void VuoGlContext_setGlobalRootContext(void *rootContext);

	VuoGlContextPool()
	{
		poolSemaphore = dispatch_semaphore_create(1);

		if (!rootContext)
			rootContext = createContext(NULL);

//		fprintf(stderr, "VuoGlContextPool() rootContext=%p\n", rootContext);
	}
	/// @todo provide a way to shut down entire system and release the contexts.

	CGLContextObj createContext(CGLContextObj rootContext)
	{
		CGLPixelFormatObj pf;
		if (rootContext)
			pf = CGLGetPixelFormat(rootContext);
		else
		{
			CGLPixelFormatAttribute pfa[] = {
				kCGLPFAAccelerated,
				kCGLPFAWindow,
				kCGLPFANoRecovery,
				kCGLPFADoubleBuffer,
				kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
				kCGLPFADepthSize, (CGLPixelFormatAttribute) 16,
				kCGLPFAMultisample,
				kCGLPFASampleBuffers, (CGLPixelFormatAttribute) 1,
				kCGLPFASamples, (CGLPixelFormatAttribute) 4,
				(CGLPixelFormatAttribute) 0
			};

			GLint npix;
			CGLError error = CGLChoosePixelFormat(pfa, &pf, &npix);
			if (error != kCGLNoError)
			{
				fprintf(stderr, "Error: VuoGlContextPool::createContext() failed: %s\n", CGLErrorString(error));
				return NULL;
			}
		}

		CGLContextObj context;
		{
			CGLError error = CGLCreateContext(pf, rootContext, &context);
			CGLDestroyPixelFormat(pf);
			if (error != kCGLNoError)
			{
				fprintf(stderr, "Error: VuoGlContextPool::createContext() failed: %s\n", CGLErrorString(error));
				return NULL;
			}
		}

		return context;
	}

	dispatch_semaphore_t poolSemaphore; ///< Serializes access to @c rootContext, @c allSharedContexts and @c avaialbleSharedContexts.
	static CGLContextObj rootContext;
	vector<CGLContextObj> allSharedContexts;
	vector<CGLContextObj> avaialbleSharedContexts;
};
VuoGlContextPool *VuoGlContextPool::singletonInstance = NULL;
CGLContextObj VuoGlContextPool::rootContext = NULL;

/**
 * Specifies a platform-specific context to be used as the base for all of Vuo's shared GL contexts.
 *
 * On Mac, this should be a @c CGLContext.
 *
 * Must be called before any Vuo composition is loaded, and before any other @c VuoGlContext_* methods.
 */
void VuoGlContext_setGlobalRootContext(void *rootContext)
{
	if (VuoGlContextPool::singletonInstance)
	{
		fprintf(stderr, "Error: VuoGlContext_setGlobalRootContext() was called after VuoGlContextPool was initialized.  Ignoring the new rootContext.\n");
		return;
	}

	VuoGlContextPool::rootContext = (CGLContextObj)rootContext;
}

/**
 * Finds an unused GL context in the process-wide shared context pool (or creates one if none is available),
 * marks it used, activates it on the current thread, and returns it.
 *
 * Can be called from any thread.
 */
VuoGlContext VuoGlContext_use(void)
{
	VuoGlContextPool *p = VuoGlContextPool::getPool();
	return (VuoGlContext)p->use();
}

/**
 * Activates the specified @c VuoGlContext on the current thread.
 *
 * Can be called from any thread.
 */
void VuoGlContext_useSpecific(VuoGlContext glContext)
{
	CGLSetCurrentContext((CGLContextObj)glContext);
}

/**
 * Deactivates the current thread's GL context, and throws it back in the pool.
 */
void VuoGlContext_disuse(void)
{
	VuoGlContextPool *p = VuoGlContextPool::getPool();
	CGLContextObj cglContext = (CGLContextObj)CGLGetCurrentContext();
	glFlush();
	CGLFlushDrawable(cglContext);
	CGLSetCurrentContext(NULL);
	p->disuse(cglContext);
}

/**
 * Throws the specified @c VuoGlContext back in the pool.
 *
 * Can be called from any thread.
 */
void VuoGlContext_disuseSpecific(VuoGlContext glContext)
{
	VuoGlContextPool *p = VuoGlContextPool::getPool();
	CGLContextObj cglContext = (CGLContextObj)glContext;
	CGLContextObj cglContextCurrent = CGLGetCurrentContext();
	CGLSetCurrentContext(cglContext);
	glFlush();
	CGLFlushDrawable(cglContext);
	CGLSetCurrentContext(cglContextCurrent);
	p->disuse(cglContext);
}

/**
 * Helper for @c VGL().
 */
void _VGL(const char *file, const unsigned int line, const char *func)
{
	GLenum error = glGetError();
	if (error == GL_NO_ERROR)
		return;

	// Text from http://www.opengl.org/sdk/docs/man/xhtml/glGetError.xml
	const char *errorString = "(unknown)";
	if (error == GL_INVALID_ENUM)
		errorString = "GL_INVALID_ENUM (An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.)";
	else if (error == GL_INVALID_VALUE)
		errorString = "GL_INVALID_VALUE (A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.)";
	else if (error == GL_INVALID_OPERATION)
		errorString = "GL_INVALID_OPERATION (The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.)";
	else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
	{
		errorString = "GL_INVALID_FRAMEBUFFER_OPERATION (The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.)";
		fprintf(stderr, "# %s:%d :: %s() OpenGL error %d: %s\n", file, line, func, error, errorString);

		GLenum framebufferError = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		// Text from http://www.khronos.org/opengles/sdk/docs/man/xhtml/glCheckFramebufferStatus.xml
		const char *framebufferErrorString = "(unknown)";
		if (framebufferError == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
			framebufferErrorString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT (Not all framebuffer attachment points are framebuffer attachment complete. This means that at least one attachment point with a renderbuffer or texture attached has its attached object no longer in existence or has an attached image with a width or height of zero, or the color attachment point has a non-color-renderable image attached, or the depth attachment point has a non-depth-renderable image attached, or the stencil attachment point has a non-stencil-renderable image attached.)";
//		else if (framebufferError == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
//			framebufferErrorString = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS (Not all attached images have the same width and height.)";
		else if (framebufferError == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
			framebufferErrorString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT (No images are attached to the framebuffer.)";
		else if (framebufferError == GL_FRAMEBUFFER_UNSUPPORTED)
			framebufferErrorString = "GL_FRAMEBUFFER_UNSUPPORTED (The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.)";
		else if (framebufferError == GL_FRAMEBUFFER_COMPLETE)
			framebufferErrorString = "GL_FRAMEBUFFER_COMPLETE (?)";
		fprintf(stderr, "# %s:%d :: %s() OpenGL framebuffer error %d: %s\n", file, line, func, framebufferError, framebufferErrorString);

		return;
	}
	else if (error == GL_OUT_OF_MEMORY)
		errorString = "GL_OUT_OF_MEMORY (There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.)";
	else if (error == GL_STACK_UNDERFLOW)
		errorString = "GL_STACK_UNDERFLOW (An attempt has been made to perform an operation that would cause an internal stack to underflow.)";
	else if (error == GL_STACK_OVERFLOW)
		errorString = "GL_STACK_OVERFLOW (An attempt has been made to perform an operation that would cause an internal stack to overflow.)";

	fprintf(stderr, "# %s:%d :: %s() OpenGL error %d: %s\n", file, line, func, error, errorString);
}
