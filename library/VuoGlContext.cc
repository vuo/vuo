/**
 * @file
 * VuoGlContext implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <vector>
#include <algorithm>
#include <bitset>
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
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <objc/objc-runtime.h>
#include <pthread.h>


static CGLContextObj VuoGlContext_create(CGLContextObj rootContext);
dispatch_semaphore_t VuoGlContext_poolSemaphore;  ///< Serializes access to @c VuoGlContext_root, @c VuoGlContext_allSharedContexts and @c VuoGlContext_avaialbleSharedContexts.
static CGLContextObj VuoGlContext_root = NULL;  ///< This process's global root context.
vector<CGLContextObj> VuoGlContext_allSharedContexts;  ///< All contexts created by @ref VuoGlContext_create (including those currently being used).
vector<CGLContextObj> VuoGlContext_avaialbleSharedContexts;  ///< All unused contexts created by @ref VuoGlContext_create.

static dispatch_once_t VuoGlContextPoolCreated = 0;	///< Make sure this process only has a single GL Context Pool.
static pthread_key_t VuoGlContextPerformKey; ///< Tracks whether perform is already in the current thread's callstack.

static bool VuoGlContext_infoLogging = true;  ///< Whether to log info about each OpenGL Renderer and Metal Device.

/**
 * Specifies whether to log general info about each OpenGL Renderer and Metal Device.
 * (Errors and warnings are always logged regardless of this value.)
 */
void VuoGlContext_setInfoLogging(bool enabled)
{
    VuoGlContext_infoLogging = enabled;
}

/**
 * Logs info about all available renderers.
 */
static void VuoGlContext_renderers(void)
{
	if (!VuoGlContext_infoLogging)
		return;

	VuoList_VuoScreen screensList = VuoScreen_getList();
	unsigned long screenCount = VuoListGetCount_VuoScreen(screensList);
	VuoScreen *screens = VuoListGetData_VuoScreen(screensList);
	VuoLocal(screensList);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
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
			VUserLog("    Display mask       : %s (0x%x)%s",
				std::bitset<32>(displayMask).to_string().c_str(),
				displayMask, (displayMask & 0xff) == 0xff ? " (any)" : "");
			if ((displayMask & 0xff) != 0xff)
				for (unsigned long i = 0; i < screenCount; ++i)
					if (displayMask & screens[i].displayMask)
						VUserLog("                         %s %s",
							std::bitset<32>(screens[i].displayMask).to_string().c_str(),
							screens[i].name);
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
				VUserLog("    Error: %s", CGLErrorString(error));
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
				VUserLog("    Error: %s", CGLErrorString(error));
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
	}
	CGLDestroyRendererInfo(ri);
