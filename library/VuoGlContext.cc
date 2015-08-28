/**
 * @file
 * VuoGlContext implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <vector>
#include <algorithm>
using namespace std;

#include "module.h"
#include "VuoGlContext.h"

#include <OpenGL/CGLMacro.h>

#include <dispatch/dispatch.h>


static dispatch_once_t VuoGlContextPoolCreated;	///< Make sure this process only has a single GL Context Pool.

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
		dispatch_once(&VuoGlContextPoolCreated, ^{
						   singletonInstance = new VuoGlContextPool();
					   });
		return singletonInstance;
	}

	/**
	 * Finds an unused GL context in the process-wide shared context pool (or creates one if none is available),
	 * marks it used, and returns it.
	 *
	 * @threadAny
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
//				VLog("Found existing context %p.", context);
			}
			else
			{
				context = createContext(rootContext);
				if (!context)
				{
					VLog("Error: Couldn't create a context.");
					return NULL;
				}
				allSharedContexts.push_back(context);
//				VLog("Created context %p.", context);
			}
		}
		dispatch_semaphore_signal(poolSemaphore);

		return context;
	}

	/**
	 * Throws the specified context back in the pool.
	 *
	 * @threadAny
	 */
	void disuse(CGLContextObj context)
	{
//		VLog("%p", context);

		dispatch_semaphore_wait(poolSemaphore, DISPATCH_TIME_FOREVER);
		{
			if (std::find(allSharedContexts.begin(), allSharedContexts.end(), context) != allSharedContexts.end())
				avaialbleSharedContexts.push_back(context);
			else
				VLog("Error: Disued context %p, which isn't in the global share pool.  I'm not going to muddy the waters.", context);
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

//		VLog("rootContext=%p", rootContext);
	}
	/// @todo provide a way to shut down entire system and release the contexts.

	CGLContextObj createContext(CGLContextObj rootContext)
	{
		CGLPixelFormatObj pf;
		if (rootContext)
			pf = CGLGetPixelFormat(rootContext);
		else
			pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(true);

		CGLContextObj context;
		{
			CGLError error = CGLCreateContext(pf, rootContext, &context);
			CGLDestroyPixelFormat(pf);
			if (error != kCGLNoError)
			{
				VLog("Error: %s\n", CGLErrorString(error));
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
 *
 * @threadAny
 */
void VuoGlContext_setGlobalRootContext(void *rootContext)
{
	if (VuoGlContextPool::singletonInstance)
	{
		VLog("Error: Called after VuoGlContextPool was initialized.  Ignoring the new rootContext.");
		return;
	}

	VuoGlContextPool::rootContext = (CGLContextObj)rootContext;
}

/**
 * Finds an unused GL context in the process-wide shared context pool (or creates one if none is available),
 * marks it used, and returns it.
 *
 * @threadAny
 */
VuoGlContext VuoGlContext_use(void)
{
	VuoGlContextPool *p = VuoGlContextPool::getPool();
	return (VuoGlContext)p->use();
}

/**
 * Check whether the specified attachment point @c pname is still bound.
 * (This is defined as a macro in order to stringify the argument.)
 */
#define VuoGlCheckBinding(pname)																						\
{																														\
	GLint value;																										\
	glGetIntegerv(pname, &value);																						\
	if (value)																											\
		VuoLog(file, line, func, #pname " (value %d) was still active when the context was disused. (This may result in leaks.)", value);	\
}

/**
 * Check whether the specified attachment point @c pname is still bound.
 * (This is defined as a macro in order to stringify the argument.)
 */
#define VuoGlCheckTextureBinding(pname, unit)																								\
{																																			\
	GLint value;																															\
	glGetIntegerv(pname, &value);																											\
	if (value)																																\
		VuoLog(file, line, func, #pname " (texture %d on unit %d) was still active when the context was disused. (This may result in leaks.)", value, unit);	\
}

/**
 * Helper for @ref VuoGlContext_disuse.
 */
void VuoGlContext_disuseF(VuoGlContext glContext, const char *file, const unsigned int line, const char *func)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

// Prior to https://b33p.net/kosada/node/6536.  Pretty sure there are no longer needed (FBOs should take care of their own glFlushRendererAPPLE() calls; visible contexts should take care of their own CGLFlushDrawable() calls).
//	glFlush();
//	CGLFlushDrawable(cgl_ctx);

#if 0
	VGL();

	// Check whether there are any stale bindings (to help identify leaks).
	// (Some checks are commented because they aren't available in OpenGL 2.1.)
	VuoGlCheckBinding(GL_ARRAY_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_ATOMIC_COUNTER_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_COPY_READ_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_COPY_WRITE_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_DRAW_INDIRECT_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_DISPATCH_INDIRECT_BUFFER_BINDING);
	VuoGlCheckBinding(GL_DRAW_FRAMEBUFFER_BINDING);
	VuoGlCheckBinding(GL_ELEMENT_ARRAY_BUFFER_BINDING);
	VuoGlCheckBinding(GL_FRAMEBUFFER_BINDING);
	VuoGlCheckBinding(GL_PIXEL_PACK_BUFFER_BINDING);
	VuoGlCheckBinding(GL_PIXEL_UNPACK_BUFFER_BINDING);
//	VuoGlCheckBinding(GL_PROGRAM_PIPELINE_BINDING);
	VuoGlCheckBinding(GL_READ_FRAMEBUFFER_BINDING);
	VuoGlCheckBinding(GL_RENDERBUFFER_BINDING);
//	VuoGlCheckBinding(GL_SAMPLER_BINDING);
//	VuoGlCheckBinding(GL_SHADER_STORAGE_BUFFER_BINDING);

	GLint textureUnits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &textureUnits);
	for (GLuint i=0; i<textureUnits; ++i)
	{
		glActiveTexture(GL_TEXTURE0+i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_1D,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_1D_ARRAY_EXT,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_2D,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_2D_ARRAY_EXT,i);
//		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_2D_MULTISAMPLE,i);
//		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_3D,i);
//		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_BUFFER,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_CUBE_MAP,i);
		VuoGlCheckTextureBinding(GL_TEXTURE_BINDING_RECTANGLE_ARB,i);
	}

	VuoGlCheckBinding(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_EXT);
	VuoGlCheckBinding(GL_UNIFORM_BUFFER_BINDING_EXT);
	VuoGlCheckBinding(GL_VERTEX_ARRAY_BINDING_APPLE);
#endif

	VuoGlContextPool *p = VuoGlContextPool::getPool();
	// Acquire semaphore and add the context to the pool.
	p->disuse(cgl_ctx);
}

/**
 * Returns a platform-specific OpenGL pixelformat description.
 *
 * On Mac OS X, this is a @c CGLPixelFormatObj.
 */
void *VuoGlContext_makePlatformPixelFormat(bool hasDepthBuffer)
{
	CGLPixelFormatAttribute pfa[] = {
		kCGLPFAAccelerated,
		kCGLPFAWindow,
		kCGLPFANoRecovery,
		kCGLPFADoubleBuffer,
		kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
		kCGLPFADepthSize, (CGLPixelFormatAttribute) (hasDepthBuffer ? 16 : 0),
		// Multisampling breaks point rendering on some GPUs.  https://b33p.net/kosada/node/8225#comment-31324
//		kCGLPFAMultisample,
//		kCGLPFASampleBuffers, (CGLPixelFormatAttribute) 1,
//		kCGLPFASamples, (CGLPixelFormatAttribute) 4,
		(CGLPixelFormatAttribute) 0
	};

	CGLPixelFormatObj pf;
	GLint npix;
	CGLError error = CGLChoosePixelFormat(pfa, &pf, &npix);
	if (error != kCGLNoError)
	{
		VLog("Error: %s", CGLErrorString(error));
		return NULL;
	}

	return (void *)pf;
}

/**
 * Helper for @c VGL().
 */
void _VGL(CGLContextObj cgl_ctx, const char *file, const unsigned int line, const char *func)
{
	GLint vertexOnGPU, fragmentOnGPU;
	CGLGetParameter(cgl_ctx, kCGLCPGPUVertexProcessing, &vertexOnGPU);
	if (!vertexOnGPU)
		VuoLog(file, line, func, "OpenGL warning: Falling back to software renderer for vertex shader.  This will slow things down.");
	CGLGetParameter(cgl_ctx, kCGLCPGPUFragmentProcessing, &fragmentOnGPU);
	if (!fragmentOnGPU)
		VuoLog(file, line, func, "OpenGL warning: Falling back to software renderer for fragment shader.  This will slow things down.");

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
		VuoLog(file, line, func, "OpenGL error %d: %s", error, errorString);

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
		VuoLog(file, line, func, "OpenGL framebuffer error %d: %s", framebufferError, framebufferErrorString);

		return;
	}
	else if (error == GL_OUT_OF_MEMORY)
		errorString = "GL_OUT_OF_MEMORY (There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.)";
	else if (error == GL_STACK_UNDERFLOW)
		errorString = "GL_STACK_UNDERFLOW (An attempt has been made to perform an operation that would cause an internal stack to underflow.)";
	else if (error == GL_STACK_OVERFLOW)
		errorString = "GL_STACK_OVERFLOW (An attempt has been made to perform an operation that would cause an internal stack to overflow.)";

	VuoLog(file, line, func, "OpenGL error %d: %s", error, errorString);
}
