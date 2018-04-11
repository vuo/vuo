/**
 * @file
 * VuoGlContext implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <vector>
#include <algorithm>
#include <map>
using namespace std;

#include "module.h"
#include "VuoGlContext.h"
#include "VuoCglPixelFormat.h"
#include "VuoScreenCommon.h"

#include <OpenGL/CGLMacro.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

#include <dispatch/dispatch.h>
#include <mach-o/dyld.h>


static dispatch_once_t VuoGlContextPoolCreated = 0;	///< Make sure this process only has a single GL Context Pool.

/**
 * Logs info about all available renderers.
 */
static void VuoGlContext_renderers(void)
{
	VuoList_VuoScreen screensList = VuoScreen_getList();
	unsigned long screenCount = VuoListGetCount_VuoScreen(screensList);
	VuoScreen *screens = VuoListGetData_VuoScreen(screensList);
	VuoLocal(screensList);

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
		VUserLog("Renderer %d: %s", i, VuoCglRenderer_getText(rendererID));

		GLint online;
		if (CGLDescribeRenderer(ri, i, kCGLRPOnline, &online) == kCGLNoError)
			VUserLog("    Online             : %s", online ? "yes" : "no");

		GLint accelerated;
		if (CGLDescribeRenderer(ri, i, kCGLRPAccelerated, &accelerated) == kCGLNoError)
			VUserLog("    Accelerated        : %s", accelerated ? "yes" : "no");

		GLint videoMegabytes = 0;
		if (CGLDescribeRenderer(ri, i, kCGLRPVideoMemoryMegabytes, &videoMegabytes) == kCGLNoError
		 && videoMegabytes)
			VUserLog("    Video memory       : %d MB", videoMegabytes);

		GLint textureMegabytes = 0;
		if (CGLDescribeRenderer(ri, i, kCGLRPTextureMemoryMegabytes, &textureMegabytes) == kCGLNoError
		 && textureMegabytes)
			VUserLog("    Texture memory     : %d MB", textureMegabytes);

		GLint displayMask;
		if (CGLDescribeRenderer(ri, i, kCGLRPDisplayMask, &displayMask) == kCGLNoError)
		{
			VUserLog("    Display mask       : 0x%x%s", displayMask, displayMask==0xff ? " (any)" : "");
			if (displayMask != 0xff)
				for (unsigned long i = 0; i < screenCount; ++i)
					if (displayMask & screens[i].displayMask)
						VUserLog("                         %s", screens[i].name);
		}

		GLint glVersion = 0;
		if (CGLDescribeRenderer(ri, i, /*kCGLRPMajorGLVersion*/ (CGLRendererProperty)133, &glVersion) == kCGLNoError)
			VUserLog("    OpenGL version     : %d", glVersion);

		CGLPixelFormatObj pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(false, false, displayMask);
		if (pf != (CGLPixelFormatObj)-1)
		{
			CGLContextObj cgl_ctx;
			CGLError error = CGLCreateContext(pf, NULL, &cgl_ctx);
			if (error != kCGLNoError)
				VUserLog("    Error: %s\n", CGLErrorString(error));
			else
			{
				GLint maxTextureSize;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
				VUserLog("    OpenGL 2           : %s (%s) maxTextureSize=%d", glGetString(GL_RENDERER), glGetString(GL_VERSION), maxTextureSize);
				CGLDestroyContext(cgl_ctx);
			}
		}
		else
			VUserLog("    (Can't create an OpenGL 2 context on this renderer.)");

		pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(false, true, displayMask);
		if (pf != (CGLPixelFormatObj)-1)
		{
			CGLContextObj cgl_ctx;
			CGLError error = CGLCreateContext(pf, NULL, &cgl_ctx);
			if (error != kCGLNoError)
				VUserLog("    Error: %s\n", CGLErrorString(error));
			else
			{
				GLint maxTextureSize;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
				VUserLog("    OpenGL Core Profile: %s (%s) maxTextureSize=%d", glGetString(GL_RENDERER), glGetString(GL_VERSION), maxTextureSize);
				CGLDestroyContext(cgl_ctx);
			}
		}
		else
			VUserLog("    (Can't create an OpenGL Core Profile context on this renderer.)");

		GLint cl = 0;
		if (CGLDescribeRenderer(ri, i, kCGLRPAcceleratedCompute, &cl) == kCGLNoError)
			VUserLog("    OpenCL supported   : %s", cl ? "yes" : "no");
	}
	CGLDestroyRendererInfo(ri);


	const char *gldriver = "GLDriver";
	size_t gldriverLen = strlen(gldriver);
	for(unsigned int i = 0; i < _dyld_image_count(); ++i)
	{
		const char *dylibPath = _dyld_get_image_name(i);
		size_t len = strlen(dylibPath);
		// If the image name ends with "GLDriver"…
		if (strcmp(dylibPath + len - gldriverLen, gldriver) == 0)
		{
			// Trim off the path and the common "GLDriver" suffix.
			char *z = strdup(strrchr(dylibPath, '/')+1);
			z[strlen(z)-gldriverLen] = 0;
			VUserLog("Driver: %s", z);
			free(z);
		}
	}
}

