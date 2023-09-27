/**
 * @file
 * vuo.image.pixellate node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Pixellate Image Radially",
					 "keywords" : [
						 "pixels", "lofi", "simplify", "mosaic", "censor",
						 "circle", "arc", "round", "pie",
						 "pixelate", // American spelling
						 "filter"
					 ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ "PixellateImageRadially.vuo" ]
					 }
				 });

static const char * pixelFragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;

	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform vec2 viewportSize;
	uniform vec2 pixelSize;
	uniform vec2 center;

	vec2 transform(vec2 textureCoordinate)
	{
		vec2 normCoord = 2.0 * textureCoordinate - 1.0;
		vec2 normCenter = 2.0 * center - 1.0;

		normCoord -= normCenter;
		normCoord.y /= aspectRatio;

		float r = length(normCoord); // to polar coords
		float theta = atan(normCoord.y, normCoord.x); // to polar coords

		// Move the origin so there's only a single discontinuity (at theta=0) instead of two (at theta=0 and theta=π).
		if (theta < 0.)
			theta += 2. * 3.14159;

		r -= mod(r, pixelSize.x);// + 0.03;
		theta -= mod(theta, pixelSize.y);

		// Center the big pixel around the sampled texture coordinate.
		r += pixelSize.x/2.;
		theta += pixelSize.y/2.;

		normCoord.x = r * cos(theta);
		normCoord.y = r * sin(theta) * aspectRatio;

		normCoord += normCenter;
		return normCoord / 2.0 + 0.5;
	}

	void main(void)
	{
		vec2 s = .25/viewportSize;
		vec2 sx = vec2(s.x, 0.);
		vec2 sy = vec2(0., s.y);
		vec4 a = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sx));
		vec4 b = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sx));
		vec4 c = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate - sy));
		vec4 d = VuoGlsl_sample(texture, transform(fragmentTextureCoordinate + sy));
		gl_FragColor = (a+b+c+d)/4.;
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

	instance->shader = VuoShader_make("Pixellate Image Radially");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, pixelFragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.05}) pixelDiameter,
		VuoInputData(VuoReal, {"default":8.0, "suggestedMin":0, "suggestedMax":360, "suggestedStep":1}) pixelAngle,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoOutputData(VuoImage) pixellatedImage
)
{
	if (!image)
	{
		*pixellatedImage = NULL;
		return;
	}

	int w = image->pixelsWide, h = image->pixelsHigh;

	VuoShader_setUniform_VuoImage((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "pixelSize", (VuoPoint2d){
		VuoShader_samplerSizeFromVuoSize(VuoReal_makeNonzero(pixelDiameter)),
		VuoReal_makeNonzero(pixelAngle) * M_PI/180.
	});
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));

	*pixellatedImage = VuoImageRenderer_render((*instance)->shader, w, h, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
