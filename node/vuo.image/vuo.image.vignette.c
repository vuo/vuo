/**
 * @file
 * vuo.image.vignette node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Vignette Image",
					  "keywords" : [ "border", "surround", "encapsulate", "darken", "post-process", "circle", "oval", "soften", "fade", "edge", "old", "daguerreotype", "filter" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "VignetteMovie.vuo" ]
					  }
				 });

static const char * vignetteFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform float innerRadius;
	uniform float outerRadius;
	// uniform vec2 scale;	// if image is not square, multiply texCoord by this to account for stretch

	// http://www.geeks3d.com/20091020/shader-library-lens-circle-post-processing-effect-glsl/
	void main(void)
	{
		vec4 col = texture2D(texture, fragmentTextureCoordinate.xy);
		float dist = distance(fragmentTextureCoordinate.xy, vec2(0.5,0.5));
		col.rgb *= smoothstep(outerRadius, innerRadius, dist);
		gl_FragColor = col;
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
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":4}) width,
		VuoInputData(VuoReal, {"default":0.33, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoOutputData(VuoImage) vignettedImage
)
{
	if (!image)
		return;

	int w = image->pixelsWide, h = image->pixelsHigh;
	VuoShader frag = VuoShader_make("Vignette Shader", VuoShader_getDefaultVertexShader(), vignetteFragmentShader);
	VuoRetain(frag);
	VuoShader_resetTextures(frag);
	VuoShader_addTexture(frag, (*instance)->glContext, "texture", image);
	// VuoShader_setUniformPoint2d(frag, (*instance)->glContext, "scale", w < h ? VuoPoint2d_make(1., h/(float)w) : VuoPoint2d_make(w/(float)h, 1.));

	float radius = width/2.;
	float sharp = sharpness;
	if(sharpness < 0) sharp = 0;
	if(sharpness > 1) sharp = 1;
	float innerRadius = radius * sharp;
	float outerRadius = radius * (2-sharp);

	// Make sure outerRadius is always a little larger than innerRadius, so smoothstep() doesn't invert its condition.
	outerRadius += .0001;

	VuoShader_setUniformFloat(frag, (*instance)->glContext, "innerRadius", innerRadius);
	VuoShader_setUniformFloat(frag, (*instance)->glContext, "outerRadius", outerRadius);

	*vignettedImage = VuoImageRenderer_draw((*instance)->imageRenderer, frag, w, h);

	VuoRelease(frag);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
