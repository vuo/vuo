/**
 * @file
 * vuo.image.tileable node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Make Tileable Image",
					  "keywords" : [
						  "filter",
						  "seamless",
						  "tilable", // common misspelling
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "TileMovie.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float blendWidth;
	uniform float aspectRatio;

	void main(void)
	{
		vec2 p = fract(fragmentTextureCoordinate.xy + blendWidth/2.);

		float spx = smoothstep(0., blendWidth, p.x);
		float spy = smoothstep(0., blendWidth, p.y);

		p -= vec2(blendWidth/2.);
		p *= 1. - blendWidth;
		p += vec2(blendWidth/2.);

		gl_FragColor =
			  VuoGlsl_sample(texture,  p                                      ) *     spx  *     spy
			+ VuoGlsl_sample(texture, (p + vec2(1.-blendWidth, 0.           ))) * (1.-spx) *     spy
			+ VuoGlsl_sample(texture, (p + vec2(1.-blendWidth, 1.-blendWidth))) * (1.-spx) * (1.-spy)
			+ VuoGlsl_sample(texture, (p + vec2(0.,            1.-blendWidth))) *     spx  * (1.-spy);
	}
);

struct nodeInstanceData
{
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	instance->shader = VuoShader_make("Make Image Tileable Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1.5}) blendWidth,
	VuoOutputData(VuoImage) tileableImage
)
{
	if (!image)
	{
		*tileableImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture",          image);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "blendWidth", VuoReal_clamp(VuoShader_samplerSizeFromVuoSize(blendWidth), 0, .999));

	*tileableImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
