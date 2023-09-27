/**
 * @file
 * vuo.image.vignette node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Vignette Image",
					  "keywords" : [ "border", "surround", "encapsulate", "darken", "post-process", "circle", "oval", "soften", "fade", "edge", "old", "daguerreotype", "filter" ],
					  "version" : "1.1.3",
					  "node": {
						  "isDeprecated": true,
						  "exampleCompositions" : [ "VignetteImage.vuo" ]
					  }
				 });

static const char * vignetteFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform vec4 edgeColor;
	uniform float innerRadius;
	uniform float outerRadius;
	uniform bool replaceOpacity;

	// http://www.geeks3d.com/20091020/shader-library-lens-circle-post-processing-effect-glsl/
	void main(void)
	{
		vec4 col = texture2D(texture, fragmentTextureCoordinate);
		col.a = clamp(col.a, 0., 1.);	// for floating-point textures

		float dist = distance(fragmentTextureCoordinate, vec2(0.5,0.5));
		vec3 mixed = mix(edgeColor.rgb, col.rgb, smoothstep(outerRadius, innerRadius, dist) );
		float a = mix(edgeColor.a, col.a, smoothstep(outerRadius, innerRadius, dist));

		vec4 mixed2 = replaceOpacity ? vec4(mixed,a) : mix(col, vec4(mixed, a), edgeColor.a);
		gl_FragColor = mixed2;
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Vignette Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, vignetteFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) color,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":4}) width,
		VuoInputData(VuoReal, {"default":0.33, "suggestedMin":0, "suggestedMax":1}) sharpness,
		VuoInputData(VuoBoolean, {"default":true}) replaceOpacity,
		VuoOutputData(VuoImage) vignettedImage
)
{
	if (!image)
	{
		*vignettedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	// VuoShader_setUniform_VuoPoint2d((*instance)->shader, "scale", w < h ? VuoPoint2d_make(1., h/(float)w) : VuoPoint2d_make(w/(float)h, 1.));

	float radius = width/4.;
	if(radius < 0.) radius = 0.;
	if(radius > 2.) radius = 2.;

	float sharp = sharpness;
	if(sharpness < 0) sharp = 0;
	if(sharpness > 1) sharp = 1;
	float innerRadius = radius * sharp;
	float outerRadius = radius * (2-sharp);

	// Make sure outerRadius is always a little larger than innerRadius, so smoothstep() doesn't invert its condition.
	outerRadius += .0001;

	VuoShader_setUniform_VuoReal ((*instance)->shader, "innerRadius", innerRadius);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "outerRadius", outerRadius);
	VuoShader_setUniform_VuoColor((*instance)->shader, "edgeColor", color);
	VuoShader_setUniform_VuoBoolean ((*instance)->shader, "replaceOpacity", replaceOpacity);

	*vignettedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
