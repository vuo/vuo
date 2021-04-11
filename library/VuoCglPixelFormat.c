/**
 * @file
 * VuoCglPixelFormat implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
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
	if (a == kCGLPFAOffScreen)							return strdup("kCGLPFAOffScreen");
	if (a == kCGLPFAWindow)								return strdup("kCGLPFAWindow");
	if (a == kCGLPFACompliant)							return strdup("kCGLPFACompliant");
	if (a == kCGLPFAPBuffer)							return strdup("kCGLPFAPBuffer");
	if (a == kCGLPFARemotePBuffer)						return strdup("kCGLPFARemotePBuffer");
	if (a == kCGLPFASingleRenderer)						return strdup("kCGLPFASingleRenderer");
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
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
		kCGLPFAOffScreen,
		kCGLPFAWindow,
		kCGLPFACompliant,
		kCGLPFAPBuffer,
		kCGLPFARemotePBuffer,
		kCGLPFASingleRenderer,
		kCGLPFARobust,
		kCGLPFAMPSafe,
		kCGLPFAMultiScreen,
		kCGLPFAFullScreen,
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
#pragma clang diagnostic pop
}

/**
 * Returns a string containing a verbal description of the specified OpenGL rendererID (`kCGLCPCurrentRendererID`).
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoCglRenderer_getText(int rendererID)
{
	rendererID &= kCGLRendererIDMatchingMask;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if      (rendererID == kCGLRendererGenericFloatID          ) return strdup("Apple Software Renderer (GenericFloat)");
	else if (rendererID == kCGLRendererAppleSWID               ) return strdup("Apple Software Renderer (AppleSW)");
	else if (rendererID == kCGLRendererATIRage128ID            ) return strdup("ATI Rage 128");
	else if (rendererID == kCGLRendererATIRadeonID             ) return strdup("ATI Radeon");
	else if (rendererID == kCGLRendererATIRageProID            ) return strdup("ATI Rage Pro");
	else if (rendererID == kCGLRendererATIRadeon8500ID         ) return strdup("ATI Radeon 8500");
	else if (rendererID == kCGLRendererATIRadeon9700ID         ) return strdup("ATI Radeon 9700");
	else if (rendererID == kCGLRendererATIRadeonX1000ID        ) return strdup("ATI Radeon X1xxx");
	else if (rendererID == kCGLRendererATIRadeonX2000ID        ) return strdup("ATI Radeon HD 2xxx/4xxx");
	else if (rendererID == kCGLRendererATIRadeonX3000ID        ) return strdup("ATI Radeon HD 5xxx/6xxx");
	else if (rendererID == kCGLRendererATIRadeonX4000ID        ) return strdup("ATI Radeon HD 7xxx");
	else if (rendererID == 0x21d00                             ) return strdup("AMD Radeon X5000"); // unconfirmed
	else if (rendererID == 0x21e00                             ) return strdup("AMD Radeon X6000");
	else if (rendererID == kCGLRendererGeForce2MXID            ) return strdup("NVIDIA GeForce 2MX/4MX");
	else if (rendererID == kCGLRendererGeForce3ID              ) return strdup("NVIDIA GeForce 3/4Ti");
	else if (rendererID == kCGLRendererGeForceFXID             ) return strdup("NVIDIA GeForce 5xxx/6xxx/7xxx or Quadro FX 4500");
	else if (rendererID == kCGLRendererGeForce8xxxID           ) return strdup("NVIDIA GeForce 8xxx/9xxx/1xx/2xx/3xx or Quadro 4800");
	else if (rendererID == kCGLRendererGeForceID               ) return strdup("NVIDIA GeForce 6xx or Quadro 4000/K5000");
	else if (rendererID == kCGLRendererVTBladeXP2ID            ) return strdup("VTBladeXP2");
	else if (rendererID == kCGLRendererIntel900ID              ) return strdup("Intel GMA 950");
	else if (rendererID == kCGLRendererIntelX3100ID            ) return strdup("Intel X3100");
	else if (rendererID == kCGLRendererIntelHDID               ) return strdup("Intel HD 3000");
	else if (rendererID == kCGLRendererIntelHD4000ID           ) return strdup("Intel HD 4000");
	else if (rendererID == kCGLRendererIntelHD5000ID           ) return strdup("Intel HD 5000 (Iris)");
	else if (rendererID == kCGLRendererMesa3DFXID              ) return strdup("Mesa 3DFX");
#pragma clang diagnostic pop

	char *t;
	asprintf(&t, "unknown (0x%x)", rendererID);
	return t;
}