#pragma clang diagnostic pop


	typedef void *(*mtlCopyAllDevicesType)(void);
	mtlCopyAllDevicesType mtlCopyAllDevices = (mtlCopyAllDevicesType)dlsym(RTLD_DEFAULT, "MTLCopyAllDevices");
	if (mtlCopyAllDevices)
	{
		CFArrayRef mtlDevices = (CFArrayRef)mtlCopyAllDevices();
		int mtlDeviceCount = CFArrayGetCount(mtlDevices);
		if (mtlDeviceCount)
			VUserLog("Metal devices:");
		for (int i = 0; i < mtlDeviceCount; ++i)
		{
			id dev = (id)CFArrayGetValueAtIndex(mtlDevices, i);
			id devName = ((id (*)(id, SEL))objc_msgSend)(dev, sel_getUid("name"));
			const char *devNameZ = ((char * (*)(id, SEL))objc_msgSend)(devName, sel_getUid("UTF8String"));
			VUserLog("    %s (%s):", devNameZ, class_getName(object_getClass(dev)));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("registryID")))
			VUserLog("        ID                                  : %p", ((id (*)(id, SEL))objc_msgSend)(dev, sel_getUid("registryID")));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("location")))
			{
			unsigned long location = ((unsigned long (*)(id, SEL))objc_msgSend)(dev, sel_getUid("location"));
			VUserLog("        Location                            : %s (ID %lu)",
				location == 0 ? "built-in" : (location == 1 ? "slot" : (location == 2 ? "external" : "unspecified")),
				((unsigned long (*)(id, SEL))objc_msgSend)(dev, sel_getUid("locationNumber")));
			}
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("peerGroupID")))
			VUserLog("        Peer group                          : %llu (ID %u of %u)",
				((uint64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("peerGroupID")),
				((uint32_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("peerIndex")),
				((uint32_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("peerCount")));

			if (class_respondsToSelector(object_getClass(dev), sel_getUid("recommendedMaxWorkingSetSize")))
			VUserLog("        Recommended max working-set size    : %lld MiB", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("recommendedMaxWorkingSetSize"))/1048576);
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("maxBufferLength")))
			VUserLog("        Max buffer length                   : %lld MiB", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("maxBufferLength"))/1048576);
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("maxThreadgroupMemoryLength")))
			VUserLog("        Threadgroup memory                  : %lld B", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("maxThreadgroupMemoryLength")));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("sparseTileSizeInBytes")))
			VUserLog("        Sparse tile size                    : %llu B", ((uint64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("sparseTileSizeInBytes")));

			VUserLog("        Low-power                           : %s", ((bool (*)(id, SEL))objc_msgSend) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("isRemovable")))
			VUserLog("        Removable                           : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("isRemovable")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("hasUnifiedMemory")))
			VUserLog("        Unified memory                      : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("hasUnifiedMemory")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("maxTransferRate")))
			{
			uint64_t maxTransferRate = ((uint64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("maxTransferRate"));
			if (maxTransferRate)
			VUserLog("        Max CPU-GPU RAM transfer rate       : %llu MiB/sec", maxTransferRate/1024/1024);
			}
			VUserLog("        Headless                            : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("isHeadless")) ? "yes" : "no");

			if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFeatureSet:"), 10005))
			VUserLog("        Feature set                         : GPU Family 2 v1");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFeatureSet:"), 10004))
			VUserLog("        Feature set                         : GPU Family 1 v4");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFeatureSet:"), 10003))
			VUserLog("        Feature set                         : GPU Family 1 v3");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFeatureSet:"), 10001))
			VUserLog("        Feature set                         : GPU Family 1 v2");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFeatureSet:"), 10000))
			VUserLog("        Feature set                         : GPU Family 1 v1");
			else
			VUserLog("        Feature set                         : (unknown)");

			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsFamily:")))
			{
			// "A higher GPU version is always a superset of an earlier version in the same GPU family."
			// https://developer.apple.com/documentation/metal/mtldevice/detecting_gpu_features_and_metal_software_versions
			if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1008))
			VUserLog("        Family                              : Apple 8");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1007))
			VUserLog("        Family                              : Apple 7");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1006))
			VUserLog("        Family                              : Apple 6");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1005))
			VUserLog("        Family                              : Apple 5");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1004))
			VUserLog("        Family                              : Apple 4");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1003))
			VUserLog("        Family                              : Apple 3");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1002))
			VUserLog("        Family                              : Apple 2");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 1001))
			VUserLog("        Family                              : Apple 1");

			if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 2002))
			VUserLog("        Family                              : Mac 2");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 2001))
			VUserLog("        Family                              : Mac 1");

			if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 3003))
			VUserLog("        Family                              : Common 3");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 3002))
			VUserLog("        Family                              : Common 2");
			else if (((bool (*)(id, SEL, int))objc_msgSend)(dev, sel_getUid("supportsFamily:"), 3001))
			VUserLog("        Family                              : Common 1");
			}

			if (class_respondsToSelector(object_getClass(dev), sel_getUid("readWriteTextureSupport")))
			VUserLog("        Read-write texture support tier     : %lld", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("readWriteTextureSupport")));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("argumentBuffersSupport")))
			VUserLog("        Argument buffer support tier        : %lld", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("argumentBuffersSupport")));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("maxArgumentBufferSamplerCount")))
			VUserLog("        Max argument buffers                : %lld", ((int64_t (*)(id, SEL))objc_msgSend)(dev, sel_getUid("maxArgumentBufferSamplerCount")));
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("areProgrammableSamplePositionsSupported")))
			VUserLog("        Programmable sample position support: %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("areProgrammableSamplePositionsSupported")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("areRasterOrderGroupsSupported")))
			VUserLog("        Raster order group support          : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("areRasterOrderGroupsSupported")) ? "yes" : "no");

			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsDynamicLibraries")))
			VUserLog("        Dynamic library support             : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsDynamicLibraries")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsFunctionPointers")))
			VUserLog("        Function pointer support            : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsFunctionPointers")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsRaytracing")))
			VUserLog("        Raytracing support                  : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsRaytracing")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsShaderBarycentricCoordinates")))
			VUserLog("        Barycentric support                 : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsShaderBarycentricCoordinates")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsPrimitiveMotionBlur")))
			VUserLog("        Motion blur support                 : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsPrimitiveMotionBlur")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsPullModelInterpolation")))
			VUserLog("        Multiple interpolation support      : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsPullModelInterpolation")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supports32BitFloatFiltering")))
			VUserLog("        32-bit float support                : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supports32BitFloatFiltering")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supports32BitMSAA")))
			VUserLog("        32-bit MSAA support                 : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supports32BitMSAA")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsQueryTextureLOD")))
			VUserLog("        LOD query support                   : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsQueryTextureLOD")) ? "yes" : "no");
			if (class_respondsToSelector(object_getClass(dev), sel_getUid("supportsBCTextureCompression")))
			VUserLog("        BC texture support                  : %s", ((bool (*)(id, SEL))objc_msgSend)(dev, sel_getUid("supportsBCTextureCompression")) ? "yes" : "no");

			if (class_respondsToSelector(object_getClass(dev), sel_getUid("counterSets")))
			{
			id counterSets = ((id (*)(id, SEL))objc_msgSend)(dev, sel_getUid("counterSets"));
			id description = ((id (*)(id, SEL))objc_msgSend)(counterSets, sel_getUid("description"));
			if (description)
			{
			string descriptionS{((const char * (*)(id, SEL))objc_msgSend)(description, sel_getUid("UTF8String"))};
			std::replace(descriptionS.begin(), descriptionS.end(), '\n', ' ');
			VUserLog("        Counters                            : %s", descriptionS.c_str());
			}
			}
		}
		CFRelease(mtlDevices);
	}
}

	/**
	 * Returns the process-wide pool singleton instance.
	 */
	static void VuoGlContext_init()
	{
		dispatch_once(&VuoGlContextPoolCreated, ^{
						   VuoGlContext_poolSemaphore = dispatch_semaphore_create(1);

						   if (!VuoGlContext_root)
							   VuoGlContext_root = VuoGlContext_create(NULL);

						   int ret = pthread_key_create(&VuoGlContextPerformKey, NULL);
						   if (ret)
							   VUserLog("Couldn't create the key for storing the GL Context state: %s", strerror(errno));
					   });
	}

	/**
	 * Finds an unused GL context in the process-wide shared context pool (or creates one if none is available),
	 * marks it used, and returns it.
	 *
	 * @threadAny
	 */
	VuoGlContext VuoGlContext_use(void)
	{
		VuoGlContext_init();

		CGLContextObj context;

		dispatch_semaphore_wait(VuoGlContext_poolSemaphore, DISPATCH_TIME_FOREVER);
		{
//			VL();
//			VuoLog_backtrace();

			if (VuoGlContext_avaialbleSharedContexts.size())
			{
				context = VuoGlContext_avaialbleSharedContexts.back();
				VuoGlContext_avaialbleSharedContexts.pop_back();
//				VLog("Found existing context %p.", context);
			}
			else
			{
				context = VuoGlContext_create(VuoGlContext_root);
				if (!context)
				{
					VUserLog("Error: Couldn't create a context.");
					return NULL;
				}
				VuoGlContext_allSharedContexts.push_back(context);
//				VLog("Created context %p.", context);
			}
		}
		dispatch_semaphore_signal(VuoGlContext_poolSemaphore);

		return context;
	}

	/**
	 * Throws the specified context back in the pool.
	 *
	 * @threadAny
	 */
	void VuoGlContext_disuseF(VuoGlContext glContext, const char *file, const unsigned int linenumber, const char *func)
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;
//		VLog("%p", context);

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

		dispatch_semaphore_wait(VuoGlContext_poolSemaphore, DISPATCH_TIME_FOREVER);
		{
			if (std::find(VuoGlContext_allSharedContexts.begin(), VuoGlContext_allSharedContexts.end(), cgl_ctx) != VuoGlContext_allSharedContexts.end())
				VuoGlContext_avaialbleSharedContexts.push_back(cgl_ctx);
			else
				VUserLog("Error: Disued context %p, which isn't in the global share pool.  I'm not going to muddy the waters.", cgl_ctx);
		}
		dispatch_semaphore_signal(VuoGlContext_poolSemaphore);
	}

	/**
	 * Logs a warning if the specified OpenGL `cap`ability doesn't have `value`.
	 */
	#define VuoGlContext_checkGL(cap, value) \
		do { \
			if (glIsEnabled(cap) != value) \
			{ \
				VUserLog("Warning: Caller incorrectly left %s %s", #cap, value ? "disabled" : "enabled"); \
				VuoLog_backtrace(); \
			} \
		} while (0)

	/**
	 * Logs a warning if the specified OpenGL `key` doesn't have `value`.
	 */
	#define VuoGlContext_checkGLInt(key, value) \
		do { \
			GLint actualValue; \
			glGetIntegerv(key, &actualValue); \
			if (actualValue != value) \
			{ \
				VUserLog("Warning: Caller incorrectly left %s set to something other than %s", #key, #value); \
				VuoLog_backtrace(); \
			} \
		} while (0)



	/**
	 * Executes code using the global OpenGL context.
	 *
	 * \eg{
	 * VuoGlContext_perform(^(CGLContextObj cgl_ctx){
	 *     glClear(…);
	 *     …
	 * });
	 * }
	 *
	 * @threadAny
	 */
	void VuoGlContext_perform(void (^function)(CGLContextObj cgl_ctx))
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		if (!function)
			return;

		VuoGlContext_init();

		bool alreadyLockedOnThisThread = (bool)pthread_getspecific(VuoGlContextPerformKey);

		if (!alreadyLockedOnThisThread)
		{
			pthread_setspecific(VuoGlContextPerformKey, (void *)true);
			CGLLockContext(VuoGlContext_root);
		}

		function(VuoGlContext_root);

		// Ensure that `function` restored the standard OpenGL state.
		if (!alreadyLockedOnThisThread && VuoIsDebugEnabled())
		{
			CGLContextObj cgl_ctx = VuoGlContext_root;

			VGL();

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

		if (!alreadyLockedOnThisThread)
		{
			CGLUnlockContext(VuoGlContext_root);
			pthread_setspecific(VuoGlContextPerformKey, (void *)false);
		}
#pragma clang diagnostic pop
	}

	/**
	 * Creates a new OpenGL context, optionally shared with `rootContext`.
	 */
	static CGLContextObj VuoGlContext_create(CGLContextObj rootContext)
	{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		static dispatch_once_t info = 0;
		dispatch_once(&info, ^{
			VuoGlContext_renderers();

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				// Start logging display changes.
				VuoScreen_use();
			});
		});

		CGLPixelFormatObj pf;
		bool shouldDestroyPixelFormat = false;
		GLint displayMask;
		if (rootContext)
			pf = CGLGetPixelFormat(rootContext);
		else
		{
			Boolean overridden = false;
			displayMask = (int)CFPreferencesGetAppIntegerValue(CFSTR("displayMask"), CFSTR("org.vuo.Editor"), &overridden);
			if (!overridden)
			{
				// Maybe the preference is a hex string…
				auto displayMaskString = (CFStringRef)CFPreferencesCopyAppValue(CFSTR("displayMask"), CFSTR("org.vuo.Editor"));
				if (displayMaskString)
				{
					VuoText t = VuoText_makeFromCFString(displayMaskString);
					CFRelease(displayMaskString);
					VuoLocal(t);
					overridden = sscanf(t, "0x%x", &displayMask) == 1;
				}
				if (!overridden)
					// Still no preference, so let macOS automatically choose what it thinks is the best GPU.
					displayMask = -1;
			}

			pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(true, false, displayMask);
			shouldDestroyPixelFormat = true;
		}

		CGLContextObj context;
		{
			CGLError error = CGLCreateContext(pf, rootContext, &context);
			if (shouldDestroyPixelFormat)
				CGLDestroyPixelFormat(pf);
			if (error != kCGLNoError)
			{
				VUserLog("Error: %s", CGLErrorString(error));
				return NULL;
			}
		}

		if (VuoGlContext_infoLogging)
		{
			GLint rendererID;
			CGLGetParameter(context, kCGLCPCurrentRendererID, &rendererID);
			char *sharingText = rootContext ? VuoText_format(" (shared with %p)", rootContext) : strdup(" (not shared)");
			char *displayMaskText = rootContext ? nullptr : (displayMask == -1 ? strdup(" (macOS default)") : VuoText_format(" (selected using displayMask %s)", std::bitset<32>(displayMask).to_string().c_str()));
			VUserLog("Created OpenGL context %p%s on %s%s", context, sharingText, VuoCglRenderer_getText(rendererID), displayMaskText);
			free(sharingText);
			free(displayMaskText);
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
			glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
			glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -.5);
		}

		return context;
