/**
 * @file
 * vuo.image.pixellate node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
					 "version" : "1.1.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec2 pixelSize;
	uniform vec2 center;

	// https://stackoverflow.com/questions/5049041/how-do-i-perform-a-fast-pixellation-filter-on-an-image
	void main(void)
	{
		vec2 pos = fragmentTextureCoordinate.xy;

		if(pixelSize.x > 0)
		{
			vec2 centerOffset = mod(center - pixelSize/2., pixelSize);
			vec2 distanceFromCorner = mod(pos - centerOffset, pixelSize);
			pos = pos - distanceFromCorner + pixelSize/2.;
		}

		gl_FragColor = texture2D(texture, pos);
	}
);

struct nodeInstanceData
{
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
	VuoShader frag = VuoShader_make("Pixellation Shader");
	VuoShader_addSource(frag, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(frag);
	VuoShader_setUniform_VuoImage(frag, "texture", image);

	VuoPoint2d pixelSize2d = VuoPoint2d_multiply(VuoPoint2d_make(1., (float)w/h), VuoShader_samplerSizeFromVuoSize(pixelSize));
	VuoShader_setUniform_VuoPoint2d(frag, "pixelSize",  pixelSize2d);
	VuoShader_setUniform_VuoPoint2d(frag, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	*pixellatedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h, VuoImage_getColorDepth(image));

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
