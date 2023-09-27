/**
 * @file
 * vuo.image.wrapMode node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageWrapMode.h"
#include "VuoGlPool.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Change Wrap Mode",
					 "keywords" : [ "mirror", "rotate", "clamp", "edge", "clip", "transparent", "tile" ],
					 "version" : "2.0.1",
					 "dependencies" : [
						 "VuoGlPool",
						 "OpenGL.framework"
					 ],
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoImageWrapMode, {"default":"repeat"}) wrapMode,
		VuoOutputData(VuoImage) outputImage
)
{
	if (!image)
	{
		*outputImage = NULL;
		return;
	}

	bool forceAlpha = false;
	if (wrapMode == VuoImageWrapMode_None)
		// The OpenGL 3.3 spec says:
		// "the values of TEXTURE_BORDER_COLOR are interpreted as an RGBA color to match the texture’s internal format".
		// For GL_RGB textures, the GL_TEXTURE_BORDER_COLOR's alpha channel is ignored (treated as fully opaque).
		// So, use a texture with an alpha channel, to ensure the samples are transparent
		// when a downstream node samples outside the texture's domain of definition.
		forceAlpha = true;

	VuoImage img = VuoImage_makeCopy(image, false, 0, 0, forceAlpha);
	VuoImage_setWrapMode(img, wrapMode);
	*outputImage = img;
}