#pragma clang diagnostic pop
	}

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
	if (VuoGlContextPoolCreated)
	{
		VUserLog("Error: Called after VuoGlContextPool was initialized.  Ignoring the new rootContext.");
		return;
	}

	VUserLog("Setting global root context to %p", rootContext);
	VuoGlContext_root = VuoGlContext_create((CGLContextObj)rootContext);
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
		VuoLog(VuoLog_moduleName, file, linenumber, func, #pname " (value %d) was still active when the context was disused. (This may result in leaks.)", value);	\
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
		VuoLog(VuoLog_moduleName, file, linenumber, func, #pname " (texture %d on unit %d) was still active when the context was disused. (This may result in leaks.)", value, unit);	\
		VuoLog_backtrace();																													\
	}																																		\
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		CGLGetParameter((CGLContextObj)context, kCGLCPCurrentRendererID, &rendererID);
#pragma clang diagnostic pop
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
 *                    If the low byte is 0xff, the context will use the Apple Software Renderer.
 * @return -1 if the displayMask is invalid.
 *         NULL if another error occurred.
 */
void *VuoGlContext_makePlatformPixelFormat(bool hasDepthBuffer, bool openGL32Core, GLint displayMask)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
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
							  VUserLog("Error: %s", CGLErrorString(error));
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
	if ((displayMask & 0xff) == 0xff && displayMask != -1)
	{
		pfa[pfaIndex++] = kCGLPFARendererID;
		pfa[pfaIndex++] = (CGLPixelFormatAttribute) kCGLRendererGenericFloatID;
	}

	pfa[pfaIndex] = (CGLPixelFormatAttribute) 0;

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
#pragma clang diagnostic pop
}