/**
 * Called when display settings change (a display is plugged in or unplugged, resolution is changed, …).
 */
void VuoGlContext_reconfig(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo)
{
	if (!(flags & kCGDisplaySetModeFlag))
		return;

	VuoList_VuoScreen screensList = VuoScreen_getList();
	unsigned long screenCount = VuoListGetCount_VuoScreen(screensList);
	VuoScreen *screens = VuoListGetData_VuoScreen(screensList);
	VuoLocal(screensList);

	for (unsigned long i = 0; i < screenCount; ++i)
		if (screens[i].id == display)
			VUserLog("Display reconfigured: %s", screens[i].name);
}

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
//			VL();
//			VuoLog_backtrace();

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
					VUserLog("Error: Couldn't create a context.");
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
				VUserLog("Error: Disued context %p, which isn't in the global share pool.  I'm not going to muddy the waters.", context);
		}
		dispatch_semaphore_signal(poolSemaphore);
	}

	/**
	 * Logs a warning if the specified OpenGL `cap`ability doesn't have `value`.
	 */
	#define VuoGlContext_checkGL(cap, value) \
		do { \
			if (glIsEnabled(cap) != value) \
				VUserLog("Warning: Caller incorrectly left %s %s", #cap, value ? "disabled" : "enabled"); \
		} while (0)

	/**
	 * Logs a warning if the specified OpenGL `key` doesn't have `value`.
	 */
	#define VuoGlContext_checkGLInt(key, value) \
		do { \
			GLint actualValue; \
			glGetIntegerv(key, &actualValue); \
			if (actualValue != value) \
				VUserLog("Warning: Caller incorrectly left %s set to something other than %s", #key, #value); \
		} while (0)

	/**
	 * Executes code using the global OpenGL context.
	 *
	 * @threadAny
	 */
	void perform(void (^function)(CGLContextObj cgl_ctx))
	{
		CGLLockContext(rootContext);

		function(rootContext);

		// Ensure that `function` restored the standard OpenGL state.
		if (VuoIsDebugEnabled())
		{
			CGLContextObj cgl_ctx = rootContext;

			VuoGlContext_checkGLInt(GL_DEPTH_WRITEMASK,      true);
			VuoGlContext_checkGL   (GL_DEPTH_TEST,           false);

			VuoGlContext_checkGL   (GL_CULL_FACE,            true);
			VuoGlContext_checkGLInt(GL_CULL_FACE_MODE,       GL_BACK);

			VuoGlContext_checkGL   (GL_BLEND,                true);

			VuoGlContext_checkGLInt(GL_BLEND_SRC_RGB,        GL_ONE);
			VuoGlContext_checkGLInt(GL_BLEND_DST_RGB,        GL_ONE_MINUS_SRC_ALPHA);
			VuoGlContext_checkGLInt(GL_BLEND_SRC_ALPHA,      GL_ONE);
			VuoGlContext_checkGLInt(GL_BLEND_DST_ALPHA,      GL_ONE_MINUS_SRC_ALPHA);

			VuoGlContext_checkGLInt(GL_BLEND_EQUATION_RGB,   GL_FUNC_ADD);
			VuoGlContext_checkGLInt(GL_BLEND_EQUATION_ALPHA, GL_FUNC_ADD);

			VuoGlContext_checkGL(GL_SAMPLE_ALPHA_TO_COVERAGE, false);
			VuoGlContext_checkGL(GL_SAMPLE_ALPHA_TO_ONE,      false);

			/// @todo check other changes Vuo makes
		}

		CGLUnlockContext(rootContext);
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
		static dispatch_once_t info = 0;
		dispatch_once(&info, ^{
			if (VuoIsDebugEnabled())
			{
				VuoGlContext_renderers();

				CGDisplayRegisterReconfigurationCallback(VuoGlContext_reconfig, NULL);
			}
		});

		CGLPixelFormatObj pf;
		bool shouldDestroyPixelFormat = false;
		if (rootContext)
			pf = CGLGetPixelFormat(rootContext);
		else
		{
			pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(true, false, -1);
			shouldDestroyPixelFormat = true;
		}

		CGLContextObj context;
		{
			CGLError error = CGLCreateContext(pf, rootContext, &context);
			if (shouldDestroyPixelFormat)
				CGLDestroyPixelFormat(pf);
			if (error != kCGLNoError)
			{
				VUserLog("Error: %s\n", CGLErrorString(error));
				return NULL;
			}
		}

		if (VuoIsDebugEnabled())
		{
			GLint rendererID;
			CGLGetParameter(context, kCGLCPCurrentRendererID, &rendererID);
			VUserLog("Created OpenGL context %p on %s", context, VuoCglRenderer_getText(rendererID));
		}

		// https://developer.apple.com/library/content/technotes/tn2085/_index.html
		// But it doesn't seem to actually improve performance any on the various workloads I tried.
//		CGLEnable(context, kCGLCEMPEngine);

		// Set the context's default state to commonly-used values,
		// to hopefully minimize subsequent state changes.
		{
			CGLContextObj cgl_ctx = context;
			glDepthMask(true);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
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
 * On Mac, this should be a `CGLContext`.  The `CGLContext` must be unlocked when calling this function,
 * but after that you may lock it at any time (Vuo doesn't require it to be locked or unlocked).
 *
 * Must be called before any Vuo composition is loaded, and before any other @c VuoGlContext_* methods.
 *
 * @threadAny
 */
void VuoGlContext_setGlobalRootContext(void *rootContext)
{
	if (VuoGlContextPool::singletonInstance)
	{
		VUserLog("Error: Called after VuoGlContextPool was initialized.  Ignoring the new rootContext.");
		return;
	}

	CGLPixelFormatObj pf = CGLGetPixelFormat((CGLContextObj)rootContext);
	CGLError error = CGLCreateContext(pf, (CGLContextObj)rootContext, &VuoGlContextPool::rootContext);
	if (error != kCGLNoError)
	{
		VUserLog("Error: %s\n", CGLErrorString(error));
		return;
	}
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
 * Executes code using the global OpenGL context.
 *
 * \eg{
 * VuoGlContext_perform(^(CGLContextObj cgl_ctx){
 *     glClear(…);
 *     …
 * });
 * }
 */
void VuoGlContext_perform(void (^function)(CGLContextObj cgl_ctx))
{
	VuoGlContextPool *p = VuoGlContextPool::getPool();
	p->perform(function);
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
	{																																		\
		VuoLog(file, linenumber, func, #pname " (value %d) was still active when the context was disused. (This may result in leaks.)", value);	\
		VuoLog_backtrace();																													\
	}																																		\
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
	{																																		\
		VuoLog(file, linenumber, func, #pname " (texture %d on unit %d) was still active when the context was disused. (This may result in leaks.)", value, unit);	\
		VuoLog_backtrace();																													\
	}																																		\
}

/**
 * Helper for @ref VuoGlContext_disuse.
 */
void VuoGlContext_disuseF(VuoGlContext glContext, const char *file, const unsigned int linenumber, const char *func)
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
	for (GLint i=0; i<textureUnits; ++i)
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
 * Returns the maximum supported multisampling level for this GPU.
 *
 * Multisampling is known to break point rendering on some GPUs, so we intentionally disable it on those.
 */
int VuoGlContext_getMaximumSupportedMultisampling(VuoGlContext context)
{
	static GLint supportedSamples = 0;
	static dispatch_once_t multisamplingCheck = 0;
	dispatch_once(&multisamplingCheck, ^{
		GLint rendererID;
		CGLGetParameter((CGLContextObj)context, kCGLCPCurrentRendererID, &rendererID);
		rendererID &= kCGLRendererIDMatchingMask;

		CGLContextObj cgl_ctx = (CGLContextObj)context;
		const char *renderer = (const char *)glGetString(GL_RENDERER);

		if (rendererID == kCGLRendererIntelHD4000ID                     // https://b33p.net/kosada/node/8225#comment-31324
		 || rendererID == /*kCGLRendererIntelHD5000ID*/ 0x00024500      // https://b33p.net/kosada/node/10595
		 || strcmp(renderer, "NVIDIA GeForce 320M OpenGL Engine") == 0) // https://b33p.net/kosada/node/13477
			supportedSamples = 0;
		else
		{
			glGetIntegerv(GL_MAX_SAMPLES, &supportedSamples);
			if (supportedSamples == 1)
				supportedSamples = 0;
		}
	});
	return supportedSamples;
}

/**
 * Returns a platform-specific OpenGL pixelformat description.
 *
 * On Mac OS X, this is a @c CGLPixelFormatObj.
 *
 * @param hasDepthBuffer If true, the returned context will have a depth buffer.
 * @param openGL32Core If true, the returned context will be OpenGL 3.2 Core Profile.  If false, OpenGL 2.1.
 * @param displayMask If -1, the context will not be restricted by display.
 *                    If nonzero, the context will be restricted to the specified displays (`CGDisplayIDToOpenGLDisplayMask()`).
 *                    If 0xff, the context will use the Apple Software Renderer.
 * @return -1 if the displayMask is invalid.
 *         NULL if another error occurred.
 */
void *VuoGlContext_makePlatformPixelFormat(bool hasDepthBuffer, bool openGL32Core, GLint displayMask)
{
	// Check whether it's OK to use multisampling on this GPU.
	static dispatch_once_t multisamplingCheck = 0;
	static int multisample = 0;
	dispatch_once(&multisamplingCheck, ^{
					  // Create a temporary context so we can get the GPU renderer string.
					  CGLContextObj cgl_ctx;
					  {
						  CGLPixelFormatObj pf;
						  {
							  CGLPixelFormatAttribute pfa[14] = {
								  kCGLPFAAccelerated,
								  kCGLPFAAllowOfflineRenderers,
//								  kCGLPFANoRecovery,
//								  kCGLPFADoubleBuffer,
								  kCGLPFABackingVolatile,
								  kCGLPFAColorSize, (CGLPixelFormatAttribute) 24,
								  kCGLPFADepthSize, (CGLPixelFormatAttribute) (hasDepthBuffer ? 16 : 0),
								  (CGLPixelFormatAttribute) 0
							  };
							  GLint npix;
							  CGLError error = CGLChoosePixelFormat(pfa, &pf, &npix);
							  if (error != kCGLNoError)
							  {
								  VUserLog("Error: %s", CGLErrorString(error));
								  return;
							  }
						  }

						  CGLError error = CGLCreateContext(pf, NULL, &cgl_ctx);
						  CGLDestroyPixelFormat(pf);
						  if (error != kCGLNoError)
						  {
							  VUserLog("Error: %s\n", CGLErrorString(error));
							  return;
						  }
					  }


					  // If the user set the `multisample` preference, use it.
					  Boolean overridden = false;
					  multisample = (int)CFPreferencesGetAppIntegerValue(CFSTR("multisample"), CFSTR("org.vuo.Editor"), &overridden);

					  if (!overridden)
					  {
						  // …otherwise enable 4x multisampling (unless there's a known problem with this GPU model).

						  int supportedSamples = VuoGlContext_getMaximumSupportedMultisampling(cgl_ctx);
						  multisample = MIN(4, supportedSamples);
					  }

					  CGLDestroyContext(cgl_ctx);
				  });


	CGLPixelFormatAttribute pfa[18];
	int pfaIndex = 0;

	// If requesting a specific display, don't require acceleration.
	if (displayMask == -1)
		pfa[pfaIndex++] = kCGLPFAAccelerated;

	pfa[pfaIndex++] = kCGLPFAAllowOfflineRenderers;
//	pfa[pfaIndex++] = kCGLPFANoRecovery;

	// https://b33p.net/kosada/node/12525
//	pfa[pfaIndex++] = kCGLPFADoubleBuffer;
	pfa[pfaIndex++] = kCGLPFABackingVolatile;

	pfa[pfaIndex++] = kCGLPFAColorSize; pfa[pfaIndex++] = (CGLPixelFormatAttribute) 24;
	pfa[pfaIndex++] = kCGLPFADepthSize; pfa[pfaIndex++] = (CGLPixelFormatAttribute) (hasDepthBuffer ? 16 : 0);
	if (openGL32Core)
	{
		pfa[pfaIndex++] = kCGLPFAOpenGLProfile; pfa[pfaIndex++] = (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core;
	}

	if (multisample)
	{
		pfa[pfaIndex++] = kCGLPFAMultisample;
		pfa[pfaIndex++] = kCGLPFASampleBuffers; pfa[pfaIndex++] = (CGLPixelFormatAttribute) 1;
		pfa[pfaIndex++] = kCGLPFASamples;       pfa[pfaIndex++] = (CGLPixelFormatAttribute) multisample;
	}

	if (displayMask >= 0)
	{
		pfa[pfaIndex++] = kCGLPFADisplayMask;
		pfa[pfaIndex++] = (CGLPixelFormatAttribute) displayMask;
	}

	// software renderer
	if (displayMask == 0xff)
	{
		pfa[pfaIndex++] = kCGLPFARendererID;
		pfa[pfaIndex++] = (CGLPixelFormatAttribute) kCGLRendererGenericFloatID;
	}

	pfa[pfaIndex++] = (CGLPixelFormatAttribute) 0;

	CGLPixelFormatObj pf;
	GLint npix;
	CGLError error = CGLChoosePixelFormat(pfa, &pf, &npix);
	if (error == kCGLBadDisplay)
		return (CGLPixelFormatObj)-1;
	else if (error != kCGLNoError)
	{
		VUserLog("Error: %s", CGLErrorString(error));
		return NULL;
	}

	return (void *)pf;
}

void _VGL_describe(GLenum error, CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func);

/**
 * Helper for @c VGL().
 */
void _VGL(CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func)
{
	GLint vertexOnGPU, fragmentOnGPU;
	CGLGetParameter(cgl_ctx, kCGLCPGPUVertexProcessing, &vertexOnGPU);
	if (!vertexOnGPU)
		VuoLog(file, linenumber, func, "OpenGL warning: Falling back to software renderer for vertex shader.  This will slow things down.");
	CGLGetParameter(cgl_ctx, kCGLCPGPUFragmentProcessing, &fragmentOnGPU);
	if (!fragmentOnGPU)
		VuoLog(file, linenumber, func, "OpenGL warning: Falling back to software renderer for fragment shader.  This will slow things down.");

	bool foundError = false;
	do
	{
		GLenum error = glGetError();
		if (error == GL_NO_ERROR)
			break;
		_VGL_describe(error, cgl_ctx, file, linenumber, func);
		foundError = true;
	} while (true);

	if (foundError)
		VuoLog_backtrace();
}

/**
 * Logs text describing the specified OpenGL error.
 */
void _VGL_describe(GLenum error, CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func)
{
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
		VuoLog(file, linenumber, func, "OpenGL error %d: %s", error, errorString);

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
		else if (framebufferError == GL_FRAMEBUFFER_UNDEFINED)
			framebufferErrorString = "GL_FRAMEBUFFER_UNDEFINED";
		VuoLog(file, linenumber, func, "OpenGL framebuffer error %d: %s", framebufferError, framebufferErrorString);

		return;
	}
	else if (error == GL_OUT_OF_MEMORY)
		errorString = "GL_OUT_OF_MEMORY (There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.)";
	else if (error == GL_STACK_UNDERFLOW)
		errorString = "GL_STACK_UNDERFLOW (An attempt has been made to perform an operation that would cause an internal stack to underflow.)";
	else if (error == GL_STACK_OVERFLOW)
		errorString = "GL_STACK_OVERFLOW (An attempt has been made to perform an operation that would cause an internal stack to overflow.)";

	VuoLog(file, linenumber, func, "OpenGL error %d: %s", error, errorString);
	VuoLog_backtrace();
}

typedef std::map<VuoGlContext, GLuint> VuoShaderContextType; ///< Type for VuoShaderContextMap.
/**
 * The currently-active shader on each context.
 *
 * This is placed in VuoGlContext (a dylib) instead of VuoShader (a static module) to ensure there's only one instance per process.
 */
VuoShaderContextType VuoShaderContextMap;
