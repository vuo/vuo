/**
 * @file
 * VuoCglPixelFormat implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCglPixelFormat.h"

#include <string.h>

#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoCglPixelFormat"
				 });
#endif


/**
 * Returns a string containing a verbal description of the specified CGLPixelFormatAttribute code.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoCglPixelFormat_getAttributeText(CGLPixelFormatAttribute a)
{
	if (a == kCGLPFAAllRenderers)						return strdup("kCGLPFAAllRenderers");
	if (a == kCGLPFATripleBuffer)						return strdup("kCGLPFATripleBuffer");
	if (a == kCGLPFADoubleBuffer)						return strdup("kCGLPFADoubleBuffer");
	if (a == kCGLPFAColorSize)							return strdup("kCGLPFAColorSize");
	if (a == kCGLPFAAlphaSize)							return strdup("kCGLPFAAlphaSize");
	if (a == kCGLPFADepthSize)							return strdup("kCGLPFADepthSize");
	if (a == kCGLPFAStencilSize)						return strdup("kCGLPFAStencilSize");
	if (a == kCGLPFAMinimumPolicy)						return strdup("kCGLPFAMinimumPolicy");
	if (a == kCGLPFAMaximumPolicy)						return strdup("kCGLPFAMaximumPolicy");
	if (a == kCGLPFASampleBuffers)						return strdup("kCGLPFASampleBuffers");
	if (a == kCGLPFASamples)							return strdup("kCGLPFASamples");
	if (a == kCGLPFAColorFloat)							return strdup("kCGLPFAColorFloat");
	if (a == kCGLPFAMultisample)						return strdup("kCGLPFAMultisample");
	if (a == kCGLPFASupersample)						return strdup("kCGLPFASupersample");
	if (a == kCGLPFASampleAlpha)						return strdup("kCGLPFASampleAlpha");
	if (a == kCGLPFARendererID)							return strdup("kCGLPFARendererID");
	if (a == kCGLPFANoRecovery)							return strdup("kCGLPFANoRecovery");
	if (a == kCGLPFAAccelerated)						return strdup("kCGLPFAAccelerated");
	if (a == kCGLPFAClosestPolicy)						return strdup("kCGLPFAClosestPolicy");
	if (a == kCGLPFABackingStore)						return strdup("kCGLPFABackingStore");
	if (a == 77 /*kCGLPFABackingVolatile*/)				return strdup("kCGLPFABackingVolatile");
	if (a == kCGLPFADisplayMask)						return strdup("kCGLPFADisplayMask");
	if (a == kCGLPFAAllowOfflineRenderers)				return strdup("kCGLPFAAllowOfflineRenderers");
	if (a == kCGLPFAAcceleratedCompute)					return strdup("kCGLPFAAcceleratedCompute");
	if (a == kCGLPFAOpenGLProfile)						return strdup("kCGLPFAOpenGLProfile");
	if (a == 101 /*kCGLPFASupportsAutomaticGraphicsSwitching*/)	return strdup("kCGLPFASupportsAutomaticGraphicsSwitching");
	if (a == kCGLPFAVirtualScreenCount)					return strdup("kCGLPFAVirtualScreenCount");
	if (a == kCGLPFAAuxBuffers)							return strdup("kCGLPFAAuxBuffers");
	if (a == kCGLPFAAccumSize)							return strdup("kCGLPFAAccumSize");
	if (a == kCGLPFAAuxDepthStencil)					return strdup("kCGLPFAAuxDepthStencil");
	if (a == kCGLPFAStereo)								return strdup("kCGLPFAStereo");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if (a == kCGLPFAOffScreen)							return strdup("kCGLPFAOffScreen");
#pragma clang diagnostic pop

	if (a == kCGLPFAWindow)								return strdup("kCGLPFAWindow");
	if (a == kCGLPFACompliant)							return strdup("kCGLPFACompliant");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if (a == kCGLPFAPBuffer)							return strdup("kCGLPFAPBuffer");