/**
 * Returns true if the specified `context` is OpenGL 3.2+ Core Profile.
 * @version200New
 */
bool VuoGlContext_isOpenGL32Core(VuoGlContext context)
{
	CGLContextObj cgl_ctx = (CGLContextObj)context;

	// GL_VERSION is something like:
	//    - Core Profile:          `4.1 NVIDIA-10.17.5 355.10.05.45f01`
	//    - Core Profile:          `4.1 ATI-2.4.10`
	//    - Compatibility Profile: `2.1 NVIDIA-10.17.5 355.10.05.45f01`
	//    - Compatibility Profile: `2.1 ATI-2.4.10`
	const unsigned char *contextVersion = glGetString(GL_VERSION);
	return contextVersion[0] == '3'
		|| contextVersion[0] == '4';
}


void _VGL_describe(GLenum error, CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func);

/**
 * Helper for @c VGL().
 */
void _VGL(CGLContextObj cgl_ctx, const char *file, const unsigned int linenumber, const char *func)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	GLint vertexOnGPU, fragmentOnGPU;
	CGLGetParameter(cgl_ctx, kCGLCPGPUVertexProcessing, &vertexOnGPU);
	if (!vertexOnGPU)
		VuoLog(VuoLog_moduleName, file, linenumber, func, "OpenGL warning: Falling back to software renderer for vertex shader.  This will slow things down.");
	CGLGetParameter(cgl_ctx, kCGLCPGPUFragmentProcessing, &fragmentOnGPU);
	if (!fragmentOnGPU)
		VuoLog(VuoLog_moduleName, file, linenumber, func, "OpenGL warning: Falling back to software renderer for fragment shader.  This will slow things down.");
