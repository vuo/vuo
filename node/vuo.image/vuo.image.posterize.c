/**
 * @file
 * vuo.image.filter.posterize node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Posterize Image",
					  "keywords" : [
						  "filter",
						  "stroke", "alias", "contour", "edge", "banding", "raster", "gif", "quantize", "gradient", "reduce",
						  "retro", ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "PosterizeImage.vuo" ]
					  }
				 });

/**
 * http://www.geeks3d.com/20091027/shader-library-posterization-post-processing-effect-glsl/
 */
static const char *posterizeFrag = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float gamma;
	uniform int numColors;

	void main()
	{
		vec4 ca = VuoGlsl_sample(texture, fragmentTextureCoordinate);
		vec3 c = ca.rgb;
		c = pow(c, vec3(gamma));
		c = c * numColors;
		c = floor(c);
		c = c / numColors;
		c = pow(c, vec3(1./gamma));
		c = clamp(c, 0., 1.);
		gl_FragColor = vec4(c, ca.a);
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

	instance->shader = VuoShader_make("Posterize Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, posterizeFrag);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoInteger, {"default":6, "suggestedMin":2, "suggestedMax":32}) colors,
		VuoOutputData(VuoImage) posterizedImage
)
{
	if (!image)
	{
		*posterizedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoReal((*instance)->shader, "gamma", .6);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "numColors", MAX(2,colors));

	*posterizedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
