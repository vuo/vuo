/**
 * @file
 * vuo.image.mipmap node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include <OpenGL/CGLMacro.h>
#include <OpenGL/OpenGL.h>

#include "VuoGlPool.h"
#include "VuoImageWrapMode.h"

VuoModuleMetadata({
	"title" : "Improve Downscaling Quality",
	"keywords" : [
		"lod", "scale", "resize", "blur", "smooth",
		"size", "fix moire", "fix flicker",
	],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoGlPool",
		"OpenGL.framework"
	],
	"node" : {
		"exampleCompositions" : [ "CompareDownscalingForPerspective.vuo", "CompareDownscalingForResizing.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoImage) image,
	VuoOutputData(VuoImage) outputImage)
{
	if (!image)
	{
		*outputImage = NULL;
		return;
	}

	VuoImage img = VuoImage_makeCopy(image, false, 0, 0, false);
	VuoGlContext_perform(^(CGLContextObj cgl_ctx) {
		glBindTexture(img->glTextureTarget, img->glTextureName);
		glGenerateMipmap(img->glTextureTarget);
		glTexParameteri(img->glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(img->glTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2);
		glBindTexture(img->glTextureTarget, 0);
		glFlushRenderAPPLE();
	});
	*outputImage = img;
}