#pragma clang diagnostic pop

	if (a == kCGLPFARemotePBuffer)						return strdup("kCGLPFARemotePBuffer");
	if (a == kCGLPFASingleRenderer)						return strdup("kCGLPFASingleRenderer");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if (a == kCGLPFARobust)								return strdup("kCGLPFARobust");
	if (a == kCGLPFAMPSafe)								return strdup("kCGLPFAMPSafe");
	if (a == kCGLPFAMultiScreen)						return strdup("kCGLPFAMultiScreen");
	if (a == kCGLPFAFullScreen)							return strdup("kCGLPFAFullScreen");
#pragma clang diagnostic pop

	char *text;
	asprintf(&text, "unknown (%d)", a);
	return text;
}

/**
 * Logs the differences between two pixel formats.
 */
void VuoCglPixelFormat_logDiff(CGLPixelFormatObj a, CGLPixelFormatObj b)
{
	CGLPixelFormatAttribute pfas[] = {
		kCGLPFAAllRenderers,
		kCGLPFATripleBuffer,
		kCGLPFADoubleBuffer,
		kCGLPFAColorSize,
		kCGLPFAAlphaSize,
		kCGLPFADepthSize,
		kCGLPFAStencilSize,
		kCGLPFAMinimumPolicy,
		kCGLPFAMaximumPolicy,
		kCGLPFASampleBuffers,
		kCGLPFASamples,
		kCGLPFAColorFloat,
		kCGLPFAMultisample,
		kCGLPFASupersample,
		kCGLPFASampleAlpha,
		kCGLPFARendererID,
		kCGLPFANoRecovery,
		kCGLPFAAccelerated,
		kCGLPFAClosestPolicy,
		kCGLPFABackingStore,
		77, //kCGLPFABackingVolatile,
		kCGLPFADisplayMask,
		kCGLPFAAllowOfflineRenderers,
		kCGLPFAAcceleratedCompute,
		kCGLPFAOpenGLProfile,
		101, //kCGLPFASupportsAutomaticGraphicsSwitching,
//		kCGLPFAVirtualScreenCount,
		kCGLPFAAuxBuffers,
		kCGLPFAAccumSize,
		kCGLPFAAuxDepthStencil,
		kCGLPFAStereo,

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		kCGLPFAOffScreen,
#pragma clang diagnostic pop

		kCGLPFAWindow,
		kCGLPFACompliant,

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		kCGLPFAPBuffer,
#pragma clang diagnostic pop

		kCGLPFARemotePBuffer,
		kCGLPFASingleRenderer,

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		kCGLPFARobust,
		kCGLPFAMPSafe,
		kCGLPFAMultiScreen,
		kCGLPFAFullScreen,
#pragma clang diagnostic pop
	};
	int pfaCount = sizeof(pfas)/sizeof(pfas[0]);

	VUserLog("CGLPixelFormat differences:");
	bool different = false;
	for (int i = 0; i < pfaCount; ++i)
	{
		char *s = VuoCglPixelFormat_getAttributeText(pfas[i]);
		GLint valueA, valueB;
		CGLError error = CGLDescribePixelFormat(a, 0, pfas[i], &valueA);
		if (error != kCGLNoError)
		{
			VUserLog("	Error retrieving PFA %s: %s", s, CGLErrorString(error));
			free(s);
			continue;
		}
		error = CGLDescribePixelFormat(b, 0, pfas[i], &valueB);
		if (error != kCGLNoError)
		{
			VUserLog("	Error retrieving PFA %s: %s", s, CGLErrorString(error));
			free(s);
			continue;
		}

		if (valueA != valueB)
		{
			VUserLog("	%30s: %d vs %d", s, valueA, valueB);
			different = true;
		}

		free(s);
	}

	if (!different)
		VUserLog("	(none)");
}
