/**
 * @file
 * vuo.image.reduceHaze node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Reduce Haze",
					 "keywords" : [
						 "UV", "ultraviolet",
						 "contrast",
						 "gradient", "graduated", "fade",
						 "remove",
						 "sky", "cloud", "fog", "smog", "mist",
						 "filter", "polarize"
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	// Inputs
	uniform sampler2D texture;
	uniform float reductionAtTop;
	uniform float reductionAtBottom;
	uniform vec4 hazeColor;
	varying vec2 fragmentTextureCoordinate;

	void main()
	{
		float d = mix(reductionAtBottom, reductionAtTop, fragmentTextureCoordinate.y);

		vec4 c = VuoGlsl_sample(texture, fragmentTextureCoordinate);
		c.rgb /= c.a;

		c.rgb = (c.rgb - d * hazeColor.rgb) / (1.0 - d);

		c.rgb *= c.a;

		gl_FragColor = c;
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

	instance->shader = VuoShader_make("Reduce Haze");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) hazeColor,
	VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1,"name":"Reduction at Top"}) reductionAtTop,
	VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1,"name":"Reduction at Bottom"}) reductionAtBottom,
	VuoOutputData(VuoImage) dehazedImage
)
{
	if (! image)
	{
		*dehazedImage = NULL;
		return;
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoColor((*instance)->shader, "hazeColor", hazeColor);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "reductionAtTop", reductionAtTop);
	VuoShader_setUniform_VuoReal ((*instance)->shader, "reductionAtBottom", reductionAtBottom);

	// Render.
	*dehazedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