#pragma clang diagnostic pop

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
		VuoLog(VuoLog_moduleName, file, linenumber, func, "OpenGL error %d: %s", error, errorString);

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
		VuoLog(VuoLog_moduleName, file, linenumber, func, "OpenGL framebuffer error %d: %s", framebufferError, framebufferErrorString);

		return;
	}
	else if (error == GL_OUT_OF_MEMORY)
		errorString = "GL_OUT_OF_MEMORY (There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.)";
	else if (error == GL_STACK_UNDERFLOW)
		errorString = "GL_STACK_UNDERFLOW (An attempt has been made to perform an operation that would cause an internal stack to underflow.)";
	else if (error == GL_STACK_OVERFLOW)
		errorString = "GL_STACK_OVERFLOW (An attempt has been made to perform an operation that would cause an internal stack to overflow.)";

	VuoLog(VuoLog_moduleName, file, linenumber, func, "OpenGL error %d: %s", error, errorString);
	VuoLog_backtrace();
}

typedef std::map<VuoGlContext, GLuint> VuoShaderContextType; ///< Type for VuoShaderContextMap.
/**
 * The currently-active shader on each context.
 *
 * This is placed in VuoGlContext (a dylib) instead of VuoShader (a static module) to ensure there's only one instance per process.
 */
VuoShaderContextType VuoShaderContextMap;
