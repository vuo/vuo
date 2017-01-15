/**
 * @file
 * vuo.image.crop.pixels node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Crop Image Pixels",
					  "keywords" : [ "resize", "snip", "clip", "sample", "rectangle", "trim" ],
					  "version" : "1.1.1",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

static const char * cropFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float x;
	uniform float y;
	uniform float width;
	uniform float height;

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy * vec2(width, height) + vec2(x,y);
		gl_FragColor = texture2D(texture, pos);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Crop Image Pixels Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, cropFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0}, "suggestedMin":{"x":0,"y":0}}) topLeft,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":0}) width,
		VuoInputData(VuoInteger, {"default":256, "suggestedMin":0}) height,
		VuoOutputData(VuoImage) croppedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;
	float uv_x = topLeft.x/(float)w;
	float uv_w = width/(float)w;
	float uv_h = height/(float)h;
	float uv_y = (1-topLeft.y/(float)h)-uv_h;

	if(uv_x+uv_w > 1)
	{
		uv_w = 1-uv_x;
		w = round(uv_w * image->pixelsWide);
	}
	else
		w = width;

	if (uv_y < 0)
	{
		uv_h += uv_y;
		uv_y = 0;
		h = round(uv_h * image->pixelsHigh);
	}
	else
		h = height;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "x",       uv_x);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "y",       uv_y);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "width",   uv_w);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "height",  uv_h);

	*croppedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
