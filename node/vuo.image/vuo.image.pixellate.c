/**
 * @file
 * vuo.image.pixellate node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Pixellate Image",
					 "keywords" : [ "pixels", "lofi", "simplify", "cube", "square", "filter", "overenlarge", "mosaic", "censor",
						 "pixelate" // American spelling
					 ],
					 "version" : "1.1.1",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 viewportSize;
	uniform vec2 pixelSize;
	uniform vec2 center;

	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy;

		// Quantize the texture coordinate so it lands exactly on an output pixel,
		// since mod()ding near a big pixel boundary
		// can exaggerate the GPU's imprecise floating point math.
		pos = vec2(int(pos.x*viewportSize.x)/viewportSize.x,
				   int(pos.y*viewportSize.y)/viewportSize.y);

		vec2 centerOffset = mod(center - pixelSize/2., pixelSize);
		vec2 distanceFromCorner = mod(pos - centerOffset, pixelSize);
		pos = pos - distanceFromCorner + pixelSize/2.;
		gl_FragColor = VuoGlsl_sample(texture, pos);
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

	instance->shader = VuoShader_make("Pixellation Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.05}) pixelSize,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoOutputData(VuoImage) pixellatedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoPoint2d pixelSize2d = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), VuoShader_samplerSizeFromVuoSize(VuoReal_makeNonzero(pixelSize)));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "pixelSize",  pixelSize2d);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	*pixellatedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
