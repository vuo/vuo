/**
 * @file
 * vuo.image.flip.vertical node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageWrapMode.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Change Wrap Mode",
					 "keywords" : [ "mirror", "rotate", "clamp", "edge", "clip", "transparent" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ]
				 });

struct nodeInstanceData
{
	VuoGlContext glContext;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoImageWrapMode, {"default":"repeat"}) wrapMode,
		VuoOutputData(VuoImage) outputImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoImage img = VuoImage_makeCopy(image);

	CGLContextObj cgl_ctx = (CGLContextObj)(*instance)->glContext;

	glBindTexture( img->glTextureTarget, img->glTextureName );

	GLint wrap = GL_CLAMP_TO_BORDER;

	switch(wrapMode)
	{
		case VuoImageWrapMode_None:
			wrap = GL_CLAMP_TO_BORDER;
			break;

		case VuoImageWrapMode_ClampEdge:
			wrap = GL_CLAMP_TO_EDGE;
			break;

		case VuoImageWrapMode_Repeat:
			wrap = GL_REPEAT;
			break;

		case VuoImageWrapMode_MirroredRepeat:
			wrap = GL_MIRRORED_REPEAT;
			break;
	}

	glTexParameterf( img->glTextureTarget, GL_TEXTURE_WRAP_S, wrap );
	glTexParameterf( img->glTextureTarget, GL_TEXTURE_WRAP_T, wrap );

	glBindTexture(GL_TEXTURE_2D, 0);

	// Ensure the command queue gets executed before we return,
	// since the VuoShader might immediately be used on another context.
	glFlushRenderAPPLE();

	*outputImage = img;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoGlContext_disuse((*instance)->glContext);
}
